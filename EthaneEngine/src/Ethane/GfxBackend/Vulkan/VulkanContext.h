//
//  VulkanContext.h
//  EthaneEngine
//
//  Created by 邱奕翔 on 2025/4/28.
//
#pragma once

#include "ethpch.h"
#include <optional>
#include <atomic>

#include "Vulkan.h"

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
        Entry(const char* entryName, bool isOptional = false, uint32_t checkVersion = 0)
            : name(entryName)
            , optional(isOptional)
            , version(checkVersion)
        {
        }

        std::string name;
        bool        optional{ false };
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
    // Feature structs for device extensions live in VulkanPhysicalDevice::FeatureChain
    void AddDeviceExtension(const char* name, bool optional = false, uint32_t version = 0);
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
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> presentModes;
};

class VulkanPhysicalDevice
{
public:
    // Every feature struct the engine knows about. Core 1.0/1.1/1.2 (requires a Vulkan 1.2+ device),
    // followed by device-extension structs, which are unlink()ed when their extension isn't enabled.
    // To support a new extension's features: add its struct here and map it to the extension name
    // in VulkanPhysicalDevice::InitPhysicalFeatures.
    using FeatureChain  = vk::StructureChain<vk::PhysicalDeviceFeatures2,
                                             vk::PhysicalDeviceVulkan11Features,
                                             vk::PhysicalDeviceVulkan12Features,
                                             vk::PhysicalDeviceAccelerationStructureFeaturesKHR,  // VK_KHR_acceleration_structure
                                             vk::PhysicalDeviceRayTracingPipelineFeaturesKHR>;    // VK_KHR_ray_tracing_pipeline
    using PropertyChain = vk::StructureChain<vk::PhysicalDeviceProperties2,
                                             vk::PhysicalDeviceVulkan11Properties,
                                             vk::PhysicalDeviceVulkan12Properties>;

    // This struct holds all core feature information for a physical device
    struct PhysicalDeviceInfo
    {
        FeatureChain  Features;   // supported (extension structs unlinked if the extension isn't enabled)
        PropertyChain Properties;
    };

    struct QueueFamilyIndices
    {
        std::optional<uint32_t> Graphics;
        std::optional<uint32_t> Compute;
        std::optional<uint32_t> Transfer;
        std::optional<uint32_t> Present;
        
        bool isComplete()
        {
            return Graphics.has_value() && Present.has_value();
        }
    };
    
public:
    void Init(const vk::raii::Instance& instance, const ContextCreateInfo& info, vk::SurfaceKHR surface);
    void Destroy();
    
    bool IsInitialized() const { return static_cast<bool>(*m_PhysicalDevice); }

    void PrintSelectedDeviceInfo() const;
    SwapChainSupportDetails GetSwapchainSupport(vk::SurfaceKHR surface) const { return QuerySwapChainSupport(m_PhysicalDevice, surface);}

    // Getter
    const vk::raii::PhysicalDevice& Get() const { return m_PhysicalDevice; }
    const PhysicalDeviceInfo& GetInfo() const { return m_PhysicalInfo; }
    const QueueFamilyIndices& GetQueueFamilyIndices() const { return m_QueueFamilyIndices; }
    // Features to enable at device creation; pass &GetEnabledFeatures().get<vk::PhysicalDeviceFeatures2>() as pNext
    const FeatureChain& GetEnabledFeatures() const { return m_EnabledFeatures; }
    bool IsExtensionEnabled(const char* name) const;

private:
    std::vector<vk::raii::PhysicalDevice> GetCompatibleDevices(const vk::raii::Instance& instance, const ContextCreateInfo& info);
    bool HasMandatoryExtensions(const vk::raii::PhysicalDevice& device, const ContextCreateInfo& info);
    bool InitPhysicalFeatures(const ContextCreateInfo& info);

    // Utilities
    int32_t RateDeviceSuitability(const vk::raii::PhysicalDevice& device, vk::SurfaceKHR surface);
    SwapChainSupportDetails QuerySwapChainSupport(const vk::raii::PhysicalDevice& device, vk::SurfaceKHR surface) const;
    QueueFamilyIndices FindQueueFamilies(const vk::raii::PhysicalDevice& device, vk::QueueFlags queueFamilyFlags, vk::SurfaceKHR surface);
protected:
    // Device info and handle
    PhysicalDeviceInfo m_PhysicalInfo;
    vk::raii::PhysicalDevice m_PhysicalDevice = nullptr;
    
    // Device Extension
    std::unordered_set<std::string> m_SupportedDeviceExtensions;
    std::vector<std::string> m_UsedDeviceExtensions;

    // Features to enable at device creation (supported features minus explicit opt-outs)
    FeatureChain m_EnabledFeatures;

    const vk::QueueFlags m_ConstRequestedQueueTypes = vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute | vk::QueueFlagBits::eTransfer;
    QueueFamilyIndices m_QueueFamilyIndices;

