//
//  GfxSmokeTest.cpp
//  EthaneEngine
//
#include "ethpch.h"
#include "GfxSmokeTest.h"
#include "GfxBackendAPI.h"
#include "Ethane/Core/Log.h"

#include <GLFW/glfw3.h>

#include <cstdlib>
#include <cstring>

namespace Ethane::GfxSmokeTest {

static const char* Mode()
{
#ifdef ETH_DEBUG
    const char* value = std::getenv("ETH_GFX_SMOKE_TEST");
    return value != nullptr ? value : "";
#else
    return "";
#endif
}

bool IsEnabled()
{
    return std::strcmp(Mode(), "1") == 0 || ExitAfterRun();
}

bool ExitAfterRun()
{
    return std::strcmp(Mode(), "exit") == 0;
}

void Run(GfxBackendAPI& backend)
{
    ETH_CORE_INFO("[GfxSmokeTest] start");

    // Matches the vertex input of assets/shaders/test.glsl
    struct Vertex { float Position[3]; float Color[3]; float TexCoord[2]; };
    const Vertex vertices[] = {
        { {-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f} },
        { { 0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f} },
        { { 0.5f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f} },
        { {-0.5f,  0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f} },
    };
    const uint32_t indices[] = { 0, 1, 2, 2, 3, 0 };

    {
        // Buffers: device-local, uploaded through a staging buffer + single-use command buffer
        RefCountPtr<IVertexBuffer> vertexBuffer = backend.CreateVertexBuffer(sizeof(vertices));
        vertexBuffer->SetData(vertices, sizeof(vertices));
        ETH_CORE_INFO("[GfxSmokeTest] vertex buffer: {0} bytes uploaded", sizeof(vertices));

        RefCountPtr<IIndexBuffer> indexBuffer = backend.CreateIndexBuffer(sizeof(indices));
        indexBuffer->SetData(indices, sizeof(indices));
        ETH_CORE_INFO("[GfxSmokeTest] index buffer: {0} indices uploaded", indexBuffer->GetCount());

        // Partial upload at an offset into the destination buffer (second triangle only)
        indexBuffer->SetData(indices + 3, 3 * sizeof(uint32_t), 3 * sizeof(uint32_t));
        ETH_CORE_INFO("[GfxSmokeTest] index buffer: partial upload at offset done");

        // Shader: GLSL -> SPIR-V, reflection, shader modules, descriptor set layouts
        RefCountPtr<IShader> shader = backend.CreateShader("assets/shaders/test.glsl");
        ETH_CORE_INFO("[GfxSmokeTest] shader: {0}", shader->GetName());

        // Render pass: one color attachment + depth (formats supported on Apple GPUs)
        RefCountPtr<IRenderPass> renderPass = backend.CreateRenderPass({ ImageFormat::BGRA }, ImageFormat::DEPTH32F, true);
        ETH_CORE_INFO("[GfxSmokeTest] render pass created");

        // Pipeline: layout from shader reflection + fixed-function state
        PipelineSpecification spec;
        spec.Shader = shader;
        spec.RenderPass = renderPass;
        spec.Layout = {
            { ShaderDataType::Float3, "a_Position" },
            { ShaderDataType::Float3, "a_Color" },
            { ShaderDataType::Float2, "a_TexCoord" },
        };
        spec.CullMode = CullMode::BACK;
        RefCountPtr<IPipeline> pipeline = backend.CreatePipeline(spec);
        ETH_CORE_INFO("[GfxSmokeTest] pipeline created");

        // Everything is released here, in reverse order of creation
    }

    ETH_CORE_INFO("[GfxSmokeTest] PASSED (all resources released)");
}

bool TickFrame(void* glfwWindow)
{
    constexpr int kResizeAtFrame = 5;
    constexpr int kCloseAtFrame  = 30;
    static int s_Frame = 0;

    s_Frame++;
    if (s_Frame == kResizeAtFrame) {
        GLFWwindow* window = static_cast<GLFWwindow*>(glfwWindow);
        int width = 0, height = 0;
        glfwGetWindowSize(window, &width, &height);
        // the framebuffer-size callback fires on a later poll and goes through Application::OnWindowResize
        glfwSetWindowSize(window, width + 120, height + 80);
        ETH_CORE_INFO("[GfxSmokeTest] resizing window {0}x{1} -> {2}x{3}", width, height, width + 120, height + 80);
    }
    if (s_Frame == kCloseAtFrame) {
        ETH_CORE_INFO("[GfxSmokeTest] {0} frames run, closing", s_Frame);
        return true;
    }
    return false;
}

}
