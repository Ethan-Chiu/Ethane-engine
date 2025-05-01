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

void ContextCreateInfo::AddDeviceExtension(const char* name, bool optional, void* pFeatureStruct, uint32_t version)
{
    DeviceExtensions.emplace_back(name, optional, pFeatureStruct, version);
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


static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_types,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data) {
    switch (message_severity) {
    default:
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        ETH_CORE_ERROR(callback_data->pMessage);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        ETH_CORE_WARN(callback_data->pMessage);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        ETH_CORE_INFO(callback_data->pMessage);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        ETH_CORE_TRACE(callback_data->pMessage);
        break;
    }
    return VK_FALSE;
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
    VkPhysicalDeviceAccelerationStructureFeaturesKHR accelFeature{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR };
    m_ContextInfo.AddDeviceExtension(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, true, &accelFeature);  // To build acceleration structures
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rtPipelineFeature{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR };
    m_ContextInfo.AddDeviceExtension(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME, true, &rtPipelineFeature);  // To use vkCmdTraceRaysKHR
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
    vkDeviceWaitIdle(m_Device.GetVulkanDevice());

    m_Device.Destroy();
    m_PhysicalDevice.Destroy();

    ETH_CORE_INFO("Destroying Vulkan debugger...");
    if (m_DebugMessenger != VK_NULL_HANDLE) {
        auto vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_VulkanInstance, "vkDestroyDebugUtilsMessengerEXT");
        vkDestroyDebugUtilsMessengerEXT(m_VulkanInstance, m_DebugMessenger, nullptr);
    }

    ETH_CORE_INFO("Destroying Vulkan instance...");
    vkDestroyInstance(m_VulkanInstance, nullptr);
    m_VulkanInstance = nullptr;
}

bool VulkanContext::InitInstance()
{
    auto& info = m_ContextInfo;
    // Application Info
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = info.AppTitle.c_str();
    appInfo.pEngineName = info.AppEngine.c_str();
    appInfo.apiVersion = VK_MAKE_VERSION(info.ApiMajor, info.ApiMinor, 0);

    if (info.VerboseUsed)
    {
        uint32_t version;
        VkResult result = vkEnumerateInstanceVersion(&version);
        VK_CHECK_RESULT(result);
        ETH_CORE_INFO("_______________");
        ETH_CORE_INFO("Vulkan Version:");
        ETH_CORE_INFO(" - available:  {0}.{1}.{2}", VK_VERSION_MAJOR(version), VK_VERSION_MINOR(version), VK_VERSION_PATCH(version));
        ETH_CORE_INFO(" - requesting: {0}.{1}.{2}", info.ApiMajor, info.ApiMinor, 204);
    }

    {
        // Get all layers
        // Check if this layer is available at instance level
        uint32_t instanceLayerCount;
        VK_CHECK_RESULT(vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr));
        std::vector<VkLayerProperties> instanceLayerProperties(instanceLayerCount);
        VK_CHECK_RESULT(vkEnumerateInstanceLayerProperties(&instanceLayerCount, instanceLayerProperties.data()));

        if (VulkanContextUtils::CheckAndFillLayerProperties(instanceLayerProperties, info.InstanceLayers, m_UsedInstanceLayers) != VK_SUCCESS)
        {
            return false;
        }
            
        // Log available properties
        if (info.VerboseAvailable)
        {
            ETH_CORE_TRACE("Vulkan Instance Layers:");
            for (const VkLayerProperties& layer : instanceLayerProperties)
            {
                ETH_CORE_TRACE("  {0}", layer.layerName, VK_VERSION_MAJOR(layer.specVersion),
                    VK_VERSION_MINOR(layer.specVersion), VK_VERSION_PATCH(layer.specVersion), layer.implementationVersion, layer.description);
            }
        }
    }

    {
        // Get all instance extensions
        uint32_t extensionCount;
        VK_CHECK_RESULT(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr));
        std::vector<VkExtensionProperties> extensionProperties(extensionCount);
        VK_CHECK_RESULT(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensionProperties.data()));

        std::vector<void*> featureStructs;
        if (VulkanContextUtils::CheckAndFillExtensionProperties(extensionProperties, info.InstanceExtensions, m_UsedInstanceExtensions, featureStructs) != VK_SUCCESS)
        {
            return false;
        }

        if (info.VerboseAvailable)
        {
            ETH_CORE_INFO("Available Instance Extensions :");
            for (auto extension : extensionProperties)
            {
                ETH_CORE_INFO("  {0} (v. {1})", std::string(extension.extensionName), extension.specVersion);
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

    VkInstanceCreateInfo instanceCreateInfo{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    instanceCreateInfo.pApplicationInfo = &appInfo;
    instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(usedInstanceExtensions.size());
    instanceCreateInfo.ppEnabledExtensionNames = usedInstanceExtensions.data();
    instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(usedInstanceLayers.size());
    instanceCreateInfo.ppEnabledLayerNames = usedInstanceLayers.data();
    instanceCreateInfo.pNext = info.InstanceCreateInfoExt;

    VK_CHECK_RESULT(vkCreateInstance(&instanceCreateInfo, nullptr, &m_VulkanInstance));

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
    uint32_t log_severity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT; // VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
    debugCreateInfo.messageSeverity = log_severity;
    debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    debugCreateInfo.pfnUserCallback = VulkanDebugCallback;

    auto vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_VulkanInstance, "vkCreateDebugUtilsMessengerEXT");
    ETH_CORE_ASSERT(vkCreateDebugUtilsMessengerEXT != NULL, "Failed to create debug messenger!");
    VK_CHECK_RESULT(vkCreateDebugUtilsMessengerEXT(m_VulkanInstance, &debugCreateInfo, nullptr, &m_DebugMessenger));
    ETH_CORE_INFO("Vulkan debugger created.");
}


namespace VulkanContextUtils{

VkResult CheckAndFillLayerProperties(const std::vector<VkLayerProperties>& properties,
                                     const ContextCreateInfo::EntryArray& requested,
                                     std::vector<std::string>& used)
{
    for (const auto& itr : requested)
    {
        bool found = false;
        for (const auto& property : properties)
        {
            if (strcmp(itr.name.c_str(), property.layerName) == 0)
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
    
bool CheckExtensionProperties(const std::vector<VkExtensionProperties>& properties,
                              const ContextCreateInfo::EntryArray& requested,
                              bool bVerbose)
{
    for (const auto& itr : requested)
    {
        bool found = false;
        for (const auto& property : properties)
        {
            if (strcmp(itr.name.c_str(), property.extensionName) == 0)
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

VkResult CheckAndFillExtensionProperties(const std::vector<VkExtensionProperties>& properties,
                                         const ContextCreateInfo::EntryArray& requested,
                                         std::vector<std::string>& used,
                                         std::vector<void*>& featureStructs)
{
    for (const auto& itr : requested)
    {
        bool found = false;
        for (const auto& property : properties)
        {
            if (strcmp(itr.name.c_str(), property.extensionName) == 0 && (itr.version == 0 || itr.version == property.specVersion))
            {
                found = true;
                break;
            }
        }

        if (found)
        {
            used.push_back(itr.name);
            if (itr.pFeatureStruct)
            {
                featureStructs.push_back(itr.pFeatureStruct);
            }
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
