#pragma once

#include "Ethane/Renderer/Mesh.h"
#include "RayTracingBuilder.h"

// test
// #include "Platform/Vulkan/VulkanVertexBuffer.h"
// #include "Platform/Vulkan/VulkanIndexBuffer.h"


namespace Ethane {

	// TODO: investigate
	// The OBJ model
	struct RTObjModel
	{
		uint32_t     NbIndices{ 0 };
		uint32_t     NbVertices{ 0 };
		Ref<VertexBuffer> VertexBuffer;    // Device buffer of all 'Vertex'
		Ref<IndexBuffer> IndexBuffer;     // Device buffer of the indices forming triangles
	};
	struct RTObjInstance
	{
		glm::mat4 Transform;    // Matrix of the instance
		uint32_t ObjIndex{ 0 };  // Model index reference
	};

	class RayTracing
	{
	public:
		RayTracing() = default;
		~RayTracing() = default;

		void InitRayTracing();
		auto MeshToVkGeometryKHR(const Mesh& mesh);

		void CreateBottomLevelAS();
		void CreateTopLevelAS();

		void CreateRtDescriptorSet();
		void UpdateRtDescriptorSet();

		// void DestroyResources();

	private:
		VkPhysicalDeviceRayTracingPipelinePropertiesKHR m_RtProperties{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR};
		VkDevice m_Device;

		std::vector<Mesh> m_Models;
		std::vector<RTObjInstance> m_Instances;

		RayTracingBuilder m_RTBuilder;

		// nvvk::DescriptorSetBindings                     m_RTDescSetLayoutBind;
		// VkDescriptorPool                                m_RTDescPool;
		// VkDescriptorSetLayout                           m_RTDescSetLayout;
		// VkDescriptorSet                                 m_RTDescSet;
	};

}
