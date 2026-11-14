#pragma once

#include "Ethane.h"
#include "Ethane/GfxBackend/GfxObject.h"

namespace Ethane {

	// A spinning textured quad drawn with assets/shaders/test.glsl: exercises uniform buffers,
	// textures and binding sets (descriptor set 0 = { viewproj uniform, u_Texture })
	class TexturedQuadLayer : public Layer
	{
	public:
		TexturedQuadLayer() : Layer("TexturedQuadLayer") {}

		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnUpdate(Timestep ts) override;
		virtual void OnSubmitRender(RenderWork& work) override;

	private:
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
