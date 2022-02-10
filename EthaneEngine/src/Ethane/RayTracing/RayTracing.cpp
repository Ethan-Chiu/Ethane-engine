#include <ethpch.h>
#include "RayTracing.h"

#include <numeric>
#include "Ethane/Platform/Vulkan/VulkanContext.h"

namespace Ethane {

	// TODO: mtf Buffer utils
	inline VkDeviceAddress getBufferDeviceAddress(VkDevice device, VkBuffer buffer)
	{
		VkBufferDeviceAddressInfo info = { VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
		info.buffer = buffer;
		return vkGetBufferDeviceAddress(device, &info);
	}

	// TODO: mtf vulkan utils/ray tracing utils
	// Convert a Mat4x4 to the matrix required by acceleration structures
	inline VkTransformMatrixKHR GlmMat4toTransformMatrixKHR(glm::mat4 matrix)
	{
		glm::mat4 temp = glm::transpose(matrix);
		VkTransformMatrixKHR out_matrix;
		memcpy(&out_matrix, &temp, sizeof(VkTransformMatrixKHR));
		return out_matrix;
	}


	////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////


	void RayTracing::InitRayTracing()
	{
		// Request ray tracing properties
		VkPhysicalDeviceProperties2 props2{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
		props2.pNext = &m_RtProperties;
		vkGetPhysicalDeviceProperties2(VulkanContext::GetPhysicalDevice()->GetVulkanPhysicalDevice(), &props2);

		m_Device = VulkanContext::GetDevice()->GetVulkanDevice();

		uint32_t queueIndex = VulkanContext::GetPhysicalDevice()->GetQueueFamilyIndices().Graphics.value();

		m_RTBuilder.Setup(m_Device, queueIndex);
	}


	//--------------------------------------------------------------------------------------------------
	// Converting a OBJ primitive to the ray tracing geometry used for the BLAS
	//
	auto RayTracing::MeshToVkGeometryKHR(const Mesh& model)
	{
		// BLAS builder requires raw device addresses
		VkDeviceAddress vertexAddress = getBufferDeviceAddress(m_Device, std::dynamic_pointer_cast<VulkanVertexBuffer>(model.m_VertexBuffer)->GetVulkanBuffer());
		VkDeviceAddress indexAddress = getBufferDeviceAddress(m_Device, std::dynamic_pointer_cast<VulkanIndexBuffer>(model.m_IndexBuffer)->GetVulkanBuffer());

		// Describe buffer as array of VertexObj
		VkAccelerationStructureGeometryTrianglesDataKHR triangles{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR };
		triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
		triangles.vertexData.deviceAddress = vertexAddress;
		triangles.vertexStride = sizeof(Vertex);
		// Describe index data 
		triangles.indexType = VK_INDEX_TYPE_UINT32;
		triangles.indexData.deviceAddress = indexAddress;
		// triangles.transformData = {};
		triangles.maxVertex = model.GetVertexNum();

		// Setting up the build info of the acceleration
		VkAccelerationStructureGeometryKHR asGeom{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
		asGeom.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
		asGeom.flags = VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_KHR;  // For AnyHit
		asGeom.geometry.triangles = triangles;

		VkAccelerationStructureBuildRangeInfoKHR offset;
		offset.firstVertex = 0;
		offset.primitiveCount = model.GetIndexNum() / 3;  // Nb triangles
		offset.primitiveOffset = 0;
		offset.transformOffset = 0;

		RayTracingBuilder::BlasInput input;
		input.asGeometry.emplace_back(asGeom);
		input.asBuildOffsetInfo.emplace_back(offset);
		return input;
	}


	void RayTracing::CreateBottomLevelAS()
	{
		// BLAS - Storing each primitive in a geometry
		std::vector<RayTracingBuilder::BlasInput> allBlas;
		allBlas.reserve(m_Models.size());
		for (const auto& obj : m_Models)
		{
			auto blas = MeshToVkGeometryKHR(obj);

			// We could add more geometry in each BLAS, but we add only one for now
			allBlas.emplace_back(blas);
		}

		m_RTBuilder.BuildBlas(allBlas, VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR);
	}


	void RayTracing::CreateTopLevelAS()
	{
		std::vector<VkAccelerationStructureInstanceKHR> tlas;
		tlas.reserve(m_Instances.size());

		for (const RTObjInstance& inst : m_Instances)
		{
			VkAccelerationStructureInstanceKHR rayInst{};
			rayInst.transform = GlmMat4toTransformMatrixKHR(inst.Transform);  // Position of the instance
			rayInst.instanceCustomIndex = inst.ObjIndex; // gl_InstanceCustomIndexEXT
			rayInst.accelerationStructureReference = m_RTBuilder.GetBlasDeviceAddress(inst.ObjIndex);
			rayInst.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
			rayInst.mask = 0xFF;  // Only be hit if rayMask & instance.mask != 0
			rayInst.instanceShaderBindingTableRecordOffset = 0;     // We will use the same hit group for all objects
			tlas.emplace_back(rayInst);
		}

		m_RTBuilder.BuildTlas(tlas, VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);
	}


	//--------------------------------------------------------------------------------------------------
	// This descriptor set holds the Acceleration structure and the output image
	//
	void RayTracing::CreateRtDescriptorSet()
	{
		// Top-level acceleration structure, usable by both the ray generation and the closest hit (to shoot shadow rays)
		// m_RTDescSetLayoutBind.addBinding(RtxBindings::eTlas, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1,
		// 	VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);  // TLAS
		// m_RTDescSetLayoutBind.addBinding(RtxBindings::eOutImage, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1,
		// 	VK_SHADER_STAGE_RAYGEN_BIT_KHR);  // Output image
		// 
		// m_RTDescPool = m_RTDescSetLayoutBind.createPool(m_Device);
		// m_RTDescSetLayout = m_RTDescSetLayoutBind.createLayout(m_Device);
		// 
		// VkDescriptorSetAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		// allocateInfo.descriptorPool = m_RTDescPool;
		// allocateInfo.descriptorSetCount = 1;
		// allocateInfo.pSetLayouts = &m_RTDescSetLayout;
		// vkAllocateDescriptorSets(m_Device, &allocateInfo, &m_RTDescSet);
		// 
		// 
		// VkAccelerationStructureKHR                   tlas = m_RTBuilder.GetAccelerationStructure();
		// VkWriteDescriptorSetAccelerationStructureKHR descASInfo{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR };
		// descASInfo.accelerationStructureCount = 1;
		// descASInfo.pAccelerationStructures = &tlas;
		// VkDescriptorImageInfo imageInfo{ {}, m_offscreenColor.descriptor.imageView, VK_IMAGE_LAYOUT_GENERAL };
		// 
		// std::vector<VkWriteDescriptorSet> writes;
		// writes.emplace_back(m_RTDescSetLayoutBind.makeWrite(m_RTDescSet, RtxBindings::eTlas, &descASInfo));
		// writes.emplace_back(m_RTDescSetLayoutBind.makeWrite(m_RTDescSet, RtxBindings::eOutImage, &imageInfo));
		// vkUpdateDescriptorSets(m_Device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
	}

	//--------------------------------------------------------------------------------------------------
	// Writes the output image to the descriptor set
	// - Required when changing resolution
	//
	void RayTracing::UpdateRtDescriptorSet()
	{
		// (1) Output buffer
		// VkDescriptorImageInfo imageInfo{ {}, m_offscreenColor.descriptor.imageView, VK_IMAGE_LAYOUT_GENERAL };
		// VkWriteDescriptorSet  wds = m_RTDescSetLayoutBind.makeWrite(m_RTDescSet, RtxBindings::eOutImage, &imageInfo);
		// vkUpdateDescriptorSets(m_Device, 1, &wds, 0, nullptr);
	}

	//--------------------------------------------------------------------------------------------------
	// Destroying all allocations
	//
	// void RayTracing::DestroyResources()
	// {
		// vkDestroyPipeline(m_Device, m_graphicsPipeline, nullptr);
		// vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
		// vkDestroyDescriptorPool(m_device, m_descPool, nullptr);
		// vkDestroyDescriptorSetLayout(m_device, m_descSetLayout, nullptr);

		// m_alloc.destroy(m_bGlobals);
		// m_alloc.destroy(m_bObjDesc);

		// for (auto& m : m_Models)
		// {
			// m_alloc.destroy(m.vertexBuffer);
			// m_alloc.destroy(m.indexBuffer);
			// m_alloc.destroy(m.matColorBuffer);
			// m_alloc.destroy(m.matIndexBuffer);
		// }

		// for (auto& t : m_textures)
		// {
		// 	m_alloc.destroy(t);
		// }

		//#Post
		// m_alloc.destroy(m_offscreenColor);
		// m_alloc.destroy(m_offscreenDepth);
		// vkDestroyPipeline(m_device, m_postPipeline, nullptr);
		// vkDestroyPipelineLayout(m_device, m_postPipelineLayout, nullptr);
		// vkDestroyDescriptorPool(m_device, m_postDescPool, nullptr);
		// vkDestroyDescriptorSetLayout(m_device, m_postDescSetLayout, nullptr);
		// vkDestroyRenderPass(m_device, m_offscreenRenderPass, nullptr);
		// vkDestroyFramebuffer(m_device, m_offscreenFramebuffer, nullptr);


		// #VKRay
		// m_RTBuilder.destroy();
		// vkDestroyPipeline(m_device, m_rtPipeline, nullptr);
		// vkDestroyPipelineLayout(m_device, m_rtPipelineLayout, nullptr);
		// vkDestroyDescriptorPool(m_device, m_rtDescPool, nullptr);
		// vkDestroyDescriptorSetLayout(m_device, m_rtDescSetLayout, nullptr);
		// m_alloc.destroy(m_rtSBTBuffer);

		// m_alloc.deinit();
	// }
}