#pragma once

#include "Ethane/Renderer/Material.h"

#include "VulkanShader.h"

// TODO: temp remove 
#include "VulkanTexture.h"

namespace Ethane {

	class VulkanMaterial : public Material
	{
	public:
		VulkanMaterial(const Ref<Shader>& shader, const std::string& name);
		VulkanMaterial(Ref<Material> material, const std::string& name);
		virtual ~VulkanMaterial();

		virtual void Invalidate() override;

		virtual void Set(const std::string& name, float value) override;
		virtual void Set(const std::string& name, int value) override;
		virtual void Set(const std::string& name, uint32_t value) override;
		virtual void Set(const std::string& name, bool value) override;
		virtual void Set(const std::string& name, const glm::ivec2& value) override;
		virtual void Set(const std::string& name, const glm::ivec3& value) override;
		virtual void Set(const std::string& name, const glm::ivec4& value) override;
		virtual void Set(const std::string& name, const glm::vec2& value) override;
		virtual void Set(const std::string& name, const glm::vec3& value) override;
		virtual void Set(const std::string& name, const glm::vec4& value) override;
		virtual void Set(const std::string& name, const glm::mat3& value) override;
		virtual void Set(const std::string& name, const glm::mat4& value) override;

		virtual void Set(const std::string& name, const Ref<Texture2D>& texture) override;
		virtual void Set(const std::string& name, const Ref<Texture2D>& texture, uint32_t arrayIndex) override;
		virtual void Set(const std::string& name, const Ref<Image2D>& image);
		// virtual void Set(const std::string& name, const Ref<TextureCube>& texture) override;

		virtual float& GetFloat(const std::string& name) override;
		virtual int32_t& GetInt(const std::string& name) override;
		virtual uint32_t& GetUInt(const std::string& name) override;
		virtual bool& GetBool(const std::string& name) override;
		virtual glm::vec2& GetVector2(const std::string& name) override;
		virtual glm::vec3& GetVector3(const std::string& name) override;
		virtual glm::vec4& GetVector4(const std::string& name) override;
		virtual glm::mat3& GetMatrix3(const std::string& name) override;
		virtual glm::mat4& GetMatrix4(const std::string& name) override;

		// TODO: 
		virtual Ref<Texture2D> GetTexture2D(const std::string& name) override { return nullptr; };
		// virtual Ref<TextureCube> GetTextureCube(const std::string& name) override;

		// TODO: 
		virtual Ref<Texture2D> TryGetTexture2D(const std::string& name) override { return nullptr; };
		// virtual Ref<TextureCube> TryGetTextureCube(const std::string& name) override;

		template <typename T>
		void Set(const std::string& name, const T& value)
		{
			
		}

		template<typename T>
		T& Get(const std::string& name)
		{
			return *(T*)(0);
		}

		void ApplyMaterial();

		// Getter
		virtual uint32_t GetFlags() const override { return m_MaterialFlags; }
		virtual bool GetFlag(MaterialFlag flag) const { return (uint32_t)flag & m_MaterialFlags; }

		virtual Ref<Shader> GetShader() override { return m_Shader; }
		virtual const std::string& GetName() const override { return m_Name; }
		VkDescriptorSet GetDescriptorSet(uint32_t frameIndex) const { return !m_DescriptorSetsAndPool.DescriptorSets.empty() ? m_DescriptorSetsAndPool.DescriptorSets[frameIndex] : nullptr; }
		
		// Buffer GetUniformStorageBuffer() { return m_UniformStorageBuffer; }

	private:
		void Init();

		// TODO: set Texture
		void SetVulkanDescriptor(const std::string& name, const Ref<Texture2D>& texture);
		void SetVulkanDescriptor(const std::string& name, const Ref<Texture2D>& texture, uint32_t arrayIndex);
		void SetVulkanDescriptor(const std::string& name, const Ref<Image2D>& image);
		// void SetVulkanDescriptor(const std::string& name, const Ref<TextureCube>& texture);

		// const ShaderUniform* FindUniformDeclaration(const std::string& name);
		// const ShaderResourceDeclaration* FindResourceDeclaration(const std::string& name);
	private:
		std::string m_Name;
		uint32_t m_MaterialFlags = 0;
		Ref<Shader> m_Shader;

		// resource binding map
		struct ResourceBinding {
			std::string Name;
			std::vector<VkWriteDescriptorSet> WriteDescriptors; // frame
		};
		std::vector<ResourceBinding> m_ResourceBindings;


		std::vector<Ref<Texture>> m_Textures; // TODO: Texture should only be stored as images
		std::vector<std::vector<Ref<Texture>>> m_TextureArrays;
		std::vector<Ref<Image>> m_Images;

		VulkanShader::DescriptorSetsAndPool m_DescriptorSetsAndPool;


		// Write Descriptors
		std::unordered_map<uint32_t, VkWriteDescriptorSet> m_DescriptorArrays; // binding
		std::vector<std::vector<VkWriteDescriptorSet>> m_WriteDescriptors; // frame
	};

}
