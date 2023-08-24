#include "ethpch.h"

#include "GBuffer.h"

#include "Ethane/Systems/ShaderSystem.h"

namespace Ethane {

	GBuffer::GBuffer()
	{
		CreateImages();
		
		RenderTargetSpecification targetSpec;
		targetSpec.Attachments = { m_ImagePosition.get(), m_ImageNormal.get(), m_ImageAlbedo.get(), m_Depth.get()};
		targetSpec.IsTargetImage = false;
		targetSpec.ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
		targetSpec.DebugName = "offscreen";
		targetSpec.Width = 2048;
		targetSpec.Height = 2048;
		targetSpec.SwapChainTarget = false;
		m_OffscreenTarget = RenderTarget::Create(targetSpec);

		PipelineSpecification pipelineSpecification;
		pipelineSpecification.Layout = {
			{ ShaderDataType::Float3, "a_Position" },
			{ ShaderDataType::Float3, "a_Normal" },
			{ ShaderDataType::Float3, "a_Tangent" },
			{ ShaderDataType::Float3, "a_Binormal" },
			{ ShaderDataType::Float2, "a_TexCoord" },
		};
		ShaderSystem::Load("./assets/shaders/offscreen.glsl");
		auto shader = ShaderSystem::Get("offscreen");
		pipelineSpecification.CullMode = CullMode::BACK;
		pipelineSpecification.Shader = shader;
		pipelineSpecification.RenderPass = m_OffscreenTarget->GetRenderPass();
		m_OffscreenPipeline = Pipeline::Create(pipelineSpecification);
	}

	void GBuffer::CreateImages()
	{	
		ImageSpecification spec;
		spec.Width = 2048;
		spec.Height = 2048;
		spec.Format = ImageFormat::RGBA16F;
		spec.Usage = ImageUsage::Attachment;

		// (World space) Positions
		m_ImagePosition = Image2D::Create(spec);
		// (World space) Normals
		m_ImageNormal = Image2D::Create(spec);
		// Albedo (color)
		spec.Format = ImageFormat::RGBA;
		m_ImageAlbedo = Image2D::Create(spec);

		// Depth
		spec.Format = ImageFormat::DEPTH24STENCIL8;
		spec.Usage = ImageUsage::Attachment;
		m_Depth = Image2D::Create(spec);
	}

}