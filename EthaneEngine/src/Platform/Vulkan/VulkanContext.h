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

		virtual void BeginFrame() override; // TODO: test
		virtual void SwapBuffers() override;
		
		virtual void OnResize(uint32_t width, uint32_t height) override;

		// TODO: test
		virtual void ShutDown() override { vkDeviceWaitIdle(m_Device->GetVulkanDevice()); };

		// getter
		static VkInstance GetInstance() { return s_VulkanInstance; }
		static Ref<VulkanPhysicalDevice> GetPhysicalDevice() { return m_PhysicalDevice; } // TODO
		static Ref<VulkanDevice> GetDevice() { return m_Device; } // TODO
		static VulkanSwapChain GetSwapChain() { return m_SwapChain; } // TODO
	private:
		GLFWwindow* m_WindowHandle;

		inline static VkInstance s_VulkanInstance;
		VkDebugReportCallbackEXT m_DebugReportCallback = VK_NULL_HANDLE;

		inline static Ref<VulkanPhysicalDevice> m_PhysicalDevice; // TODO
		inline static Ref<VulkanDevice> m_Device; // TODO

		inline static VulkanSwapChain m_SwapChain; // TODO
	};


	///////////////////////////////////////////////////////////////////////////
	// helper funtions ////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////


	namespace Utils {

		inline uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
		{
			auto physicalDevice = VulkanContext::GetPhysicalDevice()->GetVulkanPhysicalDevice();

			VkPhysicalDeviceMemoryProperties memProperties;
			vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

			for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
				if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
					return i;
				}
			}
		}

		inline void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
			auto device = VulkanContext::GetDevice()->GetVulkanDevice();

			VkBufferCreateInfo bufferInfo{};
			bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferInfo.size = size;
			bufferInfo.usage = usage;
			bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
				throw std::runtime_error("failed to create buffer!");
			}

			VkMemoryRequirements memRequirements;
			vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

			VkMemoryAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocInfo.allocationSize = memRequirements.size;
			allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

			if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
				throw std::runtime_error("failed to allocate buffer memory!");
			}

			vkBindBufferMemory(device, buffer, bufferMemory, 0);
		}

		inline void CopyBuffer(VkBuffer dstBuffer, VkBuffer srcBuffer, VkDeviceSize size) {
			// create command buffer
			VkCommandBuffer copyCmdBuffer = VulkanContext::GetDevice()->CreateCommandBuffer(QueueFamilyTypes::Graphics, true, true);

			// record command
			VkBufferCopy copyRegion{};
			copyRegion.srcOffset = 0;
			copyRegion.dstOffset = 0;
			copyRegion.size = size;
			vkCmdCopyBuffer(copyCmdBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

			// submit command buffer
			VulkanContext::GetDevice()->SubmitCommandBuffer(copyCmdBuffer);
		}
	}

}