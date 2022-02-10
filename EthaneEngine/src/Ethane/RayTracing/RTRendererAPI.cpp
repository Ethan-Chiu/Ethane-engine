#include "ethpch.h"
#include "RTRendererAPI.h"

#include "RTRenderer.h"

namespace Ethane {

	Ref<RTRendererAPI> RTRendererAPI::Create()
	{
		return CreateRef<RTRenderer>();
	}

}