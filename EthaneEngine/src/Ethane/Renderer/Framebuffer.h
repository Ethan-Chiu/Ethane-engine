#pragma once

#include "Ethane/Core/Base.h"
#include "Image.h"
#include <map>

namespace Ethane {

	class Framebuffer;

	// enum class FramebufferTextureFormat
	// {
	// 	None = 0,
	// 
	// 	// Color
	// 	RGBA8,
	// 	RED_INTEGER,
	// 
	// 	// Depth/stencil
	// 	DEPTH24STENCIL8,
	// 
	// 	// Defaults
	// 	Depth = DEPTH24STENCIL8
	// };

	struct FramebufferTextureSpecification
	{
		// TODO: remove FramebufferTextureFormat
		FramebufferTextureSpecification() = default;
		// FramebufferTextureSpecification(FramebufferTextureFormat format)
		// 	: TextureFormat(format) {}
		FramebufferTextureSpecification(ImageFormat format)
			: Format(format) {}
	

		ImageFormat Format; // TODO
		// FramebufferTextureFormat TextureFormat = FramebufferTextureFormat::None;
		// TODO: filtering/wrap
	};

	struct FramebufferAttachmentSpecification
	{
		FramebufferAttachmentSpecification() = default;
		FramebufferAttachmentSpecification(std::initializer_list<FramebufferTextureSpecification> attachments)
			: Attachments(attachments) {}

		std::vector<FramebufferTextureSpecification> Attachments;
	};

	struct FramebufferSpecification
	{
		uint32_t Width, Height;
		glm::vec4 ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
		FramebufferAttachmentSpecification Attachments;
		uint32_t Samples = 1;

		bool SwapChainTarget = false;

		bool ClearOnLoad = true;

		bool Blend = true;


		// Specify existing images to attach instead of creating
		// new images. attachment index -> image
		std::map<uint32_t, Ref<Image2D>> ExistingImages;

		// Note: these are used to attach multi-layered depth images and color image arrays
		Ref<Image2D> ExistingImage;
		std::vector<uint32_t> ExistingImageLayers;

		// At the moment this will just create a new render pass with an existing framebuffer
		Ref<Framebuffer> ExistingFramebuffer;

		std::string DebugName = "framebuffer";
	};

	class Framebuffer 
	{
	public:
		virtual ~Framebuffer() = default;

		virtual void Bind() = 0;
		virtual void Unbind() = 0;

		virtual void Resize(uint32_t width, uint32_t height) = 0;
		virtual int ReadPixel(uint32_t attachmentIndex, int x, int y) = 0;

		virtual void ClearAttachment(uint32_t attachmentIndex, int value) = 0;

		// Getter
		virtual uint32_t GetColorAttachmentRendererID(uint32_t index = 0) const = 0;

		virtual const FramebufferSpecification& GetSpecification() const = 0;

		static Ref<Framebuffer> Create(const FramebufferSpecification& spec);
	};

}