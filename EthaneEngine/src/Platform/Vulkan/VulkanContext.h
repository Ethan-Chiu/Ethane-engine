#pragma once

#include "Ethane/Renderer/GraphicsContext.h"

#include <vulkan/vulkan.h>
#include "VulkanDevice.h"
#include "VulkanSwapChain.h"

struct GLFWwindow;

namespace Ethane {

	class VulkanContext : public GraphicsContext
	{
	public:
		VulkanContext(GLFWwindow* windowHandle);
		virtual ~VulkanContext();

		virtual void Init() override;
		virtual void SwapBuffers() override;
		
		virtual void OnResize(uint32_t width, uint32_t height) override;

		// TODO: test
		virtual void ShutDown() override { vkDeviceWaitIdle(m_Device->GetVulkanDevice()); };

		// getter
		static VkInstance GetInstance() { return s_VulkanInstance; }
		static Ref<VulkanPhysicalDevice> GetPhysicalDevice() { return m_PhysicalDevice; } // TODO
		static Ref<VulkanDevice> GetDevice() { return m_Device; } // TODO
	private:
		GLFWwindow* m_WindowHandle;

		inline static VkInstance s_VulkanInstance;
		VkDebugReportCallbackEXT m_DebugReportCallback = VK_NULL_HANDLE;

		inline static Ref<VulkanPhysicalDevice> m_PhysicalDevice; // TODO
		inline static Ref<VulkanDevice> m_Device; // TODO

		VulkanSwapChain m_SwapChain;
	};

}