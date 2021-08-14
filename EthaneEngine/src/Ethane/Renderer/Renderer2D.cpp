#include "ethpch.h"
#include "Renderer2D.h"

#include "Shader.h"
#include "UniformBuffer.h"
#include "RenderCommand.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Pipeline.h"
#include "IndexBuffer.h"

namespace Ethane {

	struct QuadVertex
	{
		glm::vec3 Position;
		glm::vec4 Color;
		glm::vec2 TexCoord;
		float TexIndex;
		float TilingFactor;

		// Editor only
		int EntityID;
	};

	struct LineVertex
	{
		glm::vec3 Position;
		glm::vec4 Color;
	};

	struct Renderer2DData
	{
		static const uint32_t MaxQuads = 10000;
		static const uint32_t MaxVertices = 4 * MaxQuads;
		static const uint32_t MaxIndices = 6 * MaxQuads;
		static const uint32_t MaxTextureSlots = 32;

		static const uint32_t MaxLines = 20000;
		static const uint32_t MaxLineVertices = MaxLines * 2;
		static const uint32_t MaxLineIndices = MaxLines * 6;

		Ref<Pipeline> QuadPipeline;
		Ref<VertexBuffer> QuadVertexBuffer;
		Ref<IndexBuffer> QuadIndexBuffer;

		Ref<Shader> TextureShader;
		Ref<Texture2D> WhiteTexture;

		uint32_t QuadIndexCount = 0;
		QuadVertex* QuadVertexBufferBase = nullptr;
		QuadVertex* QuadVertexBufferPtr = nullptr;

		std::array<Ref<Texture2D>, MaxTextureSlots> TextureSlots;
		uint32_t TextureSlotIndex = 1;

		glm::vec4 QuadVertexPositions[4];

		// lines
		// Ref<Pipeline> LinePipeline;
		// Ref<VertexBuffer> LineVertexBuffer;
		// Ref<IndexBuffer> LineIndexBuffer;
		// Ref<Shader> LineShader;
		// 
		// uint32_t LineIndexCount = 0;
		// LineVertex* LineVertexBufferBase = nullptr;
		// LineVertex* LIneVertexBufferPtr = nullptr;

		Renderer2D::Statistics Stats;

		struct CameraData
		{
			glm::mat4 ViewProjection;
		};
		CameraData CameraBuffer;
			// delete
			// Ref<UniformBuffer> CameraUniformBuffer;

		// Grid
		// struct TransformData
		// {
		// 	glm::mat4 Transform;
		// };
		struct SettingsData
		{
			float GridScale = 16.025f;
			float GridRes = 0.025f;
		};
		// TransformData TransformBuffer;
		// Ref<UniformBuffer> TransformUniformBuffer;
		SettingsData SettingsBuffer;
		Ref<UniformBuffer> SettingsUniformBuffer;
		Ref<Shader> GridShader;
	};

	static Renderer2DData s_Data;

	void Renderer2D::Init()
	{
		ETH_PROFILE_FUNCTION();
		{
			s_Data.QuadVertexBuffer = VertexBuffer::Create(s_Data.MaxVertices * sizeof(QuadVertex));

			PipelineSpecification pipelineSpecification;
			pipelineSpecification.Layout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float4, "a_Color" },
				{ ShaderDataType::Float2, "a_TexCoord" },
				{ ShaderDataType::Float, "a_TexIndex" },
				{ ShaderDataType::Float, "a_TilingFactor" },
				{ ShaderDataType::Int, "a_EntityID" },
			};
			s_Data.QuadPipeline = Pipeline::Create(pipelineSpecification);	

			s_Data.QuadVertexBufferBase = new QuadVertex[s_Data.MaxVertices];

			uint32_t* quadIndices = new uint32_t[s_Data.MaxIndices];
			uint32_t offset = 0;
			for (uint32_t i = 0; i < s_Data.MaxIndices; i += 6)
			{
				quadIndices[i + 0] = offset + 0;
				quadIndices[i + 1] = offset + 1;
				quadIndices[i + 2] = offset + 2;
		
				quadIndices[i + 3] = offset + 2;
				quadIndices[i + 4] = offset + 3;
				quadIndices[i + 5] = offset + 0;
		
				offset += 4;
			}
		
