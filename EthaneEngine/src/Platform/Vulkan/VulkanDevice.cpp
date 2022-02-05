#include "ethpch.h"
#include "VulkanContext.h"
#include "VulkanDevice.h"

namespace Ethane {

	Ref<VulkanPhysicalDevice> VulkanPhysicalDevice::Init(const std::vector<uint32_t>& compatibleDeviceIndices, VkSurfaceKHR surface)
	{
		return CreateRef<VulkanPhysicalDevice>(compatibleDeviceIndices, surface);
	}

	VulkanPhysicalDevice::VulkanPhysicalDevice(const std::vector<uint32_t>& compatibleDeviceIndices, VkSurfaceKHR surface)
		:m_Surface(surface)
	{
		auto vkInstance = VulkanContext::GetInstance();

		// List devices
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(vkInstance, &deviceCount, nullptr);
		ETH_CORE_ASSERT(deviceCount > 0, "failed to find GPUs with Vulkan support!");
		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(vkInstance, &deviceCount, devices.data());

		// Select device 
		std::multimap<uint32_t, VkPhysicalDevice> candidates;
		for (const auto& deviceIndex : compatibleDeviceIndices)
		{
			int score = rateDeviceSuitability(devices[deviceIndex]);
			candidates.insert(std::make_pair(score, devices[deviceIndex]));
		}
		if (candidates.rbegin()->first > 0)
		{
			m_PhysicalDevice = candidates.rbegin()->second;
		}
		else
		{
			ETH_CORE_ASSERT("failed to find a suitable GPU");
		}

		// Get memory properties
		vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &m_MemoryProperties);

		// Queue families
		m_QueueFamilyIndices = FindQueueFamilies(m_PhysicalDevice, m_RequestedQueueTypes);

