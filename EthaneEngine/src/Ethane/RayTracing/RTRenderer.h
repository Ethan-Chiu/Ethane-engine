#pragma once

#include "RTRendererAPI.h"
#include "RayTracing.h"

namespace Ethane {

	class RTRenderer: public RTRendererAPI {
	public:
		virtual ~RTRenderer() = default;

		virtual void Init() override;

		virtual void BeginFrame() override;
		virtual void EndFrame() override;

		static Ref<RTRendererAPI> Create();

	private:
		void CreateOffscreenRender();

	private:
		RayTracing m_RT;
	};

}