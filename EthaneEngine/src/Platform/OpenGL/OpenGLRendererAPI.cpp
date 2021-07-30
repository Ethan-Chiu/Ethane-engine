#include "ethpch.h"
#include "OpenGLRendererAPI.h"
#include "Platform/OpenGL/OpenGLShader.h"

#include <glad/glad.h>

namespace Ethane {

	void OpenGLRendererAPI::Init()
	{
		ETH_PROFILE_FUNCTION();

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glEnable(GL_DEPTH_TEST);
	}

	void OpenGLRendererAPI::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
	{
		glViewport(x, y, width, height);
	}

	void OpenGLRendererAPI::SetClearColor(const glm::vec4& color)
	{
		glClearColor(color.r, color.g, color.b, color.a);
	}

	void OpenGLRendererAPI::Clear()
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void OpenGLRendererAPI::DrawIndexed(uint32_t indexCount)
	{
		uint32_t count = indexCount;
		// == -1 ? vertexArray->GetIndexBuffer()->GetCount() : indexCount;
		glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, nullptr);
		glBindTexture(GL_TEXTURE_2D, 0);
	}


	void OpenGLRendererAPI::DrawMesh(Ref<Mesh> mesh, const glm::mat4& transform)
	{
		mesh->m_VertexBuffer->Bind();
		mesh->m_Pipeline->Bind();
		mesh->m_IndexBuffer->Bind();

		auto shader = mesh->m_MeshShader;
		uint32_t uboIndex = std::dynamic_pointer_cast<OpenGLShader>(shader)->GetUniformBufferIndex(1);

		// auto& materials = mesh->GetMaterials();
		for (Submesh& submesh : mesh->m_Submeshes)
		{
				// Material
				// auto material = materials[submesh.MaterialIndex].As<OpenGLMaterial>();
				// auto shader = material->GetShader().As<OpenGLShader>();
			// auto shader = mesh->m_MeshShader;
				// material->UpdateForRendering();

			auto transformUniform = transform * submesh.Transform;
			// shader->Bind();
			ETH_CORE_INFO("{0}", log_mat4{ transformUniform });

			std::dynamic_pointer_cast<OpenGLShader>(shader)->SetUniformBuffer(uboIndex, &transformUniform, sizeof(transformUniform));
			// std::dynamic_pointer_cast<OpenGLShader>(shader)->UploadUniformMat4("u_Transform", transformUniform);
			// shader->SetUniform("u_Renderer.Transform", transformUniform);

			// Renderer::Submit([submesh, material]()
			// 	{
			// if (material->GetFlag(MaterialFlag::DepthTest))
			glEnable(GL_DEPTH_TEST);
			// else
			// 	glDisable(GL_DEPTH_TEST);

			glDrawElementsBaseVertex(GL_TRIANGLES, submesh.IndexCount, GL_UNSIGNED_INT, (void*)(sizeof(uint32_t) * submesh.BaseIndex), submesh.BaseVertex);
			// 	});
		}
	}
}