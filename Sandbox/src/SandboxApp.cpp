#include <Ethane.h>
//-----EntryPoint-----
#include <Ethane/Core/EntryPoint.h>


// #include "Platform/vulkan/VulkanPipeline.h" // TODO: remove this
// #include "Platform/vulkan/VulkanVertexBuffer.h" // TODO: remove this
// #include "Platform/vulkan/VulkanIndexBuffer.h" // TODO: remove this
// #include "Platform/vulkan/VulkanUniformBuffer.h" // TODO: remove this
// #include "Platform/vulkan/VulkanTexture.h" // TODO: remove this
// #include "Platform/vulkan/VulkanShader.h" // TODO: remove this
// #include "Platform/vulkan/VulkanRenderPass.h"// TODO: remove this
// // TODO: remove
// #include "imgui.h"
// #include "examples/imgui_impl_vulkan.h"

using namespace Ethane;

class ExampleLayer : public Ethane::Layer
{
private:
    

public:
	ExampleLayer()
		:Layer("Example")
	{

	}

	virtual ~ExampleLayer()
	{

	}

	virtual void OnAttach() override
	{
        // // TODO: remove this test 
        // VertexBufferLayout layout = {
        //         { ShaderDataType::Float3, "a_Position" },
        //         { ShaderDataType::Float3, "a_Color" },
        //         { ShaderDataType::Float2, "a_TexCoord" }
        // };
        // Ref<VulkanShader> vulkanShader = CreateRef<VulkanShader>("assets/shaders/test.glsl");
        // 
        // 
        // RenderPassSpecification renderPassSpec{};
        // Ref<VulkanRenderPass> renderPass = CreateRef<VulkanRenderPass>(renderPassSpec, m_RenderPass);
        // PipelineSpecification pipelineSpec{ vulkanShader, renderPass, layout };
        // if (m_Pipeline == nullptr)
        //     m_Pipeline = CreateRef<VulkanPipeline>(pipelineSpec);
        // 
        // 
        // VulkanShader::DescriptorSetsAndPool descriptorSets = vulkanShader->CreateDescriptorSets(0, MAX_FRAMES_IN_FLIGHT);
        // 
        // // Creaet Texture
        // m_Texture2D = CreateRef<VulkanTexture2D>("assets/textures/test.png");
        // 
        // // TODO: move
        // // create uniform buffer & update descriptor sets
        // m_UniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        // for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        // {
        //     std::vector<VkWriteDescriptorSet> writeDescriptors{};
        // 
        //     // Uniform buffer
        //     m_UniformBuffers[i] = CreateRef<VulkanUniformBuffer>(sizeof(glm::mat4), 0);
        //     VkWriteDescriptorSet& uboWriteDescriptor = writeDescriptors.emplace_back();
        //     uboWriteDescriptor = *vulkanShader->GetWriteDescriptorSet(0, "UniformBufferObject");
        //     uboWriteDescriptor.dstSet = descriptorSets.DescriptorSets[i];
        //     uboWriteDescriptor.dstArrayElement = 0;
        //     uboWriteDescriptor.pBufferInfo = &m_UniformBuffers[i]->GetDescriptorBufferInfo();
        // 
        //     // Image Sampler
        //     VkWriteDescriptorSet& samplerWriteDescriptor = writeDescriptors.emplace_back();
        //     samplerWriteDescriptor = *vulkanShader->GetWriteDescriptorSet(0, "u_Texture");
        //     samplerWriteDescriptor.dstSet = descriptorSets.DescriptorSets[i];
        //     samplerWriteDescriptor.dstArrayElement = 0;
        //     samplerWriteDescriptor.pImageInfo = &m_Texture2D->GetDescriptorImageInfo();
        //     vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptors.size()), writeDescriptors.data(), 0, nullptr);
        // }
        // 
        // 
        // struct Vertex {
        //     glm::vec3 pos;
        //     glm::vec3 color;
        //     glm::vec2 texCoord;
        // };
        // const std::vector<Vertex> vertices = {
        //     {{-0.5f, -0.5f,  0.0}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        //     {{ 0.5f, -0.5f,  0.0}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
        //     {{ 0.5f,  0.5f,  0.0}, {0.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
        //     {{-0.5f,  0.5f,  0.0}, {0.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
        // 
        //     {{-0.5f, -0.5f, -0.5}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        //     {{ 0.5f, -0.5f, -0.5}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
        //     {{ 0.5f,  0.5f, -0.5}, {0.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
        //     {{-0.5f,  0.5f, -0.5}, {0.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
        //     // {0.0f, -0.5f},
        //     // {0.5f, 0.5f },
        //     // {-0.5f, 0.5f},
        // };
        // if (m_VertexBuffer == nullptr)
        //     m_VertexBuffer = CreateRef<VulkanVertexBuffer>((void*)vertices.data(), sizeof(vertices[0]) * vertices.size());
        // 
        // const std::vector<uint32_t> indices = {
        //     0, 1, 2, 2, 3, 0,
        //     4, 5, 6, 6, 7, 4
        // };
        // if (m_IndexBuffer == nullptr)
        //     m_IndexBuffer = CreateRef<VulkanIndexBuffer>((void*)indices.data(), sizeof(indices[0]) * indices.size());
        // 
        // for (size_t i = 0; i < m_CommandBuffers.size(); i++) {
        //     VkCommandBufferBeginInfo beginInfo{};
        //     beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        //     beginInfo.flags = 0; // Optional
        //     beginInfo.pInheritanceInfo = nullptr; // Optional
        // 
        //     VK_CHECK_RESULT(vkBeginCommandBuffer(m_CommandBuffers[i], &beginInfo));
        // 
        //     std::array<VkClearValue, 2> clearValues{};
        //     clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
        //     clearValues[1].depthStencil = { 1.0f, 0 };
        // 
        //     VkRenderPassBeginInfo renderPassInfo{};
        //     renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        //     renderPassInfo.renderPass = m_RenderPass;
        //     renderPassInfo.framebuffer = m_Framebuffers[i];
        //     renderPassInfo.renderArea.offset = { 0, 0 };
        //     renderPassInfo.renderArea.extent = m_Extent;
        //     renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());;
        //     renderPassInfo.pClearValues = clearValues.data();
        //     vkCmdBeginRenderPass(m_CommandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        // 
        //     VkViewport viewport{};
        //     viewport.x = 0.0f;
        //     viewport.y = 0.0f;
        //     viewport.width = (float)m_Extent.width;
        //     viewport.height = (float)m_Extent.height;
        //     viewport.minDepth = 0.0f;
        //     viewport.maxDepth = 1.0f;
        //     vkCmdSetViewport(m_CommandBuffers[i], 0, 1, &viewport);
        // 
        //     VkRect2D scissor{};
        //     scissor.offset = { 0, 0 };
        //     scissor.extent = m_Extent;
        //     vkCmdSetScissor(m_CommandBuffers[i], 0, 1, &scissor);
        // 
        //     vkCmdBindPipeline(m_CommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline->GetVulkanPipeline());
        // 
        //     VkBuffer vertexbuffer = m_VertexBuffer->GetVulkanBuffer();
        //     VkDeviceSize offsets[] = { 0 };
        //     vkCmdBindVertexBuffers(m_CommandBuffers[i], 0, 1, &vertexbuffer, offsets);
        // 
        //     VkBuffer indexbuffer = m_IndexBuffer->GetVulkanBuffer();
        //     vkCmdBindIndexBuffer(m_CommandBuffers[i], indexbuffer, 0, VK_INDEX_TYPE_UINT32);
        // 
        //     VkDescriptorSet _descriptorSet = descriptorSets.DescriptorSets[i];
        //     vkCmdBindDescriptorSets(m_CommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline->GetPipelineLayout(), 0, 1, &_descriptorSet, 0, nullptr);
        // 
        //     vkCmdDrawIndexed(m_CommandBuffers[i], static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
        //     // vkCmdDraw(m_CommandBuffers[i], 3, 1, 0, 0);
        // 
        //     // ImGui test
        //     ImGui::Render();
        //     ImDrawData* main_draw_data = ImGui::GetDrawData();
        //     ImGui_ImplVulkan_RenderDrawData(main_draw_data, m_CommandBuffers[i]);
        // 
        //     vkCmdEndRenderPass(m_CommandBuffers[i]);
        //     if (vkEndCommandBuffer(m_CommandBuffers[i]) != VK_SUCCESS) {
        //         throw std::runtime_error("failed to record command buffer!");
        //     }
        // }
	}

	virtual void OnDetach() override
	{

	}

	void OnUpdate(Ethane::Timestep ts) override
	{

	}

	virtual void OnImGuiRender() override
	{
	}

	void OnEvent(Ethane::Event& e) override
	{
	}
private:
    // TODO: remove these
    // VkRenderPass m_RenderPass; // TODO: remove this maybe ?
    // Ref<VulkanVertexBuffer> m_VertexBuffer = nullptr;
    // Ref<VulkanIndexBuffer> m_IndexBuffer = nullptr;
    // Ref<VulkanPipeline> m_Pipeline = nullptr;
    // std::vector<Ref<VulkanUniformBuffer>> m_UniformBuffers = {};
    // Ref<VulkanTexture2D> m_Texture2D = nullptr;
};

class Sandbox : public Ethane::Application
{
public:
	Sandbox()
	{
		PushLayer(new ExampleLayer());
	}
	~Sandbox()
	{

	}
};

Ethane::Application* Ethane::CreateApplication(ApplicationCommandLineArgs args)
{
	RendererAPI::SetAPI(RendererAPI::API::Vulkan);
	return new Sandbox();
}