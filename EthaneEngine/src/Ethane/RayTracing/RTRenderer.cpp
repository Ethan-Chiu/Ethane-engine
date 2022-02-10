#include "ethpch.h"
#include "RTRenderer.h"

namespace Ethane {

	void RTRenderer::Init() {


		m_RT.InitRayTracing();
		m_RT.CreateBottomLevelAS();
		m_RT.CreateTopLevelAS();
	}

	void RTRenderer::BeginFrame() {

	}

	void RTRenderer::EndFrame() {

	}

    //--------------------------------------------------------------------------------------------------
    // Creating an offscreen frame buffer and the associated render pass
    //
    // void RTRenderer::CreateOffscreenRender()
    // {
    //     {
    //         FramebufferSpecification geoFramebufferSpec;
    //         geoFramebufferSpec.Attachments = { ImageFormat::RGBA32F, ImageFormat::Depth };
    //         geoFramebufferSpec.Samples = 1;
    //         geoFramebufferSpec.ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
    //         geoFramebufferSpec.DebugName = "Off screen";
    //         // TODO width and height
    //         geoFramebufferSpec.Width = 1600;
    //         geoFramebufferSpec.Height = 900;
    //         Ref<Framebuffer> framebuffer = Framebuffer::Create(geoFramebufferSpec);
    // 
    //         RenderPassSpecification renderPassSpec;
    //         renderPassSpec.TargetFramebuffer = framebuffer;
    //         // renderPassSpec.DebugName = "";
    //         m_OffscreenRenderPass = RenderPass::Create(renderPassSpec);
    //     }
    // }
}