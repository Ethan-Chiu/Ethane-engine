#pragma once

#include <glm/glm.hpp>

#include "Mesh.h"

namespace Ethane {

	class RendererAPI
	{
	public:
		enum class API
		{
			None = 0, OpenGL = 1, Vulkan = 2
		};

	public:
		virtual ~RendererAPI() = default;

		virtual void Init() = 0;
		virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;
		virtual void SetClearColor(const glm::vec4& color) = 0;
		virtual void Clear() = 0;

		virtual void DrawIndexed(uint32_t indexCount = 0) = 0;

		virtual void DrawMesh(Ref<Mesh> mesh, const glm::mat4& transform = glm::mat4(1.0f)) = 0;

		inline static API GetAPI() { return s_API; }
		inline static void SetAPI(API _API) { s_API = _API; }

	private:
		static API s_API;
	};

}