#include "ethpch.h"
#include "VulkanUtils.h"
#include "VulkanShader.h"

#include "Ethane/Core/Timer.h"
#include "Ethane/Utils/FileUtils.hpp"


namespace Ethane {

	VulkanShader::VulkanShader(const VulkanDevice* device, const std::string& filepath)
		: m_Device(device), m_FilePath(filepath)
	{
		ETH_PROFILE_FUNCTION();
		ETH_CORE_INFO("Create Shader: {0}", filepath);

		auto lastSlash = filepath.find_last_of("/\\");
		lastSlash = lastSlash == std::string::npos ? 0 : lastSlash + 1;
		auto lastDot = filepath.rfind('.');
		auto count = lastDot == std::string::npos ? filepath.size() - lastSlash : lastDot - lastSlash;
		m_Name = filepath.substr(lastSlash, count);

		std::string source = FileUtils::ReadFile(filepath);

		Timer timer;

		VulkanShaderCompiler& compiler = VulkanShaderCompiler::GetInstance();
        std::unordered_map<VkShaderStageFlagBits, std::vector<uint32_t>> shaderBinary;
		VulkanShaderCompiler::CompileOutput outputs{ shaderBinary };
		VulkanShaderCompiler::CompileParam param{ m_FilePath, source };
        param.ForceCompile = true;
		compiler.Compile(param, outputs);
		VulkanShaderCompiler::ReflectOutput reflectOutputs{ m_ShaderDescriptorSetsReflect, m_PushConstantRanges };
		VulkanShaderCompiler::ReflectParam reflectParam{ outputs.ShaderBinary };
		compiler.Reflect(reflectParam, reflectOutputs);
		
		// profiling
		ETH_CORE_TRACE("[Time] compile: {0}ms", timer.ElapsedMillis());
		timer.Reset();
        
        CreatePipelineShaderStage(shaderBinary);
        CreateDescriptorLayouts();
	}

	VulkanShader::~VulkanShader()
	{
        Destroy();
	}

	void VulkanShader::Destroy()
	{
        if (m_DescriptorSetLayouts.empty() && m_ShaderModules.empty()) {
            return;
        }
        m_Device->Get().waitIdle();

		m_PipelineShaderStageCreateInfos.clear();
		m_DescriptorSetLayouts.clear();
		m_ShaderModules.clear();
	}

	void VulkanShader::CreatePipelineShaderStage(const std::unordered_map<VkShaderStageFlagBits, std::vector<uint32_t>>& shaderData)
	{
		const vk::raii::Device& device = m_Device->Get();
		m_PipelineShaderStageCreateInfos.clear();
		m_ShaderModules.clear();
		m_ShaderModules.reserve(shaderData.size());
		for (const auto& [stage, data] : shaderData)
		{
			ETH_CORE_ASSERT(data.size());
			const vk::raii::ShaderModule& module = m_ShaderModules.emplace_back(device, vk::ShaderModuleCreateInfo{
				.codeSize = data.size() * sizeof(uint32_t),
				.pCode    = data.data()});

			m_PipelineShaderStageCreateInfos.push_back(vk::PipelineShaderStageCreateInfo{
				.stage  = static_cast<vk::ShaderStageFlagBits>(stage),
				.module = *module,
				.pName  = "main"});
		}
	}

