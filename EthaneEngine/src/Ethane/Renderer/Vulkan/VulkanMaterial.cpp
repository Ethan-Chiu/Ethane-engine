#include "ethpch.h"
#include "VulkanMaterial.h"

#include "Ethane/Renderer/Renderer.h"

#include "VulkanContext.h"
#include "VulkanTexture.h"
#include "VulkanImage.h"
#include "VulkanPipeline.h"
#include "VulkanUniformBuffer.h"

// #include "Ethane/Core/Timer.h"
#include "ShaderUtils/VulkanShaderSystem.h"

namespace Ethane {

	VulkanMaterial::VulkanMaterial(const Ref<Shader>& shader, const std::string& name)
		: m_Shader(shader), m_Name(name),
		m_WriteDescriptors(Renderer::GetConfig().FramesInFlight)
	{
		Init();
	}

	VulkanMaterial::VulkanMaterial(Ref<Material> material, const std::string& name)
		: m_Shader(material->GetShader()), m_Name(name),
		m_WriteDescriptors(Renderer::GetConfig().FramesInFlight)
	{
		if (name.empty())
			m_Name = material->GetName();

		auto vulkanMaterial = std::dynamic_pointer_cast<VulkanMaterial>(material);
		
		m_Textures = vulkanMaterial->m_Textures;
		m_TextureArrays = vulkanMaterial->m_TextureArrays;
		m_Images = vulkanMaterial->m_Images;
	}

	VulkanMaterial::~VulkanMaterial()
	{
	}

	void VulkanMaterial::Init()
	{
		m_MaterialFlags |= (uint32_t)MaterialFlag::DepthTest;
		m_MaterialFlags |= (uint32_t)MaterialFlag::Blend;


		Invalidate();
	}

