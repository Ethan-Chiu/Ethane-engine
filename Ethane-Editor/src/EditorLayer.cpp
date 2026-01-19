#include "EditorLayer.h"
#include "imgui.h"

#include "Ethane/Utils/PlatformUtils.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Ethane/Scene/Components.h"

#include "Ethane/GfxBackend/Resource.h"

namespace Ethane {

	EditorLayer::EditorLayer()
		:Layer("EditorLayer")
	{
	}

	void EditorLayer::OnAttach()
    {
        // ReportMemResource report_new{"NewDelete", NewDeleteMemResource()};
		ReportMemResource report_new{"NewDelete", NewDeleteMemResource()};
		LinearMemResource linear_new{&report_new};
        PMRAdapter alloc(linear_new);
        std::pmr::set_default_resource(&alloc);
        
        std::pmr::vector<int> testAlloc;
        for (uint32_t i = 0; i < 50; i++) {
            testAlloc.push_back(i);
        }
//        m_ActiveScene = CreateRef<Scene>();
//        m_ViewportRenderer = CreateRef<SceneRenderer>(m_ActiveScene);
//        
//        auto& rendererConfig = Renderer::GetRendererConfig();
//        m_ViewportRenderer->SetViewportSize(rendererConfig.DefaultWindowWidth, rendererConfig.DefaultWindowHeight);
//        m_ViewportWidth = rendererConfig.DefaultWindowWidth;
//        m_ViewportHeight = rendererConfig.DefaultWindowHeight;
//        
//        m_EditorCamera = EditorCamera(30.0f, 1.778f, 0.1f, 1000.0f);
//        m_EditorCamera.SetViewportSize(Renderer::GetRendererConfig().DefaultWindowWidth, Renderer::GetRendererConfig().DefaultWindowHeight);
//
//        auto newEntity = m_ActiveScene->CreateEntity("Cube");
//        m_Mesh = AssetManager::GetAssetMesh("res/meshes/default/Cube.fbx");
//        m_Mesh->Upload();
//        m_Mat = Material::Create(ShaderSystem::Get("test3D").get());
//        newEntity.AddComponent<MeshComponent>(m_Mesh, m_Mat);
        
//        RefCountPtr t1();   // ref count = 1
//        {
//            RefCountPtr<Texture> t2 = t1;          // ref count = 2
//            std::cout << "Inside scope, count = " << t1->use_count() << "\n";
//        }
//        // t2 is destroyed, calling release(); count = 1
//        std::cout << "After scope, count = " << t1->use_count() << "\n";
//        t1 = nullptr;
	}

	void EditorLayer::OnDetach()
	{
//        m_ViewportRenderer->Shutdown();
	}

	void EditorLayer::OnUpdate(Timestep ts)
	{
//        if (m_NeedResize)
//        {
//        }
//        
//        m_EditorCamera.OnUpdate(ts);
//        
//        m_ActiveScene->OnUpdateEditor(m_ViewportRenderer, ts, m_EditorCamera);
	}

	void EditorLayer::OnEvent(Event& e)
	{
//        m_EditorCamera.OnEvent(e);
//        
//        EventDispatcher dispatcher(e);
//        dispatcher.Dispatch<WindowResizeEvent>(ETH_BIND_EVENT_FN(EditorLayer::OnResize));
//        dispatcher.Dispatch<KeyPressedEvent>(ETH_BIND_EVENT_FN(EditorLayer::OnKeyPressed));
	}

    

	void EditorLayer::OnImGuiRender()
	{
//        ImGui::Begin("Control Panel");
//        ImGui::Text("HIHIHI");
//        ImGui::End();
	}

    bool EditorLayer::OnResize(WindowResizeEvent& e)
    {
//        ETH_CORE_TRACE("editor layer {0} {1}", e.GetWidth(), e.GetHeight());
//        m_ViewportRenderer->SetViewportSize(e.GetWidth(), e.GetHeight());
//        m_EditorCamera.SetViewportSize(e.GetWidth(), e.GetHeight());
//        m_ViewportWidth = e.GetWidth();
//        m_ViewportHeight = e.GetHeight();
//        
//        m_NeedResize = true;
//        return false;
    }

    bool EditorLayer::OnKeyPressed(KeyPressedEvent& e)
    {
        //Shortcuts
//        if(e.GetRepeatCount() > 0)
//            return false;
//    
//        bool control = Input::IsKeyPressed(Key::LeftControl) || Input::IsKeyPressed(Key::RightControl);
//        bool shift = Input::IsKeyPressed(Key::LeftShift) || Input::IsKeyPressed(Key::RightShift);
//        switch (e.GetKeyCode())
//        {
//            default:
//                break;
//        }
//        return false;
    }

}
