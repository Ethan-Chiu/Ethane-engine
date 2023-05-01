#pragma once

#include <string>
#include "Ethane/Core/Base.h"

#include "Image.h"

namespace Ethane {

	struct TextureSpec
	{
		//uint32_t Id;

		uint32_t Width;

		uint32_t Height;

		uint8_t ChannelCount = 4;
	};

	class TextureOld
	{
	public:
		virtual ~TextureOld() = default;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;

		virtual void SetData(void* data, uint32_t size) = 0;

		virtual bool operator==(const TextureOld& other) const = 0;
	};

	class Texture2D : public TextureOld
	{
	public:
		static Ref<Texture2D> Create(const std::string& path);
		static Ref<Texture2D> Create(TextureSpec spec);
		static Ref<Texture2D> Create(Ref<Image2D> image);
	};
}