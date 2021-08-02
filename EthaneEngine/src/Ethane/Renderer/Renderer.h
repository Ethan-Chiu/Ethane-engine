#pragma once

#include "RenderCommand.h"
#include "RenderCommandQueue.h"

#include "OrthographicCamera.h"
#include "EditorCamera.h"

#include "Renderer2D.h"

#include "Shader.h"
#include "Mesh.h"

namespace Ethane {
	
	class Renderer
	{
	public:
		static void Init();
		static void Shutdown();

		static void OnWindowResize(uint32_t width, uint32_t height);

		static void BeginScene(OrthographicCamera& camera);
		static void BeginFrame();
		static void EndFrame();


		static void RenderMesh(Ref<Mesh> mesh, const glm::mat4& transform = glm::mat4(1.0f));

		// static void Submit(const Ref<Shader>& shader, const Ref<VertexArray>& vertexArray, const glm::mat4& transform)
		static void Submit(const Ref<Shader>& shader, const uint32_t indexCount, const glm::mat4& transform = glm::mat4(1.0f));

		template<typename FuncT>
		static void Submit(FuncT&& func)
		{
			auto renderCmd = [](void* ptr) {
				auto pFunc = (FuncT*)ptr;
				(*pFunc)();
				pFunc->~FuncT();
			};
			auto storageBuffer = GetRenderCommandQueue().Allocate(renderCmd, sizeof(func));
			new (storageBuffer) FuncT(std::forward<FuncT>(func));
		}

		static void WaitAndRender();

		inline static RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }

	private:
		static RenderCommandQueue& GetRenderCommandQueue();

	private:
		struct SceneData
		{
			glm::mat4 ViewProjectionMatrix;
		};

		static Scope<SceneData> s_SceneData;
		static RenderCommandQueue* s_CommandQueue;
	};

}