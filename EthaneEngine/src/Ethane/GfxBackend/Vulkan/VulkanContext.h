//
//  VulkanContext.h
//  EthaneEngine
//
//  Created by 邱奕翔 on 2025/4/28.
//
#pragma once

#include <optional>
#include <vulkan/vulkan.h>

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


enum class QueueFamilyTypes {
    Graphics = 0,
    Compute = 1,
    Transfer = 2
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class VulkanPhysicalDevice
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

    struct QueueFamilyIndices
    {
        std::optional<uint32_t> Graphics;
        std::optional<uint32_t> Compute;
        std::optional<uint32_t> Transfer;
        std::optional<uint32_t> Present;
        
        bool isComplete()
        {
            return Graphics.has_value();
        }
    };
    
public:
    void Init(VkInstance vkInstance, const ContextCreateInfo& info, VkSurfaceKHR surface);
    void Destroy();
    
    bool IsInitialized() const { return m_PhysicalDevice != VK_NULL_HANDLE; }

    SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) const;
    void PrintSelectedDeviceInfo();

    // Getter
    VkPhysicalDevice GetVulkanPhysicalDevice() const { return m_PhysicalDevice; }
    const QueueFamilyIndices& GetQueueFamilyIndices() const { return m_QueueFamilyIndices; }

private:
    std::vector<uint32_t> GetCompatibleDevices(VkInstance vkInstance, const ContextCreateInfo& info);
    bool HasMandatoryExtensions(VkPhysicalDevice device, const ContextCreateInfo& info);
    bool InitPhysicalFeatures(const ContextCreateInfo& info);

    // Utilities
    int32_t RateDeviceSuitability(VkPhysicalDevice device, VkSurfaceKHR surface = VK_NULL_HANDLE);
    QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device, uint32_t queueFamilyFlags, VkSurfaceKHR surface);
protected:
    // Device info and handle
    PhysicalDeviceInfo m_PhysicalInfo;
    VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
    
    // Device Extension
    std::unordered_set<std::string> m_SupportedDeviceExtensions;
    std::vector<std::string> m_UsedDeviceExtensions;
    VkPhysicalDeviceFeatures2 m_UsedDeviceFeatures;

    const uint32_t m_ConstRequestedQueueTypes = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT;
    QueueFamilyIndices m_QueueFamilyIndices;

    friend class VulkanDevice;
    friend class VulkanContext;
};


class VulkanDevice
{
public:
    VulkanDevice(const VulkanPhysicalDevice& physicalDevice);
    ~VulkanDevice() = default;
    void Destroy();

    void SubmitCommandBuffer(VkCommandBuffer commandBuffer, QueueFamilyTypes type = QueueFamilyTypes::Graphics);

    // Getter
    VkDevice GetVulkanDevice() const { return m_LogicalDevice; }
    const VulkanPhysicalDevice* GetPhysicalDevice() const { return m_PhysicalDevice; }

    VkQueue GetGraphicsQueue() const { return m_GraphicsQueue; }
    VkQueue GetComputeQueue() const { return m_ComputeQueue; }

    VkCommandPool GetGraphicsCommandPool() const { return m_GraphicsCommandPool; }
    VkCommandPool GetComputeCommandPool() const { return m_ComputeCommandPool; }
private:
    void QueueCreateInfo();

private:
    VkDevice m_LogicalDevice = VK_NULL_HANDLE;
    const VulkanPhysicalDevice* m_PhysicalDevice;

    std::vector<VkDeviceQueueCreateInfo> m_QueueCreateInfos;

    VkQueue m_GraphicsQueue, m_ComputeQueue, m_TransferQueue;

    VkCommandPool m_GraphicsCommandPool, m_ComputeCommandPool;
};


class VulkanGfxBackendAPI;
class VulkanContext
{
public:

protected:
    void Init(ContextCreateInfo& info);
    bool InitDevice(VkSurfaceKHR surface);
    void Destroy();
    
private:
    bool InitInstance();
    void InitDebugUtils();
    
    void FindSupportFormat();
    std::vector<VkFormat> FindAllSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
    
    friend VulkanGfxBackendAPI;
protected:
    VkInstance m_VulkanInstance = nullptr;
    VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;
    ContextCreateInfo m_ContextInfo;

    // Instance Extension
    std::vector<std::string> m_UsedInstanceLayers;
    std::vector<std::string> m_UsedInstanceExtensions;

    // Image format
    std::vector<VkFormat> m_SupportedSampleFormat;
    VkFormat m_DepthFormat;
    
    VulkanPhysicalDevice m_PhysicalDevice;
    VulkanDevice m_Device;
};

namespace VulkanContextUtils {

VkResult CheckAndFillLayerProperties(const std::vector<VkLayerProperties>& properties,
                              const ContextCreateInfo::EntryArray& requested,
                              std::vector<std::string>& used);

bool CheckExtensionProperties(const std::vector<VkExtensionProperties>& properties,
                              const ContextCreateInfo::EntryArray& requested,
                              bool bVerbose);

VkResult CheckAndFillExtensionProperties(const std::vector<VkExtensionProperties>& properties,
                                         const ContextCreateInfo::EntryArray& requested,
                                         std::vector<std::string>& used,
                                         std::vector<void*>& featureStructs);

static const char* VulkanVendorIDToString(uint32_t vendorID)
{
    switch (vendorID)
    {
    case 0x10DE: return "NVIDIA";
    case 0x1002: return "AMD";
    case 0x8086: return "INTEL";
    case 0x13B5: return "ARM";
    }
    return "Unknown";
}
}
}
