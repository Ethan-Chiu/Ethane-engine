#include "ethpch.h"

#include "VulkanShader.h"
#include "VulkanContext.h"

#include "VulkanRendererAPI.h"

#include <shaderc/shaderc.hpp>
#include <spirv_cross/spirv_glsl.hpp>

namespace Ethane {

	namespace Utils {

		static VkShaderStageFlagBits ShaderTypeFromString(const std::string& type)
		{
			if (type == "vertex")                       return VK_SHADER_STAGE_VERTEX_BIT;
			if (type == "fragment" || type == "pixel")  return VK_SHADER_STAGE_FRAGMENT_BIT;
			if (type == "compute")                      return VK_SHADER_STAGE_COMPUTE_BIT;

			return VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
		}

		static shaderc_shader_kind VkShaderStageToShaderC(VkShaderStageFlagBits stage)
		{
			switch (stage)
			{
			case VK_SHADER_STAGE_VERTEX_BIT:    return shaderc_vertex_shader;
			case VK_SHADER_STAGE_FRAGMENT_BIT:  return shaderc_fragment_shader;
			case VK_SHADER_STAGE_COMPUTE_BIT:   return shaderc_compute_shader;
			}
			ETH_CORE_ASSERT(false);
			return (shaderc_shader_kind)0;
		}

		static const char* GLShaderStageToString(VkShaderStageFlagBits stage)
		{
			switch (stage)
			{
			case VK_SHADER_STAGE_VERTEX_BIT:   return "VERTEX_SHADER";
			case VK_SHADER_STAGE_FRAGMENT_BIT: return "FRAGMENT_SHADER";
			case VK_SHADER_STAGE_COMPUTE_BIT:  return "COMPUTE_SHADER";
			}
			ETH_CORE_ASSERT(false);
			return nullptr;
		}

		static const char* GetCacheDirectory()
		{
			// TODO: make sure the assets directory is valid
			return "assets/cache/shader/vulkan";
		}

		static void CreateCacheDirectoryIfNeeded()
		{
			std::string cacheDirectory = GetCacheDirectory();
			if (!std::filesystem::exists(cacheDirectory))
				std::filesystem::create_directories(cacheDirectory);
		}

		static const char* VkShaderStageCachedFileExtension(VkShaderStageFlagBits stage)
		{
			switch (stage)
			{
			case VK_SHADER_STAGE_VERTEX_BIT:    return ".cached_vulkan.vert";
			case VK_SHADER_STAGE_FRAGMENT_BIT:  return ".cached_vulkan.frag";
			case VK_SHADER_STAGE_COMPUTE_BIT:   return ".cached_vulkan.comp";
			}
			ETH_CORE_ASSERT(false);
			return "";
		}

	}

	VulkanShader::VulkanShader(const std::string& filepath)
		:m_FilePath(filepath)
	{
		ETH_PROFILE_FUNCTION();
		ETH_CORE_INFO("Create Shader: {0}", filepath);

		Utils::CreateCacheDirectoryIfNeeded();

		auto lastSlash = filepath.find_last_of("/\\");
		lastSlash = lastSlash == std::string::npos ? 0 : lastSlash + 1;
		auto lastDot = filepath.rfind('.');
		auto count = lastDot == std::string::npos ? filepath.size() - lastSlash : lastDot - lastSlash;
		m_Name = filepath.substr(lastSlash, count);

		std::string source = ReadFile(filepath);
		auto shaderSources = PreProcess(source);

		std::unordered_map<VkShaderStageFlagBits, std::vector<uint32_t>> shaderData;
		CompileOrGetVulkanBinaries(shaderSources, shaderData, true);
		CreatePipelineShaderStage(shaderData);
		Reflect(shaderData);
		CreateDescriptorLayouts();
	}

	VulkanShader::~VulkanShader()
	{
	}

	void VulkanShader::Cleanup()
	{
		VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();
		for (auto descriptorSetLayout : m_DescriptorSetLayouts)
		{
			vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
		}

		for (auto& [flag, shadermodule] : m_ShaderModule)
		{
			ETH_CORE_INFO("123 {0}", Utils::GLShaderStageToString(flag));
			vkDestroyShaderModule(device, shadermodule, nullptr);
		}
	}

