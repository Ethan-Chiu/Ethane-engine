#pragma once

#include "OrthographicCamera.h"
#include "Ethane/Renderer/Camera.h"
#include "Ethane/Renderer/EditorCamera.h"

#include "Texture.h"
#include "SubTexture2D.h"

#include "Ethane/Scene/Components.h"

#include "Ethane/Asset/ShaderLibrary.h"

#include "Ethane/Renderer/RenderCommandBuffer.h"

namespace Ethane {

	class Renderer2D
	{
	public:
		struct Config
		{
			static const uint32_t MaxQuads = 10000;
			static const uint32_t MaxVertices = 4 * MaxQuads;
			static const uint32_t MaxIndices = 6 * MaxQuads;
			static const uint32_t MaxTextureSlots = 32;

			static const uint32_t MaxLines = 20000;
			static const uint32_t MaxLineVertices = MaxLines * 2;
			static const uint32_t MaxLineIndices = MaxLines * 6;
		};
	public:
		Renderer2D();
		~Renderer2D();

		void Init();
		void Shutdown();

		void StartBatch();
		void BeginScene(const Camera& camera, glm::mat4& transform);
		void BeginScene(const OrthographicCamera& camera);
		void BeginScene(const EditorCamera& camera);

		void EndScene();
		void Flush();

		// Primitives
		void DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color);
		void DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color);
		void DrawQuad(const glm::vec3& position, const glm::vec2& size, const Ref<Texture2D>& texture, float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f));
		void DrawQuad(const glm::vec2& position, const glm::vec2& size, const Ref<Texture2D>& texture, float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f));
		void DrawQuad(const glm::vec3& position, const glm::vec2& size, const Ref<SubTexture2D>& subtexture, float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f));
		void DrawQuad(const glm::vec2& position, const glm::vec2& size, const Ref<SubTexture2D>& subtexture, float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f));

		void DrawQuad(const glm::mat4& transform, const glm::vec4& color, int entityID = -1);
		void DrawQuad(const glm::mat4& transform, const Ref<Texture2D>& texture, float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f), int entityID = -1);
		void DrawQuad(const glm::mat4& transform, const Ref<SubTexture2D>& subtexture, float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f), int entityID = -1);

		void DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const glm::vec4& color);
		void DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const glm::vec4& color);
		void DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const Ref<Texture2D>& texture, float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f));
		void DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const Ref<Texture2D>& texture, float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f));
		void DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const Ref<SubTexture2D>& subtexture, float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f));
		void DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const Ref<SubTexture2D>& subtexture, float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f));

		void DrawSprite(const glm::mat4& transform, SpriteRendererComponent& src, int entityID);
		void DrawTexture(const glm::mat4& transform, Texture2DRendererComponent& src, int entityID);
		void DrawTexture(const glm::mat4& transform, SubTexture2DRendererComponent& src, int entityID);

		// Stats
		struct Statistics
		{
			uint32_t DrawCalls = 0;
			uint32_t QuadCount = 0;

			uint32_t GetTotalVertexCount() const { return QuadCount * 4; }
			uint32_t GetTotalIndexCount() const { return QuadCount * 6; }
		};
		void ResetStats();
		Statistics GetStats();
	private:
		void NextBatch();

	private:
		Ref<RenderCommandBuffer> m_CommandBuffer;

		glm::mat4 m_ViewProjection;

		struct CameraUB
		{
			glm::mat4 ViewProjection;
		};

		Ref<Texture2D> m_WhiteTexture;

		// Quad
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

		Ref<Pipeline> m_QuadPipeline;
		Ref<VertexBuffer> m_QuadVertexBuffer;
		Ref<IndexBuffer> m_QuadIndexBuffer;
		Ref<Material> m_QuadMaterial;

		uint32_t m_QuadIndexCount = 0;
		QuadVertex* m_QuadVertexBufferBase = nullptr;
		QuadVertex* m_QuadVertexBufferPtr = nullptr;

		std::array<Ref<Texture2D>, Config::MaxTextureSlots> m_TextureSlots;
		uint32_t m_TextureSlotIndex = 1;

		glm::vec4 m_QuadVertexPositions[4];

		// line
		struct LineVertex
		{
			glm::vec3 Position;
			glm::vec4 Color;
		};
		// Ref<Pipeline> LinePipeline;
		// Ref<VertexBuffer> LineVertexBuffer;
		// Ref<IndexBuffer> LineIndexBuffer;
		// Ref<Shader> LineShader;
		// 
		// uint32_t LineIndexCount = 0;
		// LineVertex* LineVertexBufferBase = nullptr;
		// LineVertex* LIneVertexBufferPtr = nullptr;

		Statistics m_Stats;
	};

}