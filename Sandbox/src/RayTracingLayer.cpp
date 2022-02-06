#include <Ethane.h>

#include "RayTracingLayer.h"

#include "imgui/imgui.h"

using namespace Ethane;

RayTracingLayer::RayTracingLayer()
	:Layer("RayTracing")
{

}

void RayTracingLayer::OnAttach()
{
	// Ref<Mesh> mesh = AssetManager::GetAssetMesh("resources/meshes/default/Cube.fbx");

	// m_RT.LoadMesh(mesh);
	// #VKRay
	// m_RT.InitRayTracing();
	// m_RT.CreateBottomLevelAS();
	// m_RT.CreateTopLevelAS();
}

void RayTracingLayer::OnDetach()
{

}

void RayTracingLayer::OnUpdate(Ethane::Timestep ts)
{

}

void RayTracingLayer::OnImGuiRender()
{
	ImGui::Text("test");
}

void RayTracingLayer::OnEvent(Ethane::Event& e)
{

}