	std::unordered_map<VkShaderStageFlagBits, std::string> VulkanShader::PreProcess(const std::string& source)
	{
		ETH_PROFILE_FUNCTION();

		std::unordered_map<VkShaderStageFlagBits, std::string> shaderSources;

		const char* typeToken = "#type";
		size_t typeTokenLength = strlen(typeToken);
		size_t pos = source.find(typeToken, 0);
		while (pos != std::string::npos)
		{
			size_t eol = source.find_first_of("\r\n", pos);
			ETH_CORE_ASSERT(eol != std::string::npos, "Syntex error");
			size_t begin = pos + typeTokenLength + 1;
			std::string type = source.substr(begin, eol - begin);
			ETH_CORE_ASSERT(Utils::ShaderTypeFromString(type), "Invalid shader type specification!");

			size_t nextLinePos = source.find_first_not_of("\r\n", eol);
			ETH_CORE_ASSERT(nextLinePos != std::string::npos, "Syntax error");
			pos = source.find(typeToken, nextLinePos);
			shaderSources[Utils::ShaderTypeFromString(type)] = (pos == std::string::npos) ? source.substr(nextLinePos) : source.substr(nextLinePos, pos - nextLinePos);
		}

		return shaderSources;
	}

	void VulkanShader::CompileOrGetVulkanBinaries(const std::unordered_map<VkShaderStageFlagBits, std::string>& shaderSources, std::unordered_map<VkShaderStageFlagBits, std::vector<uint32_t>>& outputBinary, bool forceCompile)
	{
		ETH_PROFILE_FUNCTION();

		shaderc::Compiler compiler;
		shaderc::CompileOptions options;
		options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
		options.SetWarningsAsErrors();
		options.SetGenerateDebugInfo();
		const bool optimize = false;

		if (optimize)
			options.SetOptimizationLevel(shaderc_optimization_level_performance);

		std::filesystem::path cacheDirectory = Utils::GetCacheDirectory();

		for (auto&& [stage, source] : shaderSources)
		{

			std::filesystem::path shaderFilePath = m_FilePath;
			std::filesystem::path cachedPath = cacheDirectory / (shaderFilePath.filename().string() + Utils::VkShaderStageCachedFileExtension(stage));

			std::ifstream in(cachedPath, std::ios::in | std::ios::binary);
			if (in.is_open() && !forceCompile) 			{
				in.seekg(0, std::ios::end);
				auto size = in.tellg();
				in.seekg(0, std::ios::beg);
				auto& data = outputBinary[stage];
				data.resize(size / sizeof(uint32_t));
				in.read((char*)data.data(), size);
				in.close();
			}
			else
			{
				shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(source, Utils::VkShaderStageToShaderC(stage), m_FilePath.c_str(), options);
				if (module.GetCompilationStatus() != shaderc_compilation_status_success)
				{
					ETH_CORE_ERROR(module.GetErrorMessage());
					ETH_CORE_ASSERT(false);
				}

				outputBinary[stage] = std::vector<uint32_t>(module.cbegin(), module.cend());

				std::ofstream out(cachedPath, std::ios::out | std::ios::binary);
				if (out.is_open())
				{
					auto& data = outputBinary[stage];
					out.write((char*)data.data(), data.size() * sizeof(uint32_t));
					out.flush();
					out.close();
				}
			}
		}
	}

	void VulkanShader::CreatePipelineShaderStage(const std::unordered_map<VkShaderStageFlagBits, std::vector<uint32_t>>& shaderData)
	{
		VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();
		m_PipelineShaderStageCreateInfos.clear();
		for (auto [stage, data] : shaderData)
		{
			ETH_CORE_ASSERT(data.size());
			VkShaderModuleCreateInfo moduleCreateInfo{};

			moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			moduleCreateInfo.codeSize = data.size() * sizeof(uint32_t);
			moduleCreateInfo.pCode = data.data();

			VK_CHECK_RESULT(vkCreateShaderModule(device, &moduleCreateInfo, nullptr, &m_ShaderModule[stage]));

			VkPipelineShaderStageCreateInfo& shaderStage = m_PipelineShaderStageCreateInfos.emplace_back();
			shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStage.stage = stage;
			shaderStage.module = m_ShaderModule[stage];
			shaderStage.pName = "main";
		}
	}

