#include "ethpch.h"
#include "Ethane/Renderer/Renderer.h"

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

	Renderer::Renderer(GfxBackendAPI& backend, const RefCountPtr<IRenderPass>& renderPass, const std::string& meshShader)
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

	void Renderer::Render(ICommandList& commandList, const FramePacket& packet, IRenderTarget& target)
	{
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
			item.MeshRef->Draw(commandList, item.Transform, packet.MaterialsOf(item));

			m_Stats.DrawItems++;
			for (const MeshNode& node : item.MeshRef->GetNodes())
			{
				m_Stats.DrawCalls++;
				m_Stats.Triangles += item.MeshRef->GetSubmeshes()[node.SubmeshIndex].IndexCount / 3;
			}
		}
		commandList.EndRenderPass();
	}

}
