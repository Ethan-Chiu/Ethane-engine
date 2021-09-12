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

	// void VulkanMaterial::SetVulkanDescriptor(const std::string& name, const Ref<Texture2D>& texture)
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
	// 	m_ResidentDescriptors[binding] = std::make_shared<PendingDescriptor>(PendingDescriptor{ PendingDescriptorType::Texture2D, *wds, {}, texture.As<Texture>(), nullptr });
	// 	m_PendingDescriptors.push_back(m_ResidentDescriptors.at(binding));
	// 
	// }

	// void VulkanMaterial::SetVulkanDescriptor(const std::string& name, const Ref<Texture2D>& texture, uint32_t arrayIndex)
	// {
	// 	const ShaderResourceDeclaration* resource = FindResourceDeclaration(name);
	// 	ETH_CORE_ASSERT(resource);
	// 
	// 	uint32_t binding = resource->GetRegister();
	// 	// Texture is already set
	// 	if (binding < m_TextureArrays.size() && m_TextureArrays[binding].size() < arrayIndex && texture->GetHash() == m_TextureArrays[binding][arrayIndex]->GetHash())
	// 		return;
	// 
	// 	if (binding >= m_TextureArrays.size())
	// 		m_TextureArrays.resize(binding + 1);
	// 
	// 	if (arrayIndex >= m_TextureArrays[binding].size())
	// 		m_TextureArrays[binding].resize(arrayIndex + 1);
	// 
	// 	m_TextureArrays[binding][arrayIndex] = texture;
	// 
	// 	const VkWriteDescriptorSet* wds = m_Shader.As<VulkanShader>()->GetDescriptorSet(name);
	// 	ETH_CORE_ASSERT(wds);
	// 	if (m_ResidentDescriptorArrays.find(binding) == m_ResidentDescriptorArrays.end())
	// 	{
	// 		m_ResidentDescriptorArrays[binding] = std::make_shared<PendingDescriptorArray>(PendingDescriptorArray{ PendingDescriptorType::Texture2D, *wds, {}, {}, {} });
	// 	}
	// 
	// 	auto& residentDesriptorArray = m_ResidentDescriptorArrays.at(binding);
	// 	if (arrayIndex >= residentDesriptorArray->Textures.size())
	// 		residentDesriptorArray->Textures.resize(arrayIndex + 1);
	// 
	// 	residentDesriptorArray->Textures[arrayIndex] = texture;
	// 
	// 	//m_PendingDescriptors.push_back(m_ResidentDescriptors.at(binding));
	// 
	// }


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

	// void VulkanMaterial::SetVulkanDescriptor(const std::string& name, const Ref<Image2D>& image)
	// {
	// 	// HZ_CORE_VERIFY(image);
	// 	ETH_CORE_ASSERT(image.As<VulkanImage2D>()->GetImageInfo().ImageView, "ImageView is null");
	// 
	// 	const ShaderResourceDeclaration* resource = FindResourceDeclaration(name);
	// 	// HZ_CORE_VERIFY(resource);
	// 
	// 	uint32_t binding = resource->GetRegister();
	// 	// TODO: replace with set/map
	// 	if (binding < m_Images.size() && m_Images[binding] && m_ImageHashes.at(binding) == image->GetHash())
	// 		return;
	// 
	// 	if (resource->GetRegister() >= m_Images.size())
	// 		m_Images.resize(resource->GetRegister() + 1);
	// 	m_Images[resource->GetRegister()] = image;
	// 	m_ImageHashes[resource->GetRegister()] = image->GetHash();
	// 
	// 	const VkWriteDescriptorSet* wds = m_Shader.As<VulkanShader>()->GetDescriptorSet(name);
	// 	ETH_CORE_ASSERT(wds);
	// 	m_ResidentDescriptors[binding] = std::make_shared<PendingDescriptor>(PendingDescriptor{ PendingDescriptorType::Image2D, *wds, {}, nullptr, image.As<Image>() });
	// 	m_PendingDescriptors.push_back(m_ResidentDescriptors.at(binding));
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
		auto vulkanDevice = VulkanContext::GetDevice()->GetVulkanDevice();

		// std::vector<VkDescriptorImageInfo> arrayImageInfos;

		uint32_t frameIndex = VulkanContext::GetSwapChain().GetCurrentFrameIndex();// Renderer::GetCurrentFrameIndex();

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
		// TODO: temp remove
		if(m_Texture2D == nullptr)
			m_Texture2D = CreateRef<VulkanTexture2D>("assets/textures/test.png");

		for (uint32_t i = 0; i < 3; i++)
		{
			auto descriptorSet = vulkanShader->CreateDescriptorSets(0);
			m_DescriptorSetsAndPool[i] = descriptorSet;

			std::vector<VkWriteDescriptorSet> writeDescriptors{};

			VkWriteDescriptorSet& samplerWriteDescriptor = writeDescriptors.emplace_back();
			samplerWriteDescriptor = *vulkanShader->GetWriteDescriptorSet(0, "u_Texture");
			samplerWriteDescriptor.dstSet = m_DescriptorSetsAndPool[i].DescriptorSets[0];
			samplerWriteDescriptor.dstArrayElement = 0;
			samplerWriteDescriptor.pImageInfo = &m_Texture2D->GetDescriptorImageInfo();
			vkUpdateDescriptorSets(vulkanDevice, static_cast<uint32_t>(writeDescriptors.size()), writeDescriptors.data(), 0, nullptr);

		}

	}

}