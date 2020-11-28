#include "ethpch.h"
#include "RenderCommand.h"

#include "Platform/OpenGL/OpenGLRendererAPI.h"

namespace Ethane {

	RendererAPI* RenderCommand::s_RendererAPI = new OpenGLRendererAPI;

}