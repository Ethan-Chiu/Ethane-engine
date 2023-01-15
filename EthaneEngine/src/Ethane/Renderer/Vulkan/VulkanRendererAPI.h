#pragma once

#include "Ethane/Renderer/Mesh.h"
// #include "Ethane/Renderer/RendererAPI.h"

// #include "VulkanMaterial.h"
#include "VulkanUniformBufferSet.h"

#include "vulkan/vulkan.h"

#include "VulkanRenderCommandBuffer.h"

#include "VulkanMaterial.h"


namespace Ethane {

	class VulkanRendererAPI : public RendererAPI
	{
	public:
		struct VulkanRendererData
		{
			// RendererCapabilities RenderCaps;
			// Ref<Texture2D> BRDFLut;

			Ref<VertexBuffer> QuadVertexBuffer;
			Ref<IndexBuffer> QuadIndexBuffer;
			VulkanShader::DescriptorSetsAndPool QuadDescriptorSet;

			// std::unordered_map<SceneRenderer*, std::vector<VulkanShader::DescriptorSetsAndPool>> RendererDescriptorSet;
			VkDescriptorSet ActiveRendererDescriptorSet = nullptr;
			std::vector<VkDescriptorPool> DescriptorPools;

			// TODO: stats
			std::vector<uint32_t> DescriptorPoolAllocationCount;

			// Default samplers
			VkSampler SamplerClamp = nullptr;

			std::set<Ref<VulkanMaterial>> UpdatedMaterial = {};
			// int32_t SelectedDrawCall = -1;
			// int32_t DrawCallCount = 0;
		};
	public:
		VulkanRendererAPI() = default;
		~VulkanRendererAPI() = default;

		virtual void Init() override;

		void BeginFrame() override;
		void EndFrame() override {};

		static void BeginRenderCommandBuffer(Ref<RenderCommandBuffer> renderCommandBuffer);
		static void EndRenderCommandBuffer();

		static void BeginRenderPass(const Ref<RenderPass>& renderPass, bool explicitClear = false); // Ref<RenderCommandBuffer> renderCommandBuffer, 
		static void EndRenderPass(); // Ref<RenderCommandBuffer> renderCommandBuffer
	private:
		static void UpdateMaterialForRendering(Ref<VulkanMaterial> material);
		static void CmdBindMaterial(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, Ref<VulkanMaterial> material, uint32_t frameIndex);

	public:
		// TODO:
		void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override {};
		void SetClearColor(const glm::vec4& color) override {};
		void Clear() override {};

		static VkDescriptorSet AllocateDescriptorSet(VkDescriptorSetAllocateInfo& allocInfo);

		// Draw
		virtual void DrawIndexed(uint32_t indexCount = 0) override {}; // TODO: remove
		virtual void DrawMesh(Ref<Mesh> mesh, const glm::mat4& transform = glm::mat4(1.0f)) override {}; // TODO: remove

		static void DrawQuad(Ref<Pipeline> pipeline, Ref<Material> material, const glm::mat4& transform) ; // Ref<StorageBufferSet> storageBufferSet

		static void DrawFullscreenQuad(Ref<Pipeline> pipeline, Ref<Material> material); // Ref<RenderCommandBuffer> renderCommandBuffer,
		// void SubmitFullscreenQuad(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Material> material) override;

		static void DrawMesh(Ref<Pipeline> pipeline, Ref<Mesh> mesh, Ref<Material> material, const glm::mat4& transform); // , Ref<MaterialTable> materialTable
		// virtual void DrawMesh(Ref<Mesh> mesh, const glm::mat4& transform = glm::mat4(1.0f)) override;

		static void DrawGeometry(Ref<Pipeline> pipeline, Ref<VertexBuffer> vertexbuffer, Ref<IndexBuffer> indexbuffer, Ref<Material> material, const glm::mat4& transform = glm::mat4(1.0f), uint32_t indexCount = 0);

		// Update uniform buffer value
		static void SetUniformBuffer(uint32_t binding, uint32_t set, const void* data, uint32_t size, uint32_t offset = 0);
	private:
		inline static Ref<RenderCommandBuffer> s_RenderCommandBuffer;

		static VulkanRendererData* s_Data;
	};

}