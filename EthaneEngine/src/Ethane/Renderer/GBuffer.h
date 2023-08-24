#pragma once

#include "Image.h"
#include "RenderTarget.h"
#include "Pipeline.h"

namespace Ethane {

	class SceneRenderer;
	class GBuffer 
	{
	public:
		GBuffer();

		const RenderTarget* GetOffscreenTarget() const { return m_OffscreenTarget.get(); }
		Ref<Pipeline> GetOffscreenPipeline() const { return m_OffscreenPipeline; }

	private:
		void CreateImages();

	protected:
		Ref<Image2D> m_ImagePosition;
		Ref<Image2D> m_ImageNormal;
		Ref<Image2D> m_ImageAlbedo;
		Ref<Image2D> m_Depth;

		Scope<RenderTarget> m_OffscreenTarget = nullptr;

		Ref<Pipeline> m_OffscreenPipeline;
	
		friend SceneRenderer;
	};

}