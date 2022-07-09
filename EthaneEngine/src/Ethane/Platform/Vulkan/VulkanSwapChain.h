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

// TODO: test
#include "VulkanFramebuffer.h"

// TODO: remove
#include "imgui.h"
#include "examples/imgui_impl_vulkan_with_textures.h"

struct GLFWwindow;

namespace Ethane{

	class VulkanSwapChain
	{
	public:
		VulkanSwapChain() = default;
		~VulkanSwapChain();

		void Create(VkSurfaceKHR surface, const Ref<VulkanDevice>& device, uint32_t width, uint32_t height, bool vsync);
		void Destroy();

		void CleanupSwapChain(VkSwapchainKHR swapchain);

		void OnResize(uint32_t width, uint32_t height);

		void BeginFrame();
		void DrawFrame();

		// Getter
		VkSurfaceKHR GetSurface() const { return m_Surface; }
		VkFormat GetImageFormat() const { return m_ImageFormat; }
		VkFormat GetDepthFormat() const { return m_DepthFormat; }
		VkRenderPass GetRenderPass() { return m_RenderPass; } // test
		uint32_t GetImageCount() { return m_ImageCount; } // test
		uint32_t GetWidth() { return m_Extent.width; }// test
		uint32_t GetHeight() { return m_Extent.height; }// test
		uint32_t GetCurrentFrameIndex() { return m_CurrentFrame; }// test
		VkFramebuffer GetCurrentFramebuffer() { return m_Framebuffers[m_CurrentImageIndex]; } // test
		VkCommandBuffer GetCurrentCommandBuffer() { return m_CommandBuffers[m_CurrentFrame]; } // test
		VkCommandBuffer GetCommandBuffer(uint32_t frameIndex) { return m_CommandBuffers[frameIndex]; } // test
		uint32_t GetMaxFramesInFlight() { return MAX_FRAMES_IN_FLIGHT; } // TODO

	private:
		VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
		VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
		VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
		VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

		// void CreateDepthResources();
		void CreateRenderPass(); //test

		void Resize();

		void AcquireNextImage();
		void Present(VkQueue queue, VkSemaphore signalSemaphore = VK_NULL_HANDLE);
	private:
		VkSwapchainKHR m_SwapChain = nullptr;

		Ref<VulkanDevice> m_Device;
		Ref<VulkanPhysicalDevice> m_PhysicalDevice;

		VkSurfaceKHR m_Surface;
		uint32_t m_Width = 0, m_Height = 0;
		VkExtent2D m_Extent;
		bool m_NeedResize = false;

		VkFormat m_ImageFormat;
		VkColorSpaceKHR m_ImageColorSpace;

		uint32_t m_ImageCount = 0;
		std::vector<VkImage> m_Images;
		std::vector<VkImageView> m_ImageViews;

		VkFormat m_DepthFormat;
		Ref<VulkanImage2D> m_DepthAttachment;

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
		VulkanShader::DescriptorSetsAndPool m_DescriptorSets;
		Ref<VulkanPipeline> m_Pipeline = nullptr;
		std::vector<Ref<VulkanUniformBuffer>> m_UniformBuffers = {};
		Ref<VulkanTexture2D> m_Texture2D = nullptr;
		ImTextureID m_TextureID = nullptr;
	};

}