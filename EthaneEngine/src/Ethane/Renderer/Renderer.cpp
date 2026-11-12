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

		// GPU objects can still be in use by the frames in flight (2) after the last frame that drew
		// them was recorded: keep unused per-element materials this many Render calls before releasing
		constexpr uint64_t kFramesBeforeRelease = 3;

	}

	Renderer::Renderer(GfxBackendAPI& backend, AssetLibrary& assets, const RefCountPtr<IRenderPass>& renderPass, const std::string& meshShader)
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

		ReleaseUnused();
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
		auto apply = [&](MaterialData data) {
			data.BaseColor = values.BaseColor;
			data.Metallic = values.Metallic;
			data.Roughness = values.Roughness;
			if (values.BaseColorTexture.IsValid())
				if (RefCountPtr<ITexture2D> texture = m_Assets->GetTexture(values.BaseColorTexture))
					data.BaseColorTexture = texture;
			return data;
		};

		ElementMaterialSet& entry = m_ElementMaterials[element];
		if (entry.Materials.empty() || entry.Mesh != meshId)
		{
			std::vector<MaterialData> data;
			for (const MaterialData& base : mesh.GetMaterials())
				data.push_back(apply(base));
			entry.Mesh = meshId;
			entry.Values = values;
			entry.Materials = Material::CreateAll(*m_Backend, m_MeshShader, data);
		}
		else if (entry.Values != values)
		{
			const auto& bases = mesh.GetMaterials();
			for (size_t i = 0; i < entry.Materials.size() && i < bases.size(); i++)
				entry.Materials[i]->SetData(apply(bases[i]));
			entry.Values = values;
		}
		entry.LastUsed = m_Frame;
		return entry.Materials;
	}

	void Renderer::ReleaseUnused()
	{
		std::erase_if(m_ElementMaterials, [&](const auto& entry) { return m_Frame - entry.second.LastUsed > kFramesBeforeRelease; });
	}

}
