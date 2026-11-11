#pragma once

#include "Ethane/ImGui/ImGuiLayer.h"

struct GLFWwindow;

namespace Ethane {

    class VulkanContext;
    class VulkanWindowTarget;

    // ImGuiLayer through the GLFW + Vulkan backends of Dear ImGui. The UI is drawn into the window's own
    // render pass; the backend keeps its own pipeline, buffers, font texture and descriptor pool.
    class VulkanImGuiLayer : public ImGuiLayer
    {
    public:
        // NOTE: `windowTarget` must outlive the layer (Application detaches layers before destroying it)
        VulkanImGuiLayer(GfxBackendAPI& backend, IWindowTarget& windowTarget, void* nativeWindow);

        virtual void OnAttach() override;
        virtual void OnDetach() override;

        virtual void Begin() override;
        virtual void End() override;
        virtual void Render(ICommandList& commandList) override;

    private:
        const VulkanContext& m_Context;
        VulkanWindowTarget& m_WindowTarget;
        GLFWwindow* m_Window;
    };

}
