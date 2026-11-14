#include "ethpch.h"
#include "VulkanImGuiLayer.h"
#include "Ethane/Renderer/RenderWork.h"

#include "VulkanGfxBackendAPI.h"
#include "VulkanWindowTarget.h"
#include "VulkanCommandBuffer.h"
#include "VulkanRenderTarget.h"

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

namespace Ethane {

    namespace {
        void CheckVkResult(VkResult result)
        {
            if (result != VK_SUCCESS) {
                ETH_CORE_ERROR("ImGui Vulkan backend: VkResult {0}", static_cast<int>(result));
            }
        }
    }

    VulkanImGuiLayer::VulkanImGuiLayer(GfxBackendAPI& backend, IWindowTarget& windowTarget, void* nativeWindow)
        : m_Context(static_cast<VulkanGfxBackendAPI&>(backend).GetContext())
        , m_WindowTarget(static_cast<VulkanWindowTarget&>(windowTarget))
        , m_Window(static_cast<GLFWwindow*>(nativeWindow))
    {
    }

    void VulkanImGuiLayer::OnAttach()
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        // Multi-viewport (ImGui windows dragged out into their own OS windows) stays off for now: each one
        // would get a swapchain created by the backend, outside the engine's window target.

        // Fonts are rasterized at the window's framebuffer scale (Retina) by ImGui itself
        io.Fonts->AddFontFromFileTTF("assets/fonts/opensans/OpenSans-Bold.ttf", 17.0f);
        io.FontDefault = io.Fonts->AddFontFromFileTTF("assets/fonts/opensans/OpenSans-Regular.ttf", 17.0f);
        if (!io.FontDefault) {
            ETH_CORE_WARN("ImGui: OpenSans not found under assets/fonts, using the built-in font");
        }

        ImGui::StyleColorsDark();
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowRounding = 0.0f;
        SetDarkThemeColors();

        // Chains to the engine's GLFW callbacks, which were installed when the window was created
        ImGui_ImplGlfw_InitForVulkan(m_Window, true);

        const VulkanDevice& device = m_Context.GetDevice();
        ImGui_ImplVulkan_InitInfo info{};
        info.ApiVersion = m_Context.GetApiVersion();
        info.Instance = *m_Context.GetInstance();
        info.PhysicalDevice = *m_Context.GetPhysicalDevice().Get();
        info.Device = *device.Get();
        info.QueueFamily = m_Context.GetPhysicalDevice().GetQueueFamilyIndices().Graphics.value();
        info.Queue = *device.GetGraphicsQueue();
        info.DescriptorPoolSize = 64; // backend-owned pool: font atlas + textures shown with ImGui::Image
        // Copies of the per-frame vertex / index buffers. A copy is rewritten once every ImageCount frames,
        // which must be more than the frames the GPU can still be reading: kMaxFramesInFlight + 1 is safe.
        info.MinImageCount = 2;
        info.ImageCount = VulkanDevice::kMaxFramesInFlight + 1;
        // Same render pass as the layers draw in (color + depth, one subpass); the backend's pipeline
        // ignores depth. NOTE: the window recreates its render pass only if the swapchain format changes.
        info.PipelineInfoMain.RenderPass = m_WindowTarget.GetRenderPassHandle();
        info.PipelineInfoMain.Subpass = 0;
        info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        info.CheckVkResultFn = CheckVkResult;
        ImGui_ImplVulkan_Init(&info);
    }

    void VulkanImGuiLayer::OnDetach()
    {
        ReleaseUnusedTextures(true);
        // The descriptor sets are freed through the deletion queue: run it before the backend's pool goes
        m_Context.GetDevice().WaitIdle();
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    void VulkanImGuiLayer::Begin()
    {
        m_FrameCounter++;
        ReleaseUnusedTextures(false);
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    struct VulkanImGuiLayer::DrawDataCopy
    {
        ImDrawData Data;
        ~DrawDataCopy()
        {
            for (ImDrawList* list : Data.CmdLists) {
                IM_DELETE(list);
            }
        }
    };

    void VulkanImGuiLayer::End()
    {
        ImGui::Render();
        ImDrawData* drawData = ImGui::GetDrawData();
        if (!drawData) {
            m_DrawData.reset();
            return;
        }

        // Texture updates (font glyphs, new / destroyed textures) change ImGui's own texture records, so they
        // happen here, on the UI thread, not when recording. They submit to the graphics queue.
        if (drawData->Textures) {
            for (ImTextureData* texture : *drawData->Textures) {
                if (texture->Status != ImTextureStatus_OK) {
                    std::lock_guard pool(*m_DescriptorPoolMutex);
                    std::unique_lock queue = m_Context.GetDevice().LockQueue();
                    ImGui_ImplVulkan_UpdateTexture(texture);
                }
            }
        }

        // A copy that owns its draw lists: recording draws it, whenever and wherever that happens
        auto copy = std::make_shared<DrawDataCopy>();
        copy->Data = *drawData;
        for (ImDrawList*& list : copy->Data.CmdLists) {
            list = list->CloneOutput();
        }
        copy->Data.Textures = nullptr; // updated above
        m_DrawData = std::move(copy);
    }

    void VulkanImGuiLayer::SubmitRender(RenderWork& work)
    {
        if (!m_DrawData) {
            return;
        }
        work.Window([drawData = std::move(m_DrawData)](ICommandList& commandList) {
            // Sets its own pipeline, viewport and scissor
            ImGui_ImplVulkan_RenderDrawData(&drawData->Data, static_cast<VulkanCommandBuffer&>(commandList).GetHandle());
        });
    }

    ImTextureID VulkanImGuiLayer::GetTextureID(const IRenderTarget& target)
    {
        const auto& vulkanTarget = static_cast<const VulkanRenderTarget&>(target);
        auto [it, inserted] = m_Textures.try_emplace(vulkanTarget.GetImageId());
        if (inserted) {
            std::lock_guard pool(*m_DescriptorPoolMutex);
            it->second.Set = ImGui_ImplVulkan_AddTexture(vulkanTarget.GetColorView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }
        it->second.LastUsedFrame = m_FrameCounter;
        return reinterpret_cast<ImTextureID>(it->second.Set);
    }

    void VulkanImGuiLayer::ReleaseUnusedTextures(bool all)
    {
        // Sets the last UI frame didn't use (e.g. of an image replaced by a resize). Frames in flight may
        // still draw with them, so they're freed through the device's deletion queue.
        for (auto it = m_Textures.begin(); it != m_Textures.end();) {
            if (all || it->second.LastUsedFrame + 1 < m_FrameCounter) {
                const VkDescriptorSet set = it->second.Set;
                m_Context.GetDevice().Retire(VulkanDeletionQueue::Call([set, mutex = m_DescriptorPoolMutex] {
                    std::lock_guard pool(*mutex);
                    ImGui_ImplVulkan_RemoveTexture(set);
                }));
                it = m_Textures.erase(it);
            } else {
                ++it;
            }
        }
    }

}
