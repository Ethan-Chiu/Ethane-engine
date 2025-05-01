#include "ethpch.h"
#include "VulkanContext.h"
#include "VulkanDevice.h"

namespace Ethane {

void VulkanPhysicalDevice::Init(VkInstance vkInstance, const ContextCreateInfo& info, VkSurfaceKHR surface)
{
    // Get compatible devices
    auto compatibleDeviceIndices = GetCompatibleDevices(vkInstance, info);
    if (compatibleDeviceIndices.empty()) {
        ETH_CORE_ERROR("No compatible device found");
    }
    
    // Get devices
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(vkInstance, &deviceCount, nullptr);
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(vkInstance, &deviceCount, devices.data());

    // Select device
    std::multimap<uint32_t, VkPhysicalDevice> candidates;
    for (const auto& deviceIndex : compatibleDeviceIndices)
    {
        int score = RateDeviceSuitability(devices[deviceIndex]);
        candidates.insert(std::make_pair(score, devices[deviceIndex]));
    }
    if (candidates.rbegin()->first >= 0)
    {
        m_PhysicalDevice = candidates.rbegin()->second;
    }
    
    ETH_CORE_ASSERT(m_PhysicalDevice != nullptr, "failed to find a suitable GPU");
    
    InitPhysicalFeatures(info);

    // Queue families
    m_QueueFamilyIndices = FindQueueFamilies(m_PhysicalDevice, m_ConstRequestedQueueTypes, surface);

    PrintSelectedDeviceInfo();
}

void VulkanPhysicalDevice::Destroy()
{
    ETH_CORE_INFO("Destroying physical device...");
    m_QueueFamilyIndices.Graphics.reset();
    m_QueueFamilyIndices.Compute.reset();
    m_QueueFamilyIndices.Transfer.reset();
    m_QueueFamilyIndices.Present.reset();
}

// Returns the list of devices or groups compatible with the mandatory extensions
std::vector<uint32_t> VulkanPhysicalDevice::GetCompatibleDevices(VkInstance vkInstance, const ContextCreateInfo& info)
{
    std::vector<uint32_t> compatibleDevices;
    
    // TODO: investigate device group
    // std::vector<VkPhysicalDeviceGroupProperties> groups;
    // if (info.useDeviceGroups)
    // {
    //     groups = getPhysicalDeviceGroups();
    //     nbElems = static_cast<uint32_t>(groups.size());
    // }
    
    // List devices
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(vkInstance, &deviceCount, nullptr);
    ETH_CORE_ASSERT(deviceCount > 0, "failed to find GPUs with Vulkan support!");
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(vkInstance, &deviceCount, devices.data());

    if (info.VerboseCompatibleDevices)
    {
        ETH_CORE_INFO("____________________");
        ETH_CORE_INFO("Compatible Devices :");
    }

    uint32_t compatible = 0;
    for (uint32_t elemId = 0; elemId < deviceCount; elemId++)
    {
        VkPhysicalDevice physicalDevice = devices[elemId]; // info.useDeviceGroups ? groups[elemId].devices[0] :

        // Note: all physical devices in a group are identical
        if (HasMandatoryExtensions(physicalDevice, info))
        {
            compatibleDevices.push_back(elemId);
            if (info.VerboseCompatibleDevices)
            {
                VkPhysicalDeviceProperties props;
                vkGetPhysicalDeviceProperties(physicalDevice, &props);
                ETH_CORE_INFO("  {0}: {1}", compatible, props.deviceName);
                compatible++;
            }
        }
        else if (info.VerboseCompatibleDevices)
        {
            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties(physicalDevice, &props);
            ETH_CORE_INFO("Skipping physical device {0}", props.deviceName);
        }
    }
    if (info.VerboseCompatibleDevices)
    {
        ETH_CORE_INFO("Physical devices found: {0}", compatible);
        if (compatible > 0)
        {
            ETH_CORE_INFO("{0}", compatible);
        }
        else
        {
            ETH_CORE_ERROR("No compatible device");
        }
    }

    return compatibleDevices;
}

// Return true if all extensions in info, marked as required are available on the physicalDevice
bool VulkanPhysicalDevice::HasMandatoryExtensions(VkPhysicalDevice device, const ContextCreateInfo& info)
{
    std::vector<VkExtensionProperties> extensionProperties;

    uint32_t count;
    VK_CHECK_RESULT(vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr));
    extensionProperties.resize(count);
    VK_CHECK_RESULT(vkEnumerateDeviceExtensionProperties(device, nullptr, &count, extensionProperties.data()));
    extensionProperties.resize(std::min(extensionProperties.size(), size_t(count)));

