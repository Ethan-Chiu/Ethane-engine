#include "ethpch.h"
#include "VulkanMaterial.h"

#include "Ethane/Renderer/Renderer.h"

#include "VulkanContext.h"
#include "VulkanTexture.h"
#include "VulkanImage.h"
#include "VulkanPipeline.h"
#include "VulkanUniformBuffer.h"

// #include "Ethane/Core/Timer.h"

namespace Ethane {

	VulkanMaterial::VulkanMaterial(const Ref<Shader>& shader, const std::string& name)
		: m_Shader(shader), m_Name(name),
		m_WriteDescriptors(Renderer::GetConfig().FramesInFlight)
	{
		Init();
		// Renderer::RegisterShaderDependency(shader, this);
	}

	VulkanMaterial::VulkanMaterial(Ref<Material> material, const std::string& name)
		: m_Shader(material->GetShader()), m_Name(name),
		m_WriteDescriptors(Renderer::GetConfig().FramesInFlight)
	{
		if (name.empty())
			m_Name = material->GetName();

		// Renderer::RegisterShaderDependency(m_Shader, this);

		auto vulkanMaterial = std::dynamic_pointer_cast<VulkanMaterial>(material);
		
		// m_UniformStorageBuffer = Buffer::Copy(vulkanMaterial->m_UniformStorageBuffer.Data, vulkanMaterial->m_UniformStorageBuffer.Size);

		// m_ResidentDescriptors = vulkanMaterial->m_ResidentDescriptors;
		// m_ResidentDescriptorArrays = vulkanMaterial->m_ResidentDescriptorArrays;
		// m_PendingDescriptors = vulkanMaterial->m_PendingDescriptors;
		m_Textures = vulkanMaterial->m_Textures;
		m_TextureArrays = vulkanMaterial->m_TextureArrays;
		m_Images = vulkanMaterial->m_Images;
		// m_ImageHashes = vulkanMaterial->m_ImageHashes;
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
	}

	// void VulkanMaterial::AllocateStorage()
	// {
	// 	const auto& shaderBuffers = m_Shader->GetShaderBuffers();
	// 
	// 	if (shaderBuffers.size() > 0)
	// 	{
	// 		uint32_t size = 0;
	// 		for (auto [name, shaderBuffer] : shaderBuffers)
	// 			size += shaderBuffer.Size;
	// 
	// 		m_UniformStorageBuffer.Allocate(size);
	// 		m_UniformStorageBuffer.ZeroInitialize();
	// 	}
	// }
	// 
	// void VulkanMaterial::OnShaderReloaded()
	// {
	// 	return;
	// 	AllocateStorage();
	// }

	// const ShaderUniform* VulkanMaterial::FindUniformDeclaration(const std::string& name)
	// {
	// 	const auto& shaderBuffers = m_Shader->GetShaderBuffers();
	// 
	// 	ETH_CORE_ASSERT(shaderBuffers.size() <= 1, "We currently only support ONE material buffer!");
	// 
	// 	if (shaderBuffers.size() > 0)
	// 	{
	// 		const ShaderBuffer& buffer = (*shaderBuffers.begin()).second;
	// 		if (buffer.Uniforms.find(name) == buffer.Uniforms.end())
	// 			return nullptr;
	// 
	// 		return &buffer.Uniforms.at(name);
	// 	}
	// 	return nullptr;
	// }

	// const ShaderResourceDeclaration* VulkanMaterial::FindResourceDeclaration(const std::string& name)
	// {
	// 	auto& resources = m_Shader->GetResources();
	// 	for (const auto& [n, resource] : resources)
	// 	{
	// 		if (resource.GetName() == name)
	// 			return &resource;
	// 	}
	// 	return nullptr;
	// }

