#include "ethpch.h"
#include "Ethane/Renderer/Renderer.h"

#include "Ethane/Asset/AssetLibrary.h"
#include "Ethane/GfxBackend/GfxBackendAPI.h"
#include "Ethane/Renderer/Material.h"
#include "Ethane/Renderer/Mesh.h"

namespace Ethane {

	namespace {

		// layout(set = 0, binding = 0) uniform Frame in mesh.glsl (std140)
		struct FrameUniforms
		{
			glm::mat4 ViewProj;
			glm::vec4 CameraPosition; // xyz
			glm::vec4 LightDirection; // xyz: towards the light; w unused
			glm::vec4 LightRadiance;  // rgb: color * intensity (0 when there's no light)
		};


	}

	Renderer::Renderer(GfxBackendAPI& backend, AssetLibrary& assets, const RefCountPtr<IRenderPass>& renderPass,
	                   const std::string& meshShader, const std::string& pickShader)
		: m_Backend(&backend), m_Assets(&assets)
	{
		m_MeshShader = backend.CreateShader(meshShader);

		PipelineSpecification spec;
		spec.Shader = m_MeshShader;
		spec.RenderPass = renderPass;
		spec.Layout = Mesh::GetVertexLayout();
		spec.CullMode = CullMode::BACK;
		m_MeshPipeline = backend.CreatePipeline(spec);

		m_FrameUniforms = backend.CreateUniformBuffer(sizeof(FrameUniforms));
		m_FrameBindings = backend.CreateBindingSet(m_MeshShader, 0);
		m_FrameBindings->SetUniformBuffer(0, m_FrameUniforms);

		// Picking: ids in a 1x1 R32UI image, cleared to 0 ("nothing"); depth keeps the nearest one
		m_PickTarget = backend.CreateRenderTarget(RenderTargetSpecification{
			.Width = 1, .Height = 1, .ColorFormat = ImageFormat::R32UI, .ClearColor = { 0.0f, 0.0f, 0.0f, 0.0f } });
		m_PickShader = backend.CreateShader(pickShader);
		PipelineSpecification pickSpec;
		pickSpec.Shader = m_PickShader;
		pickSpec.RenderPass = m_PickTarget->GetRenderPass();
		pickSpec.Layout = Mesh::GetVertexLayout();
		pickSpec.CullMode = CullMode::BACK;
		m_PickPipeline = backend.CreatePipeline(pickSpec);
		m_PickUniforms = backend.CreateUniformBuffer(sizeof(glm::mat4));
		m_PickBindings = backend.CreateBindingSet(m_PickShader, 0);
		m_PickBindings->SetUniformBuffer(0, m_PickUniforms);
	}

	Renderer::~Renderer() = default;

	void Renderer::Render(ICommandList& commandList, const FramePacket& packet, IRenderTarget& target)
	{
		m_Frame++;

		FrameUniforms frame{};
		frame.ViewProj = packet.View.ViewProjection();
		frame.CameraPosition = glm::vec4(packet.View.Position, 1.0f);
		if (!packet.DirectionalLights.empty())
		{
			const DirectionalLight& light = packet.DirectionalLights.front();
			frame.LightDirection = glm::vec4(-glm::normalize(light.Direction), 0.0f);
			frame.LightRadiance = glm::vec4(light.Color * light.Intensity, 0.0f);
		}
		m_FrameUniforms->SetData(&frame, sizeof(frame));

		m_Stats = {};
		commandList.BeginRenderPass(target);
		commandList.BindPipeline(*m_MeshPipeline);
		commandList.BindBindingSet(*m_FrameBindings);
		for (const DrawItem& item : packet.DrawItems())
		{
			m_Stats.DrawItems++;
			Ref<Mesh> mesh = m_Assets->GetMesh(item.Mesh);
			if (!mesh)
			{
				m_Stats.MissingMeshes++;
				continue;
			}

			const MaterialParams* params = packet.ParamsOf(item);
			const auto& materials = params
				? ElementMaterials(item.Element, item.Mesh, *mesh, *params)
				: MeshMaterials(item.Mesh, *mesh);
			mesh->Draw(commandList, item.Transform, materials);

			for (const MeshNode& node : mesh->GetNodes())
			{
				m_Stats.DrawCalls++;
				m_Stats.Triangles += mesh->GetSubmeshes()[node.SubmeshIndex].IndexCount / 3;
			}
		}
		commandList.EndRenderPass();

		CollectPick(); // an earlier request's answer, if its frame has finished
		std::optional<glm::uvec2> pick;
		{
			std::lock_guard lock(m_PickMutex);
			pick = std::exchange(m_PickRequest, std::nullopt);
		}
		if (pick)
			RecordPick(commandList, packet, target, *pick);

		ReleaseUnused();
		std::lock_guard lock(m_StatsMutex);
		m_PublishedStats = m_Stats;
	}

	void Renderer::RequestPick(glm::uvec2 pixel)
	{
		std::lock_guard lock(m_PickMutex);
		m_PickRequest = pixel;
	}

	std::optional<ElementId> Renderer::TakePick()
	{
		std::lock_guard lock(m_PickMutex);
		return std::exchange(m_PickResult, std::nullopt);
	}