	void VulkanMaterial::Invalidate()
	{
		ETH_CORE_TRACE("VulkanMaterial::Invalidate ({0})", m_Name);

		uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;
		auto shader = std::dynamic_pointer_cast<VulkanShader>(m_Shader);

		m_ResourceBindings.clear();
		const auto& WDSetsBase = shader->RetrieveWriteDescriptorSetsBase();
		const uint32_t maxSet = WDSetsBase.size();
		for (uint32_t set = 0; set < maxSet; ++set) {
			const auto& WdsBase = WDSetsBase[set];
			for (auto&& [name, wdsMeta] : WdsBase)
			{
				ResourceBinding resource;
				resource.Name = name;
				resource.Set = set;

				VkWriteDescriptorSet wds = wdsMeta.WriteDescriptor;
				if(wds.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
				{
					for (uint32_t frame = 0; frame < framesInFlight; ++frame) {
						auto uniformBuffer = VulkanShaderSystem::GetUniformBuffer(frame, set, wds.dstBinding);
						wds.dstArrayElement = 0;
						wds.pBufferInfo = &uniformBuffer->GetDescriptorBufferInfo();
						resource.WriteDescriptors.push_back(wds);
					}

				} else if(wds.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
				{
					for (uint32_t frame = 0; frame < framesInFlight; ++frame) {
						auto tex = VulkanShaderSystem::GetDefaultTexture();
						wds.dstArrayElement = 0;
						wds.pImageInfo = &(std::dynamic_pointer_cast<VulkanTexture2D>(tex)->GetDescriptorImageInfo());
						resource.WriteDescriptors.push_back(wds);
					}
				}
				
				m_ResourceBindings.push_back(resource);
			}
		}

		m_Pool = shader->CreateDescriptorPool(framesInFlight);
		m_DescriptorSets.resize(framesInFlight);
		for (uint32_t frame = 0; frame < framesInFlight; ++frame) {
			for (uint32_t set = 0; set < maxSet; ++set) {
				m_DescriptorSets[frame].push_back(shader->CreateDescriptorSet(set, m_Pool));
			}
		}
	}

	void VulkanMaterial::SetVulkanDescriptor(const std::string& name, const Ref<Texture2D>& texture)
	{
		// find resource // TODO: improve this
		uint32_t id = 0;
		for (uint32_t i = 0; i < m_ResourceBindings.size(); ++i) {
			if (m_ResourceBindings[i].Name == name) {
				id = i;
				break;
			}
		}
		ResourceBinding& resource = m_ResourceBindings[id];

		uint32_t binding = resource.WriteDescriptors[0].dstBinding;

		// Texture is already set
		// if (binding < m_Textures.size() && m_Textures[binding] ) // already in list TODO: check if they are the same
		// 	return;
		if (binding >= m_Textures.size())
			m_Textures.resize(binding + 1);

		m_Textures[binding] = texture;
	
		for (uint32_t f = 0; f < resource.WriteDescriptors.size(); ++f) {
			auto tex = VulkanShaderSystem::GetDefaultTexture();
			resource.WriteDescriptors[f].pImageInfo = &(std::dynamic_pointer_cast<VulkanTexture2D>(m_Textures[binding])->GetDescriptorImageInfo());
		}
	}

	void VulkanMaterial::SetVulkanDescriptor(const std::string& name, const Ref<Texture2D>& texture, uint32_t arrayIndex)
	{
		// TODO: broken code

		uint32_t binding = 1; // TODO
		// Texture is already set
		if (binding < m_TextureArrays.size() && m_TextureArrays[binding].size() < arrayIndex) // already in list TODO: check if they are the same
			return;
		if (binding >= m_TextureArrays.size())
			m_TextureArrays.resize(binding + 1);
		if (arrayIndex >= m_TextureArrays[binding].size())
			m_TextureArrays[binding].resize(arrayIndex + 1);
	
		m_TextureArrays[binding][arrayIndex] = texture;

		if (m_DescriptorArrays.find(binding) == m_DescriptorArrays.end())
		{
			const VkWriteDescriptorSet* wds = &(std::dynamic_pointer_cast<VulkanShader>(m_Shader)->RetrieveWriteDescriptorSetBase(0, name)->WriteDescriptor);
			ETH_CORE_ASSERT(wds);
			m_DescriptorArrays[binding] = *wds;
		}
	}

	void VulkanMaterial::SetVulkanDescriptor(const std::string& name, const Ref<Image2D>& image)
	{
		ETH_CORE_ASSERT(std::dynamic_pointer_cast<VulkanImage2D>(image)->GetImageInfo().ImageView, "ImageView is null");

		// find resource // TODO: improve this
		uint32_t id = 0;
		for (uint32_t i = 0; i < m_ResourceBindings.size(); ++i) {
			if (m_ResourceBindings[i].Name == name) {
				id = i;
				break;
			}
		}
		ResourceBinding& resource = m_ResourceBindings[id];

		uint32_t binding = resource.WriteDescriptors[0].dstBinding;
		// TODO: replace with set/map
		// if (binding < m_Images.size() && m_Images[binding]) // already in list TODO: check if they are the same
		// 	return;
		if (binding >= m_Images.size())
			m_Images.resize(binding+1);

		m_Images[binding] = image;

		for (uint32_t f = 0; f < resource.WriteDescriptors.size(); ++f) {
			auto tex = VulkanShaderSystem::GetDefaultTexture();
			resource.WriteDescriptors[f].pImageInfo = &(tex->GetDescriptorImageInfo());
		}
	}

	void VulkanMaterial::Set(const std::string& name, float value)
	{
		Set<float>(name, value);
	}

	void VulkanMaterial::Set(const std::string& name, int value)
	{
		Set<int>(name, value);
	}

	void VulkanMaterial::Set(const std::string& name, uint32_t value)
	{
		Set<uint32_t>(name, value);
	}

	void VulkanMaterial::Set(const std::string& name, bool value)
	{
		// Bools are 4-byte ints
		Set<int>(name, (int)value);
	}

	void VulkanMaterial::Set(const std::string& name, const glm::ivec2& value)
	{
		Set<glm::ivec2>(name, value);
	}

	void VulkanMaterial::Set(const std::string& name, const glm::ivec3& value)
	{
		Set<glm::ivec3>(name, value);
	}

	void VulkanMaterial::Set(const std::string& name, const glm::ivec4& value)
	{
		Set<glm::ivec4>(name, value);
	}

	void VulkanMaterial::Set(const std::string& name, const glm::vec2& value)
	{
		Set<glm::vec2>(name, value);
	}

	void VulkanMaterial::Set(const std::string& name, const glm::vec3& value)
	{
		Set<glm::vec3>(name, value);
	}

	void VulkanMaterial::Set(const std::string& name, const glm::vec4& value)
	{
		Set<glm::vec4>(name, value);
	}

	void VulkanMaterial::Set(const std::string& name, const glm::mat3& value)
	{
		Set<glm::mat3>(name, value);
	}

	void VulkanMaterial::Set(const std::string& name, const glm::mat4& value)
	{
		Set<glm::mat4>(name, value);
	}

	void VulkanMaterial::Set(const std::string& name, const Ref<Texture2D>& texture)
	{
		SetVulkanDescriptor(name, texture);
	}

	void VulkanMaterial::Set(const std::string& name, const Ref<Texture2D>& texture, uint32_t arrayIndex)
	{
		SetVulkanDescriptor(name, texture, arrayIndex);
	}

	void VulkanMaterial::Set(const std::string& name, const Ref<Image2D>& image)
	{
		SetVulkanDescriptor(name, image);
	}

	// Get Resources
	float& VulkanMaterial::GetFloat(const std::string& name)
	{
		return Get<float>(name);
	}

	int32_t& VulkanMaterial::GetInt(const std::string& name)
	{
		return Get<int32_t>(name);
	}

	uint32_t& VulkanMaterial::GetUInt(const std::string& name)
	{
		return Get<uint32_t>(name);
	}

	bool& VulkanMaterial::GetBool(const std::string& name)
	{
		return Get<bool>(name);
	}

	glm::vec2& VulkanMaterial::GetVector2(const std::string& name)
	{
		return Get<glm::vec2>(name);
	}

	glm::vec3& VulkanMaterial::GetVector3(const std::string& name)
	{
		return Get<glm::vec3>(name);
	}

	glm::vec4& VulkanMaterial::GetVector4(const std::string& name)
	{
		return Get<glm::vec4>(name);
	}

	glm::mat3& VulkanMaterial::GetMatrix3(const std::string& name)
	{
		return Get<glm::mat3>(name);
	}

	glm::mat4& VulkanMaterial::GetMatrix4(const std::string& name)
	{
		return Get<glm::mat4>(name);
	}

	void VulkanMaterial::ApplyMaterial()
	{
		auto device = VulkanContext::GetDevice()->GetVulkanDevice();

		uint32_t frameIndex = VulkanContext::GetSwapChain()->GetCurrentFrameIndex();// Renderer::GetCurrentFrameIndex();
		
		auto vulkanShader = std::dynamic_pointer_cast<VulkanShader>(m_Shader);

		// TODO: test
		std::vector<VkDescriptorImageInfo> arrayImageInfos;
		std::vector<VkWriteDescriptorSet> writeDescriptors{};

		// update uniform buffer and texture sampler
		for (auto&& resource : m_ResourceBindings) {
			auto& wds = resource.WriteDescriptors[frameIndex];
			if (wds.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
			{
				writeDescriptors.emplace_back(wds).dstSet = m_DescriptorSets[frameIndex][resource.Set];
			}
			else if (wds.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
			{
				writeDescriptors.emplace_back(wds).dstSet = m_DescriptorSets[frameIndex][resource.Set];
			}
		}
		// update array descriptors
		if (!m_DescriptorArrays.empty())
		{
			for (auto&& [binding, wds] : m_DescriptorArrays)
			{
				for (auto tex : m_TextureArrays[binding])
				{
					Ref<VulkanTexture2D> texture = std::dynamic_pointer_cast<VulkanTexture2D>(tex);
					arrayImageInfos.emplace_back(texture->GetDescriptorImageInfo());
				}
				auto& writeDescritorSet = writeDescriptors.emplace_back(wds);
				writeDescritorSet.dstSet = m_DescriptorSets[frameIndex][0];
				writeDescritorSet.descriptorCount = arrayImageInfos.size();
				writeDescritorSet.pImageInfo = arrayImageInfos.data();
			}
		}
		vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptors.size()), writeDescriptors.data(), 0, nullptr);

		m_WriteDescriptors[frameIndex].clear();
		m_DescriptorArrays.clear();
	}

}