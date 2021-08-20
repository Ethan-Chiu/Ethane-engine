#pragma once

#include "Vulkan.h"
#include "VulkanDevice.h"

#include "VulkanPipeline.h" // TODO: remove this
#include "VulkanVertexBuffer.h" // TODO: remove this
#include "VulkanIndexBuffer.h" // TODO: remove this
#include "VulkanUniformBuffer.h" // TODO: remove this
#include "VulkanTexture.h" // TODO: remove this
#include "VulkanShader.h" // TODO: remove this
#include "VulkanRenderPass.h"// TODO: remove this

struct GLFWwindow;

namespace Ethane{

	class VulkanSwapChain
	{
	public:
		struct SwapChainSupportDetails {
			VkSurfaceCapabilitiesKHR capabilities;
			std::vector<VkSurfaceFormatKHR> formats;
			std::vector<VkPresentModeKHR> presentModes;
		};

		VulkanSwapChain() = default;
		~VulkanSwapChain();

		void Init(VkInstance instance, GLFWwindow* windowHandle);
		void Create(const Ref<VulkanDevice>& device, uint32_t* width, uint32_t* height, bool vsync);
		
		void CleanupSwapChain(VkSwapchainKHR swapchain);
		void Cleanup();

		void OnResize(uint32_t width, uint32_t height);

		void BeginFrame();
		void DrawFrame();

		SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

		// Getter
		VkSurfaceKHR GetSurface() const { return m_Surface; };
		VkRenderPass GetRenderPass() { return m_RenderPass; } // test
	private:
		void CreateSurface(GLFWwindow* windowHandle);
		VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
		VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
		VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
		VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

		uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties); // TODO: remove

		void CreateDepthResources();
		void CreateRenderPass(); //test

		void AcquireNextImage();
		void PresentQueue(VkQueue queue, VkSemaphore* signalSemaphore = nullptr);
	private:
		VkSwapchainKHR m_SwapChain = nullptr;

		VkInstance m_Instance;
		Ref<VulkanDevice> m_Device;
		Ref<VulkanPhysicalDevice> m_PhysicalDevice;

		uint32_t m_ImageCount = 0;
		std::vector<VkImage> m_Images;
		std::vector<VkImageView> m_ImageViews;
		VkFormat m_ImageFormat;
		VkExtent2D m_Extent;

		VkImage m_DepthImage;
		VkDeviceMemory m_DepthImageMemory;
		VkImageView m_DepthImageView;
		VkFormat m_DepthFormat;

		VkSurfaceKHR m_Surface;
		uint32_t m_Width = 0, m_Height = 0;

		VkRenderPass m_RenderPass; // TODO: remove this maybe ?
		std::vector<VkFramebuffer> m_Framebuffers;

		VkCommandPool m_CommandPool = nullptr;
		std::vector<VkCommandBuffer> m_CommandBuffers;

		bool m_VSync = false;

		const uint32_t MAX_FRAMES_IN_FLIGHT = 3;
		std::vector<VkSemaphore> m_ImageAvailableSemaphores;
		std::vector<VkSemaphore> m_RenderFinishedSemaphores;
		std::vector<VkFence> m_InFlightFences;
		std::vector<VkFence> m_ImagesInFlight;

		uint32_t m_CurrentFrame = 0;
		uint32_t m_CurrentImageIndex = 0;


		// TODO: remove these
		Ref<VulkanVertexBuffer> m_VertexBuffer = nullptr;
		Ref<VulkanIndexBuffer> m_IndexBuffer = nullptr;
		Ref<VulkanPipeline> m_Pipeline = nullptr;
		std::vector<Ref<VulkanUniformBuffer>> m_UniformBuffers = {};
		Ref<VulkanTexture2D> m_Texture2D = nullptr;
	};

}