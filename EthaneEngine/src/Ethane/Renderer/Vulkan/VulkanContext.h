#pragma once

#include "Ethane/Renderer/GraphicsContext.h"

#include <vulkan/vulkan.h>
#include "VulkanDevice.h"
#include "VulkanSwapChain.h"
#include "VulkanCommandBuffer.h"

struct GLFWwindow;

namespace Ethane {

	struct ContextCreateInfo
	{
		// ContextCreateInfo(bool bUseValidation = true);
		// 
		// void setVersion(uint32_t major, uint32_t minor);
		
		// Add/Remove Instance extension/layer & device extension
		void AddInstanceExtension(const char* name, bool optional = false);
		void AddInstanceLayer(const char* name, bool optional = false);
		void AddDeviceExtension(const char* name, bool optional = false, void* pFeatureStruct = nullptr, uint32_t version = 0);
		// 
		void RemoveInstanceExtension(const char* name);
		void RemoveInstanceLayer(const char* name);
		void RemoveDeviceExtension(const char* name);

		// instance properties
		std::string AppEngine = "Ethane";
		std::string AppTitle = "EthaneEngine";

		// api version
		uint32_t ApiMajor{ 1 };
		uint32_t ApiMinor{ 3 };

		// Information printed at Context::init time
		bool VerboseUsed = true;
#ifdef ETH_DEBUG
		bool Validation = true;
		bool VerboseAvailable = true;
#else
		bool Validation = false;
		bool VerboseAvailable = false;
#endif
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
		// void* deviceCreateInfoExt{ nullptr };
	};

	class VulkanContext : public GraphicsContext
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

	public:
		VulkanContext(GLFWwindow* windowHandle);
		virtual ~VulkanContext() {};

		virtual void Init() override;
		virtual void Shutdown() override;

		virtual bool BeginFrame() override; // TODO: test
		virtual void SwapBuffers() override;
		
		virtual void OnResize(uint32_t width, uint32_t height) override;

		// getter
		static VkInstance GetInstance() { return s_VulkanInstance; }
		static Ref<VulkanPhysicalDevice> GetPhysicalDevice() { return m_PhysicalDevice; }
		static Ref<VulkanDevice> GetDevice() { return m_Device; }
		static VulkanSwapChain GetSwapChain() { return m_SwapChain; }

	private:
		bool InitInstance(const ContextCreateInfo& info);
		void InitDebugUtils();
		VkResult CreateSurface();
		bool InitDevice(const ContextCreateInfo& info, std::vector<uint32_t> compatibleDevices);

		std::vector<uint32_t> GetCompatibleDevices(const ContextCreateInfo& info);
		VkResult FillFilteredNameArray(std::vector<std::string>& used,
			const std::vector<VkLayerProperties>& properties,
			const ContextCreateInfo::EntryArray& requested);
		VkResult FillFilteredNameArray(std::vector<std::string>& used,
			const std::vector<VkExtensionProperties>& properties,
			const ContextCreateInfo::EntryArray& requested,
			std::vector<void*>& featureStructs);
		bool HasMandatoryExtensions(VkPhysicalDevice physicalDevice, const ContextCreateInfo& info, bool bVerbose);
		bool CheckEntryArray(const std::vector<VkExtensionProperties>& properties, const ContextCreateInfo::EntryArray& requested, bool bVerbose);
		void InitPhysicalFeatures(PhysicalDeviceInfo& info, VkPhysicalDevice physicalDevice, uint32_t versionMajor, uint32_t versionMinor);

	private:
		GLFWwindow* m_WindowHandle;

		inline static VkInstance s_VulkanInstance;
		VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;

		// Instance Extension
		std::vector<std::string> m_UsedInstanceLayers;
		std::vector<std::string> m_UsedInstanceExtensions;

		// Device Extension
		std::unordered_set<std::string> m_SupportedDeviceExtensions;
		std::vector<std::string> m_UsedDeviceExtensions;

		VkSurfaceKHR m_Surface;

		PhysicalDeviceInfo m_PhysicalInfo;
		
		// TODO: change to non-static
		inline static Ref<VulkanPhysicalDevice> m_PhysicalDevice;
		inline static Ref<VulkanDevice> m_Device;

		inline static VulkanSwapChain m_SwapChain;
	};


	///////////////////////////////////////////////////////////////////////////
	// helper funtions ////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////

	// TODO: move
	namespace Utils {

		inline uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
		{
			auto physicalDevice = VulkanContext::GetPhysicalDevice()->GetVulkanPhysicalDevice();

			VkPhysicalDeviceMemoryProperties memProperties;
			vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

			for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
				if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
					return i;
				}
			}
		}

		inline void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
			auto device = VulkanContext::GetDevice()->GetVulkanDevice();
		
			VkBufferCreateInfo bufferInfo{};
			bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferInfo.size = size;
			bufferInfo.usage = usage;
			bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		
			if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
				throw std::runtime_error("failed to create buffer!");
			}
		
			VkMemoryRequirements memRequirements;
			vkGetBufferMemoryRequirements(device, buffer, &memRequirements);
		
			VkMemoryAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocInfo.allocationSize = memRequirements.size;
			allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);
		
			if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
				throw std::runtime_error("failed to allocate buffer memory!");
			}
		
			vkBindBufferMemory(device, buffer, bufferMemory, 0);
		}

		inline void CopyBuffer(VkBuffer dstBuffer, VkBuffer srcBuffer, VkDeviceSize size) {
			// create command buffer
			VkCommandBuffer copyCmdBuffer = VulkanContext::GetDevice()->CreateCommandBuffer(QueueFamilyTypes::Graphics, true, true);

			// record command
			VkBufferCopy copyRegion{};
			copyRegion.srcOffset = 0;
			copyRegion.dstOffset = 0;
			copyRegion.size = size;
			vkCmdCopyBuffer(copyCmdBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

			// submit command buffer
			VulkanContext::GetDevice()->SubmitCommandBuffer(copyCmdBuffer);
		}
	}

}