	void Renderer::RecordPick(ICommandList& commandList, const FramePacket& packet, const IRenderTarget& target, glm::uvec2 pixel)
	{
		// The view narrowed to one pixel of `target`: that pixel's center goes to the middle of clip space and
		// its edges to the edges, so the 1x1 image sees exactly what the pixel shows. The viewport is flipped
		// (row 0 at the top), so the top row is NDC y = +1.
		const RenderTargetSpecification& spec = target.GetSpecification();
		const float width = static_cast<float>(spec.Width), height = static_cast<float>(spec.Height);
		const float centerX = 2.0f * (static_cast<float>(pixel.x) + 0.5f) / width - 1.0f;
		const float centerY = 1.0f - 2.0f * (static_cast<float>(pixel.y) + 0.5f) / height;
		glm::mat4 narrow(1.0f);
		narrow[0][0] = width;
		narrow[1][1] = height;
		narrow[3][0] = -centerX * width; // in clip space: scaled by w, so it shifts NDC by -centerX * width
		narrow[3][1] = -centerY * height;
		const glm::mat4 viewProjection = narrow * packet.View.ViewProjection();
		m_PickUniforms->SetData(&viewProjection, sizeof(viewProjection));

		PendingPick pending;
		commandList.BeginRenderPass(*m_PickTarget);
		commandList.BindPipeline(*m_PickPipeline);
		commandList.BindBindingSet(*m_PickBindings);
		const auto items = packet.DrawItems();
		pending.Elements.reserve(items.size());
		for (uint32_t i = 0; i < items.size(); i++)
		{
			pending.Elements.push_back(items[i].Element);
			Ref<Mesh> mesh = m_Assets->GetMesh(items[i].Mesh);
			if (!mesh)
				continue;
			const uint32_t id = i + 1; // 0 is "nothing"
			commandList.PushConstants(&id, sizeof(id), sizeof(glm::mat4)); // after the model matrix
			mesh->Draw(commandList, items[i].Transform, {}); // no materials: the pick shader has none
		}
		commandList.EndRenderPass();

		pending.Readback = m_Backend->CreateReadbackBuffer(sizeof(uint32_t));
		commandList.CopyPixelToReadback(*m_PickTarget, 0, 0, *pending.Readback);
		m_PendingPick = std::move(pending); // replaces an older pick still in flight
	}

	void Renderer::CollectPick()
	{
		uint32_t id = 0;
		if (!m_PendingPick || !m_PendingPick->Readback->Read(&id, sizeof(id)))
			return; // none, or its frame hasn't finished yet
		const ElementId element = id > 0 && id <= m_PendingPick->Elements.size() ? m_PendingPick->Elements[id - 1] : ElementId{};
		m_PendingPick.reset();
		std::lock_guard lock(m_PickMutex);
		m_PickResult = element;
	}

	const std::vector<Ref<Material>>& Renderer::MeshMaterials(AssetId id, const Mesh& mesh)
	{
		auto it = m_MeshMaterials.find(id);
		if (it == m_MeshMaterials.end())
			it = m_MeshMaterials.emplace(id, Material::CreateAll(*m_Backend, m_MeshShader, mesh.GetMaterials())).first;
		return it->second;
	}

	const std::vector<Ref<Material>>& Renderer::ElementMaterials(ElementId element, AssetId meshId, const Mesh& mesh, const MaterialParams& values)
	{
		// The texture params ask for, if it's available yet: textures load in the background, so it may
		// arrive a few frames after the materials were made
		const RefCountPtr<ITexture2D> texture = values.BaseColorTexture.IsValid() ? m_Assets->GetTexture(values.BaseColorTexture) : RefCountPtr<ITexture2D>();
		auto apply = [&](MaterialData data) {
			data.BaseColor = values.BaseColor;
			data.Metallic = values.Metallic;
			data.Roughness = values.Roughness;
			if (texture)
				data.BaseColorTexture = texture; // else the mesh file's own, until it has loaded
			return data;
		};

		ElementMaterialSet& entry = m_ElementMaterials[element];
		if (entry.Materials.empty() || entry.Mesh != meshId)
		{
			std::vector<MaterialData> data;
			for (const MaterialData& base : mesh.GetMaterials())
				data.push_back(apply(base));
			entry.Mesh = meshId;
			entry.Materials = Material::CreateAll(*m_Backend, m_MeshShader, data);
		}
		else if (entry.Values != values || entry.Texture.get() != texture.get())
		{
			// New values, or the texture finished loading since the materials were last set
			const auto& bases = mesh.GetMaterials();
			for (size_t i = 0; i < entry.Materials.size() && i < bases.size(); i++)
				entry.Materials[i]->SetData(apply(bases[i]));
		}
		entry.Values = values;
		entry.Texture = texture;
		entry.LastUsed = m_Frame;
		return entry.Materials;
	}

	void Renderer::ReleaseUnused()
	{
		// Materials of elements this frame didn't draw. Safe right away: the backend keeps GPU objects alive
		// until the frames that used them have finished.
		std::erase_if(m_ElementMaterials, [&](const auto& entry) { return entry.second.LastUsed != m_Frame; });
	}

}
