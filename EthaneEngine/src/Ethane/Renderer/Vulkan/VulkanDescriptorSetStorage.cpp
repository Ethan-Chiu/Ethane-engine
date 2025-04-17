//
//  VulkanDescriptorSetStorage.cpp
//  EthaneEngine
//
//  Created by 邱奕翔 on 2023/10/13.
//
//#include "ethpch.h"
//#include "VulkanDescriptorSetStorage.h"
//
//namespace Ethane {
//    
//    VulkanDescriptorPool::VulkanDescriptorPool(const VulkanDevice* device)
//        :m_Device(device)
//    {
//        VkDescriptorPool out_pool;
//        std::vector<VkDescriptorPoolSize> poolSizes;
//        VkDescriptorPoolCreateInfo descriptorPoolInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
//        descriptorPoolInfo.pNext = nullptr;
//        descriptorPoolInfo.poolSizeCount = (uint32_t)poolSizes.size();
//        descriptorPoolInfo.pPoolSizes = poolSizes.data();
//        descriptorPoolInfo.maxSets = (uint32_t)(numberOfSets * setCount);
//        VK_CHECK_RESULT(vkCreateDescriptorPool(device->GetVulkanDevice(), &descriptorPoolInfo, nullptr, &m_Handle));
//    }
//
//}
