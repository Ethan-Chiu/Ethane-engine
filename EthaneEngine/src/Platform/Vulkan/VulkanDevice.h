#pragma once

#include <optional>
#include "Vulkan.h"

namespace Ethane {

	enum class QueueFamilyTypes {
		Graphics = 0,
		Compute = 1,
		Transfer = 2
	};


	class VulkanPhysicalDevice
	{
	public:
		struct QueueFamilyIndices
		{
			std::optional<uint32_t> Graphics;
			std::optional<uint32_t> Compute;
			std::optional<uint32_t> Transfer;
			// add
			// std::optional<uint32_t> Present;

			bool isComplete()
			{
				// add
				return Graphics.has_value() ; // && Present.has_value()
			}
		};

	public:
		VulkanPhysicalDevice(const std::vector<uint32_t>& compatibleDeviceIndices, VkSurfaceKHR surface); // test param VkSurfaceKHR surface
		~VulkanPhysicalDevice();

		static Ref<VulkanPhysicalDevice> Init(const std::vector<uint32_t>& compatibleDeviceIndices, VkSurfaceKHR surface); // test VkSurfaceKHR surface

		// Getter
		VkPhysicalDevice GetVulkanPhysicalDevice() const { return m_PhysicalDevice; }
		const QueueFamilyIndices& GetQueueFamilyIndices() const { return m_QueueFamilyIndices; }
		VkPhysicalDeviceProperties2 GetProperties() { return m_Properties; }
	private:
		uint32_t rateDeviceSuitability(VkPhysicalDevice device);
		bool checkDeviceExtensionSupport(VkPhysicalDevice device);

		QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device, uint32_t queueFamilyFlags);
		void QueueCreateInfo();

		bool IsExtensionSupported(const std::string& extensionName) const;

	private:
		const uint32_t m_RequestedQueueTypes = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT;
		
		VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;

		// Features and properties
		VkPhysicalDeviceMemoryProperties m_MemoryProperties;
		VkPhysicalDeviceFeatures2 m_Features;
		VkPhysicalDeviceProperties2 m_Properties;

		std::unordered_set<std::string> m_SupportedExtensions;
		std::vector<std::string> m_UsedDeviceExtensions;

		QueueFamilyIndices m_QueueFamilyIndices;
		std::vector<VkDeviceQueueCreateInfo> m_QueueCreateInfos;

		// for selecting device
		VkSurfaceKHR m_Surface; // no much use

	friend class VulkanDevice;
	friend class VulkanContext;
	};

	class VulkanDevice
	{
	public:
		VulkanDevice(const Ref<VulkanPhysicalDevice>& physicalDevice, VkPhysicalDeviceFeatures2 enabledFeatures2);
		~VulkanDevice();
		void Cleanup();


		static Ref<VulkanDevice> Create(const Ref<VulkanPhysicalDevice>& physicalDevice, VkPhysicalDeviceFeatures2 enabledFeatures2);

		VkCommandBuffer CreateCommandBuffer(QueueFamilyTypes type = QueueFamilyTypes::Graphics, bool oneTimeUse = false, bool begin = false);
		void SubmitCommandBuffer(VkCommandBuffer commandBuffer, QueueFamilyTypes type = QueueFamilyTypes::Graphics);

		VkCommandBuffer CreateSecondaryCommandBuffer();

		// Getter
		VkDevice GetVulkanDevice() { return m_LogicalDevice; }
		Ref<VulkanPhysicalDevice> GetPhysicalDevice() { return m_PhysicalDevice; }

		VkQueue GetGraphicsQueue() { return m_GraphicsQueue; }
		VkQueue GetComputeQueue() { return m_ComputeQueue; }

		VkCommandPool GetGraphicsCommandPool() { return m_GraphicsCommandPool; }
		VkCommandPool GetComputeCommandPool() { return m_GraphicsCommandPool; }

	private:
		VkDevice m_LogicalDevice = VK_NULL_HANDLE;
		Ref<VulkanPhysicalDevice> m_PhysicalDevice;
		VkPhysicalDeviceFeatures2 m_EnabledFeatures2;

		VkQueue m_GraphicsQueue, m_ComputeQueue;

		VkCommandPool m_GraphicsCommandPool, m_ComputeCommandPool;

		// bool m_EnableDebugMarkers = false;
	};

	namespace Utils
	{
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