#pragma once

#include "RenderCommand.h"
#include "RenderCommandQueue.h"

#include "OrthographicCamera.h"
#include "EditorCamera.h"

#include "Renderer2D.h"

#include "Shader.h"
#include "Mesh.h"

namespace Ethane {
	
	struct RendererConfig
	{
		uint32_t FramesInFlight = 3;

		// "Experimental" features
		bool ComputeEnvironmentMaps = true;

		// Tiering settings
		uint32_t EnvironmentMapResolution = 1024;
		uint32_t IrradianceMapComputeSamples = 512;
	};

	class Renderer
	{
	public:
		static void Init();
		static void Shutdown();

		static void OnWindowResize(uint32_t width, uint32_t height);

		static void BeginFrame();
		static void EndFrame();

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

		// Stats // TODO: temp
		struct Statistics
		{
			uint32_t DrawCalls = 0;
			uint32_t QuadCount = 0;
		};
		static void ResetStats() {};

		// Getter
		static RendererConfig& GetConfig() { return s_Config; }
		// static Statistics GetStats() {}; // TODO: temp

	private:
		static RenderCommandQueue& GetRenderCommandQueue();

	private:
		static RenderCommandQueue* s_CommandQueue;
		static RendererConfig s_Config;
	};

}