			s_Data.QuadIndexBuffer = IndexBuffer::Create(quadIndices, s_Data.MaxIndices);
			delete[] quadIndices;
		}

		s_Data.WhiteTexture = Texture2D::Create(1, 1);
		uint32_t whiteTextureData = 0xffffffff;
		s_Data.WhiteTexture->SetData(&whiteTextureData, sizeof(uint32_t));

		int32_t samplers[s_Data.MaxTextureSlots];
		for (uint32_t i = 0; i < s_Data.MaxTextureSlots; i++)
			samplers[i] = i;

		s_Data.TextureShader = ShaderLibrary::Get("Texture");

		s_Data.TextureSlots[0] = s_Data.WhiteTexture;

		s_Data.QuadVertexPositions[0] = { -0.5f, -0.5f, 0.0f, 1.0f };
		s_Data.QuadVertexPositions[1] = {  0.5f, -0.5f, 0.0f, 1.0f };
		s_Data.QuadVertexPositions[2] = {  0.5f,  0.5f, 0.0f, 1.0f };
		s_Data.QuadVertexPositions[3] = { -0.5f,  0.5f, 0.0f, 1.0f };

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

			// delete
			// s_Data.CameraUniformBuffer = UniformBuffer::Create(sizeof(Renderer2DData::CameraData), 0);


		// grid
		// s_Data.SettingsUniformBuffer = UniformBuffer::Create(sizeof(Renderer2DData::SettingsData), 1);
		// s_Data.GridShader = Shader::Create("assets/shaders/Grid.glsl");
	}

	void Renderer2D::Shutdown()
	{
		ETH_PROFILE_FUNCTION();

		delete[] s_Data.QuadVertexBufferBase;
	}

	void Renderer2D::StartBatch()
	{
		s_Data.QuadIndexCount = 0;
		s_Data.QuadVertexBufferPtr = s_Data.QuadVertexBufferBase;
		s_Data.TextureSlotIndex = 1;
	}

	void Renderer2D::BeginScene(const Camera& camera, glm::mat4& transform)
	{
		ETH_PROFILE_FUNCTION();

		s_Data.CameraBuffer.ViewProjection = camera.GetProjection() * glm::inverse(transform);
		s_Data.TextureShader->SetUniformBufferByBindingPoint(0, &s_Data.CameraBuffer, sizeof(Renderer2DData::CameraData));

		StartBatch();
	}

	void Renderer2D::BeginScene(const OrthographicCamera& camera)
	{
		ETH_PROFILE_FUNCTION();

		s_Data.TextureShader->Bind();
		s_Data.TextureShader->SetUniformBufferByBindingPoint(0, &camera.GetViewProjectionMatrix(), sizeof(Renderer2DData::CameraData));

		StartBatch();
	}

	void Renderer2D::BeginScene(const EditorCamera& camera)
	{
		ETH_PROFILE_FUNCTION();

		s_Data.CameraBuffer.ViewProjection = camera.GetViewProjection();
		// s_Data.TextureShader->SetUniformBufferByName("Camera", &s_Data.CameraBuffer, sizeof(Renderer2DData::CameraData));
		s_Data.TextureShader->SetUniformBufferByBindingPoint(0, &s_Data.CameraBuffer, sizeof(Renderer2DData::CameraData));


		// Grid
		// s_Data.SettingsUniformBuffer->SetData(&s_Data.SettingsBuffer, sizeof(Renderer2DData::SettingsData));

		StartBatch();
	}

	void Renderer2D::EndScene()
	{
		ETH_PROFILE_FUNCTION();

		Flush();
	}

	void Renderer2D::Flush()
	{
		if (s_Data.QuadIndexCount == 0)
			return; // Nothing to draw

		uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.QuadVertexBufferPtr - (uint8_t*)s_Data.QuadVertexBufferBase);
		s_Data.QuadVertexBuffer->SetData(s_Data.QuadVertexBufferBase, dataSize);

		for (uint32_t i = 0; i < s_Data.TextureSlotIndex; i++)
			s_Data.TextureSlots[i]->Bind(i);

		s_Data.TextureShader->Bind();

		s_Data.QuadPipeline->Bind();
		s_Data.QuadIndexBuffer->Bind();
		RenderCommand::DrawIndexed(s_Data.QuadIndexCount);


		// Grid
		// s_Data.GridShader->Bind();

		s_Data.Stats.DrawCalls++;
	}

	void Renderer2D::NextBatch()
	{
		ETH_PROFILE_FUNCTION();

		Flush();
		StartBatch();
	}

	//DrawQuad
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

		if (s_Data.QuadIndexCount >= Renderer2DData::MaxIndices)
			NextBatch();

		constexpr size_t quadVertexCount = 4;
		constexpr glm::vec2 textureCoords[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };
		const float textureIndex = 0.0f;
		const float tilingFactor = 1.0f;

		for (size_t i = 0; i < quadVertexCount; i++)
		{
			s_Data.QuadVertexBufferPtr->Position = transform * s_Data.QuadVertexPositions[i];
			s_Data.QuadVertexBufferPtr->Color = color;
			s_Data.QuadVertexBufferPtr->TexCoord = textureCoords[i];
			s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
			s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
			s_Data.QuadVertexBufferPtr->EntityID = entityID;
			s_Data.QuadVertexBufferPtr++;
		}

		s_Data.QuadIndexCount += 6;

		s_Data.Stats.QuadCount++;
	}

	void Renderer2D::DrawQuad(const glm::mat4& transform, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor, int entityID)
	{
		ETH_PROFILE_FUNCTION();

		if (s_Data.QuadIndexCount >= Renderer2DData::MaxIndices)
			NextBatch();

		constexpr size_t quadVertexCount = 4;
		constexpr glm::vec2 textureCoords[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };
		constexpr glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f };

		float textureIndex = 0.0f;

		for (uint32_t i = 1; i < s_Data.TextureSlotIndex; i++)
		{
			if (*s_Data.TextureSlots[i].get() == *texture.get())
			{
				textureIndex = float(i);
				break;
			}
		}

		if (textureIndex == 0.0f)
		{
			textureIndex = (float)s_Data.TextureSlotIndex;
			s_Data.TextureSlots[s_Data.TextureSlotIndex] = texture;
			s_Data.TextureSlotIndex++;
		}

		for (size_t i = 0; i < quadVertexCount; i++)
		{
			s_Data.QuadVertexBufferPtr->Position = transform * s_Data.QuadVertexPositions[i];
			s_Data.QuadVertexBufferPtr->Color = tintColor;
			s_Data.QuadVertexBufferPtr->TexCoord = textureCoords[i];
			s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
			s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
			s_Data.QuadVertexBufferPtr->EntityID = entityID;
			s_Data.QuadVertexBufferPtr++;
		}

		s_Data.QuadIndexCount += 6;

		s_Data.Stats.QuadCount++;
	}

	void Renderer2D::DrawQuad(const glm::mat4& transform, const Ref<SubTexture2D>& subtexture, float tilingFactor, const glm::vec4& tintColor, int entityID)
	{
		ETH_PROFILE_FUNCTION();

		if (s_Data.QuadIndexCount >= Renderer2DData::MaxIndices)
			NextBatch();

		constexpr size_t quadVertexCount = 4;
		constexpr glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
		const glm::vec2* textureCoords = subtexture->GetTexCoords();
		const Ref<Texture2D> texture = subtexture->GetTexture();

		float textureIndex = 0.0f;

		for (uint32_t i = 1; i < s_Data.TextureSlotIndex; i++)
		{
			if (*s_Data.TextureSlots[i].get() == *texture.get())
			{
				textureIndex = float(i);
				break;
			}
		}

		if (textureIndex == 0.0f)
		{
			textureIndex = (float)s_Data.TextureSlotIndex;
			s_Data.TextureSlots[s_Data.TextureSlotIndex] = texture;
			s_Data.TextureSlotIndex++;
		}

		for (size_t i = 0; i < quadVertexCount; i++)
		{
			s_Data.QuadVertexBufferPtr->Position = transform * s_Data.QuadVertexPositions[i];
			s_Data.QuadVertexBufferPtr->Color = tintColor;
			s_Data.QuadVertexBufferPtr->TexCoord = textureCoords[i];
			s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
			s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
			s_Data.QuadVertexBufferPtr->EntityID = entityID;
			s_Data.QuadVertexBufferPtr++;
		}

		s_Data.QuadIndexCount += 6;

		s_Data.Stats.QuadCount++;
	}


	//DrawRotatedQuad
	void Renderer2D::DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const glm::vec4& color)
	{
		DrawRotatedQuad({ position.x, position.y, 0.0f }, size, rotation, color);
	}

	void Renderer2D::DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const glm::vec4& color)
	{
		ETH_PROFILE_FUNCTION();

		if (s_Data.QuadIndexCount >= Renderer2DData::MaxIndices)
			NextBatch();

		constexpr size_t quadVertexCount = 4;
		constexpr glm::vec2 textureCoords[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };
		const float textureIndex = 0.0f;
		const float tilingFactor = 1.0f;

		glm::mat4 transform = glm::scale(glm::rotate(glm::translate(glm::mat4(1.0f), position), glm::radians(rotation), { 0.0f, 0.0f, 1.0f }), { size.x, size.y, 1.0f });

		for (size_t i = 0; i < quadVertexCount; i++)
		{
			s_Data.QuadVertexBufferPtr->Position = transform * s_Data.QuadVertexPositions[i];
			s_Data.QuadVertexBufferPtr->Color = color;
			s_Data.QuadVertexBufferPtr->TexCoord = textureCoords[i];
			s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
			s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
			s_Data.QuadVertexBufferPtr++;
		}

		s_Data.QuadIndexCount += 6;
		
		s_Data.Stats.QuadCount++;
	}

	void Renderer2D::DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor)
	{
		DrawRotatedQuad({ position.x, position.y, 0.0f }, size, rotation, texture, tilingFactor, tintColor);
	}

	void Renderer2D::DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor)
	{
		ETH_PROFILE_FUNCTION();

		if (s_Data.QuadIndexCount >= Renderer2DData::MaxIndices)
			NextBatch();

		constexpr size_t quadVertexCount = 4;
		constexpr glm::vec2 textureCoords[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };
		constexpr glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f };

		float textureIndex = 0.0f;

		for (uint32_t i = 1; i < s_Data.TextureSlotIndex; i++)
		{
			if (*s_Data.TextureSlots[i].get() == *texture.get())
			{
				textureIndex = float(i);
				break;
			}
		}

		if (textureIndex == 0.0f)
		{
			textureIndex = (float)s_Data.TextureSlotIndex;
			s_Data.TextureSlots[s_Data.TextureSlotIndex] = texture;
			s_Data.TextureSlotIndex++;
		}

		glm::mat4 transform = glm::scale(glm::rotate(glm::translate(glm::mat4(1.0f), position), glm::radians(rotation), { 0.0f, 0.0f, 1.0f }), { size.x, size.y, 1.0f });

		for (size_t i = 0; i < quadVertexCount; i++)
		{
			s_Data.QuadVertexBufferPtr->Position = transform * s_Data.QuadVertexPositions[i];
			s_Data.QuadVertexBufferPtr->Color = tintColor;
			s_Data.QuadVertexBufferPtr->TexCoord = textureCoords[i];
			s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
			s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
			s_Data.QuadVertexBufferPtr++;
		}

		s_Data.QuadIndexCount += 6;

		s_Data.Stats.QuadCount++;
	}

	void Renderer2D::DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const Ref<SubTexture2D>& subtexture, float tilingFactor, const glm::vec4& tintColor)
	{
		DrawRotatedQuad({ position.x, position.y, 0.0f }, size, rotation, subtexture, tilingFactor, tintColor);
	}

	void Renderer2D::DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const Ref<SubTexture2D>& subtexture, float tilingFactor, const glm::vec4& tintColor)
	{
		ETH_PROFILE_FUNCTION();

		if (s_Data.QuadIndexCount >= Renderer2DData::MaxIndices)
			NextBatch();

		constexpr size_t quadVertexCount = 4;
		constexpr glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
		const glm::vec2* textureCoords = subtexture->GetTexCoords();
		const Ref<Texture2D> texture = subtexture->GetTexture();

		float textureIndex = 0.0f;

		for (uint32_t i = 1; i < s_Data.TextureSlotIndex; i++)
		{
			if (*s_Data.TextureSlots[i].get() == *texture.get())
			{
				textureIndex = float(i);
				break;
			}
		}

		if (textureIndex == 0.0f)
		{
			textureIndex = (float)s_Data.TextureSlotIndex;
			s_Data.TextureSlots[s_Data.TextureSlotIndex] = texture;
			s_Data.TextureSlotIndex++;
		}

		glm::mat4 transform = glm::scale(glm::rotate(glm::translate(glm::mat4(1.0f), position), glm::radians(rotation), { 0.0f, 0.0f, 1.0f }), { size.x, size.y, 1.0f });

		for (size_t i = 0; i < quadVertexCount; i++)
		{
			s_Data.QuadVertexBufferPtr->Position = transform * s_Data.QuadVertexPositions[i];
			s_Data.QuadVertexBufferPtr->Color = tintColor;
			s_Data.QuadVertexBufferPtr->TexCoord = textureCoords[i];
			s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
			s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
			s_Data.QuadVertexBufferPtr++;
		}

		s_Data.QuadIndexCount += 6;

		s_Data.Stats.QuadCount++;
	}

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

	void Renderer2D::ResetStats()
	{
		memset(&s_Data.Stats, 0, sizeof(Statistics));
	}

	Renderer2D::Statistics Renderer2D::GetStats()
	{
		return s_Data.Stats;
	}
}