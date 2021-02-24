#pragma once

#include "OrthographicCamera.h"
#include "Ethane/Renderer/Camera.h"
#include "Ethane/Renderer/EditorCamera.h"

#include "Texture.h"

namespace Ethane {

	class Renderer3D
	{
	public:
		static void Init();
		static void Shutdown();

		static void BeginScene(const Camera& camera, glm::mat4& transform);
		static void BeginScene(const OrthographicCamera& camera);
		static void BeginScene(const EditorCamera& camera);

		static void EndScene();
		static void Flush();

		//Primitives
		static void DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const Ref<SubTexture2D>& subtexture, float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f));
		// Stats
		// struct Statistics
		// {
		// 	uint32_t DrawCalls = 0;
		// 	uint32_t QuadCount = 0;
		// 
		// 	uint32_t GetTotalVertexCount() const { return QuadCount * 4; }
		// 	uint32_t GetTotalIndexCount() const { return QuadCount * 6; }
		// };
		// static void ResetStats();
		// static Statistics GetStats();
	private:
		static void FlushAndReset();
	};

}