    return VulkanContextUtils::CheckExtensionProperties(extensionProperties, info.DeviceExtensions, info.VerboseCompatibleDevices);
}


bool VulkanPhysicalDevice::InitPhysicalFeatures(const ContextCreateInfo& info)
{
    // extensions
    uint32_t extCount = 0;
    std::vector<VkExtensionProperties> extensionProperties;
    vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &extCount, nullptr);
    if (extCount > 0)
    {
        extensionProperties.resize(extCount);
        if (vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &extCount, &extensionProperties.front()) == VK_SUCCESS)
        {
            ETH_CORE_TRACE("Selected physical device has {0} extensions", extCount);
            for (const auto& ext : extensionProperties)
            {
                m_SupportedDeviceExtensions.emplace(ext.extensionName);
                ETH_CORE_INFO("  {0}", ext.extensionName);
            }
        }
    }
    
    std::vector<void*> featureStructs;
    if (VulkanContextUtils::CheckAndFillExtensionProperties(extensionProperties, info.DeviceExtensions, m_UsedDeviceExtensions, featureStructs) != VK_SUCCESS)
    {
        ETH_CORE_ERROR("Device extensions not satisfied");
        return false;
    }
    
    if (info.VerboseUsed)
    {
        ETH_CORE_INFO("________________________");
        ETH_CORE_INFO("Used Device Extensions: ");
        for (const auto& it : m_UsedDeviceExtensions)
        {
            ETH_CORE_INFO("  {0}", it.c_str());
        }
    }
    
    // features
    VkPhysicalDeviceFeatures2   features2{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
    VkPhysicalDeviceProperties2 properties2{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };

    if (info.ApiMajor == 1 && info.ApiMinor >= 2)
    {
        features2.pNext = &m_PhysicalInfo.features11;
        m_PhysicalInfo.features11.pNext = &m_PhysicalInfo.features12;
        m_PhysicalInfo.features12.pNext = nullptr;

        m_PhysicalInfo.properties12.driverID = VK_DRIVER_ID_NVIDIA_PROPRIETARY;
        m_PhysicalInfo.properties12.supportedDepthResolveModes = VK_RESOLVE_MODE_MAX_BIT;
        m_PhysicalInfo.properties12.supportedStencilResolveModes = VK_RESOLVE_MODE_MAX_BIT;

        properties2.pNext = &m_PhysicalInfo.properties11;
        m_PhysicalInfo.properties11.pNext = &m_PhysicalInfo.properties12;
        m_PhysicalInfo.properties12.pNext = nullptr;
    }

    
    // helper struct to link extensions together
    struct ExtensionHeader
    {
        VkStructureType sType;
        void* pNext;
    };

    // use the features2 chain to append extensions' features
    if (!featureStructs.empty())
    {
        // build up chain of all used extension features
        for (size_t i = 0; i < featureStructs.size(); i++)
        {
            auto* header = reinterpret_cast<ExtensionHeader*>(featureStructs[i]);
            header->pNext = i < featureStructs.size() - 1 ? featureStructs[i + 1] : nullptr;
        }
        
        // append to the end of current feature2 struct
        ExtensionHeader* lastCoreFeature = (ExtensionHeader*)&features2;
        while (lastCoreFeature->pNext != nullptr)
        {
            lastCoreFeature = (ExtensionHeader*)lastCoreFeature->pNext;
        }
        lastCoreFeature->pNext = featureStructs[0];
    }

    // query feature support
    vkGetPhysicalDeviceFeatures2(m_PhysicalDevice, &features2);
    vkGetPhysicalDeviceProperties2(m_PhysicalDevice, &properties2);

    m_UsedDeviceFeatures = features2;
    m_PhysicalInfo.features10 = features2.features;
    m_PhysicalInfo.properties10 = properties2.properties;
    
    // disable some features
    if (info.DisableRobustBufferAccess)
    {
        m_UsedDeviceFeatures.features.robustBufferAccess = VK_FALSE;
    }
    
    return true;
}



