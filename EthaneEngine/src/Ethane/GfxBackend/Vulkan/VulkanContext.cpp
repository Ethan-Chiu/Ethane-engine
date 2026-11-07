//
//  VulkanContext.cpp
//  EthaneEngine
//
//  Created by 邱奕翔 on 2025/4/28.
//
#include "ethpch.h"
#include "VulkanContext.h"
#include "VulkanUtils.h"
#include <GLFW/glfw3.h>

namespace Ethane {

//--------------------------------------------------------------------------------------------------
//
void ContextCreateInfo::AddInstanceLayer(const char* name, bool optional)
{
    InstanceLayers.emplace_back(name, optional);
}

void ContextCreateInfo::AddInstanceExtension(const char* name, bool optional)
{
    InstanceExtensions.emplace_back(name, optional);
}

void ContextCreateInfo::AddDeviceExtension(const char* name, bool optional, uint32_t version)
{
    DeviceExtensions.emplace_back(name, optional, version);
}

void ContextCreateInfo::RemoveInstanceLayer(const char* name)
{
    for (size_t i = 0; i < InstanceLayers.size(); i++)
    {
        if (strcmp(InstanceLayers[i].name.c_str(), name) == 0)
        {
            InstanceLayers.erase(InstanceLayers.begin() + i);
        }
    }
}
void ContextCreateInfo::RemoveInstanceExtension(const char* name)
{
    for (size_t i = 0; i < InstanceExtensions.size(); i++)
    {
        if (strcmp(InstanceExtensions[i].name.c_str(), name) == 0)
        {
            InstanceExtensions.erase(InstanceExtensions.begin() + i);
        }
    }
}

void ContextCreateInfo::RemoveDeviceExtension(const char* name)
{
    for (size_t i = 0; i < DeviceExtensions.size(); i++)
    {
        if (strcmp(DeviceExtensions[i].name.c_str(), name) == 0)
        {
            DeviceExtensions.erase(DeviceExtensions.begin() + i);
        }
    }
}
//--------------------------------------------------------------------------------------------------


static VKAPI_ATTR vk::Bool32 VKAPI_CALL VulkanDebugCallback(
    vk::DebugUtilsMessageSeverityFlagBitsEXT message_severity,
    vk::DebugUtilsMessageTypeFlagsEXT message_types,
    const vk::DebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data) {
    switch (message_severity) {
    default:
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
        ETH_CORE_ERROR(callback_data->pMessage);
        break;
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
        ETH_CORE_WARN(callback_data->pMessage);
        break;
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
        ETH_CORE_INFO(callback_data->pMessage);
        break;
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose:
        ETH_CORE_TRACE(callback_data->pMessage);
        break;
    }
    return vk::False;
}

void VulkanContext::Init(ContextCreateInfo& info)
{
    m_ContextInfo = info;
    ETH_CORE_ASSERT(glfwVulkanSupported(), "GLFW vulkan support error");
    
    //--------------------------------------------------------------------------------------------------
    // Setup instance extensions/layers & device extension
    uint32_t required_extension_count = 0;
    const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&required_extension_count);
    
    for (uint8_t i = 0; i < required_extension_count; i++)
    {
        ETH_CORE_TRACE("{0} is needed by glfw", glfw_extensions[i]);
        m_ContextInfo.AddInstanceExtension(glfw_extensions[i]);
    }
    m_ContextInfo.AddDeviceExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    m_ContextInfo.AddDeviceExtension("VK_KHR_portability_subset", true);
    // Add this for MacOS
    m_ContextInfo.AddInstanceExtension(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME, true);
    if (m_ContextInfo.Validation)
    {
        m_ContextInfo.AddInstanceExtension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, true);
        m_ContextInfo.AddInstanceExtension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
        const char* validationLayerName = "VK_LAYER_KHRONOS_validation";
        m_ContextInfo.AddInstanceLayer(validationLayerName);
        const char* fpsLayerName = "VK_LAYER_LUNARG_monitor";
        m_ContextInfo.AddInstanceLayer(fpsLayerName, true);
    }
    // RayTracing: Activate the ray tracing extension
    // NOTE: their feature structs are part of VulkanPhysicalDevice::FeatureChain
    m_ContextInfo.AddDeviceExtension(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, true);  // To build acceleration structures
    m_ContextInfo.AddDeviceExtension(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME, true);  // To use vkCmdTraceRaysKHR
    m_ContextInfo.AddDeviceExtension(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME, true);  // Required by ray tracing pipeline
    
    // NOTE: use this to enable validation features
    VkValidationFeatureEnableEXT enables[] = { VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT };
    VkValidationFeaturesEXT features = { VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT };
    features.enabledValidationFeatureCount = 1;
    features.pEnabledValidationFeatures = enables;
    // contextCreateInfo.InstanceCreateInfoExt = &features;

    //--------------------------------------------------------------------------------------------------
    // Create Instance
    if (!InitInstance()) {
        ETH_CORE_ERROR("Instance creation failed!");
    }

    //--------------------------------------------------------------------------------------------------
    // Physical device & logical device
    // if (!InitDevice(contextCreateInfo)) {
    //     ETH_CORE_ERROR("Device initialization failed");
    // }
}