	void VulkanMaterial::SetVulkanDescriptor(const std::string& name, const Ref<Texture2D>& texture)
	{
		uint32_t binding = 0; // TODO
		// Texture is already set
		if (binding < m_Textures.size() && m_Textures[binding] ) // already in list TODO: check if they are the same
			return;
		if (binding >= m_Textures.size())
			m_Textures.resize(binding + 1);

		m_Textures[binding] = texture;
	
		const VkWriteDescriptorSet* wds = std::dynamic_pointer_cast<VulkanShader>(m_Shader)->GetWriteDescriptorSet(0, name);
		ETH_CORE_ASSERT(wds);
	}

	void VulkanMaterial::SetVulkanDescriptor(const std::string& name, const Ref<Texture2D>& texture, uint32_t arrayIndex)
	{
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
			const VkWriteDescriptorSet* wds = std::dynamic_pointer_cast<VulkanShader>(m_Shader)->GetWriteDescriptorSet(0, name);
			ETH_CORE_ASSERT(wds);
			m_DescriptorArrays[binding] = *wds;
		}
	}

	void VulkanMaterial::SetVulkanDescriptor(const std::string& name, const Ref<Image2D>& image)
	{
		ETH_CORE_ASSERT(std::dynamic_pointer_cast<VulkanImage2D>(image)->GetImageInfo().ImageView, "ImageView is null");

		uint32_t binding = 0; // TODO
		// TODO: replace with set/map
		// if (binding < m_Images.size() && m_Images[binding]) // already in list TODO: check if they are the same
		// 	return;
		if (binding >= m_Images.size())
			m_Images.resize(binding+1);

		m_Images[binding] = image;

		const VkWriteDescriptorSet* wds = std::dynamic_pointer_cast<VulkanShader>(m_Shader)->GetWriteDescriptorSet(0, name);
		ETH_CORE_ASSERT(wds);

		// TODO: test
		auto device = VulkanContext::GetDevice()->GetVulkanDevice();
		uint32_t frameIndex = VulkanContext::GetSwapChain().GetCurrentFrameIndex();// Renderer::GetCurrentFrameIndex();

		VkWriteDescriptorSet writeDescriptor{};

		writeDescriptor = *wds;
		writeDescriptor.dstArrayElement = 0;
		writeDescriptor.pImageInfo = &(std::dynamic_pointer_cast<VulkanImage2D>(m_Images[binding])->GetDescriptor()); // TODO: change

		m_WriteDescriptors[frameIndex].push_back(writeDescriptor);
		
	}

	// void VulkanMaterial::SetVulkanDescriptor(const std::string& name, const Ref<TextureCube>& texture)
	// {
	// 	const ShaderResourceDeclaration* resource = FindResourceDeclaration(name);
	// 	ETH_CORE_ASSERT(resource);
	// 
	// 	uint32_t binding = resource->GetRegister();
	// 	// Texture is already set
	// 	if (binding < m_Textures.size() && m_Textures[binding] && texture->GetHash() == m_Textures[binding]->GetHash())
	// 		return;
	// 
	// 	if (binding >= m_Textures.size())
	// 		m_Textures.resize(binding + 1);
	// 	m_Textures[binding] = texture;
	// 
	// 	const VkWriteDescriptorSet* wds = m_Shader.As<VulkanShader>()->GetDescriptorSet(name);
	// 	ETH_CORE_ASSERT(wds);
	// 	m_ResidentDescriptors[binding] = std::make_shared<PendingDescriptor>(PendingDescriptor{ PendingDescriptorType::TextureCube, *wds, {}, texture.As<Texture>(), nullptr });
	// 	m_PendingDescriptors.push_back(m_ResidentDescriptors.at(binding));
	// 
	// }

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

	// Ref<Texture2D> VulkanMaterial::GetTexture2D(const std::string& name)
	// {
	// 	return GetResource<Texture2D>(name);
	// }
	// 
	// Ref<Texture2D> VulkanMaterial::TryGetTexture2D(const std::string& name)
	// {
	// 	return TryGetResource<Texture2D>(name);
	// }

	void VulkanMaterial::UpdateForRendering(const std::vector<std::vector<VkWriteDescriptorSet>>& uniformBufferWriteDescriptors)
	{
		auto device = VulkanContext::GetDevice()->GetVulkanDevice();

		uint32_t frameIndex = VulkanContext::GetSwapChain().GetCurrentFrameIndex();// Renderer::GetCurrentFrameIndex();

		// std::vector<VkDescriptorImageInfo> arrayImageInfos;

		// for (auto&& [binding, pd] : m_ResidentDescriptors)
		// {
		// 	if (pd->Type == PendingDescriptorType::Texture2D)
		// 	{
		// 		Ref<VulkanTexture2D> texture = std::dynamic_pointer_cast<VulkanTexture2D>(pd->Texture);
		// 		pd->ImageInfo = texture->GetVulkanDescriptorInfo();
		// 		pd->WDS.pImageInfo = &pd->ImageInfo;
		// 	}
		// 	else if (pd->Type == PendingDescriptorType::Image2D)
		// 	{
		// 		Ref<VulkanImage2D> image = std::dynamic_pointer_cast<VulkanImage2D>(pd->Image);
		// 		pd->ImageInfo = image->GetDescriptor();
		// 		pd->WDS.pImageInfo = &pd->ImageInfo;
		// 	}
		// 
		// 	m_WriteDescriptors[frameIndex].push_back(pd->WDS);
		// }

		// for (auto&& [binding, pd] : m_ResidentDescriptorArrays)
		// {
		// 	if (pd->Type == PendingDescriptorType::Texture2D)
		// 	{
		// 		for (auto tex : pd->Textures)
		// 		{
		// 			Ref<VulkanTexture2D> texture = tex.As<VulkanTexture2D>();
		// 			arrayImageInfos.emplace_back(texture->GetVulkanDescriptorInfo());
		// 		}
		// 	}
		// 	pd->WDS.pImageInfo = arrayImageInfos.data();
		// 	pd->WDS.descriptorCount = arrayImageInfos.size();
		// 	m_WriteDescriptors[frameIndex].push_back(pd->WDS);
		// }

		
		auto vulkanShader = std::dynamic_pointer_cast<VulkanShader>(m_Shader);
		// auto descriptorSet = vulkanShader->CreateDescriptorSets(0);
		// m_DescriptorSets[frameIndex] = descriptorSet;
		// for (auto& writeDescriptor : m_WriteDescriptors[frameIndex])
		// 	writeDescriptor.dstSet = descriptorSet.DescriptorSets[0];
		// vkUpdateDescriptorSets(vulkanDevice, (uint32_t)m_WriteDescriptors[frameIndex].size(), m_WriteDescriptors[frameIndex].data(), 0, nullptr);
		
		// TODO: test
		std::vector<VkDescriptorImageInfo> arrayImageInfos;
		std::vector<VkWriteDescriptorSet> writeDescriptors{};

		auto descriptorSet = vulkanShader->CreateDescriptorSets(0);
		m_DescriptorSetsAndPool[frameIndex] = descriptorSet;

		// update uniform buffer
		if (!uniformBufferWriteDescriptors.empty())
		{
			for (auto& wds : uniformBufferWriteDescriptors[frameIndex])
			{
				writeDescriptors.emplace_back(wds).dstSet = m_DescriptorSetsAndPool[frameIndex].DescriptorSets[0];
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
				writeDescritorSet.dstSet = m_DescriptorSetsAndPool[frameIndex].DescriptorSets[0];
				writeDescritorSet.descriptorCount = arrayImageInfos.size();
				writeDescritorSet.pImageInfo = arrayImageInfos.data();
			}
		}
		// update other descriptors ( textures )
		if (!m_WriteDescriptors.empty())
		{
			for (auto& wds : m_WriteDescriptors[frameIndex])
			{
				writeDescriptors.emplace_back(wds).dstSet = m_DescriptorSetsAndPool[frameIndex].DescriptorSets[0];
			}
		}
		vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptors.size()), writeDescriptors.data(), 0, nullptr);

		m_WriteDescriptors[frameIndex].clear();
		m_DescriptorArrays.clear();
	}

}