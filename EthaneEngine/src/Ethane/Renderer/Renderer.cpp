#include "ethpch.h"
#include "Renderer.h"

#include "Ethane/Platform/OpenGL/OpenGLShader.h"

namespace Ethane {

	RenderCommandQueue* Renderer::s_CommandQueue = nullptr;
	
	RendererConfig Renderer::s_Config = RendererConfig{};

	void Renderer::Init()
	{
		ETH_PROFILE_FUNCTION();

		RenderCommand::Init();
		s_CommandQueue = new RenderCommandQueue();

		// Renderer2D::Init();
	}

	void Renderer::Shutdown()
	{
		// Renderer2D::Shutdown();

		delete Renderer::s_CommandQueue;
	}

	void Renderer::OnWindowResize(uint32_t width, uint32_t height)
	{
		RenderCommand::SetViewport(0, 0, width, height);
	}

	void Renderer::BeginFrame()
	{
		RenderCommand::BeginFrame();
	}

	void Renderer::EndFrame()
	{
		RenderCommand::EndFrame();
	}

	void Renderer::WaitAndRender()
	{
		ETH_PROFILE_FUNCTION();
		s_CommandQueue->Execute();
	}

	RenderCommandQueue& Renderer::GetRenderCommandQueue()
	{
		return *s_CommandQueue;
	}
	
}