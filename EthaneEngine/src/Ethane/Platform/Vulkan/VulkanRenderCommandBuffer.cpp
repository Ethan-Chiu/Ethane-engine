#include "ethpch.h"
#include "VulkanRenderCommandBuffer.h"

#include <utility>

#include "VulkanContext.h"

namespace Ethane {

	VulkanRenderCommandBuffer::VulkanRenderCommandBuffer(uint32_t count, std::string debugName, bool swapchain)
		: m_DebugName(std::move(debugName)), m_OwnedBySwapChain(swapchain)
	{
		uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;
		Ref<VulkanDevice> device = VulkanContext::GetDevice();

		if (m_OwnedBySwapChain)
		{
			m_CommandBuffers.resize(framesInFlight);
			VulkanSwapChain& swapChain = VulkanContext::GetSwapChain();// Application::Get().GetWindow().GetSwapChain();
			for (uint32_t frame = 0; frame < framesInFlight; frame++)
				m_CommandBuffers[frame] = VulkanContext::GetCurrentCommandBuffer(frame).GetHandle();//swapChain.GetCommandBuffer(frame);

			// VkQueryPoolCreateInfo queryPoolCreateInfo = {};
			// queryPoolCreateInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
			// queryPoolCreateInfo.pNext = nullptr;
		}
		else
		{
			VkCommandPoolCreateInfo cmdPoolInfo = {};
			cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			cmdPoolInfo.queueFamilyIndex = VulkanContext::GetPhysicalDevice()->GetQueueFamilyIndices().Graphics.value();
			cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
			VK_CHECK_RESULT(vkCreateCommandPool(device->GetVulkanDevice(), &cmdPoolInfo, nullptr, &m_CommandPool));

			VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
			commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			commandBufferAllocateInfo.commandPool = m_CommandPool;
			commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			if (count == 0)
				count = framesInFlight;
			commandBufferAllocateInfo.commandBufferCount = count;
			m_CommandBuffers.resize(count);
			VK_CHECK_RESULT(vkAllocateCommandBuffers(device->GetVulkanDevice(), &commandBufferAllocateInfo, m_CommandBuffers.data()));

			VkFenceCreateInfo fenceCreateInfo{};
			fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
			m_WaitFences.resize(framesInFlight);
			for (auto& fence : m_WaitFences)
				VK_CHECK_RESULT(vkCreateFence(device->GetVulkanDevice(), &fenceCreateInfo, nullptr, &fence));

			// VkQueryPoolCreateInfo queryPoolCreateInfo = {};
			// queryPoolCreateInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
			// queryPoolCreateInfo.pNext = nullptr;
		}
	}

	VulkanRenderCommandBuffer::~VulkanRenderCommandBuffer()
	{
		if (m_OwnedBySwapChain)
			return;

		VkCommandPool commandPool = m_CommandPool;
		// Renderer::SubmitResourceFree([commandPool]()
		// 	{
				auto device = VulkanContext::GetDevice()->GetVulkanDevice();
				vkDestroyCommandPool(device, commandPool, nullptr);
		// });
	}

	void VulkanRenderCommandBuffer::Begin()
	{
		// Ref<VulkanRenderCommandBuffer> instance = this;
		// Renderer::Submit([instance]() mutable
		// 	{
		uint32_t frameIndex = VulkanContext::GetSwapChain().GetCurrentFrameIndex();// Renderer::GetCurrentFrameIndex();

		VkCommandBufferBeginInfo cmdBufInfo = {};
		cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmdBufInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		cmdBufInfo.pNext = nullptr;

		VkCommandBuffer commandBuffer = m_CommandBuffers[frameIndex];
		VK_CHECK_RESULT(vkBeginCommandBuffer(commandBuffer, &cmdBufInfo));
		// 	});
	}

	void VulkanRenderCommandBuffer::End()
	{
		// Ref<VulkanRenderCommandBuffer> instance = this;
		// Renderer::Submit([instance]()
		// 	{
		uint32_t frameIndex = VulkanContext::GetSwapChain().GetCurrentFrameIndex();
		VkCommandBuffer commandBuffer = m_CommandBuffers[frameIndex];
		VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));
		// 	});
	}

	void VulkanRenderCommandBuffer::Submit()
	{
		if (m_OwnedBySwapChain)
			return;

		// Ref<VulkanRenderCommandBuffer> instance = this;
		// Renderer::Submit([instance]() mutable
		// 	{
		uint32_t frameIndex = VulkanContext::GetSwapChain().GetCurrentFrameIndex();

		auto device = VulkanContext::GetDevice()->GetVulkanDevice();

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		VkCommandBuffer commandBuffer = m_CommandBuffers[frameIndex];
		submitInfo.pCommandBuffers = &commandBuffer;

		VK_CHECK_RESULT(vkWaitForFences(device, 1, &m_WaitFences[frameIndex], VK_TRUE, UINT64_MAX));
		VK_CHECK_RESULT(vkResetFences(device, 1, &m_WaitFences[frameIndex]));
		VK_CHECK_RESULT(vkQueueSubmit(VulkanContext::GetDevice()->GetGraphicsQueue(), 1, &submitInfo, m_WaitFences[frameIndex]));
		// });
	}

}