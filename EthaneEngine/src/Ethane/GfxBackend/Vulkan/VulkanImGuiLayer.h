#pragma once

#include "Ethane/ImGui/ImGuiLayer.h"
#include "Vulkan.h"

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
        virtual ImTextureID GetTextureID(const IRenderTarget& target) override;

    private:
        // Releases the descriptor sets of render target images the last UI frame didn't show (all: every one)
        void ReleaseUnusedTextures(bool all);

    private:
        const VulkanContext& m_Context;
        VulkanWindowTarget& m_WindowTarget;
        GLFWwindow* m_Window;

        // Descriptor sets of render target color images, by VulkanRenderTarget::GetImageId()
        struct TextureEntry
        {
            VkDescriptorSet Set = VK_NULL_HANDLE;
            uint64_t LastUsedFrame = 0;
        };
        std::unordered_map<uint64_t, TextureEntry> m_Textures;
        uint64_t m_FrameCounter = 0; // UI frames built (Begin() calls)
    };

}
