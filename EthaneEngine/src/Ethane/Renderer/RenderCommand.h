#pragma once

#include "RendererAPI.h"

namespace Ethane {

	class RenderCommand
	{
	public:
		static void Init();

		inline static void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
		{
			s_RendererAPI->SetViewport(x, y, width, height);
		}

		inline static void SetClearColor(const glm::vec4& color)
		{
			s_RendererAPI->SetClearColor(color);
		}

		inline static void Clear()
		{
			s_RendererAPI->Clear();
		}

		inline static void BeginFrame()
		{
			s_RendererAPI -> BeginFrame();
		}

		inline static void EndFrame()
		{
			s_RendererAPI->EndFrame();
		}

		inline static void DrawIndexed(uint32_t count = -1)
		{
			s_RendererAPI->DrawIndexed(count);
		}

		inline static void DrawMesh(Ref<Mesh> mesh, const glm::mat4& transform = glm::mat4(1.0f))
		{
			s_RendererAPI->DrawMesh(mesh, transform);
		}
	private:
		static Ref<RendererAPI> s_RendererAPI;
	};

}