//
//  VulkanContext.h
//  EthaneEngine
//
//  Created by 邱奕翔 on 2025/4/28.
//
#pragma once

#include <vulkan/vulkan.h>
#include "VulkanDevice.h"

namespace Ethane {

struct ContextCreateInfo
{
    // instance properties
    std::string AppEngine = "Ethane";
    std::string AppTitle = "EthaneEngine";

    // api version
    uint32_t ApiMajor{ 1 };
    uint32_t ApiMinor{ 3 };

    // Information printed at Context::init time
    bool VerboseUsed = true;
    bool Validation = true;
    bool VerboseAvailable = true;
    bool VerboseCompatibleDevices = true;

    // may impact performance hence disable by default
    bool DisableRobustBufferAccess = true;

    // use device groups
    // bool useDeviceGroups = false;

    struct Entry
    {
        Entry(const char* entryName, bool isOptional = false, void* pointerFeatureStruct = nullptr, uint32_t checkVersion = 0)
            : name(entryName)
            , optional(isOptional)
            , pFeatureStruct(pointerFeatureStruct)
            , version(checkVersion)
        {
        }

        std::string name;
        bool        optional{ false };
        void* pFeatureStruct{ nullptr };
        uint32_t    version{ 0 };
    };

    using EntryArray = std::vector<Entry>;
    EntryArray InstanceLayers;
    EntryArray InstanceExtensions;
    EntryArray DeviceExtensions;
    void* InstanceCreateInfoExt{ nullptr };
    
    // Add/Remove Instance extension/layer & device extension
    void AddInstanceExtension(const char* name, bool optional = false);
    void AddInstanceLayer(const char* name, bool optional = false);
    void AddDeviceExtension(const char* name, bool optional = false, void* pFeatureStruct = nullptr, uint32_t version = 0);
    void RemoveInstanceExtension(const char* name);
    void RemoveInstanceLayer(const char* name);
    void RemoveDeviceExtension(const char* name);

    // Setter function
    ContextCreateInfo& SetDebug(bool debug) {
        Validation = debug, VerboseAvailable = debug;
        return *this;
    }
    
    ContextCreateInfo& SetVersion(uint32_t major, uint32_t minor) {
        ApiMajor = major, ApiMinor = minor;
        return *this;
    }
};


class VulkanGfxBackendAPI;
class VulkanContext
{
public:
    // This struct holds all core feature information for a physical device
    struct PhysicalDeviceInfo
    {
        VkPhysicalDeviceFeatures         features10{};
        VkPhysicalDeviceVulkan11Features features11{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES };
        VkPhysicalDeviceVulkan12Features features12{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };

        VkPhysicalDeviceProperties         properties10{};
        VkPhysicalDeviceVulkan11Properties properties11{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES };
        VkPhysicalDeviceVulkan12Properties properties12{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES };
    };

protected:
    void Init(ContextCreateInfo& info);
    void Destroy();
private:
    bool InitInstance(const ContextCreateInfo& info);
    bool InitDevice(const ContextCreateInfo& info, std::vector<uint32_t> compatibleDevices);
    
    void InitDebugUtils();
    VkResult FillFilteredNameArray(std::vector<std::string>& used,
        const std::vector<VkLayerProperties>& properties,
        const ContextCreateInfo::EntryArray& requested);
    VkResult FillFilteredNameArray(std::vector<std::string>& used,
        const std::vector<VkExtensionProperties>& properties,
        const ContextCreateInfo::EntryArray& requested,
        std::vector<void*>& featureStructs);
    std::vector<uint32_t> GetCompatibleDevices(const ContextCreateInfo& info);
    bool HasMandatoryExtensions(VkPhysicalDevice physicalDevice, const ContextCreateInfo& info, bool bVerbose);
    bool CheckEntryArray(const std::vector<VkExtensionProperties>& properties, const ContextCreateInfo::EntryArray& requested, bool bVerbose);
    void InitPhysicalFeatures(PhysicalDeviceInfo& info, VkPhysicalDevice physicalDevice, uint32_t versionMajor, uint32_t versionMinor);
    void FindSupportFormat();
    std::vector<VkFormat> FindAllSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
    
    friend VulkanGfxBackendAPI;
protected:
    VkInstance m_VulkanInstance = nullptr;
    VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;

    // Instance Extension
    std::vector<std::string> m_UsedInstanceLayers;
    std::vector<std::string> m_UsedInstanceExtensions;

    // Device Extension
    std::unordered_set<std::string> m_SupportedDeviceExtensions;
    std::vector<std::string> m_UsedDeviceExtensions;

    // Image format
    std::vector<VkFormat> m_SupportedSampleFormat;
    VkFormat m_DepthFormat;
    
    PhysicalDeviceInfo m_PhysicalInfo;
    Scope<VulkanPhysicalDevice> m_PhysicalDevice = nullptr;
    Scope<VulkanDevice> m_Device = nullptr;
};
}