    friend class VulkanDevice;
    friend class VulkanContext;
};

// Counts live engine resources that own objects created from the logical device (buffers, shaders,
// pipelines, swapchains, ...). Each such class holds one as a member. VulkanDevice::Destroy() reports
// any still alive: their raii destructors would otherwise run vkDestroy* on an already destroyed device.
class VulkanLiveResource
{
public:
    VulkanLiveResource() { ++s_Count; }
    VulkanLiveResource(const VulkanLiveResource&) { ++s_Count; }
    VulkanLiveResource& operator=(const VulkanLiveResource&) { return *this; }
    ~VulkanLiveResource() { --s_Count; }

    static int Count() { return s_Count.load(); }

private:
    inline static std::atomic<int> s_Count{ 0 };
};

class VulkanDevice
{
public:
    // Empty (null) device; a real one is move-assigned in VulkanContext::InitDevice
    VulkanDevice() = default;
    explicit VulkanDevice(const VulkanPhysicalDevice& physicalDevice);
    ~VulkanDevice() = default;

    // Move-only: owns raii handles
    VulkanDevice(VulkanDevice&&) noexcept = default;
    VulkanDevice& operator=(VulkanDevice&&) noexcept = default;
    VulkanDevice(const VulkanDevice&) = delete;
    VulkanDevice& operator=(const VulkanDevice&) = delete;

    // Waits for the GPU, then releases pools -> queues -> device. Safe to call on a null device.
    // All resources created from this device must be released first (see VulkanLiveResource).
    void Destroy();
    bool IsInitialized() const { return static_cast<bool>(*m_LogicalDevice); }

    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;

    // Getter
    const vk::raii::Device& Get() const { return m_LogicalDevice; }

    const vk::raii::Queue& GetGraphicsQueue() const { return m_GraphicsQueue; }
    const vk::raii::Queue& GetPresentQueue() const { return m_PresentQueue; }  // may be the same queue as graphics
    const vk::raii::Queue& GetComputeQueue() const { return m_ComputeQueue; }
    const vk::raii::CommandPool& GetGraphicsCommandPool() const { return m_GraphicsCommandPool; }
    const vk::raii::CommandPool& GetComputeCommandPool() const { return m_ComputeCommandPool; }

    // Queue / command pool for a queue type (Graphics or Compute; there is no Transfer pool)
    const vk::raii::Queue& GetQueue(QueueFamilyTypes type) const;
    const vk::raii::CommandPool& GetCommandPool(QueueFamilyTypes type) const;

private:
    const VulkanPhysicalDevice* m_PhysicalDevice = nullptr;

    // NOTE: declaration order matters, members are destroyed in reverse: pools -> queues -> device
    vk::raii::Device m_LogicalDevice = nullptr;

    vk::raii::Queue m_GraphicsQueue = nullptr;
    vk::raii::Queue m_PresentQueue  = nullptr;
    vk::raii::Queue m_ComputeQueue  = nullptr;
    vk::raii::Queue m_TransferQueue = nullptr;

    vk::raii::CommandPool m_GraphicsCommandPool = nullptr;
    vk::raii::CommandPool m_ComputeCommandPool  = nullptr;
};


class VulkanGfxBackendAPI;
class VulkanWindowTarget; // initialize devices when first window target is created, can change the context
class VulkanContext
{
public:
    // Getter
    const vk::raii::Instance& GetInstance() const { return m_VulkanInstance; }
    const VulkanPhysicalDevice& GetPhysicalDevice() const { return m_PhysicalDevice; }
    const VulkanDevice& GetDevice() const { return m_Device; }

protected:
    void Init(ContextCreateInfo& info);
    bool InitDevice(vk::SurfaceKHR surface);
    void Destroy();
    
private:
    bool InitInstance();
    void InitDebugUtils();
    
    void FindSupportFormat();
    std::vector<VkFormat> FindAllSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
    
    friend VulkanGfxBackendAPI;
    friend VulkanWindowTarget;
protected:
    // NOTE: declaration order matters for RAII destruction (reverse order): messenger -> instance -> context
    vk::raii::Context                m_Context;
    vk::raii::Instance               m_VulkanInstance = nullptr;
    vk::raii::DebugUtilsMessengerEXT m_DebugMessenger = nullptr;

    ContextCreateInfo m_ContextInfo;
    VkAllocationCallbacks m_AllocationCallbacks;

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

VkResult CheckAndFillLayerProperties(const std::vector<vk::LayerProperties>& properties,
                              const ContextCreateInfo::EntryArray& requested,
                              std::vector<std::string>& used);

bool CheckExtensionProperties(const std::vector<vk::ExtensionProperties>& properties,
                              const ContextCreateInfo::EntryArray& requested,
                              bool bVerbose);

VkResult CheckAndFillExtensionProperties(const std::vector<vk::ExtensionProperties>& properties,
                                         const ContextCreateInfo::EntryArray& requested,
                                         std::vector<std::string>& used);

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
