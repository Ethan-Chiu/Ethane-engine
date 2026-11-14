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
#include "VulkanDeletionQueue.h"

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

    // True when images of `format` with `tiling` support all of `features` (e.g. eDepthStencilAttachment)
    bool SupportsFormat(vk::Format format, vk::FormatFeatureFlags features, vk::ImageTiling tiling = vk::ImageTiling::eOptimal) const;
    // Best supported depth-stencil attachment format, chosen at Init(); what ImageFormat::Depth resolves to
    vk::Format GetDepthFormat() const { return m_DepthFormat; }

private:
    std::vector<vk::raii::PhysicalDevice> GetCompatibleDevices(const vk::raii::Instance& instance, const ContextCreateInfo& info);
    bool HasMandatoryExtensions(const vk::raii::PhysicalDevice& device, const ContextCreateInfo& info);
    bool InitPhysicalFeatures(const ContextCreateInfo& info);
    vk::Format FindDepthFormat() const;

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
    vk::Format m_DepthFormat = vk::Format::eUndefined;

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
    // Frames the CPU may record ahead of the GPU. Per-frame resources (frame command buffers, uniform buffer
    // copies, descriptor sets) keep this many copies and use GetFrameIndex() to pick theirs.
    static constexpr uint32_t kMaxFramesInFlight = 2;

    // Empty (null) device; a real one is move-assigned in VulkanContext::InitDevice
    VulkanDevice() = default;
    explicit VulkanDevice(const VulkanPhysicalDevice& physicalDevice);
    ~VulkanDevice() = default;

    // Move-only: owns raii handles
    VulkanDevice(VulkanDevice&&) noexcept = default;
    VulkanDevice& operator=(VulkanDevice&&) noexcept = default;
    VulkanDevice(const VulkanDevice&) = delete;
    VulkanDevice& operator=(const VulkanDevice&) = delete;

    // Waits for the GPU, then releases retired objects -> pools -> queues -> device. Safe to call on a null
    // device. All resources created from this device must be released first (see VulkanLiveResource).
    void Destroy();

    // Hands objects the GPU may still be using to the deletion queue (destroyed in argument order once the
    // frames that could use them have finished). Use instead of destroying in-use objects or waiting idle.
    template<typename... T>
    void Retire(T&&... objects) const { m_DeletionQueue->Retire(GetFrameSerial(), std::forward<T>(objects)...); }
    size_t RetiredCount() const { return m_DeletionQueue->Size(); }

    // Waits until the GPU is idle and destroys everything retired (shutdown, swapchain recreation)
    void WaitIdle() const;
    bool IsInitialized() const { return static_cast<bool>(*m_LogicalDevice); }

    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;

    // Getter
    const vk::raii::Device& Get() const { return m_LogicalDevice; }
    const VulkanPhysicalDevice& GetPhysicalDevice() const { return *m_PhysicalDevice; }

    const vk::raii::Queue& GetGraphicsQueue() const { return m_GraphicsQueue; }
    const vk::raii::Queue& GetPresentQueue() const { return m_PresentQueue; }  // may be the same queue as graphics
    const vk::raii::Queue& GetComputeQueue() const { return m_ComputeQueue; }
    // The frame command buffers' pools: used only by the thread recording frames
    const vk::raii::CommandPool& GetGraphicsCommandPool() const { return m_GraphicsCommandPool; }
    const vk::raii::CommandPool& GetComputeCommandPool() const { return m_ComputeCommandPool; }

    // Queue / command pool for a queue type (Graphics or Compute; there is no Transfer pool)
    const vk::raii::Queue& GetQueue(QueueFamilyTypes type) const;
    const vk::raii::CommandPool& GetCommandPool(QueueFamilyTypes type) const;

    // Thread safety. Vulkan requires queues and command pools to be used by one thread at a time:
    // - hold LockQueue() around every submit / present / waitIdle on a queue (all share one lock);
    // - one-off work (uploads, transitions) records into command buffers from the upload pool, holding
    //   LockUploadPool() from allocating a buffer until it's submitted, and again when freeing it.
    // Lock order: upload pool, then queue.
    [[nodiscard]] std::unique_lock<std::mutex> LockQueue() const { return std::unique_lock(m_Sync->Queue); }
    [[nodiscard]] std::unique_lock<std::mutex> LockUploadPool() const { return std::unique_lock(m_Sync->UploadPool); }
    const vk::raii::CommandPool& GetUploadCommandPool() const { return m_UploadCommandPool; }

    // Frame slot being recorded (0..kMaxFramesInFlight-1). Only meaningful while IsFrameInProgress():
    // the window target sets it after waiting for that slot's fence, so the slot's copies are free to write.
    uint32_t GetFrameIndex() const { return m_FrameIndex; }
    bool IsFrameInProgress() const { return m_FrameInProgress; }
    // Increases with every frame started: tells "earlier in this frame" apart from "in an older frame".
    // Readable from any thread (objects retired anywhere are tagged with it).
    uint64_t GetFrameSerial() const { return m_Sync->FrameSerial.load(std::memory_order_acquire); }
    // Called when a frame starts (after waiting for its slot's fence) and when it's submitted
    void SetFrameState(uint32_t frameIndex, bool inProgress)
    {
        m_FrameIndex = frameIndex;
        m_FrameInProgress = inProgress;
        if (inProgress) {
            const uint64_t serial = m_Sync->FrameSerial.fetch_add(1, std::memory_order_acq_rel) + 1;
            // This slot's previous frame has finished, and so has every frame before it
            if (serial > kMaxFramesInFlight) {
                m_DeletionQueue->Collect(serial - kMaxFramesInFlight);
            }
        }
    }

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
    vk::raii::CommandPool m_UploadCommandPool   = nullptr; // graphics family, transient: one-off work

    // Written by the recording thread
    uint32_t m_FrameIndex = 0;
    bool m_FrameInProgress = false;

    // Behind a pointer, like the deletion queue: the device is movable, mutexes and atomics aren't
    struct Sync
    {
        std::mutex Queue;
        std::mutex UploadPool;
        std::atomic<uint64_t> FrameSerial{ 0 };
    };
    std::unique_ptr<Sync> m_Sync = std::make_unique<Sync>();

    // Behind a pointer: the device is movable, the queue (its mutex) isn't
    std::unique_ptr<VulkanDeletionQueue> m_DeletionQueue = std::make_unique<VulkanDeletionQueue>();
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
    // The API version the instance was created with (VkApplicationInfo::apiVersion)
    uint32_t GetApiVersion() const { return VK_MAKE_API_VERSION(0, m_ContextInfo.ApiMajor, m_ContextInfo.ApiMinor, 0); }

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