	void VulkanShader::CreateDescriptorLayouts()
	{
		const vk::raii::Device& device = m_Device->Get();
		m_DescriptorSetLayouts.clear();

		uint32_t setCount = (uint32_t)m_ShaderDescriptorSetsReflect.size();
		m_WriteDescriptorSetsBase.resize(setCount);
		for (uint32_t set = 0; set < setCount; set++)
		{
			auto& shaderDescriptorSet = m_ShaderDescriptorSetsReflect[set];

			// Uniform Buffers
			// NOTE: the write-descriptor templates stay C structs: plain data for VulkanShaderSystem, no Vulkan objects
			std::vector<vk::DescriptorSetLayoutBinding> layoutBindings;
			for (auto& [binding, uniformBuffer] : shaderDescriptorSet.UniformBuffers)
			{
				vk::DescriptorSetLayoutBinding& layoutBinding = layoutBindings.emplace_back(vk::DescriptorSetLayoutBinding{
					.binding         = binding,
					.descriptorType  = vk::DescriptorType::eUniformBuffer,
					.descriptorCount = 1,
					.stageFlags      = vk::ShaderStageFlags(uniformBuffer.ShaderStage)});

				VkWriteDescriptorSet& wds = m_WriteDescriptorSetsBase[set][uniformBuffer.Name].WriteDescriptor;
				wds = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
				wds.descriptorType = static_cast<VkDescriptorType>(layoutBinding.descriptorType);
				wds.dstBinding = layoutBinding.binding;
				wds.descriptorCount = 1;
			}

			// Image Sampler
			for (auto& [binding, imageSampler] : shaderDescriptorSet.ImageSamplers)
			{
				vk::DescriptorSetLayoutBinding& layoutBinding = layoutBindings.emplace_back(vk::DescriptorSetLayoutBinding{
					.binding         = binding,
					.descriptorType  = vk::DescriptorType::eCombinedImageSampler,
					.descriptorCount = imageSampler.ArraySize,
					.stageFlags      = vk::ShaderStageFlags(imageSampler.ShaderStage)});

//				ETH_CORE_ASSERT(shaderDescriptorSet.UniformBuffers.find(binding) == shaderDescriptorSet.UniformBuffers.end(), "Binding is already present!");

				VkWriteDescriptorSet& wds = m_WriteDescriptorSetsBase[set][imageSampler.Name].WriteDescriptor;
				wds = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
				wds.descriptorType = static_cast<VkDescriptorType>(layoutBinding.descriptorType);
				wds.dstBinding = layoutBinding.binding;
				wds.descriptorCount = imageSampler.ArraySize;
			}
            
            // Storage Buffers
            for (auto& [binding, storageBuffer] : shaderDescriptorSet.StorageBuffers)
            {
                vk::DescriptorSetLayoutBinding& layoutBinding = layoutBindings.emplace_back(vk::DescriptorSetLayoutBinding{
                    .binding         = binding,
                    .descriptorType  = vk::DescriptorType::eStorageBuffer,
                    .descriptorCount = 1,
                    .stageFlags      = vk::ShaderStageFlags(storageBuffer.ShaderStage)});

                VkWriteDescriptorSet& wds = m_WriteDescriptorSetsBase[set][storageBuffer.Name].WriteDescriptor;
                wds = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
                wds.descriptorType = static_cast<VkDescriptorType>(layoutBinding.descriptorType);
                wds.dstBinding = layoutBinding.binding;
                wds.descriptorCount = 1;
            }

            // Storage Image
            for (auto& [binding, imageSampler] : shaderDescriptorSet.StorageImage)
            {
                vk::DescriptorSetLayoutBinding& layoutBinding = layoutBindings.emplace_back(vk::DescriptorSetLayoutBinding{
                    .binding         = binding,
                    .descriptorType  = vk::DescriptorType::eStorageImage,
                    .descriptorCount = imageSampler.ArraySize,
                    .stageFlags      = vk::ShaderStageFlags(imageSampler.ShaderStage)});

                VkWriteDescriptorSet& wds = m_WriteDescriptorSetsBase[set][imageSampler.Name].WriteDescriptor;
                wds = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
                wds.descriptorType = static_cast<VkDescriptorType>(layoutBinding.descriptorType);
                wds.dstBinding = layoutBinding.binding;
                wds.descriptorCount = imageSampler.ArraySize;
            }


			ETH_CORE_INFO("Creating descriptor set {0} with {1} ubo's, {2} ssbo's, {3} samplers and {4} storage images", set,
				shaderDescriptorSet.UniformBuffers.size(), shaderDescriptorSet.StorageBuffers.size(), shaderDescriptorSet.ImageSamplers.size(), shaderDescriptorSet.StorageImage.size());

			// sets are visited in order 0..setCount-1, so the layout for `set` lands at index `set`
			m_DescriptorSetLayouts.emplace_back(device, vk::DescriptorSetLayoutCreateInfo{
				.bindingCount = static_cast<uint32_t>(layoutBindings.size()),
				.pBindings    = layoutBindings.data()});
		}
	}

	// For pipeline creation
	std::vector<vk::DescriptorSetLayout> VulkanShader::GetAllDescriptorSetLayouts() const
	{
		std::vector<vk::DescriptorSetLayout> result;
		result.reserve(m_DescriptorSetLayouts.size());
		for (const auto& layout : m_DescriptorSetLayouts)
			result.push_back(*layout);

		return result;
	}

