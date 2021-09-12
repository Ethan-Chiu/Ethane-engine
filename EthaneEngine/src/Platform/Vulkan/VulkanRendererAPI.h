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
		VulkanRendererAPI() = default;
		~VulkanRendererAPI() = default;

		virtual void Init() override;

		void BeginFrame() {};
		void EndFrame() {};

		static void BeginRenderCommandBuffer(Ref<RenderCommandBuffer> renderCommandBuffer);
		static void EndRenderCommandBuffer();

		static void BeginRenderPass(const Ref<RenderPass>& renderPass, bool explicitClear = false); // Ref<RenderCommandBuffer> renderCommandBuffer, 
		static void EndRenderPass(); // Ref<RenderCommandBuffer> renderCommandBuffer

		static void UpdateMaterialForRendering(Ref<VulkanMaterial> material);

		// TODO:
		void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override {};
		void SetClearColor(const glm::vec4& color) override {};
		void Clear() override {};

		static VkDescriptorSet AllocateDescriptorSet(VkDescriptorSetAllocateInfo& allocInfo);

		virtual void DrawIndexed(uint32_t indexCount = 0) override {};
		virtual void DrawMesh(Ref<Mesh> mesh, const glm::mat4& transform = glm::mat4(1.0f)) override {};

		static void DrawQuad(Ref<Pipeline> pipeline, Ref<Material> material, const glm::mat4& transform) ; // Ref<RenderCommandBuffer> renderCommandBuffer, Ref<StorageBufferSet> storageBufferSet
		
		static void SubmitFullscreenQuad(Ref<Pipeline> pipeline, Ref<Material> material); // Ref<RenderCommandBuffer> renderCommandBuffer,
		// void SubmitFullscreenQuad(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<UniformBufferSet> uniformBufferSet, Ref<StorageBufferSet> storageBufferSet, Ref<Material> material) override;

		static void DrawMesh(Ref<Pipeline> pipeline, Ref<Mesh> mesh, Ref<Material> material, const glm::mat4& transform); // , Ref<MaterialTable> materialTable
		// virtual void DrawMesh(Ref<Mesh> mesh, const glm::mat4& transform = glm::mat4(1.0f)) override;

		// Update uniform buffer value
		static void SetUniform(uint32_t binding, uint32_t set, const void* data, uint32_t size, uint32_t offset = 0);
	private:
		inline static Ref<VulkanUniformBufferSet> s_UniformBufferSet;
		inline static Ref<RenderCommandBuffer> s_RenderCommandBuffer;


		// temp
		inline static bool tempfirst = true;
	};

}