int32_t VulkanPhysicalDevice::RateDeviceSuitability(VkPhysicalDevice device, VkSurfaceKHR surface)
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
        return -1;

    if (surface != VK_NULL_HANDLE) {
        return score;
    }
    
    QueueFamilyIndices indices = FindQueueFamilies(device, VK_QUEUE_GRAPHICS_BIT, surface);
    ETH_CORE_ASSERT(indices.Present.has_value(), "Present not support");
    // prefer indices.Graphic == indices.Present
    if (indices.Present.value() == indices.Graphics.value()) {
        score += 10;
    }

    // Check swap chain adequate
    SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(device, surface);
    bool swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();

    if (!indices.isComplete() || !deviceFeatures.samplerAnisotropy || !swapChainAdequate)
        return -1;
    
    return score;
}

	VulkanPhysicalDevice::QueueFamilyIndices VulkanPhysicalDevice::FindQueueFamilies(VkPhysicalDevice device, uint32_t flags, VkSurfaceKHR surface)
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
			for (uint32_t i = 0; i < queueFamilyCount; i++)
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
			for (uint32_t i = 0; i < queueFamilyCount; i++)
			{
				auto& queueFamily = queueFamilies[i];
				if ((queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) && ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) && ((queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) == 0))
				{
					indices.Transfer = i;
					break;
				}
			}
		}
		// Graphics queue
		// Try to find graphics queue that also support present (indices.Graphic == indices.Present)
        ETH_CORE_ASSERT(surface != nullptr);
		VkBool32 presentSupport = false;
		if (flags & VK_QUEUE_GRAPHICS_BIT)
		{
			for (uint32_t i = 0; i < queueFamilyCount; i++)
			{
				if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
				{
					if (!presentSupport)
					{
						VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport));
						if (presentSupport) {
							indices.Graphics = i;
							indices.Present = i;
						}
					}
				}
			}
		}
		// For other queue types or if no separate compute queue is present, return the first one to support the requested flags
		for (uint32_t i = 0; i < queueFamilyCount; i++)
		{
			if ((flags & VK_QUEUE_TRANSFER_BIT) && !indices.Transfer.has_value())
			{
				if (queueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT) {
					indices.Transfer = i;
					ETH_CORE_INFO("fallback transfer queue choice");
				}
			}

			if ((flags & VK_QUEUE_COMPUTE_BIT) && !indices.Compute.has_value())
			{
				if (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
					indices.Compute = i;
					ETH_CORE_INFO("fallback compute queue choice");
				}
			}

			if ((flags & VK_QUEUE_GRAPHICS_BIT) && !indices.Graphics.has_value())
			{
				if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
					indices.Graphics = i;
					ETH_CORE_INFO("fallback graphic queue choice");
				}
			}
			if (!indices.Present.has_value())
			{
				VkBool32 fallbackPresentSupport = VK_FALSE;
				VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &fallbackPresentSupport));
				if (fallbackPresentSupport) {
					indices.Present = i;
					ETH_CORE_INFO("fallback present queue choice");
				}
			}
		}

		return indices;
	}