	std::vector<vk::PushConstantRange> VulkanShader::GetPushConstantRanges() const
	{
		std::vector<vk::PushConstantRange> result;
		result.reserve(m_PushConstantRanges.size());
		for (const VkPushConstantRange& range : m_PushConstantRanges)
		{
			result.push_back(vk::PushConstantRange{
				.stageFlags = vk::ShaderStageFlags(range.stageFlags),
				.offset     = range.offset,
				.size       = range.size});
		}
		return result;
	}

	const VulkanShader::WriteDescriptorSetBase* VulkanShader::RetrieveWriteDescriptorSetBase(uint32_t set, const std::string& name) const
	{
		ETH_CORE_ASSERT(set < m_WriteDescriptorSetsBase.size());
		if (m_WriteDescriptorSetsBase[set].find(name) == m_WriteDescriptorSetsBase[set].end())
		{
			ETH_CORE_WARN("Shader {0} does not contain requested descriptor set {1}", m_Name, name);
			return nullptr;
		}
		return &m_WriteDescriptorSetsBase[set].at(name);
	}

	vk::raii::DescriptorPool VulkanShader::CreateDescriptorPool(uint32_t numberOfSets) const
	{
		std::vector<vk::DescriptorPoolSize> poolSizes;
		uint64_t setCount = m_ShaderDescriptorSetsReflect.size();
		for (uint32_t set = 0; set < setCount; set++)
		{
			auto& shaderDescriptorSet = m_ShaderDescriptorSetsReflect[set];
			if (!shaderDescriptorSet) // Empty descriptor set
				continue;

			if (shaderDescriptorSet.UniformBuffers.size())
			{
				vk::DescriptorPoolSize& typeCount = poolSizes.emplace_back();
				typeCount.type = vk::DescriptorType::eUniformBuffer;
				typeCount.descriptorCount = (uint32_t)shaderDescriptorSet.UniformBuffers.size() * numberOfSets;
			}

			if (shaderDescriptorSet.ImageSamplers.size())
			{
				vk::DescriptorPoolSize& typeCount = poolSizes.emplace_back();
				uint32_t descriptorSetCount = 0;
				for (auto&& [binding, imageSampler] : shaderDescriptorSet.ImageSamplers)
					descriptorSetCount += imageSampler.ArraySize;

				typeCount.type = vk::DescriptorType::eCombinedImageSampler;
				typeCount.descriptorCount = descriptorSetCount * numberOfSets;
			}
            
            if (shaderDescriptorSet.StorageBuffers.size())
            {
                vk::DescriptorPoolSize& typeCount = poolSizes.emplace_back();
                typeCount.type = vk::DescriptorType::eStorageBuffer;
                typeCount.descriptorCount = (uint32_t)shaderDescriptorSet.StorageBuffers.size() * numberOfSets;
            }

            if (shaderDescriptorSet.StorageImage.size())
            {
                vk::DescriptorPoolSize& typeCount = poolSizes.emplace_back();
                uint32_t descriptorSetCount = 0;
                for (auto&& [binding, imageSampler] : shaderDescriptorSet.StorageImage)
                    descriptorSetCount += imageSampler.ArraySize;

                typeCount.type = vk::DescriptorType::eStorageImage;
                typeCount.descriptorCount = descriptorSetCount * numberOfSets;
            }
		}

		// eFreeDescriptorSet: required for vk::raii::DescriptorSet, which frees itself on destruction
		return vk::raii::DescriptorPool(m_Device->Get(), vk::DescriptorPoolCreateInfo{
			.flags         = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
			.maxSets       = static_cast<uint32_t>(numberOfSets * setCount),
			.poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
			.pPoolSizes    = poolSizes.data()});
	}

	vk::raii::DescriptorSet VulkanShader::CreateDescriptorSet(uint32_t set, const vk::raii::DescriptorPool& pool) const
	{
		ETH_CORE_ASSERT(set < m_DescriptorSetLayouts.size());
		const vk::DescriptorSetLayout layout = *m_DescriptorSetLayouts[set];

		vk::raii::DescriptorSets sets(m_Device->Get(), vk::DescriptorSetAllocateInfo{
			.descriptorPool     = *pool,
			.descriptorSetCount = 1,
			.pSetLayouts        = &layout});
		return std::move(sets.front());
	}
}