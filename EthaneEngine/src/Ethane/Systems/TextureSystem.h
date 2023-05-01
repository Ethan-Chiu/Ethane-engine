#pragma once

#include "Ethane/Renderer/Image.h"

namespace Ethane {

	struct Texture
	{
		uint32_t Id;

		uint32_t Width;

		uint32_t Height;

		uint8_t ChannelCount = 4;

		Image* InternalImage;
	};

	struct TextureSystemConfig
	{
		uint32_t MaxTextureCount;
	};

	struct TextureRef
	{
		uint64_t RefCount;
		uint32_t Handle;
		bool AutoRelease;
	};

	struct TextureSystemState
	{
		TextureSystemConfig Config;
		Image* DefaultTexture;
		
		Texture* RegisteredTextures;

		std::unordered_map<std::string, TextureRef> RegisteredTextureRefs;
	};

	class TextureSystem
	{
	public:
		bool Init(uint64_t* memory_requirement, void* memory_arena, TextureSystemConfig& config);
		void Shutdown(void* memory_arena);

		Texture* AcquireTexture(const std::string& name);

	private:
		uint64_t CalculateMemoryRequirement(TextureSystemConfig& config);
		bool RegisterTexture(const std::string& name, uint32_t& outTextureId);
		bool LoadTexture(const std::string& name, Texture& outTexture);

	private:
		TextureSystemState* m_State;
	};

}