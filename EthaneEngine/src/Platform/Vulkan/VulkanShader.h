#pragma once

#include "Ethane/Renderer/Shader.h"

#include "Vulkan.h"

namespace Ethane {

	class VulkanShader : public Shader 
	{
	public:
		VulkanShader(const std::string& filepath);
		virtual ~VulkanShader();

		// temp
		virtual void Bind() const override {};
		virtual void Unbind() const override {};

		// Getter
		virtual const std::string& GetName() const override { return  m_Name; }
		const std::vector<VkPipelineShaderStageCreateInfo>& GetPipelineShaderStageCreateInfos() const { return m_PipelineShaderStageCreateInfos; }

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
		void Reflect(VkShaderStageFlagBits stage, const std::vector<uint32_t>& shaderData);

		void CreateDescriptors();
	private:
		std::string m_FilePath;
		std::string m_Name;

		// std::vector<ShaderDescriptorSet> m_ShaderDescriptorSets;

		std::unordered_map<VkShaderStageFlagBits, VkShaderModule>  m_ShaderModule;
		std::vector<VkPipelineShaderStageCreateInfo> m_PipelineShaderStageCreateInfos;
	};
}