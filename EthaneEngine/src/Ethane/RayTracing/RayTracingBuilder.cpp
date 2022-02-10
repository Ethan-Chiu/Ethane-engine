#include <ethpch.h>
#include <numeric>
#include "RayTracingBuilder.h"
#include "Ethane/Platform/Vulkan/VulkanContext.h"


namespace Ethane {


	//--------------------------------------------------------------------------------------------------
	// Initializing the allocator and querying the raytracing properties
	//
	void RayTracingBuilder::Setup(const VkDevice& device, uint32_t queueIndex)
	{
		m_Device = device;
		m_QueueIndex = queueIndex;
	}

	//--------------------------------------------------------------------------------------------------
	// Destroying all allocations
	//
	void RayTracingBuilder::Destroy()
	{
		for (auto& b : m_Blas)
		{
			// TODO: change this when resource allocator is implemented
			destroy(b);
		}
		// TODO: 
		destroy(m_Tlas);
		m_Blas.clear();
	}

	//--------------------------------------------------------------------------------------------------
	// Returning the constructed top-level acceleration structure
	//
	VkAccelerationStructureKHR RayTracingBuilder::GetAccelerationStructure() const
	{
		return m_Tlas.Accel;
	}

	//--------------------------------------------------------------------------------------------------
	// Return the device address of a Blas previously created.
	//
	VkDeviceAddress RayTracingBuilder::GetBlasDeviceAddress(uint32_t blasId)
	{
		ETH_CORE_ASSERT(size_t(blasId) < m_Blas.size());
		VkAccelerationStructureDeviceAddressInfoKHR addressInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR };
		addressInfo.accelerationStructure = m_Blas[blasId].Accel;
		return vkGetAccelerationStructureDeviceAddressKHR(m_Device, &addressInfo);
	}

	//--------------------------------------------------------------------------------------------------
	// Create all the BLAS from the vector of BlasInput
	// - There will be one BLAS per input-vector entry
	// - There will be as many BLAS as input.size()
	// - The resulting BLAS (along with the inputs used to build) are stored in m_blas,
	//   and can be referenced by index.
	// - if flag has the 'Compact' flag, the BLAS will be compacted
	//
	void RayTracingBuilder::BuildBlas(const std::vector<BlasInput>& input, VkBuildAccelerationStructureFlagsKHR flags)
	{
		uint32_t     nbBlas = static_cast<uint32_t>(input.size());
		VkDeviceSize asTotalSize{ 0 };     // Memory size of all allocated BLAS
		VkDeviceSize maxScratchSize{ 0 };  // Largest scratch size
		uint32_t     nbCompactions{ 0 };   // Nb of BLAS requesting compaction

		// Preparing the information for the acceleration build commands.
		std::vector<BuildAccelerationStructure> buildAs(nbBlas);
		for (uint32_t idx = 0; idx < nbBlas; idx++)
		{
			// Filling partially the VkAccelerationStructureBuildGeometryInfoKHR for querying the build sizes.
			// Other information will be filled in the createBlas (see #2)
			buildAs[idx].buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
			buildAs[idx].buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
			buildAs[idx].buildInfo.flags = input[idx].flags | flags;
			buildAs[idx].buildInfo.geometryCount = static_cast<uint32_t>(input[idx].asGeometry.size());
			buildAs[idx].buildInfo.pGeometries = input[idx].asGeometry.data();

			// Build range information
			buildAs[idx].rangeInfo = input[idx].asBuildOffsetInfo.data();

			// Finding sizes to create acceleration structures and scratch
			std::vector<uint32_t> maxPrimCount(input[idx].asBuildOffsetInfo.size());
			for (auto tt = 0; tt < input[idx].asBuildOffsetInfo.size(); tt++)
				maxPrimCount[tt] = input[idx].asBuildOffsetInfo[tt].primitiveCount;  // Number of primitives/triangles
			vkGetAccelerationStructureBuildSizesKHR(m_Device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
				&buildAs[idx].buildInfo, maxPrimCount.data(), &buildAs[idx].sizeInfo);

			// Extra info
			asTotalSize += buildAs[idx].sizeInfo.accelerationStructureSize;
			maxScratchSize = std::max(maxScratchSize, buildAs[idx].sizeInfo.buildScratchSize);
			nbCompactions += HasFlag(buildAs[idx].buildInfo.flags, VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR);
		}

		// Allocate the scratch buffers holding the temporary data of the acceleration structure builder
		VkBuffer scratchBuffer;
		VkDeviceMemory scratchBufferMemory;
		Utils::CreateBuffer(maxScratchSize, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			scratchBuffer, scratchBufferMemory);
		VkBufferDeviceAddressInfo bufferInfo{ VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, nullptr, scratchBuffer };
		VkDeviceAddress           scratchAddress = vkGetBufferDeviceAddress(m_Device, &bufferInfo);

		// Allocate a query pool for storing the needed size for every BLAS compaction.
		VkQueryPool queryPool{ VK_NULL_HANDLE };
		if (nbCompactions > 0)  // Is compaction requested?
		{
			assert(nbCompactions == nbBlas);  // Don't allow mix of on/off compaction
			VkQueryPoolCreateInfo qpci{ VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO };
			qpci.queryCount = nbBlas;
			qpci.queryType = VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR;
			vkCreateQueryPool(m_Device, &qpci, nullptr, &queryPool);
		}

		// Batching creation/compaction of BLAS to allow staying in restricted amount of memory
		std::vector<uint32_t> indices;  // Indices of the BLAS to create
		VkDeviceSize          batchSize{ 0 };
		VkDeviceSize          batchLimit{ 256'000'000 };  // 256 MB
		for (uint32_t idx = 0; idx < nbBlas; idx++)
		{
			indices.push_back(idx);
			batchSize += buildAs[idx].sizeInfo.accelerationStructureSize;
			// Over the limit or last BLAS element
			if (batchSize >= batchLimit || idx == nbBlas - 1)
			{
				VkCommandBuffer cmdBuf = VulkanContext::GetDevice()->CreateCommandBuffer(QueueFamilyTypes::Graphics, true, true);;
				CmdCreateBlas(cmdBuf, indices, buildAs, scratchAddress, queryPool);
				VulkanContext::GetDevice()->SubmitCommandBuffer(cmdBuf);

				if (queryPool)
				{
					VkCommandBuffer cmdBuf = VulkanContext::GetDevice()->CreateCommandBuffer(QueueFamilyTypes::Graphics, true, true);;
					CmdCompactBlas(cmdBuf, indices, buildAs, queryPool);
					VulkanContext::GetDevice()->SubmitCommandBuffer(cmdBuf);  // Submit command buffer and call vkQueueWaitIdle

					// Destroy the non-compacted version
					DestroyNonCompacted(indices, buildAs);
				}
				// Reset

				batchSize = 0;
				indices.clear();
			}
		}

		// Logging reduction
		if (queryPool)
		{
			VkDeviceSize compactSize = std::accumulate(buildAs.begin(), buildAs.end(), 0ULL, [](const auto& a, const auto& b) {
				return a + b.sizeInfo.accelerationStructureSize;
				});
			ETH_CORE_INFO(" RT BLAS: reducing from: {0} to: {1} = {2} ({3}% smaller)", asTotalSize, compactSize,
				asTotalSize - compactSize, (asTotalSize - compactSize) / float(asTotalSize) * 100.f);
		}

		// Keeping all the created acceleration structures
		for (auto& b : buildAs)
		{
			m_Blas.emplace_back(b.as);
		}

		// Clean up
		vkDestroyQueryPool(m_Device, queryPool, nullptr);
		// m_alloc->finalizeAndReleaseStaging();
		// cleanup staging buffer
		vkDestroyBuffer(m_Device, scratchBuffer, nullptr);
		vkFreeMemory(m_Device, scratchBufferMemory, nullptr);
	}

	//--------------------------------------------------------------------------------------------------
	// Creating the bottom level acceleration structure for all indices of `buildAs` vector.
	// The array of BuildAccelerationStructure was created in buildBlas and the vector of
	// indices limits the number of BLAS to create at once. This limits the amount of
	// memory needed when compacting the BLAS.
	void RayTracingBuilder::CmdCreateBlas(
		VkCommandBuffer							 cmdBuf, //RaytracingBuilderKHR::
		std::vector<uint32_t>                    indices,
		std::vector<BuildAccelerationStructure>& buildAs,
		VkDeviceAddress                          scratchAddress,
		VkQueryPool                              queryPool)
	{
		if (queryPool)  // For querying the compaction size
			vkResetQueryPool(m_Device, queryPool, 0, static_cast<uint32_t>(indices.size()));
		uint32_t queryCnt{ 0 };

		for (const auto& idx : indices)
		{
			// Actual allocation of buffer and acceleration structure.
			VkAccelerationStructureCreateInfoKHR createInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
			createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
			createInfo.size = buildAs[idx].sizeInfo.accelerationStructureSize;  // Will be used to allocate memory.
			buildAs[idx].as = CreateAcceleration(createInfo);

			// BuildInfo #2 part
			buildAs[idx].buildInfo.dstAccelerationStructure = buildAs[idx].as.Accel;  // Setting where the build lands
			buildAs[idx].buildInfo.scratchData.deviceAddress = scratchAddress;  // All build are using the same scratch buffer

			// Building the bottom-level-acceleration-structure
			vkCmdBuildAccelerationStructuresKHR(cmdBuf, 1, &buildAs[idx].buildInfo, &buildAs[idx].rangeInfo);

			// Since the scratch buffer is reused across builds, we need a barrier to ensure one build
			// is finished before starting the next one.
			VkMemoryBarrier barrier{ VK_STRUCTURE_TYPE_MEMORY_BARRIER };
			barrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
			barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
			vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
				VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 1, &barrier, 0, nullptr, 0, nullptr);

			if (queryPool)
			{
				// Add a query to find the 'real' amount of memory needed, use for compaction
				vkCmdWriteAccelerationStructuresPropertiesKHR(cmdBuf, 1, &buildAs[idx].buildInfo.dstAccelerationStructure,
					VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR, queryPool, queryCnt++);
			}
		}
	}


	//--------------------------------------------------------------------------------------------------
	// Create and replace a new acceleration structure and buffer based on the size retrieved by the
	// Query.
	void RayTracingBuilder::CmdCompactBlas(
		VkCommandBuffer                          cmdBuf,
		std::vector<uint32_t>                    indices,
		std::vector<BuildAccelerationStructure>& buildAs,
		VkQueryPool                              queryPool)
	{
		uint32_t queryCtn{ 0 };
		std::vector<AccelKHR> cleanupAS;  // previous AS to destroy

		// Get the compacted size result back
		std::vector<VkDeviceSize> compactSizes(static_cast<uint32_t>(indices.size()));
		vkGetQueryPoolResults(m_Device, queryPool, 0, (uint32_t)compactSizes.size(), compactSizes.size() * sizeof(VkDeviceSize),
			compactSizes.data(), sizeof(VkDeviceSize), VK_QUERY_RESULT_WAIT_BIT);

		for (auto idx : indices)
		{
			buildAs[idx].cleanupAS = buildAs[idx].as;           // previous AS to destroy
			buildAs[idx].sizeInfo.accelerationStructureSize = compactSizes[queryCtn++];  // new reduced size

			// Creating a compact version of the AS
			VkAccelerationStructureCreateInfoKHR asCreateInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
			asCreateInfo.size = buildAs[idx].sizeInfo.accelerationStructureSize;
			asCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
			buildAs[idx].as = CreateAcceleration(asCreateInfo);

			// Copy the original BLAS to a compact version
			VkCopyAccelerationStructureInfoKHR copyInfo{ VK_STRUCTURE_TYPE_COPY_ACCELERATION_STRUCTURE_INFO_KHR };
			copyInfo.src = buildAs[idx].buildInfo.dstAccelerationStructure;
			copyInfo.dst = buildAs[idx].as.Accel;
			copyInfo.mode = VK_COPY_ACCELERATION_STRUCTURE_MODE_COMPACT_KHR;
			vkCmdCopyAccelerationStructureKHR(cmdBuf, &copyInfo);
		}
	}

	//--------------------------------------------------------------------------------------------------
	// Destroy all the non-compacted acceleration structures
	//
	void RayTracingBuilder::DestroyNonCompacted(std::vector<uint32_t> indices, std::vector<BuildAccelerationStructure>& buildAs)
	{
		for (auto& i : indices)
		{
			vkDestroyAccelerationStructureKHR(m_Device, buildAs[i].cleanupAS.Accel, nullptr);
			vkDestroyBuffer(m_Device, buildAs[i].cleanupAS.Buffer.Buffer, nullptr);
			vkFreeMemory(m_Device, buildAs[i].cleanupAS.Buffer.BufferMemory, nullptr);
		}
	}

	//--------------------------------------------------------------------------------------------------
	// 
	void RayTracingBuilder::BuildTlas(const std::vector<VkAccelerationStructureInstanceKHR>& instances, VkBuildAccelerationStructureFlagsKHR flags, bool update)
	{
		// Cannot call buildTlas twice except to update.
		ETH_CORE_ASSERT(m_Tlas.Accel == VK_NULL_HANDLE || update);
		uint32_t countInstance = static_cast<uint32_t>(instances.size());

		// Command buffer to create the TLAS
		VkCommandBuffer cmdBuf = VulkanContext::GetDevice()->CreateCommandBuffer(QueueFamilyTypes::Graphics, true, true);

		// TODO: mtf /use VMA
		// create staging buffer
		uint32_t size = sizeof(VkAccelerationStructureInstanceKHR) * instances.size();
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		Utils::CreateBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer, stagingBufferMemory);
		// copy data to staging buffer
		void* dstData;
		vkMapMemory(m_Device, stagingBufferMemory, 0, size, 0, &dstData);
		memcpy(dstData, instances.data(), size);
		vkUnmapMemory(m_Device, stagingBufferMemory);
		//

		// Create a buffer holding the actual instance data (matrices++) for use by the AS builder
		Buffer instancesBuffer;  // Buffer of instances containing the matrices and BLAS ids
		Utils::CreateBuffer(size, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			instancesBuffer.Buffer, instancesBuffer.BufferMemory);
		VkBufferDeviceAddressInfo bufferInfo{ VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, nullptr, instancesBuffer.Buffer };
		VkDeviceAddress           instBufferAddr = vkGetBufferDeviceAddress(m_Device, &bufferInfo);

		// TODO: mtf /use VMA
		// copy from staging buffer
		// Utils::CopyBuffer(instancesBuffer.Buffer, stagingBuffer, size);
		//

		// Make sure the copy of the instance buffer are copied before triggering the acceleration structure build
		VkMemoryBarrier barrier{ VK_STRUCTURE_TYPE_MEMORY_BARRIER };
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
		vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
			0, 1, &barrier, 0, nullptr, 0, nullptr);

		// Creating the TLAS
		Buffer scratchBuffer;
		CmdCreateTlas(cmdBuf, countInstance, instBufferAddr, scratchBuffer, flags, update, false);

		// Finalizing and destroying temporary data
		VulkanContext::GetDevice()->SubmitCommandBuffer(cmdBuf);  // queueWaitIdle inside.
		// m_alloc->finalizeAndReleaseStaging();
		vkDestroyBuffer(m_Device, scratchBuffer.Buffer, nullptr);
		vkFreeMemory(m_Device, scratchBuffer.BufferMemory, nullptr);
		vkDestroyBuffer(m_Device, instancesBuffer.Buffer, nullptr);
		vkFreeMemory(m_Device, instancesBuffer.BufferMemory, nullptr);
	}


	//--------------------------------------------------------------------------------------------------
	// Low level of Tlas creation
	//
	void RayTracingBuilder::CmdCreateTlas(
		VkCommandBuffer                      cmdBuf,
		uint32_t                             countInstance,
		VkDeviceAddress                      instBufferAddr,
		Buffer&								 scratchBuffer,
		VkBuildAccelerationStructureFlagsKHR flags,
		bool                                 update,
		bool								 motion)
	{
		// Wraps a device pointer to the above uploaded instances.
		VkAccelerationStructureGeometryInstancesDataKHR instancesVk{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR };
		instancesVk.data.deviceAddress = instBufferAddr;

		// Put the above into a VkAccelerationStructureGeometryKHR. We need to put the instances struct in a union and label it as instance data.
		VkAccelerationStructureGeometryKHR topASGeometry{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
		topASGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
		topASGeometry.geometry.instances = instancesVk;

		// Find sizes
		VkAccelerationStructureBuildGeometryInfoKHR buildInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
		buildInfo.flags = flags;
		buildInfo.geometryCount = 1;
		buildInfo.pGeometries = &topASGeometry;
		buildInfo.mode = update ? VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR : VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
		buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
		buildInfo.srcAccelerationStructure = VK_NULL_HANDLE;

		VkAccelerationStructureBuildSizesInfoKHR sizeInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };
		vkGetAccelerationStructureBuildSizesKHR(m_Device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo,
			&countInstance, &sizeInfo);

		// Create TLAS
		if (update == false)
		{

			VkAccelerationStructureCreateInfoKHR createInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
			createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
			createInfo.size = sizeInfo.accelerationStructureSize;

			m_Tlas = CreateAcceleration(createInfo);
		}

		// Allocate the scratch memory
		Utils::CreateBuffer(sizeInfo.buildScratchSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			scratchBuffer.Buffer, scratchBuffer.BufferMemory);
		VkBufferDeviceAddressInfo bufferInfo{ VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, nullptr, scratchBuffer.Buffer };
		VkDeviceAddress           scratchAddress = vkGetBufferDeviceAddress(m_Device, &bufferInfo);

		// Update build information
		buildInfo.srcAccelerationStructure = update ? m_Tlas.Accel : VK_NULL_HANDLE;
		buildInfo.dstAccelerationStructure = m_Tlas.Accel;
		buildInfo.scratchData.deviceAddress = scratchAddress;

		// Build Offsets info: n instances
		VkAccelerationStructureBuildRangeInfoKHR        buildOffsetInfo{ countInstance, 0, 0, 0 };
		const VkAccelerationStructureBuildRangeInfoKHR* pBuildOffsetInfo = &buildOffsetInfo;

		// Build the TLAS
		vkCmdBuildAccelerationStructuresKHR(cmdBuf, 1, &buildInfo, &pBuildOffsetInfo);
	}


	//--------------------------------------------------------------------------------------------------
	// Refit BLAS number blasIdx from updated buffer contents.
	//
	void RayTracingBuilder::UpdateBlas(uint32_t blasIdx, BlasInput& blas, VkBuildAccelerationStructureFlagsKHR flags)
	{
		ETH_CORE_ASSERT(size_t(blasIdx) < m_Blas.size());

		// Preparing all build information, acceleration is filled later
		VkAccelerationStructureBuildGeometryInfoKHR buildInfos{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
		buildInfos.flags = flags;
		buildInfos.geometryCount = (uint32_t)blas.asGeometry.size();
		buildInfos.pGeometries = blas.asGeometry.data();
		buildInfos.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR;  // UPDATE
		buildInfos.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		buildInfos.srcAccelerationStructure = m_Blas[blasIdx].Accel;  // UPDATE
		buildInfos.dstAccelerationStructure = m_Blas[blasIdx].Accel;

		// Find size to build on the device
		std::vector<uint32_t> maxPrimCount(blas.asBuildOffsetInfo.size());
		for (auto tt = 0; tt < blas.asBuildOffsetInfo.size(); tt++)
			maxPrimCount[tt] = blas.asBuildOffsetInfo[tt].primitiveCount;  // Number of primitives/triangles
		VkAccelerationStructureBuildSizesInfoKHR sizeInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };
		vkGetAccelerationStructureBuildSizesKHR(m_Device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfos,
			maxPrimCount.data(), &sizeInfo);

		// Allocate the scratch buffers holding the temporary data of the acceleration structure builder
		Buffer scratchBuffer;
		Utils::CreateBuffer(sizeInfo.buildScratchSize, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			scratchBuffer.Buffer, scratchBuffer.BufferMemory);
		VkBufferDeviceAddressInfo bufferInfo{ VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
		bufferInfo.buffer = scratchBuffer.Buffer;
		buildInfos.scratchData.deviceAddress = vkGetBufferDeviceAddress(m_Device, &bufferInfo);

		std::vector<const VkAccelerationStructureBuildRangeInfoKHR*> pBuildOffset(blas.asBuildOffsetInfo.size());
		for (size_t i = 0; i < blas.asBuildOffsetInfo.size(); i++)
			pBuildOffset[i] = &blas.asBuildOffsetInfo[i];

		// Update the instance buffer on the device side and build the TLAS
		VkCommandBuffer cmdBuf = VulkanContext::GetDevice()->CreateCommandBuffer(QueueFamilyTypes::Graphics, true, true);

		// Update the acceleration structure. Note the VK_TRUE parameter to trigger the update,
		// and the existing BLAS being passed and updated in place
		vkCmdBuildAccelerationStructuresKHR(cmdBuf, 1, &buildInfos, pBuildOffset.data());

		VulkanContext::GetDevice()->SubmitCommandBuffer(cmdBuf);  // queueWaitIdle inside.

		destroy(scratchBuffer); // TODO: 
	}






	void RayTracingBuilder::destroy(AccelKHR& a_)
	{
		vkDestroyAccelerationStructureKHR(m_Device, a_.Accel, nullptr);
		destroy(a_.Buffer);

		a_ = AccelKHR();
	}

	void RayTracingBuilder::destroy(Buffer& b_)
	{
		vkDestroyBuffer(m_Device, b_.Buffer, nullptr);
		vkFreeMemory(m_Device, b_.BufferMemory, nullptr);

		b_ = Buffer();
	}

	AccelKHR RayTracingBuilder::CreateAcceleration(VkAccelerationStructureCreateInfoKHR& accelCreateInfo)
	{
		AccelKHR resultAccel;

		// Allocating the buffer to hold the acceleration structure
		Utils::CreateBuffer(accelCreateInfo.size, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			resultAccel.Buffer.Buffer, resultAccel.Buffer.BufferMemory);

		// Setting the buffer
		accelCreateInfo.buffer = resultAccel.Buffer.Buffer;
		// Create the acceleration structure
		vkCreateAccelerationStructureKHR(VulkanContext::GetDevice()->GetVulkanDevice(), &accelCreateInfo, nullptr, &resultAccel.Accel);

		return resultAccel;
	}
}

