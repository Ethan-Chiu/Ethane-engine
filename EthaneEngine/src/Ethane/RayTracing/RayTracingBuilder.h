#pragma once

#include "Platform/Vulkan/Vulkan.h"

namespace Ethane {

    //TODO: mtf Resource alloc
    struct Buffer
    {
        VkBuffer Buffer;
        VkDeviceMemory BufferMemory;
    };

    // TODO: investigate
    struct AccelKHR
    {
        VkAccelerationStructureKHR Accel = VK_NULL_HANDLE;
        Buffer Buffer;
    };

    // Ray tracing BLAS and TLAS builder
    class RayTracingBuilder
    {
    public:
        // Inputs used to build Bottom-level acceleration structure.
        // You manage the lifetime of the buffer(s) referenced by the VkAccelerationStructureGeometryKHRs within.
        // In particular, you must make sure they are still valid and not being modified when the BLAS is built or updated.
        struct BlasInput
        {
            // Data used to build acceleration structure geometry
            std::vector<VkAccelerationStructureGeometryKHR>       asGeometry;
            std::vector<VkAccelerationStructureBuildRangeInfoKHR> asBuildOffsetInfo;
            VkBuildAccelerationStructureFlagsKHR                  flags{ 0 };
        };

        // Initializing the allocator and querying the raytracing properties
        void Setup(const VkDevice& device, uint32_t queueIndex);

        // Destroying all allocations
        void Destroy();

        // Returning the constructed top-level acceleration structure
        VkAccelerationStructureKHR GetAccelerationStructure() const;

        // Return the Acceleration Structure Device Address of a BLAS Id
        VkDeviceAddress GetBlasDeviceAddress(uint32_t blasId);

        // Create all the BLAS from the vector of BlasInput
        void BuildBlas(const std::vector<BlasInput>& input,
            VkBuildAccelerationStructureFlagsKHR flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);

        // Refit BLAS number blasIdx from updated buffer contents.
        void UpdateBlas(uint32_t blasIdx, BlasInput& blas, VkBuildAccelerationStructureFlagsKHR flags);

        // Build TLAS for static acceleration structures
        void BuildTlas(const std::vector<VkAccelerationStructureInstanceKHR>& instances,
            VkBuildAccelerationStructureFlagsKHR flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
            bool                                 update = false);

    protected:
        std::vector<AccelKHR> m_Blas; // Bottom-level acceleration structure
        AccelKHR m_Tlas;              // Top-level acceleration structure

        // Setup
        VkDevice                 m_Device{ VK_NULL_HANDLE };
        uint32_t                 m_QueueIndex{ 0 };

        struct BuildAccelerationStructure
        {
            VkAccelerationStructureBuildGeometryInfoKHR buildInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
            VkAccelerationStructureBuildSizesInfoKHR sizeInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };
            const VkAccelerationStructureBuildRangeInfoKHR* rangeInfo;
            AccelKHR                                  as;  // result acceleration structure
            AccelKHR                                  cleanupAS;
        };

        void CmdCreateBlas(
            VkCommandBuffer                          cmdBuf,
            std::vector<uint32_t>                    indices,
            std::vector<BuildAccelerationStructure>& buildAs,
            VkDeviceAddress                          scratchAddress,
            VkQueryPool                              queryPool);
        void CmdCompactBlas(VkCommandBuffer cmdBuf, std::vector<uint32_t> indices, std::vector<BuildAccelerationStructure>& buildAs, VkQueryPool queryPool);
        
        void CmdCreateTlas(
            VkCommandBuffer                      cmdBuf,          // Command buffer
            uint32_t                             countInstance,   // number of instances
            VkDeviceAddress                      instBufferAddr,  // Buffer address of instances
            Buffer& scratchBuffer,                          // Scratch buffer for construction
            VkBuildAccelerationStructureFlagsKHR flags,           // Build creation flag
            bool                                 update,          // Update == animation
            bool                                 motion           // Motion Blur
        );
        
        void DestroyNonCompacted(std::vector<uint32_t> indices, std::vector<BuildAccelerationStructure>& buildAs);
        bool HasFlag(VkFlags item, VkFlags flag) { return (item & flag) == flag; }

    // TODO: move to other file & integrate
    private:
        void destroy(AccelKHR& a_);

        void destroy(Buffer& b_);

        AccelKHR CreateAcceleration(VkAccelerationStructureCreateInfoKHR& accelCreateInfo);
    };
}