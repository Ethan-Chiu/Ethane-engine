#include "ethpch.h"
#include "Renderer2D.h"

#include "Shader.h"
#include "UniformBuffer.h"
#include "RenderCommand.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Pipeline.h"
#include "IndexBuffer.h"

// TODO: temp
#include "Platform/Vulkan/VulkanRendererAPI.h"

namespace Ethane {

	Renderer2D::Renderer2D()
	{
		Init();
	}

	Renderer2D::~Renderer2D()
	{
		Shutdown();
	}

	void Renderer2D::Init()
	{
		m_CommandBuffer = RenderCommandBuffer::Create(0, "Renderer2D");
		ETH_PROFILE_FUNCTION();
		
		FramebufferSpecification framebufferSpec;
		framebufferSpec.Attachments = { ImageFormat::RGBA32F, ImageFormat::Depth };
		framebufferSpec.Samples = 1;
		framebufferSpec.ClearOnLoad = false;
		framebufferSpec.ClearColor = { 0.1f, 0.5f, 0.5f, 1.0f };
		framebufferSpec.DebugName = "Renderer2D Framebuffer";
		// TODO: 
		framebufferSpec.Width = 1600;
		framebufferSpec.Height = 900;
		Ref<Framebuffer> framebuffer = Framebuffer::Create(framebufferSpec);

		RenderPassSpecification renderPassSpec;
		renderPassSpec.TargetFramebuffer = framebuffer;
		Ref<RenderPass> renderPass = RenderPass::Create(renderPassSpec);

		// White Texture
		m_WhiteTexture = Texture2D::Create(1, 1);
		uint32_t whiteTextureData = 0xffffffff;
		m_WhiteTexture->SetData(&whiteTextureData, sizeof(uint32_t));

		int32_t samplers[Config::MaxTextureSlots];
		for (uint32_t i = 0; i < Config::MaxTextureSlots; i++)
			samplers[i] = i;

		m_TextureSlots[0] = m_WhiteTexture;

		// Quad Vertex
		m_QuadVertexPositions[0] = { -0.5f, -0.5f, 0.0f, 1.0f };
		m_QuadVertexPositions[1] = { 0.5f, -0.5f, 0.0f, 1.0f };
		m_QuadVertexPositions[2] = { 0.5f,  0.5f, 0.0f, 1.0f };
		m_QuadVertexPositions[3] = { -0.5f,  0.5f, 0.0f, 1.0f };

		{
			m_QuadVertexBufferBase = new QuadVertex[Config::MaxVertices];
			m_QuadVertexBuffer = VertexBuffer::Create(Config::MaxVertices * sizeof(QuadVertex));

			Ref<Shader> shader = ShaderLibrary::Get("Texture");

			// Quad Pipeline
			PipelineSpecification pipelineSpecification;
			pipelineSpecification.Shader = shader;
			pipelineSpecification.RenderPass = renderPass;
			pipelineSpecification.Layout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float4, "a_Color" },
				{ ShaderDataType::Float2, "a_TexCoord" },
				{ ShaderDataType::Float, "a_TexIndex" },
				{ ShaderDataType::Float, "a_TilingFactor" },
				// { ShaderDataType::Int, "a_EntityID" },
			};
			m_QuadPipeline = Pipeline::Create(pipelineSpecification);	

			uint32_t* quadIndices = new uint32_t[Config::MaxIndices];
			uint32_t offset = 0;
			for (uint32_t i = 0; i < Config::MaxIndices; i += 6)
			{
				quadIndices[i + 0] = offset + 0;
				quadIndices[i + 1] = offset + 1;
				quadIndices[i + 2] = offset + 2;
		
				quadIndices[i + 3] = offset + 2;
				quadIndices[i + 4] = offset + 3;
				quadIndices[i + 5] = offset + 0;
		
				offset += 4;
			}
		
			m_QuadIndexBuffer = IndexBuffer::Create(quadIndices, Config::MaxIndices);
			delete[] quadIndices;