		// Create queue infos
		QueueCreateInfo();
	}

	VulkanPhysicalDevice::~VulkanPhysicalDevice()
	{

	}

	uint32_t VulkanPhysicalDevice::rateDeviceSuitability(VkPhysicalDevice device)
	{
		int score = 0;
		VkPhysicalDeviceProperties deviceProperties;
		VkPhysicalDeviceFeatures deviceFeatures;
		vkGetPhysicalDeviceProperties(device, &deviceProperties);
		vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

		if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			score += 1000;
		}

		score += deviceProperties.limits.maxImageDimension2D;

		if (!deviceFeatures.geometryShader)
			return 0;

		QueueFamilyIndices indices = FindQueueFamilies(device, VK_QUEUE_GRAPHICS_BIT);
		
		// prefer indices.Graphic == indices.Present
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, indices.Graphics.value(), m_Surface, &presentSupport);
		ETH_CORE_ASSERT(presentSupport == VK_TRUE, "Present not support");

		// TODO: check swap chain adequate
		// bool swapChainAdequate = false;
		// if (extensionsSupported) {
		// 	SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
		// 	swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
		// }

		if (!indices.isComplete() || !deviceFeatures.samplerAnisotropy)
			return 0;
		
		return score;
	}

	VulkanPhysicalDevice::QueueFamilyIndices VulkanPhysicalDevice::FindQueueFamilies(VkPhysicalDevice device, uint32_t flags)
	{
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		QueueFamilyIndices indices;
		// Dedicated queue for compute
		// Try to find a queue family index that supports compute but not graphics
		if (flags & VK_QUEUE_COMPUTE_BIT)
		{
			for (uint32_t i = 0; i < queueFamilies.size(); i++)
			{
				auto& queueFamily = queueFamilies[i];
				if ((queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) && ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0))
				{
					indices.Compute = i;
					break;
				}
			}
		}
		// Dedicated queue for transfer
		// Try to find a queue family index that supports transfer but not graphics and compute
		if (flags & VK_QUEUE_TRANSFER_BIT)
		{
			for (uint32_t i = 0; i < queueFamilies.size(); i++)
			{
				auto& queueFamily = queueFamilies[i];
				if ((queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) && ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) && ((queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) == 0))
				{
					indices.Transfer = i;
					break;
				}
			}
		}
		// For other queue types or if no separate compute queue is present, return the first one to support the requested flags
		VkBool32 presentSupport = false;
		ETH_CORE_ASSERT(m_Surface != nullptr);
		for (uint32_t i = 0; i < queueFamilies.size(); i++)
		{
			if ((flags & VK_QUEUE_TRANSFER_BIT) && !indices.Transfer.has_value())
			{
				if (queueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
					indices.Transfer = i;
			}

			if ((flags & VK_QUEUE_COMPUTE_BIT) && !indices.Compute.has_value())
			{
				if (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
					indices.Compute = i;
			}

			if (flags & VK_QUEUE_GRAPHICS_BIT)
			{
				if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
				{
					if (!presentSupport)
					{
						vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_Surface, &presentSupport);
						if (presentSupport)
							indices.Graphics = i;
					}
				}
			}
		}

		return indices;
	}

	void VulkanPhysicalDevice::QueueCreateInfo()
	{

		std::set<uint32_t> uniqueQueueFamilies = { 
			m_QueueFamilyIndices.Graphics.value(), 
			m_QueueFamilyIndices.Compute.value(),
			m_QueueFamilyIndices.Transfer.value() 
		};

		float queuePriority(1.0f);
		for (uint32_t queueFamily : uniqueQueueFamilies) {
			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			m_QueueCreateInfos.push_back(queueCreateInfo);
		}
		
	}

	bool VulkanPhysicalDevice::IsExtensionSupported(const std::string& extensionName) const
	{
		return m_SupportedExtensions.find(extensionName) != m_SupportedExtensions.end();
	}



	////////////////////////////////////////////////////////////////////////////////////
	// Logical device 
	////////////////////////////////////////////////////////////////////////////////////
	Ref<VulkanDevice> VulkanDevice::Create(const Ref<VulkanPhysicalDevice>& physicalDevice, VkPhysicalDeviceFeatures2 enabledFeatures2)
	{
		return CreateRef<VulkanDevice>(physicalDevice, enabledFeatures2);
	}
	
	VulkanDevice::VulkanDevice(const Ref<VulkanPhysicalDevice>& physicalDevice, VkPhysicalDeviceFeatures2 enabledFeatures2)
		: m_PhysicalDevice(physicalDevice), m_EnabledFeatures2(enabledFeatures2)
	{
		std::vector<const char*> deviceExtensions;
		for (const auto& it : m_PhysicalDevice->m_UsedDeviceExtensions)
		{
			deviceExtensions.push_back(it.c_str());
		}

		VkDeviceCreateInfo deviceCreateInfo = {};
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(m_PhysicalDevice->m_QueueCreateInfos.size());
		deviceCreateInfo.pQueueCreateInfos = m_PhysicalDevice->m_QueueCreateInfos.data();

		// Enable the debug marker extension if it is present (likely meaning a debugging tool is present)
		// if (m_PhysicalDevice->IsExtensionSupported(VK_EXT_DEBUG_MARKER_EXTENSION_NAME))
		// {
		// 	deviceExtensions.push_back(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
		// 	m_EnableDebugMarkers = true;
		// }

		if (deviceExtensions.size() > 0)
		{
			deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
			deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
		}

		deviceCreateInfo.pEnabledFeatures = nullptr;
		deviceCreateInfo.pNext = &m_EnabledFeatures2;

		VK_CHECK_RESULT(vkCreateDevice(m_PhysicalDevice->GetVulkanPhysicalDevice(), &deviceCreateInfo, nullptr, &m_LogicalDevice));

		// retrieving queue handles
		vkGetDeviceQueue(m_LogicalDevice, m_PhysicalDevice->m_QueueFamilyIndices.Graphics.value(), 0, &m_GraphicsQueue);
		vkGetDeviceQueue(m_LogicalDevice, m_PhysicalDevice->m_QueueFamilyIndices.Compute.value(), 0, &m_ComputeQueue);

		// create command buffer
		VkCommandPoolCreateInfo cmdPoolInfo = {};
		cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmdPoolInfo.queueFamilyIndex = m_PhysicalDevice->m_QueueFamilyIndices.Graphics.value();
		cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		VK_CHECK_RESULT(vkCreateCommandPool(m_LogicalDevice, &cmdPoolInfo, nullptr, &m_GraphicsCommandPool));

		cmdPoolInfo.queueFamilyIndex = m_PhysicalDevice->m_QueueFamilyIndices.Compute.value();
		VK_CHECK_RESULT(vkCreateCommandPool(m_LogicalDevice, &cmdPoolInfo, nullptr, &m_ComputeCommandPool));
	}

	VulkanDevice::~VulkanDevice()
	{
	}

	void VulkanDevice::Cleanup()
	{
		// destroy command buffers
		vkDestroyCommandPool(m_LogicalDevice, m_GraphicsCommandPool, nullptr);
		vkDestroyCommandPool(m_LogicalDevice, m_ComputeCommandPool, nullptr);

		vkDeviceWaitIdle(m_LogicalDevice);

		vkDestroyDevice(m_LogicalDevice, nullptr);
	}

	VkCommandBuffer VulkanDevice::CreateCommandBuffer(QueueFamilyTypes type , bool oneTimeUse, bool begin)
	{
		VkCommandBuffer cmdBuffer;

		VkCommandBufferAllocateInfo cmdBufAllocateInfo = {};
		cmdBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdBufAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmdBufAllocateInfo.commandBufferCount = 1;

		switch (type)
		{
		case QueueFamilyTypes::Graphics: {cmdBufAllocateInfo.commandPool = m_GraphicsCommandPool; break; }
		case QueueFamilyTypes::Compute: {cmdBufAllocateInfo.commandPool = m_ComputeCommandPool; break; }
		default: 
			ETH_CORE_ASSERT(false, "No command pool");
		}

		VK_CHECK_RESULT(vkAllocateCommandBuffers(m_LogicalDevice, &cmdBufAllocateInfo, &cmdBuffer));

		if (begin)
		{
			VkCommandBufferBeginInfo cmdBufferBeginInfo{};
			cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			cmdBufferBeginInfo.flags = oneTimeUse ? VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT : 0;
			VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufferBeginInfo));
		}

		return cmdBuffer;
	}

	void VulkanDevice::SubmitCommandBuffer(VkCommandBuffer commandBuffer, QueueFamilyTypes type)
	{
		ETH_CORE_ASSERT(commandBuffer != VK_NULL_HANDLE);

		VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		// Create fence to ensure that the command buffer has finished executing
		VkFenceCreateInfo fenceCreateInfo = {};
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceCreateInfo.flags = 0;
		VkFence fence;
		VK_CHECK_RESULT(vkCreateFence(m_LogicalDevice, &fenceCreateInfo, nullptr, &fence));

		// Submit to the queue
		switch (type)
		{
		case QueueFamilyTypes::Graphics: {VK_CHECK_RESULT(vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, fence)); break; }
		case QueueFamilyTypes::Compute: {VK_CHECK_RESULT(vkQueueSubmit(m_ComputeQueue, 1, &submitInfo, fence)); break; }
		default:
			ETH_CORE_ASSERT(false, "No command pool");
		}

		// Wait for the fence to signal that command buffer has finished executing
		VK_CHECK_RESULT(vkWaitForFences(m_LogicalDevice, 1, &fence, VK_TRUE, UINT64_MAX));
		vkDestroyFence(m_LogicalDevice, fence, nullptr);

		switch (type)
		{
		case QueueFamilyTypes::Graphics: {vkFreeCommandBuffers(m_LogicalDevice, m_GraphicsCommandPool, 1, &commandBuffer); break; }
		case QueueFamilyTypes::Compute: {vkFreeCommandBuffers(m_LogicalDevice, m_ComputeCommandPool, 1, &commandBuffer); break; }
		default:
			ETH_CORE_ASSERT(false, "No command pool");
		}
	}

	VkCommandBuffer VulkanDevice::CreateSecondaryCommandBuffer()
	{
		VkCommandBuffer cmdBuffer;

		VkCommandBufferAllocateInfo cmdBufAllocateInfo = {};
		cmdBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdBufAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
		cmdBufAllocateInfo.commandBufferCount = 1;
		cmdBufAllocateInfo.commandPool = m_GraphicsCommandPool;

		VK_CHECK_RESULT(vkAllocateCommandBuffers(m_LogicalDevice, &cmdBufAllocateInfo, &cmdBuffer));
		return cmdBuffer;
	}
}
