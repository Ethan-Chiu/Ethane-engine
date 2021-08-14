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
		VulkanPhysicalDevice(VkSurfaceKHR surface); // test param VkSurfaceKHR surface
		~VulkanPhysicalDevice();

		static Ref<VulkanPhysicalDevice> Pick(VkSurfaceKHR surface); // test VkSurfaceKHR surface

		// Getter
		VkPhysicalDevice GetVulkanPhysicalDevice() const { return m_PhysicalDevice; }
		const QueueFamilyIndices& GetQueueFamilyIndices() const { return m_QueueFamilyIndices; }

	private:
		uint32_t rateDeviceSuitability(VkPhysicalDevice device);
		bool checkDeviceExtensionSupport(VkPhysicalDevice device);

		QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device, uint32_t queueFamilyFlags);
		void QueueCreateInfo();

		bool IsExtensionSupported(const std::string& extensionName) const;

	private:
		const uint32_t m_RequestedQueueTypes = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT;
		
		VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
		VkPhysicalDeviceProperties m_Properties;
		VkPhysicalDeviceFeatures m_Features;
		VkPhysicalDeviceMemoryProperties m_MemoryProperties;

		std::unordered_set<std::string> m_SupportedExtensions;

		QueueFamilyIndices m_QueueFamilyIndices;
		std::vector<VkDeviceQueueCreateInfo> m_QueueCreateInfos;

		// for selecting device
		VkSurfaceKHR m_Surface; // no much use

	friend class VulkanDevice;

	};

	class VulkanDevice
	{
	public:
		VulkanDevice(const Ref<VulkanPhysicalDevice>& physicalDevice, VkPhysicalDeviceFeatures enabledFeatures);
		~VulkanDevice();
		void Cleanup();


		static Ref<VulkanDevice> Create(const Ref<VulkanPhysicalDevice>& physicalDevice, VkPhysicalDeviceFeatures enabledFeatures);

		VkCommandBuffer CreateCommandBuffer(QueueFamilyTypes type = QueueFamilyTypes::Graphics, bool oneTimeUse = false, bool begin = false);
		void SubmitCommandBuffer(VkCommandBuffer commandBuffer, QueueFamilyTypes type = QueueFamilyTypes::Graphics);

		// Getter
		VkDevice GetVulkanDevice() { return m_LogicalDevice; }
		Ref<VulkanPhysicalDevice> GetPhysicalDevice() { return m_PhysicalDevice; }

		VkQueue GetGraphicsQueue() { return m_GraphicsQueue; }
		VkQueue GetComputeQueue() { return m_ComputeQueue; }

	private:
		VkDevice m_LogicalDevice = VK_NULL_HANDLE;
		Ref<VulkanPhysicalDevice> m_PhysicalDevice;
		VkPhysicalDeviceFeatures m_EnabledFeatures;

		VkQueue m_GraphicsQueue, m_ComputeQueue;

		VkCommandPool m_GraphicsCommandPool, m_ComputeCommandPool;

		bool m_EnableDebugMarkers = false;
	};
}