			m_QuadMaterial = Material::Create(shader, "QuadMaterial");
		}

		

		// lines
		// {
		// 	s_Data.LineVertexBuffer = VertexBuffer::Create(s_Data.MaxLineVertices * sizeof(LineVertex));
		// 
		// 	s_Data.LineShader = Shader::Create("assets/shaders/Renderer2D_Line.glsl");
		// 
		// 	PipelineSpecification pipelineSpecification;
		// 	pipelineSpecification.Layout = {
		// 		{ShaderDataType::Float3, "a_Position"},
		// 		{ShaderDataType::Float4, "a_Color"}
		// 	};
		// 	s_Data.LinePipeline = Pipeline::Create(pipelineSpecification);
		// 
		// 	s_Data.LineVertexBufferBase = new LineVertex[s_Data.MaxLineVertices];
		// 
		// 	uint32_t* lineIndices = new uint32_t[s_Data.MaxLineIndices];
		// 	for (uint32_t i = 0; i < s_Data.MaxLineIndices; i++)
		// 		lineIndices[i] = i;
		// 
		// 	s_Data.LineIndexBuffer = IndexBuffer::Create(lineIndices, s_Data.MaxLineIndices);
		// 	delete[] lineIndices;
		// }
	}

	void Renderer2D::Shutdown()
	{
		ETH_PROFILE_FUNCTION();

		delete[] m_QuadVertexBufferBase;
	}

	void Renderer2D::StartBatch()
	{
		m_QuadIndexCount = 0;
		m_QuadVertexBufferPtr = m_QuadVertexBufferBase;
		m_TextureSlotIndex = 1;
	}

	void Renderer2D::BeginScene(const Camera& camera, glm::mat4& transform)
	{
		ETH_PROFILE_FUNCTION();

		m_ViewProjection = camera.GetProjection() * glm::inverse(transform);
		// TextureShader->SetUniformBufferByBindingPoint(0, &m_ViewProjection, sizeof(CameraUB));

		StartBatch();
	}

	void Renderer2D::BeginScene(const OrthographicCamera& camera)
	{
		ETH_PROFILE_FUNCTION();

		// m_TextureShader->Bind();
		// m_TextureShader->SetUniformBufferByBindingPoint(0, &camera.GetViewProjectionMatrix(), sizeof(CameraUB));

		StartBatch();
	}

	void Renderer2D::BeginScene(const EditorCamera& camera)
	{
		ETH_PROFILE_FUNCTION();

		m_ViewProjection = camera.GetViewProjection();
		// s_Data.TextureShader->SetUniformBufferByName("Camera", &s_Data.CameraBuffer, sizeof(Renderer2DData::CameraData));
		// s_Data.TextureShader->SetUniformBufferByBindingPoint(0, &s_Data.CameraBuffer, sizeof(Renderer2DData::CameraData));

		StartBatch();
	}

	void Renderer2D::EndScene()
	{
		ETH_PROFILE_FUNCTION();

		Flush();
	}

	void Renderer2D::Flush()
	{
		if (m_QuadIndexCount == 0)
			return; // Nothing to draw

		uint32_t dataSize = (uint32_t)((uint8_t*)m_QuadVertexBufferPtr - (uint8_t*)m_QuadVertexBufferBase);
		m_QuadVertexBuffer->SetData(m_QuadVertexBufferBase, dataSize);

#if OpenGL
		for (uint32_t i = 0; i < s_Data.TextureSlotIndex; i++)
			s_Data.TextureSlots[i]->Bind(i);

		s_Data.TextureShader->Bind();

		s_Data.QuadPipeline->Bind();
		s_Data.QuadIndexBuffer->Bind();

		RenderCommand::DrawIndexed(s_Data.QuadIndexCount);
#endif
		VulkanRendererAPI::BeginRenderCommandBuffer(m_CommandBuffer);

		VulkanRendererAPI::BeginRenderPass(m_QuadPipeline->GetSpecification().RenderPass);

		for (uint32_t i = 0; i < m_TextureSlots.size(); i++)
		{
			if (m_TextureSlots[i])
				m_QuadMaterial->Set("u_Textures", m_TextureSlots[i], i);
			else
				m_QuadMaterial->Set("u_Textures", m_WhiteTexture, i);
		}

		VulkanRendererAPI::DrawGeometry(m_QuadPipeline, m_QuadVertexBuffer, m_QuadIndexBuffer, m_QuadMaterial, glm::mat4(1.0f), m_QuadIndexCount);

		VulkanRendererAPI::EndRenderPass();

		VulkanRendererAPI::EndRenderCommandBuffer();
		m_CommandBuffer->Submit();

		m_Stats.DrawCalls++;
	}

	void Renderer2D::NextBatch()
	{
		ETH_PROFILE_FUNCTION();
		Flush();
		StartBatch();
	}

	////////////////////////////////////////////////////////////////////////////////////////////////
	// Primitives
	// DrawQuad
	void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color)
	{
		DrawQuad({ position.x, position.y, 0.0f }, size, color);
	}

	void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color)
	{
		ETH_PROFILE_FUNCTION();

		glm::mat4 transform = glm::scale(glm::translate(glm::mat4(1.0f), position), { size.x, size.y, 1.0f });
		DrawQuad(transform, color);
	}

	void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor)
	{
		DrawQuad({ position.x, position.y, 0.0f }, size, texture, tilingFactor, tintColor);
	}

	void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor)
	{
		ETH_PROFILE_FUNCTION();

		glm::mat4 transform = glm::scale(glm::translate(glm::mat4(1.0f), position), { size.x, size.y, 1.0f });
		DrawQuad(transform, texture, tilingFactor, tintColor);
	}

	void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const Ref<SubTexture2D>& subtexture, float tilingFactor, const glm::vec4& tintColor)
	{
		DrawQuad({ position.x, position.y, 0.0f }, size, subtexture, tilingFactor, tintColor);
	}

	void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const Ref<SubTexture2D>& subtexture, float tilingFactor, const glm::vec4& tintColor)
	{
		ETH_PROFILE_FUNCTION();

		glm::mat4 transform = glm::scale(glm::translate(glm::mat4(1.0f), position), { size.x, size.y, 1.0f });
		DrawQuad(transform, subtexture, tilingFactor, tintColor);
	}

	void Renderer2D::DrawQuad(const glm::mat4& transform, const glm::vec4& color, int entityID)
	{
		ETH_PROFILE_FUNCTION();

		if (m_QuadIndexCount >= Config::MaxIndices)
			NextBatch();

		constexpr size_t quadVertexCount = 4;
		constexpr glm::vec2 textureCoords[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };
		const float textureIndex = 0.0f;
		const float tilingFactor = 1.0f;

		for (size_t i = 0; i < quadVertexCount; i++)
		{
			m_QuadVertexBufferPtr->Position = transform * m_QuadVertexPositions[i];
			m_QuadVertexBufferPtr->Color = color;
			m_QuadVertexBufferPtr->TexCoord = textureCoords[i];
			m_QuadVertexBufferPtr->TexIndex = textureIndex;
			m_QuadVertexBufferPtr->TilingFactor = tilingFactor;
			m_QuadVertexBufferPtr->EntityID = entityID;
			m_QuadVertexBufferPtr++;
		}

		m_QuadIndexCount += 6;

		m_Stats.QuadCount++;
	}

	void Renderer2D::DrawQuad(const glm::mat4& transform, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor, int entityID)
	{
		ETH_PROFILE_FUNCTION();

		if (m_QuadIndexCount >= Config::MaxIndices)
			NextBatch();

		constexpr size_t quadVertexCount = 4;
		constexpr glm::vec2 textureCoords[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };
		constexpr glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f };

		float textureIndex = 0.0f;

		for (uint32_t i = 1; i < m_TextureSlotIndex; i++)
		{
			if (*m_TextureSlots[i].get() == *texture.get())
			{
				textureIndex = float(i);
				break;
			}
		}

		if (textureIndex == 0.0f)
		{
			textureIndex = (float)m_TextureSlotIndex;
			m_TextureSlots[m_TextureSlotIndex] = texture;
			m_TextureSlotIndex++;
		}

		for (size_t i = 0; i < quadVertexCount; i++)
		{
			m_QuadVertexBufferPtr->Position = transform * m_QuadVertexPositions[i];
			m_QuadVertexBufferPtr->Color = tintColor;
			m_QuadVertexBufferPtr->TexCoord = textureCoords[i];
			m_QuadVertexBufferPtr->TexIndex = textureIndex;
			m_QuadVertexBufferPtr->TilingFactor = tilingFactor;
			m_QuadVertexBufferPtr->EntityID = entityID;
			m_QuadVertexBufferPtr++;
		}

		m_QuadIndexCount += 6;

		m_Stats.QuadCount++;
	}

	void Renderer2D::DrawQuad(const glm::mat4& transform, const Ref<SubTexture2D>& subtexture, float tilingFactor, const glm::vec4& tintColor, int entityID)
	{
		ETH_PROFILE_FUNCTION();

		if (m_QuadIndexCount >= Config::MaxIndices)
			NextBatch();

		constexpr size_t quadVertexCount = 4;
		constexpr glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
		const glm::vec2* textureCoords = subtexture->GetTexCoords();
		const Ref<Texture2D> texture = subtexture->GetTexture();

		float textureIndex = 0.0f;

		for (uint32_t i = 1; i < m_TextureSlotIndex; i++)
		{
			if (*m_TextureSlots[i].get() == *texture.get())
			{
				textureIndex = float(i);
				break;
			}
		}

		if (textureIndex == 0.0f)
		{
			textureIndex = (float)m_TextureSlotIndex;
			m_TextureSlots[m_TextureSlotIndex] = texture;
			m_TextureSlotIndex++;
		}

		for (size_t i = 0; i < quadVertexCount; i++)
		{
			m_QuadVertexBufferPtr->Position = transform * m_QuadVertexPositions[i];
			m_QuadVertexBufferPtr->Color = tintColor;
			m_QuadVertexBufferPtr->TexCoord = textureCoords[i];
			m_QuadVertexBufferPtr->TexIndex = textureIndex;
			m_QuadVertexBufferPtr->TilingFactor = tilingFactor;
			m_QuadVertexBufferPtr->EntityID = entityID;
			m_QuadVertexBufferPtr++;
		}

		m_QuadIndexCount += 6;

		m_Stats.QuadCount++;
	}


	//DrawRotatedQuad
	void Renderer2D::DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const glm::vec4& color)
	{
		DrawRotatedQuad({ position.x, position.y, 0.0f }, size, rotation, color);
	}

	void Renderer2D::DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const glm::vec4& color)
	{
		ETH_PROFILE_FUNCTION();

		if (m_QuadIndexCount >= Config::MaxIndices)
			NextBatch();

		constexpr size_t quadVertexCount = 4;
		constexpr glm::vec2 textureCoords[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };
		const float textureIndex = 0.0f;
		const float tilingFactor = 1.0f;

		glm::mat4 transform = glm::scale(glm::rotate(glm::translate(glm::mat4(1.0f), position), glm::radians(rotation), { 0.0f, 0.0f, 1.0f }), { size.x, size.y, 1.0f });

		for (size_t i = 0; i < quadVertexCount; i++)
		{
			m_QuadVertexBufferPtr->Position = transform * m_QuadVertexPositions[i];
			m_QuadVertexBufferPtr->Color = color;
			m_QuadVertexBufferPtr->TexCoord = textureCoords[i];
			m_QuadVertexBufferPtr->TexIndex = textureIndex;
			m_QuadVertexBufferPtr->TilingFactor = tilingFactor;
			m_QuadVertexBufferPtr++;
		}

		m_QuadIndexCount += 6;
		
		m_Stats.QuadCount++;
	}

	void Renderer2D::DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor)
	{
		DrawRotatedQuad({ position.x, position.y, 0.0f }, size, rotation, texture, tilingFactor, tintColor);
	}

	void Renderer2D::DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor)
	{
		ETH_PROFILE_FUNCTION();

		if (m_QuadIndexCount >= Config::MaxIndices)
			NextBatch();

		constexpr size_t quadVertexCount = 4;
		constexpr glm::vec2 textureCoords[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };
		constexpr glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f };

		float textureIndex = 0.0f;

		for (uint32_t i = 1; i < m_TextureSlotIndex; i++)
		{
			if (*m_TextureSlots[i].get() == *texture.get())
			{
				textureIndex = float(i);
				break;
			}
		}

		if (textureIndex == 0.0f)
		{
			textureIndex = (float)m_TextureSlotIndex;
			m_TextureSlots[m_TextureSlotIndex] = texture;
			m_TextureSlotIndex++;
		}

		glm::mat4 transform = glm::scale(glm::rotate(glm::translate(glm::mat4(1.0f), position), glm::radians(rotation), { 0.0f, 0.0f, 1.0f }), { size.x, size.y, 1.0f });

		for (size_t i = 0; i < quadVertexCount; i++)
		{
			m_QuadVertexBufferPtr->Position = transform * m_QuadVertexPositions[i];
			m_QuadVertexBufferPtr->Color = tintColor;
			m_QuadVertexBufferPtr->TexCoord = textureCoords[i];
			m_QuadVertexBufferPtr->TexIndex = textureIndex;
			m_QuadVertexBufferPtr->TilingFactor = tilingFactor;
			m_QuadVertexBufferPtr++;
		}

		m_QuadIndexCount += 6;

		m_Stats.QuadCount++;
	}

	void Renderer2D::DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const Ref<SubTexture2D>& subtexture, float tilingFactor, const glm::vec4& tintColor)
	{
		DrawRotatedQuad({ position.x, position.y, 0.0f }, size, rotation, subtexture, tilingFactor, tintColor);
	}

	void Renderer2D::DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const Ref<SubTexture2D>& subtexture, float tilingFactor, const glm::vec4& tintColor)
	{
		ETH_PROFILE_FUNCTION();

		if (m_QuadIndexCount >= Config::MaxIndices)
			NextBatch();

		constexpr size_t quadVertexCount = 4;
		constexpr glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
		const glm::vec2* textureCoords = subtexture->GetTexCoords();
		const Ref<Texture2D> texture = subtexture->GetTexture();

		float textureIndex = 0.0f;

		for (uint32_t i = 1; i < m_TextureSlotIndex; i++)
		{
			if (*m_TextureSlots[i].get() == *texture.get())
			{
				textureIndex = float(i);
				break;
			}
		}

		if (textureIndex == 0.0f)
		{
			textureIndex = (float)m_TextureSlotIndex;
			m_TextureSlots[m_TextureSlotIndex] = texture;
			m_TextureSlotIndex++;
		}

		glm::mat4 transform = glm::scale(glm::rotate(glm::translate(glm::mat4(1.0f), position), glm::radians(rotation), { 0.0f, 0.0f, 1.0f }), { size.x, size.y, 1.0f });

		for (size_t i = 0; i < quadVertexCount; i++)
		{
			m_QuadVertexBufferPtr->Position = transform * m_QuadVertexPositions[i];
			m_QuadVertexBufferPtr->Color = tintColor;
			m_QuadVertexBufferPtr->TexCoord = textureCoords[i];
			m_QuadVertexBufferPtr->TexIndex = textureIndex;
			m_QuadVertexBufferPtr->TilingFactor = tilingFactor;
			m_QuadVertexBufferPtr++;
		}

		m_QuadIndexCount += 6;

		m_Stats.QuadCount++;
	}

	// Draw with component
	void Renderer2D::DrawSprite(const glm::mat4& transform, SpriteRendererComponent& src, int entityID)
	{
		DrawQuad(transform, src.Color, entityID);
	}

	void Renderer2D::DrawTexture(const glm::mat4& transform, Texture2DRendererComponent& src, int entityID)
	{
		DrawQuad(transform, src.Texture, src.TilingFactor, src.TintColor, entityID);
	}

	void Renderer2D::DrawTexture(const glm::mat4& transform, SubTexture2DRendererComponent& src, int entityID)
	{
		DrawQuad(transform, src.SubTexture, src.TilingFactor, src.TintColor, entityID);
	}

	// stats
	void Renderer2D::ResetStats()
	{
		memset(&m_Stats, 0, sizeof(Statistics));
	}

	Renderer2D::Statistics Renderer2D::GetStats()
	{
		return m_Stats;
	}
}