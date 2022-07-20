#pragma once

#include "Ethane/Renderer/Shader.h"

#include "Vulkan.h"

#include "ShaderUtils/VulkanShaderCompiler.h"


namespace Ethane {

	class VulkanShader : public Shader
	{
	public:
		struct DescriptorSetsAndPool
		{
			VkDescriptorPool Pool = nullptr;
			std::vector<VkDescriptorSet> DescriptorSets;
		};

	public:
		VulkanShader() = default;
		VulkanShader(const std::string& filepath);
		VulkanShader(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc) {}; // temp
		virtual ~VulkanShader();

		//TODO should override
		void Destroy();

		// Getter
		virtual const std::string& GetName() const override { return  m_Name; }
		const std::vector<VkPipelineShaderStageCreateInfo>& GetPipelineShaderStageCreateInfos() const { return m_PipelineShaderStageCreateInfos; }
		const std::vector<VkPushConstantRange> GetPushConstantRanges() const { return m_PushConstantRanges; } // TODO
		std::vector<VkDescriptorSetLayout> GetAllDescriptorSetLayouts();
		const std::vector<VulkanShaderCompiler::ShaderDescriptorSetData>& GetShaderDescriptorSetData() const { return m_ShaderDescriptorSets; }
		const VkWriteDescriptorSet* GetWriteDescriptorSet(uint32_t set, const std::string& name) const;

		// TODO: remove this
		//Uniform
		virtual uint32_t GetUniformBufferIndex(uint32_t bindingPoint) { return 0; };
		virtual void SetUniformBuffer(uint32_t uboIndex, const void* data, uint32_t size, uint32_t offset = 0){};
		virtual void SetUniformBufferByBindingPoint(uint32_t bindingPoint, const void* data, uint32_t size, uint32_t offset = 0) {};
		virtual uint32_t GetUniformBufferIndex(const std::string& name) { return 0; }; // test
		virtual void SetUniformBufferByName(const std::string& name, const void* data, uint32_t size) {}; // test

	private:
		std::unordered_map<VkShaderStageFlagBits, std::string> PreProcess(const std::string& source);

		void CreatePipelineShaderStage(const std::unordered_map<VkShaderStageFlagBits, std::vector<uint32_t>>& shaderData);

		void CreateDescriptorLayouts();
	public:
		DescriptorSetsAndPool CreateDescriptorSetsAndPool(uint32_t set, uint32_t numberOfSets = 1);
		DescriptorSetsAndPool CreateDescriptorSets(uint32_t set);
	private:
		std::string m_FilePath;
		std::string m_Name;

		// Datas from Reflect
		std::vector<VulkanShaderCompiler::ShaderDescriptorSetData> m_ShaderDescriptorSets;
		
		// Discripter Set Layouts
		std::vector<VkDescriptorSetLayout> m_DescriptorSetLayouts;

		// Descriptor pool size info
		std::unordered_map<uint32_t, std::vector<VkDescriptorPoolSize>> m_DescriptorCounts;

		std::unordered_map<VkShaderStageFlagBits, VkShaderModule>  m_ShaderModule;

		// For pipeine creation
		std::vector<VkPipelineShaderStageCreateInfo> m_PipelineShaderStageCreateInfos;
		std::vector<VkPushConstantRange> m_PushConstantRanges;
	};
}