//
//  VulkanDescriptorSetStorage.h
//  EthaneEngine
//
//  Created by 邱奕翔 on 2023/10/13.
//

#pragma once

#include "VulkanContext.h"

//namespace Ethane {
//
//    class VulkanDescriptorSetsLayout
//    {
//    private:
//        std::vector<VkDescriptorSetLayout> m_SetsLayout;
//    };
//
//    class VulkanDescriptorPool
//    {
//    public:
//        VulkanDescriptorPool(const VulkanDevice* device);
//        VkDescriptorSet AcquireDescriptorSets();
//        uint32_t AllocateDescriptorSets();
//        
//        VkDescriptorPool GetHandle() { return m_Handle; }
//    private:
//        VkDescriptorPool m_Handle;
//        const VulkanDevice* m_Device = nullptr;
//        std::unordered_map<uint32_t, VkDescriptorSet> m_DescriptorSetMap;
//    };
//
//    class VulkanDescriptorPoolList
//    {
//    public:
//        VkDescriptorSet AcquireDescriptorset(const VulkanDescriptorSetsLayout& layout);
//        
//    private:
//        VulkanDevice* const m_Device = nullptr;
//        std::vector<VulkanDescriptorPool> m_PoolList = {};
//    };
//
//    class VulkanDescriptorPoolStorage
//    {
//    public:
//        VulkanDescriptorPoolStorage() = default;
//        
//        Scope<VulkanDescriptorPoolList> AquirePoolList(uint32_t type);
//        
//    private:
//        std::vector<Scope<VulkanDescriptorPoolList>> m_PoolLists;
//    };
//
//}
