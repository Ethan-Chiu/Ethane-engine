#include "ethpch.h"

#include "TextureSystem.h"

namespace Ethane {
	
#define INVALID_ID 4294967295U

	bool TextureSystem::Init(uint64_t* memory_requirement, void* memory_arena, TextureSystemConfig& config)
	{
		if (config.MaxTextureCount <= 0)
		{
			ETH_CORE_WARN("MaxTextureCount should be greater than 0, the provided value is {0}", config.MaxTextureCount);
			return false;
		}

		*memory_requirement = CalculateMemoryRequirement(config);

		if (!memory_arena)
			return true;

        m_State = static_cast<TextureSystemState*>(memory_arena);
        m_State->Config = config;

        void* array_block = (void*)((uint32_t*)memory_arena + sizeof(TextureSystemState));
        m_State->RegisteredTextures = static_cast<Texture*>(array_block);

		uint32_t count = m_State->Config.MaxTextureCount;
		for (uint32_t i = 0; i < count; ++i) {
			m_State->RegisteredTextures[i].Id = INVALID_ID;
		}

        // create_default_textures(state_ptr);

        return true;
	}

	void TextureSystem::Shutdown(void* memory_arena)
	{
		if (m_State) {
			for (uint32_t i = 0; i < m_State->Config.MaxTextureCount; ++i) {
				auto t = &m_State->RegisteredTextures[i];
				//if (t->generation != INVALID_ID) {
				//	t->Destroy();
				//}
			}

			//destroy_default_textures(state_ptr);

			m_State = nullptr;
		}
	}

	Texture* TextureSystem::AcquireTexture(const std::string& name)
	{
		uint32_t id = INVALID_ID;
		if (!RegisterTexture(name, id)) {
			ETH_CORE_ERROR("AcquireTexture failed to acquire texture");
			return nullptr;
		}

		return &m_State->RegisteredTextures[id];
	}

	bool TextureSystem::RegisterTexture(const std::string& name, uint32_t& outTextureId)
	{
		
		return true;
	}

	bool TextureSystem::LoadTexture(const std::string& name, Texture& outTexture)
	{
		return true;
	}

	uint64_t TextureSystem::CalculateMemoryRequirement(TextureSystemConfig& config)
	{
		uint64_t memoryRequirement = sizeof(TextureSystemState);

		memoryRequirement += sizeof(Texture) * config.MaxTextureCount;

		return memoryRequirement;
	}
}