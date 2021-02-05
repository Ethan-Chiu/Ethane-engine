#include "ethpch.h"
#include "OpenGLContext.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

namespace Ethane {

	OpenGLContext::OpenGLContext(GLFWwindow* windowHandle)
		:m_windowHandle(windowHandle)
	{
		ETH_CORE_ASSERT(windowHandle, "Window handle os null");
	}

	void OpenGLContext::Init()
	{
		ETH_PROFILE_FUNCTION();

		glfwMakeContextCurrent(m_windowHandle);
		int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
		ETH_CORE_ASSERT(status, "Failed to initialize Glad!");

		ETH_CORE_INFO("OpenGL Info:");
		ETH_CORE_INFO("  Vender: {0}", glGetString(GL_VENDOR));
		ETH_CORE_INFO("  Renderer: {0}", glGetString(GL_RENDERER));
		ETH_CORE_INFO("  Version: {0}", glGetString(GL_VERSION));

	#ifdef ETH_ENABLE_ASSERTS
			int versionMajor;
			int versionMinor;
			glGetIntegerv(GL_MAJOR_VERSION, &versionMajor);
			glGetIntegerv(GL_MINOR_VERSION, &versionMinor);

			ETH_CORE_ASSERT(versionMajor > 4 || (versionMajor == 4 && versionMinor >= 5), "Hazel requires at least OpenGL version 4.5!");
	#endif
	}

	void OpenGLContext::SwapBuffers()
	{
		ETH_PROFILE_FUNCTION();

		glfwSwapBuffers(m_windowHandle);
	}

}