#pragma once

#include "Ethane/Renderer/Shader.h"

#include "Vulkan.h"

namespace Ethane {

	class VulkanShader : public Shader
	{
	public:
		struct UniformBuffer
		{
			std::string Name;
			uint32_t Size = 0;
			VkShaderStageFlagBits ShaderStage = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
			VkDescriptorBufferInfo Descriptor;
			// uint32_t BindingPoint = 0;
		};

		struct ImageSampler
		{
			std::string Name;
			uint32_t DescriptorSet = 0;
			uint32_t ArraySize = 0;
			VkShaderStageFlagBits ShaderStage = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
			// uint32_t BindingPoint = 0;
		};

		struct ShaderDescriptorSetData
		{
			std::unordered_map<uint32_t, UniformBuffer*> UniformBuffers;
			std::unordered_map<uint32_t, ImageSampler> ImageSamplers;

			std::unordered_map<std::string, VkWriteDescriptorSet> WriteDescriptorSets;

			operator bool() const { return !(UniformBuffers.empty() && ImageSamplers.empty()); }
		};


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

		void Cleanup();

		// temp
		virtual void Bind() const override {};
		virtual void Unbind() const override {};

		// Getter
		virtual const std::string& GetName() const override { return  m_Name; }
		const std::vector<VkPipelineShaderStageCreateInfo>& GetPipelineShaderStageCreateInfos() const { return m_PipelineShaderStageCreateInfos; }
		const std::vector<VkPushConstantRange> GetPushConstantRanges() const { return m_PushConstantRanges; } // TODO
		std::vector<VkDescriptorSetLayout> GetAllDescriptorSetLayouts();
		const std::vector<ShaderDescriptorSetData>& GetShaderDescriptorSetData() const { return m_ShaderDescriptorSets; }
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

		void CompileOrGetVulkanBinaries(const std::unordered_map<VkShaderStageFlagBits, std::string>& shaderSources, 
											  std::unordered_map<VkShaderStageFlagBits, std::vector<uint32_t>>& outputBinary, bool forceCompile = false);
		void CreatePipelineShaderStage(const std::unordered_map<VkShaderStageFlagBits, std::vector<uint32_t>>& shaderData);
		void Reflect(const std::unordered_map<VkShaderStageFlagBits, std::vector<uint32_t>>& shaderData);
		void ReflectStage(VkShaderStageFlagBits stage, const std::vector<uint32_t>& shaderData);

		void CreateDescriptorLayouts();
	public:
		DescriptorSetsAndPool CreateDescriptorSetsAndPool(uint32_t set, uint32_t numberOfSets = 1);
		DescriptorSetsAndPool CreateDescriptorSets(uint32_t set);
	private:
		std::string m_FilePath;
		std::string m_Name;

		// Datas from Reflect
		std::vector<ShaderDescriptorSetData> m_ShaderDescriptorSets;
		
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