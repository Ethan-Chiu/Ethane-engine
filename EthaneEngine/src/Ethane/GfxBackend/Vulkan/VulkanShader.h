#pragma once

#include "VulkanContext.h"
#include "Ethane/GfxBackend/GfxObject.h"
#include "ShaderUtils/VulkanShaderCompiler.h"


namespace Ethane {

	class VulkanShader : public IShader
	{
	public:
		struct DescriptorSetsAndPool
		{
			// NOTE: declaration order matters, sets are freed before their pool is destroyed
			vk::raii::DescriptorPool Pool = nullptr;
			std::vector<vk::raii::DescriptorSet> DescriptorSets;
		};

	public:
		VulkanShader() = delete;
		VulkanShader(const VulkanDevice* device, const std::string& filepath);
		virtual ~VulkanShader();

        void Destroy();
        
		// Getter
		virtual const std::string& GetName() const override { return  m_Name; }
        
    friend class VulkanPipeline;
    friend class VulkanComputePipeline;
    friend class VulkanShaderSystem;
    protected:
		const std::vector<vk::PipelineShaderStageCreateInfo>& GetPipelineShaderStageCreateInfos() const { return m_PipelineShaderStageCreateInfos; }
		std::vector<vk::PushConstantRange> GetPushConstantRanges() const;
		std::vector<vk::DescriptorSetLayout> GetAllDescriptorSetLayouts() const;
		const std::vector<VulkanShaderCompiler::ShaderDescriptorSetData>& GetShaderDescriptorSetData() const { return m_ShaderDescriptorSetsReflect; }
        
    public:
        // Pool sized for `numberOfSets` copies of every set of this shader. Created with eFreeDescriptorSet,
        // so the raii descriptor sets allocated from it can free themselves.
        vk::raii::DescriptorPool CreateDescriptorPool(uint32_t numberOfSets = 1) const;
        vk::raii::DescriptorSet CreateDescriptorSet(uint32_t set, const vk::raii::DescriptorPool& pool) const;
        

	private:
		void CreatePipelineShaderStage(const std::unordered_map<VkShaderStageFlagBits, std::vector<uint32_t>>& shaderData);
		void CreateDescriptorLayouts();
	
	private:
        const VulkanDevice* m_Device;
        VulkanLiveResource m_LiveResource;
		std::string m_FilePath;
		std::string m_Name;
        
		// Data from Reflect
		std::vector<VulkanShaderCompiler::ShaderDescriptorSetData> m_ShaderDescriptorSetsReflect;
		std::vector<VkPushConstantRange> m_PushConstantRanges; // filled by VulkanShaderCompiler (C types)

		// Descriptor Set Layouts, indexed by set
		std::vector<vk::raii::DescriptorSetLayout> m_DescriptorSetLayouts;

		// Descriptor pool size info
		std::unordered_map<uint32_t, std::vector<VkDescriptorPoolSize>> m_DescriptorCounts;

		// For pipeine creation
		std::vector<vk::raii::ShaderModule> m_ShaderModules;
		std::vector<vk::PipelineShaderStageCreateInfo> m_PipelineShaderStageCreateInfos; // reference m_ShaderModules

	public:
		struct WriteDescriptorSetBase {
			VkWriteDescriptorSet WriteDescriptor;
		};

		const WriteDescriptorSetBase* RetrieveWriteDescriptorSetBase(uint32_t set, const std::string& name) const;
		const std::vector<std::unordered_map<std::string, WriteDescriptorSetBase>>& RetrieveWriteDescriptorSetsBase() const {
			return m_WriteDescriptorSetsBase;
		};
	private:
		// Write Descriptors Templates
		std::vector<std::unordered_map<std::string, WriteDescriptorSetBase>> m_WriteDescriptorSetsBase; // set -> [name, write_descriptor_set]
	};
}