void VulkanContext::Destroy()
{
    vkDeviceWaitIdle(m_Device.GetHandle());

    m_Device.Destroy();
    m_PhysicalDevice.Destroy();

    // RAII handles: assigning nullptr destroys the underlying object. Order matters.
    ETH_CORE_INFO("Destroying Vulkan debugger...");
    m_DebugMessenger = nullptr;

    ETH_CORE_INFO("Destroying Vulkan instance...");
    m_VulkanInstance = nullptr;
}

bool VulkanContext::InitInstance()
{
    auto& info = m_ContextInfo;
    // Application Info
    vk::ApplicationInfo appInfo{
        .pApplicationName   = info.AppTitle.c_str(),
        .applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
        .pEngineName        = info.AppEngine.c_str(),
        .engineVersion      = VK_MAKE_API_VERSION(0, 1, 0, 0),
        .apiVersion         = VK_MAKE_API_VERSION(0, info.ApiMajor, info.ApiMinor, 0)};

    if (info.VerboseUsed)
    {
        uint32_t version = m_Context.enumerateInstanceVersion();
        ETH_CORE_INFO("_______________");
        ETH_CORE_INFO("Vulkan Version:");
        ETH_CORE_INFO(" - available:  {0}.{1}.{2}", VK_VERSION_MAJOR(version), VK_VERSION_MINOR(version), VK_VERSION_PATCH(version));
        ETH_CORE_INFO(" - requesting: {0}.{1}", info.ApiMajor, info.ApiMinor);
    }

    {
        // Get all layers
        // Check if this layer is available at instance level
        std::vector<vk::LayerProperties> instanceLayerProperties = m_Context.enumerateInstanceLayerProperties();

        if (VulkanContextUtils::CheckAndFillLayerProperties(instanceLayerProperties, info.InstanceLayers, m_UsedInstanceLayers) != VK_SUCCESS)
        {
            return false;
        }
            
        // Log available properties
        if (info.VerboseAvailable)
        {
            ETH_CORE_TRACE("Vulkan Instance Layers:");
            for (const vk::LayerProperties& layer : instanceLayerProperties)
            {
                ETH_CORE_TRACE("  {0}", layer.layerName.data());
            }
        }
    }

    {
        // Get all instance extensions
        std::vector<vk::ExtensionProperties> extensionProperties = m_Context.enumerateInstanceExtensionProperties();

        if (VulkanContextUtils::CheckAndFillExtensionProperties(extensionProperties, info.InstanceExtensions, m_UsedInstanceExtensions) != VK_SUCCESS)
        {
            return false;
        }

        if (info.VerboseAvailable)
        {
            ETH_CORE_INFO("Available Instance Extensions :");
            for (const auto& extension : extensionProperties)
            {
                ETH_CORE_INFO("  {0} (v. {1})", extension.extensionName.data(), extension.specVersion);
            }
        }
    }


    if (info.VerboseUsed)
    {
        ETH_CORE_INFO("______________________");
        ETH_CORE_INFO("Used Instance Layers :");
        for (const auto& it : m_UsedInstanceLayers)
        {
            ETH_CORE_INFO("  {0}", it.c_str());
        }
        ETH_CORE_INFO("Used Instance Extensions :");
        for (const auto& it : m_UsedInstanceExtensions)
        {
            ETH_CORE_INFO("  {0}", it.c_str());
        }
        ETH_CORE_INFO("______________________");
    }


    std::vector<const char*> usedInstanceExtensions;
    std::vector<const char*> usedInstanceLayers;
    for (const auto& it : m_UsedInstanceExtensions)
    {
        usedInstanceExtensions.push_back(it.c_str());
    }
    for (const auto& it : m_UsedInstanceLayers)
    {
        usedInstanceLayers.push_back(it.c_str());
    }

    vk::InstanceCreateInfo createInfo{
        .pNext                   = info.InstanceCreateInfoExt,
        .flags                   = vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR,
        .pApplicationInfo        = &appInfo,
        .enabledLayerCount       = static_cast<uint32_t>(usedInstanceLayers.size()),
        .ppEnabledLayerNames     = usedInstanceLayers.data(),
        .enabledExtensionCount   = static_cast<uint32_t>(usedInstanceExtensions.size()),
        .ppEnabledExtensionNames = usedInstanceExtensions.data()};

    try {
        m_VulkanInstance = vk::raii::Instance(m_Context, createInfo);
    } catch (const vk::SystemError& err) {
        ETH_CORE_ERROR("Failed to create Vulkan instance: {0}", err.what());
        return false;
    }

    // Debug messenger
    for (const auto& it : usedInstanceExtensions)
    {
        if (strcmp(it, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0)
        {
            InitDebugUtils();
            break;
        }
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
// Initialize device
//
bool VulkanContext::InitDevice(VkSurfaceKHR surface)
{
    ETH_CORE_ASSERT(m_VulkanInstance != nullptr);
    m_PhysicalDevice.Init(m_VulkanInstance, m_ContextInfo, surface);
    m_Device = VulkanDevice(m_PhysicalDevice);
    return true;
}

//--------------------------------------------------------------------------------------------------
// Utility function
//
//--------------------------------------------------------------------------------------------------
// Debug messenger
void VulkanContext::InitDebugUtils() {
    using Severity = vk::DebugUtilsMessageSeverityFlagBitsEXT;
    using Type     = vk::DebugUtilsMessageTypeFlagBitsEXT;

    vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo{
        .messageSeverity = Severity::eError | Severity::eWarning, // | Severity::eInfo | Severity::eVerbose
        .messageType     = Type::eGeneral | Type::ePerformance | Type::eValidation,
        .pfnUserCallback = &VulkanDebugCallback};

    m_DebugMessenger = m_VulkanInstance.createDebugUtilsMessengerEXT(debugCreateInfo);
    ETH_CORE_INFO("Vulkan debugger created.");
}


namespace VulkanContextUtils{

VkResult CheckAndFillLayerProperties(const std::vector<vk::LayerProperties>& properties,
                                     const ContextCreateInfo::EntryArray& requested,
                                     std::vector<std::string>& used)
{
    for (const auto& itr : requested)
    {
        bool found = false;
        for (const auto& property : properties)
        {
            if (strcmp(itr.name.c_str(), property.layerName.data()) == 0)
            {
                found = true;
                break;
            }
        }

        if (found)
        {
            used.push_back(itr.name);
        }
        else if (itr.optional == false)
        {
            ETH_CORE_ERROR("Requiered layer not found: {0}", itr.name);
            return VK_ERROR_LAYER_NOT_PRESENT;
        }
    }

    return VK_SUCCESS;
}
    
bool CheckExtensionProperties(const std::vector<vk::ExtensionProperties>& properties,
                              const ContextCreateInfo::EntryArray& requested,
                              bool bVerbose)
{
    for (const auto& itr : requested)
    {
        bool found = false;
        for (const auto& property : properties)
        {
            if (strcmp(itr.name.c_str(), property.extensionName.data()) == 0)
            {
                found = true;
                break;
            }
        }

        if (!found && !itr.optional)
        {
            if (bVerbose)
            {
                ETH_CORE_INFO("Could NOT locate mandatory extension '{0}'", itr.name.c_str());
            }
            return false;
        }
    }

    return true;
}

VkResult CheckAndFillExtensionProperties(const std::vector<vk::ExtensionProperties>& properties,
                                         const ContextCreateInfo::EntryArray& requested,
                                         std::vector<std::string>& used)
{
    for (const auto& itr : requested)
    {
        bool found = false;
        for (const auto& property : properties)
        {
            if (strcmp(itr.name.c_str(), property.extensionName.data()) == 0 && (itr.version == 0 || itr.version == property.specVersion))
            {
                found = true;
                break;
            }
        }

        if (found)
        {
            used.push_back(itr.name);
        }
        else if (!itr.optional)
        {
            ETH_CORE_ERROR("VK_ERROR_EXTENSION_NOT_PRESENT: {0} - {1}", itr.name, itr.version);
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }

    return VK_SUCCESS;
}

} // namespace VulkanContextUtils
} // namespace Ethane
