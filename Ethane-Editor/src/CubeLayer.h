#pragma once

#include "Ethane.h"
#include "Ethane/GfxBackend/GfxObject.h"
#include "Ethane/Renderer/EditorCamera.h"

namespace Ethane {

	// Two intersecting textured cubes seen through the EditorCamera (Alt + left drag: orbit,
	// Alt + middle drag: pan, Alt + right drag / scroll: zoom). Exercises the window's depth buffer,
	// a per-frame camera uniform buffer and per-draw push constants (assets/shaders/cube.glsl).
	class CubeLayer : public Layer
	{
	public:
		CubeLayer() : Layer("CubeLayer") {}

		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnUpdate(Timestep ts) override;
		virtual void OnEvent(Event& e) override;
		virtual void OnRender(ICommandList& commandList) override;

	private:
		EditorCamera m_Camera{ 45.0f, 1.778f, 0.1f, 100.0f };

		RefCountPtr<IVertexBuffer> m_VertexBuffer;
		RefCountPtr<IIndexBuffer> m_IndexBuffer;
		RefCountPtr<IShader> m_Shader;
		RefCountPtr<IPipeline> m_Pipeline;
		RefCountPtr<IUniformBuffer> m_CameraBuffer;
		RefCountPtr<ITexture2D> m_Texture;
		RefCountPtr<IBindingSet> m_BindingSet;

		float m_Time = 0.0f;
	};

}
