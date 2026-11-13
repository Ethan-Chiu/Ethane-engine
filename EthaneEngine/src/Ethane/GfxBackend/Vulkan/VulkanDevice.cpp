#include "ethpch.h"
#include "VulkanContext.h"
#include "VulkanUtils.h"

namespace Ethane {

void VulkanPhysicalDevice::Init(const vk::raii::Instance& instance, const ContextCreateInfo& info, vk::SurfaceKHR surface)
{
    ETH_CORE_ASSERT(info.ApiMajor > 1 || info.ApiMinor >= 2, "Ethane requires Vulkan 1.2+ (core 1.1/1.2 feature chains)");

    // Get compatible devices
    std::vector<vk::raii::PhysicalDevice> compatibleDevices = GetCompatibleDevices(instance, info);
    if (compatibleDevices.empty()) {
        ETH_CORE_ERROR("No compatible device found");
        return;
    }

    // Select device: highest non-negative score wins
    int32_t bestScore = -1;
    for (const auto& device : compatibleDevices)
    {
        int32_t score = RateDeviceSuitability(device, surface);
        if (score > bestScore)
        {
            bestScore = score;
            m_PhysicalDevice = device;
        }
    }
    
    ETH_CORE_ASSERT(IsInitialized(), "failed to find a suitable GPU");
    if (!IsInitialized()) {
        return;
    }
    
    InitPhysicalFeatures(info);
    m_DepthFormat = FindDepthFormat();
    ETH_CORE_ASSERT(m_DepthFormat != vk::Format::eUndefined, "Device supports no depth-stencil attachment format");
    ETH_CORE_INFO("Default depth format: {0}", vk::to_string(m_DepthFormat));

    // Queue families
    m_QueueFamilyIndices = FindQueueFamilies(m_PhysicalDevice, m_ConstRequestedQueueTypes, surface);

    PrintSelectedDeviceInfo();
}

void VulkanPhysicalDevice::Destroy()
{
    ETH_CORE_INFO("Destroying physical device...");
    m_QueueFamilyIndices = {};
    m_DepthFormat = vk::Format::eUndefined;
    m_SupportedDeviceExtensions.clear();
    m_UsedDeviceExtensions.clear();
    m_PhysicalDevice = nullptr; // not an owned object, just forget the handle
}

// Returns the list of devices compatible with the mandatory extensions
std::vector<vk::raii::PhysicalDevice> VulkanPhysicalDevice::GetCompatibleDevices(const vk::raii::Instance& instance, const ContextCreateInfo& info)
{
    std::vector<vk::raii::PhysicalDevice> compatibleDevices;
    
    // TODO: investigate device group (instance.enumeratePhysicalDeviceGroups())
    
    // List devices
    vk::raii::PhysicalDevices devices(instance);
    ETH_CORE_ASSERT(!devices.empty(), "failed to find GPUs with Vulkan support!");

    if (info.VerboseCompatibleDevices)
    {
        ETH_CORE_INFO("____________________");
        ETH_CORE_INFO("Compatible Devices :");
    }

    for (const auto& physicalDevice : devices)
    {
        const vk::PhysicalDeviceProperties props = physicalDevice.getProperties();
        if (HasMandatoryExtensions(physicalDevice, info))
        {
            if (info.VerboseCompatibleDevices)
            {
                ETH_CORE_INFO("  {0}: {1}", compatibleDevices.size(), props.deviceName.data());
            }
            compatibleDevices.push_back(physicalDevice);
        }
        else if (info.VerboseCompatibleDevices)
        {
            ETH_CORE_INFO("Skipping physical device {0}", props.deviceName.data());
        }
    }

    if (info.VerboseCompatibleDevices)
    {
        ETH_CORE_INFO("Physical devices found: {0}", compatibleDevices.size());
        if (compatibleDevices.empty())
        {
            ETH_CORE_ERROR("No compatible device");
        }
    }

    return compatibleDevices;
}

// Return true if all extensions in info, marked as required are available on the physicalDevice
bool VulkanPhysicalDevice::HasMandatoryExtensions(const vk::raii::PhysicalDevice& device, const ContextCreateInfo& info)
{
    const std::vector<vk::ExtensionProperties> extensionProperties = device.enumerateDeviceExtensionProperties();
    return VulkanContextUtils::CheckExtensionProperties(extensionProperties, info.DeviceExtensions, info.VerboseCompatibleDevices);
}


bool VulkanPhysicalDevice::SupportsFormat(vk::Format format, vk::FormatFeatureFlags features, vk::ImageTiling tiling) const
{
    const vk::FormatProperties properties = m_PhysicalDevice.getFormatProperties(format);
    const vk::FormatFeatureFlags supported = (tiling == vk::ImageTiling::eLinear) ? properties.linearTilingFeatures : properties.optimalTilingFeatures;
    return (supported & features) == features;
}

vk::Format VulkanPhysicalDevice::FindDepthFormat() const
{
    // Depth-stencil first (ImageFormat::Depth promises a stencil when possible), smallest first.
    // D24S8 is missing on Apple GPUs, D32S8 is missing on some older desktop GPUs; D32 is the last resort.
    constexpr std::array candidates = {
        vk::Format::eD24UnormS8Uint,
        vk::Format::eD32SfloatS8Uint,
        vk::Format::eD32Sfloat,
    };
    for (const vk::Format format : candidates) {
        if (SupportsFormat(format, vk::FormatFeatureFlagBits::eDepthStencilAttachment)) {
            return format;
        }
    }
    return vk::Format::eUndefined;
}

bool VulkanPhysicalDevice::InitPhysicalFeatures(const ContextCreateInfo& info)
{
    // extensions
    const std::vector<vk::ExtensionProperties> extensionProperties = m_PhysicalDevice.enumerateDeviceExtensionProperties();
    ETH_CORE_TRACE("Selected physical device has {0} extensions", extensionProperties.size());
    for (const auto& ext : extensionProperties)
    {
        m_SupportedDeviceExtensions.emplace(ext.extensionName.data());
        ETH_CORE_INFO("  {0}", ext.extensionName.data());
    }
    
    if (VulkanContextUtils::CheckAndFillExtensionProperties(extensionProperties, info.DeviceExtensions, m_UsedDeviceExtensions) != VK_SUCCESS)
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

    // Build the feature chain: drop the structs of extensions we aren't enabling.
    // (Querying a struct of an extension the device doesn't support is invalid, so unlink *before* querying.)
    FeatureChain features;
    if (!IsExtensionEnabled(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME))
        features.unlink<vk::PhysicalDeviceAccelerationStructureFeaturesKHR>();
    if (!IsExtensionEnabled(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME))
        features.unlink<vk::PhysicalDeviceRayTracingPipelineFeaturesKHR>();

    // Query support for exactly the structs still linked. The templated raii getFeatures2<...>() can't be used:
    // it always queries every struct in the chain. So use the plain-handle overload that fills a chain we
    // built, dispatched through the raii-loaded function table like every other raii call.
    const vk::PhysicalDevice handle = *m_PhysicalDevice;
    const auto& dispatcher = *m_PhysicalDevice.getDispatcher();
    handle.getFeatures2(&features.get<vk::PhysicalDeviceFeatures2>(), dispatcher);
    m_PhysicalInfo.Features = features; // copy keeps the same linked/unlinked layout
    m_PhysicalInfo.Properties = m_PhysicalDevice.getProperties2<vk::PhysicalDeviceProperties2,
                                                                vk::PhysicalDeviceVulkan11Properties,
                                                                vk::PhysicalDeviceVulkan12Properties>();

    // enable everything that is supported, minus what we explicitly turn off
    m_EnabledFeatures = m_PhysicalInfo.Features;
    if (info.DisableRobustBufferAccess)
    {
        m_EnabledFeatures.get<vk::PhysicalDeviceFeatures2>().features.robustBufferAccess = vk::False;
    }
    
    return true;
}

bool VulkanPhysicalDevice::IsExtensionEnabled(const char* name) const
{
    return std::find(m_UsedDeviceExtensions.begin(), m_UsedDeviceExtensions.end(), name) != m_UsedDeviceExtensions.end();
}


int32_t VulkanPhysicalDevice::RateDeviceSuitability(const vk::raii::PhysicalDevice& device, vk::SurfaceKHR surface)
{
    const vk::PhysicalDeviceProperties deviceProperties = device.getProperties();
    const vk::PhysicalDeviceFeatures deviceFeatures = device.getFeatures();

    // hard requirements
    if (deviceProperties.apiVersion < VK_API_VERSION_1_2)
        return -1;
    if (!deviceFeatures.samplerAnisotropy)
        return -1;

    int32_t score = 0;
    if (deviceProperties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
        score += 1000;
    score += static_cast<int32_t>(deviceProperties.limits.maxImageDimension2D);
    // geometry shaders are not required (MoltenVK doesn't support them), just preferred
    if (deviceFeatures.geometryShader)
        score += 100;

    // without a surface we can't judge presentation, rate on properties only
    if (!surface) {
        return score;
    }
    
    QueueFamilyIndices indices = FindQueueFamilies(device, vk::QueueFlagBits::eGraphics, surface);
    if (!indices.isComplete())
        return -1;
    // prefer indices.Graphic == indices.Present
    if (indices.Present.value() == indices.Graphics.value()) {
        score += 10;
    }

    // Check swap chain adequate
    SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(device, surface);
    bool swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    if (!swapChainAdequate)
        return -1;
    
    return score;
}

VulkanPhysicalDevice::QueueFamilyIndices VulkanPhysicalDevice::FindQueueFamilies(const vk::raii::PhysicalDevice& device, vk::QueueFlags flags, vk::SurfaceKHR surface)
{
    using QF = vk::QueueFlagBits;
    const std::vector<vk::QueueFamilyProperties> queueFamilies = device.getQueueFamilyProperties();
    const uint32_t queueFamilyCount = static_cast<uint32_t>(queueFamilies.size());

    QueueFamilyIndices indices;
    // Dedicated queue for compute
    // Try to find a queue family index that supports compute but not graphics
    if (flags & QF::eCompute)
    {
        for (uint32_t i = 0; i < queueFamilyCount; i++)
        {
            const auto queueFlags = queueFamilies[i].queueFlags;
            if ((queueFlags & QF::eCompute) && !(queueFlags & QF::eGraphics))
            {
                indices.Compute = i;
                break;
            }
        }
    }
    // Dedicated queue for transfer
    // Try to find a queue family index that supports transfer but not graphics and compute
    if (flags & QF::eTransfer)
    {
        for (uint32_t i = 0; i < queueFamilyCount; i++)
        {
            const auto queueFlags = queueFamilies[i].queueFlags;
            if ((queueFlags & QF::eTransfer) && !(queueFlags & QF::eGraphics) && !(queueFlags & QF::eCompute))
            {
                indices.Transfer = i;
                break;
            }
        }
    }
    // Graphics queue
    // Try to find graphics queue that also support present (indices.Graphic == indices.Present)
    ETH_CORE_ASSERT(surface, "FindQueueFamilies needs a surface to determine present support");
    if (flags & QF::eGraphics)
    {
        for (uint32_t i = 0; i < queueFamilyCount; i++)
        {
            if ((queueFamilies[i].queueFlags & QF::eGraphics) && device.getSurfaceSupportKHR(i, surface))
            {
                indices.Graphics = i;
                indices.Present = i;
                break;
            }
        }
    }
    // For other queue types or if no separate compute queue is present, return the first one to support the requested flags
    for (uint32_t i = 0; i < queueFamilyCount; i++)
    {
        const auto queueFlags = queueFamilies[i].queueFlags;
        if ((flags & QF::eTransfer) && !indices.Transfer.has_value() && (queueFlags & QF::eTransfer))
        {
            indices.Transfer = i;
            ETH_CORE_INFO("fallback transfer queue choice");
        }

        if ((flags & QF::eCompute) && !indices.Compute.has_value() && (queueFlags & QF::eCompute))
        {
            indices.Compute = i;
            ETH_CORE_INFO("fallback compute queue choice");
        }

        if ((flags & QF::eGraphics) && !indices.Graphics.has_value() && (queueFlags & QF::eGraphics))
        {
            indices.Graphics = i;
            ETH_CORE_INFO("fallback graphic queue choice");
        }

        if (!indices.Present.has_value() && device.getSurfaceSupportKHR(i, surface))
        {
            indices.Present = i;
            ETH_CORE_INFO("fallback present queue choice");
        }
    }

    return indices;
}

SwapChainSupportDetails VulkanPhysicalDevice::QuerySwapChainSupport(const vk::raii::PhysicalDevice& device, vk::SurfaceKHR surface) const
{
    SwapChainSupportDetails details;
    
    if (!surface) {
        ETH_CORE_ERROR("Query swapchain support got surface := VK_NULL_HANDLE");
        return details;
    }
    
    details.capabilities = device.getSurfaceCapabilitiesKHR(surface);
    details.formats      = device.getSurfaceFormatsKHR(surface);
    details.presentModes = device.getSurfacePresentModesKHR(surface);
    ETH_CORE_ASSERT(!details.formats.empty(), "");

    return details;
}

void VulkanPhysicalDevice::PrintSelectedDeviceInfo() const
{
    if (!IsInitialized()) {
        ETH_CORE_WARN("Physical device not initialized!");
        return;
    }
    // Get memory properties & properties of the selected device
    const vk::PhysicalDeviceMemoryProperties memories = m_PhysicalDevice.getMemoryProperties();
    const vk::PhysicalDeviceProperties& properties = m_PhysicalInfo.Properties.get<vk::PhysicalDeviceProperties2>().properties;

    ETH_CORE_INFO("____________________");
    ETH_CORE_INFO("SELECTED DEVICE: {0}", properties.deviceName.data());
    ETH_CORE_INFO("queue family indices: Graphics-{0} | Present-{1} | Compute-{2} | Transfer-{3}",
        m_QueueFamilyIndices.Graphics.value(),
        m_QueueFamilyIndices.Present.value(),
        m_QueueFamilyIndices.Compute.value(),
        m_QueueFamilyIndices.Transfer.value());

    switch (properties.deviceType) {
    default:
    case vk::PhysicalDeviceType::eOther:
        ETH_CORE_INFO("GPU type is Unknown.");
        break;
    case vk::PhysicalDeviceType::eIntegratedGpu:
        ETH_CORE_INFO("GPU type is Integrated.");
        break;
    case vk::PhysicalDeviceType::eDiscreteGpu:
        ETH_CORE_INFO("GPU type is Descrete.");
        break;
    case vk::PhysicalDeviceType::eVirtualGpu:
        ETH_CORE_INFO("GPU type is Virtual.");
        break;
    case vk::PhysicalDeviceType::eCpu:
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
        if (memories.memoryHeaps[i].flags & vk::MemoryHeapFlagBits::eDeviceLocal) {
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
    : m_PhysicalDevice(&physicalDevice)
{
    const auto& indices = m_PhysicalDevice->GetQueueFamilyIndices();

    // One queue per unique family. The priority must stay alive until the device is created.
    static constexpr float queuePriority = 1.0f;
    const std::set<uint32_t> uniqueQueueFamilies = {
        indices.Present.value(),
        indices.Graphics.value(),
        indices.Compute.value(),
        indices.Transfer.value()
    };
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    for (uint32_t queueFamily : uniqueQueueFamilies)
    {
        queueCreateInfos.push_back(vk::DeviceQueueCreateInfo{
            .queueFamilyIndex = queueFamily,
            .queueCount       = 1,
            .pQueuePriorities = &queuePriority});
    }

    std::vector<const char*> deviceExtensions;
    for (const auto& it : m_PhysicalDevice->m_UsedDeviceExtensions)
    {
        deviceExtensions.push_back(it.c_str());
    }

    // Features are enabled through the pNext chain (Features2 + 1.1/1.2 + extension structs), so pEnabledFeatures stays null
    vk::DeviceCreateInfo deviceCreateInfo{
        .pNext                   = &m_PhysicalDevice->GetEnabledFeatures().get<vk::PhysicalDeviceFeatures2>(),
        .queueCreateInfoCount    = static_cast<uint32_t>(queueCreateInfos.size()),
        .pQueueCreateInfos       = queueCreateInfos.data(),
        .enabledExtensionCount   = static_cast<uint32_t>(deviceExtensions.size()),
        .ppEnabledExtensionNames = deviceExtensions.data(),
        .pEnabledFeatures        = nullptr};

    try {
        m_LogicalDevice = vk::raii::Device(m_PhysicalDevice->Get(), deviceCreateInfo);
    } catch (const vk::SystemError& err) {
        ETH_CORE_ERROR("Failed to create logical device: {0}", err.what());
        throw;
    }

    // retrieving queue handles
    m_GraphicsQueue = m_LogicalDevice.getQueue(indices.Graphics.value(), 0);
    m_PresentQueue  = m_LogicalDevice.getQueue(indices.Present.value(), 0);
    m_ComputeQueue  = m_LogicalDevice.getQueue(indices.Compute.value(), 0);
    m_TransferQueue = m_LogicalDevice.getQueue(indices.Transfer.value(), 0);

    // create command pools
    m_GraphicsCommandPool = m_LogicalDevice.createCommandPool(vk::CommandPoolCreateInfo{
        .flags            = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = indices.Graphics.value()});
    m_ComputeCommandPool = m_LogicalDevice.createCommandPool(vk::CommandPoolCreateInfo{
        .flags            = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = indices.Compute.value()});
}

void VulkanDevice::WaitIdle() const
{
    if (!IsInitialized()) {
        return;
    }
    m_LogicalDevice.waitIdle();
    m_DeletionQueue->Flush();
}

void VulkanDevice::Destroy()
{
    if (!IsInitialized()) {
        return;
    }

    // Retired objects belong to resources already released: destroy them while the device is still alive
    WaitIdle();

    if (const int live = VulkanLiveResource::Count(); live != 0) {
        ETH_CORE_ERROR("{0} Vulkan resource(s) still alive at device destruction! Release every buffer/shader/pipeline/window target before GfxBackendAPI::Shutdown().", live);
        ETH_CORE_ASSERT(false, "Vulkan resources outlive the logical device");
    }

    m_GraphicsCommandPool = nullptr;
    m_ComputeCommandPool  = nullptr;

    // queues aren't owned objects, just drop the handles
    m_GraphicsQueue = nullptr;
    m_PresentQueue  = nullptr;
    m_ComputeQueue  = nullptr;
    m_TransferQueue = nullptr;

    ETH_CORE_INFO("Destroying logical device...");
    m_LogicalDevice = nullptr;
}

const vk::raii::Queue& VulkanDevice::GetQueue(QueueFamilyTypes type) const
{
    switch (type)
    {
    case QueueFamilyTypes::Graphics: return m_GraphicsQueue;
    case QueueFamilyTypes::Compute:  return m_ComputeQueue;
    default: break;
    }
    ETH_CORE_ASSERT(false, "No queue for this queue type, falling back to graphics");
    return m_GraphicsQueue;
}

const vk::raii::CommandPool& VulkanDevice::GetCommandPool(QueueFamilyTypes type) const
{
    switch (type)
    {
    case QueueFamilyTypes::Graphics: return m_GraphicsCommandPool;
    case QueueFamilyTypes::Compute:  return m_ComputeCommandPool;
    default: break;
    }
    ETH_CORE_ASSERT(false, "No command pool for this queue type, falling back to graphics");
    return m_GraphicsCommandPool;
}

uint32_t VulkanDevice::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const
{
    const vk::PhysicalDeviceMemoryProperties memProperties = m_PhysicalDevice->Get().getMemoryProperties();
    const vk::MemoryPropertyFlags wanted(properties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1u << i)) && (memProperties.memoryTypes[i].propertyFlags & wanted) == wanted) {
            return i;
        }
    }

    ETH_CORE_ASSERT(false, "Memory type not found");
    return UINT32_MAX; // invalid index: allocation fails loudly instead of silently using memory type 0
}
}
