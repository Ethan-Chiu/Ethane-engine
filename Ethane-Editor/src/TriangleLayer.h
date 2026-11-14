#pragma once

#include "Ethane.h"
#include "Ethane/GfxBackend/GfxObject.h"

namespace Ethane {

	// Draws one colored triangle straight into the window: the smallest end-to-end use of the
	// graphics backend (vertex/index buffers, shader, pipeline, command list)
	class TriangleLayer : public Layer
	{
	public:
		TriangleLayer() : Layer("TriangleLayer") {}

		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnSubmitRender(RenderWork& work) override;

	private:
		RefCountPtr<IVertexBuffer> m_VertexBuffer;
		RefCountPtr<IIndexBuffer> m_IndexBuffer;
		RefCountPtr<IPipeline> m_Pipeline;
	};

}
