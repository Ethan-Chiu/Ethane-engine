#pragma once

#include "Ethane/Renderer/GraphicsContext.h"

struct GLFWwindow;

namespace Ethane {

	class OpenGLContext : public GraphicsContext
	{
	public:
		OpenGLContext(GLFWwindow* windowHandle);

		virtual void Init() override;
		
		virtual void BeginFrame() override {}; // TODO: test
		virtual void SwapBuffers() override;

		virtual void OnResize(uint32_t width, uint32_t height) {}

		// TODO: test
		virtual void ShutDown() override {}
	private:
		GLFWwindow* m_windowHandle;
	};

}