	void VulkanShader::Reflect(const std::unordered_map<VkShaderStageFlagBits, std::vector<uint32_t>>& shaderData)
	{
		for (auto [stage, data] : shaderData)
		{
			ReflectStage(stage, data);
		}
	}

	static std::unordered_map<uint32_t, std::unordered_map<uint32_t, VulkanShader::UniformBuffer*>> s_UniformBuffers; // set -> binding point -> buffer

	void VulkanShader::ReflectStage(VkShaderStageFlagBits stage, const std::vector<uint32_t>& shaderData)
	{
		VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();

		spirv_cross::Compiler compiler(shaderData);
		spirv_cross::ShaderResources res = compiler.get_shader_resources();

		ETH_CORE_TRACE("VulkanShader::Reflect - {0} {1}", Utils::GLShaderStageToString(stage), m_FilePath);

		ETH_CORE_TRACE("    {0} uniform buffers", res.uniform_buffers.size());
		for (const auto& resource : res.uniform_buffers)
		{
			const auto& name = resource.name;
			auto& bufferType = compiler.get_type(resource.base_type_id);
			uint32_t bufferSize = (uint32_t)compiler.get_declared_struct_size(bufferType);
			uint32_t memberCount = (uint32_t)bufferType.member_types.size();
			uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
			uint32_t descriptorSet = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);

			if (descriptorSet >= m_ShaderDescriptorSets.size())
				m_ShaderDescriptorSets.resize(descriptorSet + 1);
			
			ShaderDescriptorSetData& shaderDescriptorSet = m_ShaderDescriptorSets[descriptorSet];
			if (s_UniformBuffers[descriptorSet].find(binding) == s_UniformBuffers[descriptorSet].end())
			{
				UniformBuffer* uniformBuffer = new UniformBuffer();
				// uniformBuffer->BindingPoint = binding;
				uniformBuffer->Size = bufferSize;
				uniformBuffer->Name = name;
				uniformBuffer->ShaderStage = VK_SHADER_STAGE_ALL;
				s_UniformBuffers[descriptorSet][binding] = uniformBuffer;
			}
			else
			{
				UniformBuffer* uniformBuffer = s_UniformBuffers[descriptorSet][binding];
				if (bufferSize > uniformBuffer->Size)
					uniformBuffer->Size = bufferSize;
			
			}

			shaderDescriptorSet.UniformBuffers[binding] = s_UniformBuffers[descriptorSet][binding];

			ETH_CORE_TRACE("  {0} ({1}, {2})", name, descriptorSet, binding);
			ETH_CORE_TRACE("  Member Count: {0}", memberCount);
			ETH_CORE_TRACE("  Size: {0}", bufferSize);
			ETH_CORE_TRACE("-------------------");
		}



		ETH_CORE_INFO("Sampled Images:");
		for (const auto& resource : res.sampled_images)
		{
			const auto& name = resource.name;
			auto& baseType = compiler.get_type(resource.base_type_id);
			auto& type = compiler.get_type(resource.type_id);
			uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
			uint32_t descriptorSet = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
			uint32_t dimension = baseType.image.dim;
			uint32_t arraySize = type.array[0];
			if (arraySize == 0)
				arraySize = 1;
			if (descriptorSet >= m_ShaderDescriptorSets.size())
				m_ShaderDescriptorSets.resize(descriptorSet + 1);

			ShaderDescriptorSetData& shaderDescriptorSet = m_ShaderDescriptorSets[descriptorSet];
			auto& imageSampler = shaderDescriptorSet.ImageSamplers[binding];
			imageSampler.DescriptorSet = descriptorSet;
			imageSampler.Name = name;
			imageSampler.ShaderStage = stage;
			imageSampler.ArraySize = arraySize;
			// imageSampler.BindingPoint = binding;

			// m_Resources[name] = ShaderResourceDeclaration(name, binding, 1);

			ETH_CORE_TRACE("  {0} ({1}, {2})", name, descriptorSet, binding);
		}

#ifdef reflect

