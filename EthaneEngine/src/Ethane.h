#pragma once

//For use of Ethane Application
#include "Ethane/Core/Application.h"
//---------Log--------
#include "Ethane/Core/Log.h"
//--------------------
#include "Ethane/Core/Layer.h"
#include "Ethane/ImGui/ImGuiLayer.h"
//---------input-------
#include "Ethane/Core/Input.h"
#include "Ethane/Core/KeyCodes.h"
#include "Ethane/Core/MouseCodes.h"
//-----renderer-------
#include "Ethane/Renderer/Renderer.h"
#include "Ethane/Renderer/Renderer2D.h"
#include "Ethane/Renderer/RenderCommand.h"

#include "Ethane/Renderer/Buffer.h"
#include "Ethane/Renderer/Shader.h"
#include "Ethane/Renderer/Framebuffer.h"
#include "Ethane/Renderer/Texture.h"
#include "Ethane/Renderer/SubTexture2D.h"
#include "Ethane/Renderer/VertexArray.h"
//-------Scene--------
#include "Ethane/Scene/Scene.h"
#include "Ethane/Scene/Entity.h"
#include "Ethane/Scene/ScriptableEntity.h"
#include "Ethane/Scene/Components.h"
//-----cameras--------
#include "Ethane/Renderer/OrthographicCamera.h"
#include "Ethane/Renderer/OrthographicCameraController.h"
//-----Time Step-----
#include "Ethane/Core/Timestep.h"
//--------------------