SwapChainSupportDetails VulkanPhysicalDevice::QuerySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) const
{
    SwapChainSupportDetails details;
    
    if (surface == VK_NULL_HANDLE) {
        ETH_CORE_ERROR("Query swapchain support got surface := VK_NULL_HANDLE");
        return details;
    }
    
    // query capabilities
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    // query formats
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
    ETH_CORE_ASSERT(formatCount > 0, "");
    details.formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());

    // query present mode
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
    details.presentModes.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());

    return details;
}

	void VulkanPhysicalDevice::PrintSelectedDeviceInfo() 
	{
        if (m_PhysicalDevice == VK_NULL_HANDLE) {
            ETH_CORE_WARN("Physical device not initialized!");
            return;
        }
		// Get memory properties & properties & feature of the selected device
		VkPhysicalDeviceMemoryProperties memories;
		VkPhysicalDeviceProperties2 properties2{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
		vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &memories);
		vkGetPhysicalDeviceProperties2(m_PhysicalDevice, &properties2);

		VkPhysicalDeviceProperties properties = properties2.properties;
		ETH_CORE_INFO("____________________");
		ETH_CORE_INFO("SELECTED DEVICE: {0}", properties.deviceName);
		ETH_CORE_INFO("queue family indices: Graphics-{0} | Present-{1} | Compute-{2} | Transfer-{3}", 
			m_QueueFamilyIndices.Graphics.value(), 
			m_QueueFamilyIndices.Present.value(),
			m_QueueFamilyIndices.Compute.value(),
			m_QueueFamilyIndices.Transfer.value());

		switch (properties.deviceType) {
		default:
		case VK_PHYSICAL_DEVICE_TYPE_OTHER:
			ETH_CORE_INFO("GPU type is Unknown.");
			break;
		case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
			ETH_CORE_INFO("GPU type is Integrated.");
			break;
		case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
			ETH_CORE_INFO("GPU type is Descrete.");
			break;
		case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
			ETH_CORE_INFO("GPU type is Virtual.");
			break;
		case VK_PHYSICAL_DEVICE_TYPE_CPU:
			ETH_CORE_INFO("GPU type is CPU.");
			break;
		}

		ETH_CORE_INFO(
			"GPU Driver version: {0}.{1}.{2}",
			VK_VERSION_MAJOR(properties.driverVersion),
			VK_VERSION_MINOR(properties.driverVersion),
			VK_VERSION_PATCH(properties.driverVersion));

		// Vulkan API version.
		ETH_CORE_INFO(
			"Vulkan API version: {0}.{1}.{2}",
			VK_VERSION_MAJOR(properties.apiVersion),
			VK_VERSION_MINOR(properties.apiVersion),
			VK_VERSION_PATCH(properties.apiVersion));

		// Memory information
		for (uint32_t i = 0; i < memories.memoryHeapCount; ++i) {
			double memory_size_gib = (((double)memories.memoryHeaps[i].size) / 1024.0f / 1024.0f / 1024.0f);
			if (memories.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
				ETH_CORE_INFO("Local GPU memory: {0} GiB", std::round(memory_size_gib * 100.0) / 100.0);
			}
			else {
				ETH_CORE_INFO("Shared System memory: {0} GiB", std::round(memory_size_gib * 100.0) / 100.0);
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////////////
	// Logical device 
	////////////////////////////////////////////////////////////////////////////////////
	VulkanDevice::VulkanDevice(const VulkanPhysicalDevice& physicalDevice)
        :m_PhysicalDevice(&physicalDevice)
	{
		QueueCreateInfo();

		VkDeviceCreateInfo deviceCreateInfo = {};
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(m_QueueCreateInfos.size());
		deviceCreateInfo.pQueueCreateInfos = m_QueueCreateInfos.data();

		std::vector<const char*> deviceExtensions;
        for (const auto& it : m_PhysicalDevice->m_UsedDeviceExtensions)
		{
			deviceExtensions.push_back(it.c_str());
		}
		if (deviceExtensions.size() > 0)
		{
			deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
			deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
		}

		deviceCreateInfo.pEnabledFeatures = nullptr;
        deviceCreateInfo.pNext = &m_PhysicalDevice->m_UsedDeviceFeatures;

		VK_CHECK_RESULT(vkCreateDevice(m_PhysicalDevice->GetVulkanPhysicalDevice(), &deviceCreateInfo, nullptr, &m_LogicalDevice));

		// retrieving queue handles
		vkGetDeviceQueue(m_LogicalDevice, m_PhysicalDevice->m_QueueFamilyIndices.Graphics.value(), 0, &m_GraphicsQueue);
		vkGetDeviceQueue(m_LogicalDevice, m_PhysicalDevice->m_QueueFamilyIndices.Compute.value(), 0, &m_ComputeQueue);
		vkGetDeviceQueue(m_LogicalDevice, m_PhysicalDevice->m_QueueFamilyIndices.Transfer.value(), 0, &m_TransferQueue);

		// create command pool
		VkCommandPoolCreateInfo cmdPoolInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
		cmdPoolInfo.queueFamilyIndex = m_PhysicalDevice->m_QueueFamilyIndices.Graphics.value();
		cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		VK_CHECK_RESULT(vkCreateCommandPool(m_LogicalDevice, &cmdPoolInfo, nullptr, &m_GraphicsCommandPool));

		cmdPoolInfo.queueFamilyIndex = m_PhysicalDevice->m_QueueFamilyIndices.Compute.value();
		VK_CHECK_RESULT(vkCreateCommandPool(m_LogicalDevice, &cmdPoolInfo, nullptr, &m_ComputeCommandPool));
	}

	void VulkanDevice::Destroy()
	{
		// destroy command pools
		vkDestroyCommandPool(m_LogicalDevice, m_GraphicsCommandPool, nullptr);
		vkDestroyCommandPool(m_LogicalDevice, m_ComputeCommandPool, nullptr);

		// reset queue
		m_GraphicsQueue = nullptr;
		m_ComputeQueue = nullptr;
		m_TransferQueue = nullptr;

		ETH_CORE_INFO("Destroying logical device...");
		vkDeviceWaitIdle(m_LogicalDevice);
		vkDestroyDevice(m_LogicalDevice, nullptr);
	}

	void VulkanDevice::QueueCreateInfo()
	{
		VulkanPhysicalDevice::QueueFamilyIndices queueFamilyIndices = m_PhysicalDevice->GetQueueFamilyIndices();
		std::set<uint32_t> uniqueQueueFamilies = {
			queueFamilyIndices.Present.value(),
			queueFamilyIndices.Graphics.value(),
			queueFamilyIndices.Compute.value(),
			queueFamilyIndices.Transfer.value()
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
}