		ETH_CORE_INFO("Push Constant Buffers:");
		for (const auto& resource : res.push_constant_buffers)
		{
			const auto& bufferName = resource.name;
			auto& bufferType = compiler.get_type(resource.base_type_id);
			uint32_t bufferSize = (uint32_t)compiler.get_declared_struct_size(bufferType);
			uint32_t memberCount = uint32_t(bufferType.member_types.size());
			uint32_t bufferOffset = 0;
			if (m_PushConstantRanges.size())
				bufferOffset = m_PushConstantRanges.back().Offset + m_PushConstantRanges.back().Size;

			auto& pushConstantRange = m_PushConstantRanges.emplace_back();
			pushConstantRange.ShaderStage = shaderStage;
			pushConstantRange.Size = bufferSize - bufferOffset;
			pushConstantRange.Offset = bufferOffset;

			// Skip empty push constant buffers - these are for the renderer only
			if (bufferName.empty() || bufferName == "u_Renderer")
				continue;

			ShaderBuffer& buffer = m_Buffers[bufferName];
			buffer.Name = bufferName;
			buffer.Size = bufferSize - bufferOffset;

			ETH_CORE_TRACE("  Name: {0}", bufferName);
			ETH_CORE_TRACE("  Member Count: {0}", memberCount);
			ETH_CORE_TRACE("  Size: {0}", bufferSize);

			for (uint32_t i = 0; i < memberCount; i++)
			{
				const auto& memberName = compiler.get_member_name(bufferType.self, i);
				auto type = compiler.get_type(bufferType.member_types[i]);
				auto size = (uint32_t)compiler.get_declared_struct_member_size(bufferType, i);
				auto offset = compiler.type_struct_member_offset(bufferType, i) - bufferOffset;

				std::string uniformName = fmt::format("{}.{}", bufferName, memberName);
				buffer.Uniforms[uniformName] = ShaderUniform(uniformName, Utils::SPIRTypeToShaderUniformType(type), size, offset);
			}
		}

		ETH_CORE_INFO("Storage Buffers:");
		for (const auto& resource : resources.storage_buffers)
		{
			const auto& name = resource.name;
			auto& bufferType = compiler.get_type(resource.base_type_id);
			uint32_t memberCount = (uint32_t)bufferType.member_types.size();
			uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
			uint32_t descriptorSet = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
			uint32_t size = (uint32_t)compiler.get_declared_struct_size(bufferType);

			if (descriptorSet >= m_ShaderDescriptorSets.size())
				m_ShaderDescriptorSets.resize(descriptorSet + 1);

			ShaderDescriptorSet& shaderDescriptorSet = m_ShaderDescriptorSets[descriptorSet];
			if (s_StorageBuffers[descriptorSet].find(binding) == s_StorageBuffers[descriptorSet].end())
			{
				StorageBuffer* storageBuffer = new StorageBuffer();
				storageBuffer->BindingPoint = binding;
				storageBuffer->Size = size;
				storageBuffer->Name = name;
				storageBuffer->ShaderStage = VK_SHADER_STAGE_ALL;
				s_StorageBuffers.at(descriptorSet)[binding] = storageBuffer;
			}
			else
			{
				StorageBuffer* storageBuffer = s_StorageBuffers.at(descriptorSet).at(binding);
				if (size > storageBuffer->Size)
					storageBuffer->Size = size;
			}

			shaderDescriptorSet.StorageBuffers[binding] = s_StorageBuffers.at(descriptorSet).at(binding);

			ETH_CORE_TRACE("  {0} ({1}, {2})", name, descriptorSet, binding);
			ETH_CORE_TRACE("  Member Count: {0}", memberCount);
			ETH_CORE_TRACE("  Size: {0}", size);
			ETH_CORE_TRACE("-------------------");
		}

