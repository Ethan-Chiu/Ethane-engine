#pragma once

#include "Ethane/Core/Base.h"

namespace Ethane {

	class RTRendererAPI
	{
	public:
		virtual ~RTRendererAPI() = default;

		virtual void Init() = 0;

		virtual void BeginFrame() = 0;
		virtual void EndFrame() = 0;
		
		static Ref<RTRendererAPI> Create();
	};

}