		ETH_CORE_INFO("Storage Images:");
		for (const auto& resource : resources.storage_images)
		{
			const auto& name = resource.name;
			auto& type = compiler.get_type(resource.base_type_id);
			uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
			uint32_t descriptorSet = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
			uint32_t dimension = type.image.dim;

			if (descriptorSet >= m_ShaderDescriptorSets.size())
				m_ShaderDescriptorSets.resize(descriptorSet + 1);

			ShaderDescriptorSet& shaderDescriptorSet = m_ShaderDescriptorSets[descriptorSet];
			auto& imageSampler = shaderDescriptorSet.StorageImages[binding];
			imageSampler.BindingPoint = binding;
			imageSampler.DescriptorSet = descriptorSet;
			imageSampler.Name = name;
			imageSampler.ShaderStage = shaderStage;

			m_Resources[name] = ShaderResourceDeclaration(name, binding, 1);

			ETH_CORE_TRACE("  {0} ({1}, {2})", name, descriptorSet, binding);
		}

		ETH_CORE_INFO("===========================");

#endif

	}

	void VulkanShader::CreateDescriptorLayouts()
	{
		VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();
		// m_TypeCounts.clear();
		for (uint32_t set = 0; set < m_ShaderDescriptorSets.size(); set++)
		{
			auto& shaderDescriptorSet = m_ShaderDescriptorSets[set];

			// Uniform Buffers
			std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
			for (auto& [binding, uniformBuffer] : shaderDescriptorSet.UniformBuffers)
			{
				VkDescriptorSetLayoutBinding& layoutBinding = layoutBindings.emplace_back();
				layoutBinding.binding = binding;
				layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				layoutBinding.descriptorCount = 1;
				layoutBinding.stageFlags = uniformBuffer->ShaderStage;
				layoutBinding.pImmutableSamplers = nullptr;

				VkWriteDescriptorSet& set = shaderDescriptorSet.WriteDescriptorSets[uniformBuffer->Name];
				set = {};
				set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				set.descriptorType = layoutBinding.descriptorType;
				set.dstBinding = layoutBinding.binding;
				set.descriptorCount = 1;
			}

			// Image Sampler
			for (auto& [binding, imageSampler] : shaderDescriptorSet.ImageSamplers)
			{
				auto& layoutBinding = layoutBindings.emplace_back();
				layoutBinding.binding = binding;
				layoutBinding.descriptorCount = imageSampler.ArraySize;
				layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				layoutBinding.pImmutableSamplers = nullptr;
				layoutBinding.stageFlags = imageSampler.ShaderStage;

				ETH_CORE_ASSERT(shaderDescriptorSet.UniformBuffers.find(binding) == shaderDescriptorSet.UniformBuffers.end(), "Binding is already present!");

				VkWriteDescriptorSet& set = shaderDescriptorSet.WriteDescriptorSets[imageSampler.Name];
				set = {};
				set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				set.descriptorType = layoutBinding.descriptorType;
				set.dstBinding = layoutBinding.binding;
				set.descriptorCount = imageSampler.ArraySize;
			}

			VkDescriptorSetLayoutCreateInfo descriptorLayout = {};
			descriptorLayout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			descriptorLayout.pNext = nullptr;
			descriptorLayout.bindingCount = static_cast<uint32_t>(layoutBindings.size());
			descriptorLayout.pBindings = layoutBindings.data();

			ETH_CORE_INFO("Creating descriptor set {0} with {1} ubo's, {2} ssbo's, {3} samplers and {4} storage images", set,
				shaderDescriptorSet.UniformBuffers.size(), 0, shaderDescriptorSet.ImageSamplers.size(), 0);
				// shaderDescriptorSet.StorageBuffers.size(),
				// shaderDescriptorSet.StorageImages.size());
			if (set >= m_DescriptorSetLayouts.size())
				m_DescriptorSetLayouts.resize((size_t)(set + 1));
			VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &m_DescriptorSetLayouts[set]));
		}
	}

	std::vector<VkDescriptorSetLayout> VulkanShader::GetAllDescriptorSetLayouts()
	{
		std::vector<VkDescriptorSetLayout> result;
		result.reserve(m_DescriptorSetLayouts.size());
		for (auto& layout : m_DescriptorSetLayouts)
			result.emplace_back(layout);

		return result;
	}

	const VkWriteDescriptorSet* VulkanShader::GetWriteDescriptorSet(uint32_t set, const std::string& name) const
	{
		ETH_CORE_ASSERT(set < m_ShaderDescriptorSets.size());
		ETH_CORE_ASSERT(m_ShaderDescriptorSets[set]);
		if (m_ShaderDescriptorSets.at(set).WriteDescriptorSets.find(name) == m_ShaderDescriptorSets.at(set).WriteDescriptorSets.end())
		{
			ETH_CORE_WARN("Shader {0} does not contain requested descriptor set {1}", m_Name, name);
			return nullptr;
		}
		return &m_ShaderDescriptorSets.at(set).WriteDescriptorSets.at(name);
	}

	VulkanShader::DescriptorSetsAndPool VulkanShader::CreateDescriptorSetsAndPool(uint32_t set, uint32_t numberOfSets)
	{
		DescriptorSetsAndPool result;

		VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();

		std::unordered_map<uint32_t, std::vector<VkDescriptorPoolSize>> poolSizes;
		for (uint32_t set = 0; set < m_ShaderDescriptorSets.size(); set++)
		{
			auto& shaderDescriptorSet = m_ShaderDescriptorSets[set];
			if (!shaderDescriptorSet) // Empty descriptor set
				continue;

			if (shaderDescriptorSet.UniformBuffers.size())
			{
				VkDescriptorPoolSize& typeCount = poolSizes[set].emplace_back();
				typeCount.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				typeCount.descriptorCount = (uint32_t)shaderDescriptorSet.UniformBuffers.size() * numberOfSets;
			}
			// if (shaderDescriptorSet.StorageBuffers.size())
			// {
			// 	VkDescriptorPoolSize& typeCount = poolSizes[set].emplace_back();
			// 	typeCount.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			// 	typeCount.descriptorCount = (uint32_t)shaderDescriptorSet.StorageBuffers.size() * numberOfSets;
			// }
			if (shaderDescriptorSet.ImageSamplers.size())
			{
				VkDescriptorPoolSize& typeCount = poolSizes[set].emplace_back();
				uint32_t descriptorSetCount = 0;
				for (auto&& [binding, imageSampler] : shaderDescriptorSet.ImageSamplers)
					descriptorSetCount += imageSampler.ArraySize;
			
				typeCount.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				typeCount.descriptorCount = descriptorSetCount * numberOfSets;
			}
			// if (shaderDescriptorSet.StorageImages.size())
			// {
			// 	VkDescriptorPoolSize& typeCount = poolSizes[set].emplace_back();
			// 	typeCount.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			// 	typeCount.descriptorCount = (uint32_t)shaderDescriptorSet.StorageImages.size() * numberOfSets;
			// }

		}

		ETH_CORE_ASSERT(poolSizes.find(set) != poolSizes.end());

		VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
		descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descriptorPoolInfo.pNext = nullptr;
		descriptorPoolInfo.poolSizeCount = (uint32_t)poolSizes.at(set).size();
		descriptorPoolInfo.pPoolSizes = poolSizes.at(set).data();
		descriptorPoolInfo.maxSets = numberOfSets;

		ETH_CORE_INFO("create pool size: {0}", (uint32_t)poolSizes.at(set).size());

		VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &result.Pool));

		std::vector<VkDescriptorSetLayout> layouts(numberOfSets, m_DescriptorSetLayouts[set]);
		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = result.Pool;
		allocInfo.descriptorSetCount = numberOfSets;
		allocInfo.pSetLayouts = layouts.data();

		result.DescriptorSets.resize(numberOfSets);
		VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, result.DescriptorSets.data()));
		return result;
	}

	VulkanShader::DescriptorSetsAndPool VulkanShader::CreateDescriptorSets(uint32_t set)
	{
		ETH_CORE_ASSERT(set < m_DescriptorSetLayouts.size());
		DescriptorSetsAndPool result;

		if (m_ShaderDescriptorSets.empty())
		{
			ETH_CORE_INFO("Empty descriptor set {0}", set);
			return result;
		}

		result.Pool = nullptr;

		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &m_DescriptorSetLayouts[set];
		VkDescriptorSet descriptorSet = VulkanRendererAPI::AllocateDescriptorSet(allocInfo);
		ETH_CORE_ASSERT(descriptorSet);
		result.DescriptorSets.push_back(descriptorSet);
		return result;
	}
}