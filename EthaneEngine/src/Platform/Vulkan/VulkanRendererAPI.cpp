#include "ethpch.h"
#include "VulkanRendererAPI.h"

#include "VulkanPipeline.h"
#include "VulkanVertexBuffer.h"
#include "VulkanIndexBuffer.h"
#include "VulkanContext.h"

// #include "VulkanShader.h"
#include "VulkanTexture.h"
#include "VulkanFramebuffer.h"

namespace Ethane {

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
		std::vector<uint32_t> DescriptorPoolAllocationCount;

		// UniformBufferSet -> Shader Hash -> Frame -> WriteDescriptor
		// std::unordered_map<UniformBufferSet*, std::unordered_map<uint64_t, std::vector<std::vector<VkWriteDescriptorSet>>>> UniformBufferWriteDescriptorCache;
		// std::unordered_map<StorageBufferSet*, std::unordered_map<uint64_t, std::vector<std::vector<VkWriteDescriptorSet>>>> StorageBufferWriteDescriptorCache;

		// Default samplers
		VkSampler SamplerClamp = nullptr;

		// int32_t SelectedDrawCall = -1;
		// int32_t DrawCallCount = 0;
	};

	static VulkanRendererData* s_Data = nullptr;

	void VulkanRendererAPI::Init()
	{
		s_Data = new VulkanRendererData();
		const auto& config = Renderer::GetConfig();
		s_Data->DescriptorPools.resize(config.FramesInFlight);
		s_Data->DescriptorPoolAllocationCount.resize(config.FramesInFlight);

		// auto& caps = s_Data->RenderCaps;
		auto& properties = VulkanContext::GetDevice()->GetPhysicalDevice()->GetProperties();
		// caps.Vendor = Utils::VulkanVendorIDToString(properties.vendorID);
		// caps.Device = properties.deviceName;
		// caps.Version = std::to_string(properties.driverVersion);
		ETH_CORE_INFO("{0}, {1}, {2}", Utils::VulkanVendorIDToString(properties.vendorID), properties.deviceName, std::to_string(properties.driverVersion));

		// Utils::DumpGPUInfo();

		// Create descriptor pools
		// Renderer::Submit([]() mutable
		//	{
				// Create Descriptor Pool
				VkDescriptorPoolSize pool_sizes[] =
				{
					{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
					{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
					{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
					{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
					{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
					{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
					{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
					{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
					{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
					{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
					{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
				};
				VkDescriptorPoolCreateInfo pool_info = {};
				pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
				pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
				pool_info.maxSets = 100000;
				pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
				pool_info.pPoolSizes = pool_sizes;
				VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();
				uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;
				for (uint32_t i = 0; i < framesInFlight; i++)
				{
					VK_CHECK_RESULT(vkCreateDescriptorPool(device, &pool_info, nullptr, &s_Data->DescriptorPools[i]));
					s_Data->DescriptorPoolAllocationCount[i] = 0;
				}
		//	});

		// Create fullscreen quad
		float x = -1;
		float y = -1;
		float width = 2, height = 2;
		struct QuadVertex
		{
			glm::vec3 Position;
			glm::vec2 TexCoord;
		};

		QuadVertex* data = new QuadVertex[4];

		data[0].Position = glm::vec3(x, y, 0.0f);
		data[0].TexCoord = glm::vec2(0, 0);

		data[1].Position = glm::vec3(x + width, y, 0.0f);
		data[1].TexCoord = glm::vec2(1, 0);

		data[2].Position = glm::vec3(x + width, y + height, 0.0f);
		data[2].TexCoord = glm::vec2(1, 1);

		data[3].Position = glm::vec3(x, y + height, 0.0f);
		data[3].TexCoord = glm::vec2(0, 1);

		s_Data->QuadVertexBuffer = VertexBuffer::Create(data, 4 * sizeof(QuadVertex));
		uint32_t indices[6] = { 0, 1, 2, 2, 3, 0, };
		s_Data->QuadIndexBuffer = IndexBuffer::Create(indices, 6 * sizeof(uint32_t));

		// s_Data->BRDFLut = Renderer::GetBRDFLutTexture();

		// TODO: temp
		s_UniformBufferSet = CreateRef<VulkanUniformBufferSet>(framesInFlight);
		s_UniformBufferSet->Create(sizeof(glm::mat4), 0);
	}

	VkDescriptorSet VulkanRendererAPI::AllocateDescriptorSet(VkDescriptorSetAllocateInfo& allocInfo)
	{
		ETH_PROFILE_FUNCTION();

		uint32_t bufferIndex = VulkanContext::GetSwapChain().GetCurrentFrameIndex(); // Renderer::GetCurrentFrameIndex();
		allocInfo.descriptorPool = s_Data->DescriptorPools[bufferIndex];
		VkDevice device = VulkanContext::GetDevice()->GetVulkanDevice();
		VkDescriptorSet result;
		VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &result));
		s_Data->DescriptorPoolAllocationCount[bufferIndex] += allocInfo.descriptorSetCount;
		return result;
	}

	static const std::vector<std::vector<VkWriteDescriptorSet>>& RetrieveOrCreateUniformBufferWriteDescriptors(Ref<VulkanUniformBufferSet> uniformBufferSet, Ref<VulkanMaterial> vulkanMaterial)
	{
		ETH_PROFILE_FUNCTION();

		// size_t shaderHash = vulkanMaterial->GetShader()->GetHash();
		// if (s_Data->UniformBufferWriteDescriptorCache.find(uniformBufferSet.Raw()) != s_Data->UniformBufferWriteDescriptorCache.end())
		// {
		// 	const auto& shaderMap = s_Data->UniformBufferWriteDescriptorCache.at(uniformBufferSet.Raw());
		// 	if (shaderMap.find(shaderHash) != shaderMap.end())
		// 	{
		// 		const auto& writeDescriptors = shaderMap.at(shaderHash);
		// 		return writeDescriptors;
		// 	}
		// }

		uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;
		Ref<VulkanShader> vulkanShader = std::dynamic_pointer_cast<VulkanShader>(vulkanMaterial->GetShader());

		// TODO: currently no cache
		std::vector<std::vector<VkWriteDescriptorSet>> UniformBufferWriteDescriptorCache;

		// if (vulkanShader->HasDescriptorSet(0))
		// {
			const auto& shaderDescriptorSets = vulkanShader->GetShaderDescriptorSetData();
			if (!shaderDescriptorSets.empty())
			{
				for (auto&& [binding, shaderUB] : shaderDescriptorSets[0].UniformBuffers)
				{
					auto& writeDescriptors = UniformBufferWriteDescriptorCache;
					writeDescriptors.resize(framesInFlight);
					for (uint32_t frame = 0; frame < framesInFlight; frame++)
					{
						Ref<VulkanUniformBuffer> uniformBuffer = uniformBufferSet->Get(binding, 0, frame); // set = 0 for now

						VkWriteDescriptorSet writeDescriptorSet = {};
						writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
						writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
						writeDescriptorSet.descriptorCount = 1;
						writeDescriptorSet.dstBinding = binding;
						writeDescriptorSet.pBufferInfo = &uniformBuffer->GetDescriptorBufferInfo();
						writeDescriptors[frame].push_back(writeDescriptorSet);
					}
				}

			}
		// }
			return UniformBufferWriteDescriptorCache;
	}

	void VulkanRendererAPI::UpdateMaterialForRendering(Ref<VulkanMaterial> material)
	{
		ETH_PROFILE_FUNCTION();

		if (s_UniformBufferSet)
		{
			auto writeDescriptors = RetrieveOrCreateUniformBufferWriteDescriptors(s_UniformBufferSet, material);
			material->UpdateForRendering(writeDescriptors);
		}
		else
		{
			material->UpdateForRendering();
		}
		
	}

	void VulkanRendererAPI::BeginRenderCommandBuffer(Ref<RenderCommandBuffer> renderCommandBuffer)
	{
		s_RenderCommandBuffer = renderCommandBuffer;
		s_RenderCommandBuffer->Begin();
	}

	void VulkanRendererAPI::EndRenderCommandBuffer()
	{
		s_RenderCommandBuffer->End();
	}

	void VulkanRendererAPI::BeginRenderPass(const Ref<RenderPass>& renderPass, bool explicitClear) // Ref<RenderCommandBuffer> renderCommandBuffer, 
	{
		// Renderer::Submit([renderCommandBuffer, renderPass, explicitClear]()
		// 	{
		ETH_PROFILE_FUNCTION(fmt::format("VulkanRenderer::BeginRenderPass ({})", renderPass->GetSpecification().DebugName).c_str());

		uint32_t frameIndex = VulkanContext::GetSwapChain().GetCurrentFrameIndex(); // Renderer::GetCurrentFrameIndex();
		VkCommandBuffer commandBuffer = std::dynamic_pointer_cast<VulkanRenderCommandBuffer>(s_RenderCommandBuffer)->GetCommandBuffer(frameIndex);// VulkanContext::GetSwapChain().GetCurrentCommandBuffer();

		auto fb = renderPass->GetSpecification().TargetFramebuffer;
		Ref<VulkanFramebuffer> framebuffer = std::dynamic_pointer_cast<VulkanFramebuffer>(fb);
		const auto& fbSpec = framebuffer->GetSpecification();

		uint32_t width = framebuffer->GetWidth();
		uint32_t height = framebuffer->GetHeight();

		VkViewport viewport = {};
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRenderPassBeginInfo renderPassBeginInfo = {};
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.pNext = nullptr;
		renderPassBeginInfo.renderPass = framebuffer->GetRenderPass();
		renderPassBeginInfo.renderArea.offset.x = 0;
		renderPassBeginInfo.renderArea.offset.y = 0;
		renderPassBeginInfo.renderArea.extent.width = width;
		renderPassBeginInfo.renderArea.extent.height = height;
		if (framebuffer->GetSpecification().SwapChainTarget)
		{
			VulkanSwapChain& swapChain = VulkanContext::GetSwapChain(); // Application::Get().GetWindow().GetSwapChain();
			width = swapChain.GetWidth();
			height = swapChain.GetHeight();
			renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassBeginInfo.pNext = nullptr;
			renderPassBeginInfo.renderPass = framebuffer->GetRenderPass();
			renderPassBeginInfo.renderArea.offset.x = 0;
			renderPassBeginInfo.renderArea.offset.y = 0;
			renderPassBeginInfo.renderArea.extent.width = width;
			renderPassBeginInfo.renderArea.extent.height = height;
			renderPassBeginInfo.framebuffer = swapChain.GetCurrentFramebuffer();

			viewport.x = 0.0f;
			viewport.y = (float)height;
			viewport.width = (float)width;
			viewport.height = -(float)height;
		}
		else
		{
			width = framebuffer->GetWidth();
			height = framebuffer->GetHeight();
			renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassBeginInfo.pNext = nullptr;
			renderPassBeginInfo.renderPass = framebuffer->GetRenderPass();
			renderPassBeginInfo.renderArea.offset.x = 0;
			renderPassBeginInfo.renderArea.offset.y = 0;
			renderPassBeginInfo.renderArea.extent.width = width;
			renderPassBeginInfo.renderArea.extent.height = height;
			renderPassBeginInfo.framebuffer = framebuffer->GetVulkanFramebuffer();

			viewport.x = 0.0f;
			viewport.y = 0.0f;
			viewport.width = (float)width;
			viewport.height = (float)height;
		}

		// TODO: Does our framebuffer have a depth attachment?
		const auto& clearValues = framebuffer->GetVulkanClearValues();
		renderPassBeginInfo.clearValueCount = (uint32_t)clearValues.size();
		renderPassBeginInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		if (explicitClear)
		{
			const uint32_t colorAttachmentCount = (uint32_t)framebuffer->GetColorAttachmentCount();
			const uint32_t totalAttachmentCount = colorAttachmentCount + (framebuffer->HasDepthAttachment() ? 1 : 0);
			ETH_CORE_ASSERT(clearValues.size() == totalAttachmentCount);

			std::vector<VkClearAttachment> attachments(totalAttachmentCount);
			std::vector<VkClearRect> clearRects(totalAttachmentCount);
			for (uint32_t i = 0; i < colorAttachmentCount; i++)
			{
				attachments[i].aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				attachments[i].colorAttachment = i;
				attachments[i].clearValue = clearValues[i];

				clearRects[i].rect.offset = { (int32_t)0, (int32_t)0 };
				clearRects[i].rect.extent = { width, height };
				clearRects[i].baseArrayLayer = 0;
				clearRects[i].layerCount = 1;
			}

			if (framebuffer->HasDepthAttachment())
			{
				attachments[colorAttachmentCount].aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
				attachments[colorAttachmentCount].clearValue = clearValues[colorAttachmentCount];
				clearRects[colorAttachmentCount].rect.offset = { (int32_t)0, (int32_t)0 };
				clearRects[colorAttachmentCount].rect.extent = { width, height };
				clearRects[colorAttachmentCount].baseArrayLayer = 0;
				clearRects[colorAttachmentCount].layerCount = 1;
			}

			vkCmdClearAttachments(commandBuffer, totalAttachmentCount, attachments.data(), totalAttachmentCount, clearRects.data());

		}

		// Update dynamic viewport state
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		// Update dynamic scissor state
		VkRect2D scissor = {};
		scissor.extent.width = width;
		scissor.extent.height = height;
		scissor.offset.x = 0;
		scissor.offset.y = 0;
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
			// });
	}

	void VulkanRendererAPI::EndRenderPass() // Ref<RenderCommandBuffer> renderCommandBuffer
	{
		// Renderer::Submit([renderCommandBuffer]()
		// 	{
		ETH_PROFILE_FUNCTION("VulkanRenderer::EndRenderPass");

		uint32_t frameIndex = VulkanContext::GetSwapChain().GetCurrentFrameIndex(); // Renderer::GetCurrentFrameIndex();
		VkCommandBuffer commandBuffer = std::dynamic_pointer_cast<VulkanRenderCommandBuffer>(s_RenderCommandBuffer)->GetCommandBuffer(frameIndex);// VulkanContext::GetSwapChain().GetCurrentCommandBuffer();

		vkCmdEndRenderPass(commandBuffer);
		// 	});
	}

	void VulkanRendererAPI::DrawQuad( Ref<Pipeline> pipeline, Ref<Material> material, const glm::mat4& transform) // Ref<RenderCommandBuffer> renderCommandBuffer,
	{
		Ref<VulkanMaterial> vulkanMaterial = std::dynamic_pointer_cast<VulkanMaterial>(material);
		// Renderer::Submit([renderCommandBuffer, pipeline, uniformBufferSet, storageBufferSet, vulkanMaterial, transform]() mutable
		// 	{
		ETH_PROFILE_FUNCTION("VulkanRenderer::RenderQuad");

		uint32_t frameIndex = VulkanContext::GetSwapChain().GetCurrentFrameIndex(); // Renderer::GetCurrentFrameIndex();
		// VkCommandBuffer commandBuffer = renderCommandBuffer.As<VulkanRenderCommandBuffer>()->GetCommandBuffer(frameIndex);
		VkCommandBuffer commandBuffer = std::dynamic_pointer_cast<VulkanRenderCommandBuffer>(s_RenderCommandBuffer)->GetCommandBuffer(frameIndex);// VulkanContext::GetSwapChain().GetCurrentCommandBuffer();

		Ref<VulkanPipeline> vulkanPipeline = std::dynamic_pointer_cast<VulkanPipeline>(pipeline);

		VkPipelineLayout layout = vulkanPipeline->GetPipelineLayout();

		auto vulkanMeshVB = std::dynamic_pointer_cast<VulkanVertexBuffer>(s_Data->QuadVertexBuffer);
		VkBuffer vbMeshBuffer = vulkanMeshVB->GetVulkanBuffer();
		VkDeviceSize offsets[1] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vbMeshBuffer, offsets);

		auto vulkanMeshIB = std::dynamic_pointer_cast<VulkanIndexBuffer>(s_Data->QuadIndexBuffer);
		VkBuffer ibBuffer = vulkanMeshIB->GetVulkanBuffer();
		vkCmdBindIndexBuffer(commandBuffer, ibBuffer, 0, VK_INDEX_TYPE_UINT32);

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipeline->GetVulkanPipeline());

		// UpdateMaterialForRendering(vulkanMaterial, uniformBufferSet);

		VkDescriptorSet descriptorSet = vulkanMaterial->GetDescriptorSet(frameIndex);
		if (descriptorSet)
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &descriptorSet, 0, nullptr);

		// Buffer uniformStorageBuffer = vulkanMaterial->GetUniformStorageBuffer();

		vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &transform);
		// vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4), uniformStorageBuffer.Size, uniformStorageBuffer.Data);
		vkCmdDrawIndexed(commandBuffer, s_Data->QuadIndexBuffer->GetCount(), 1, 0, 0, 0);
		// });
	}

	void VulkanRendererAPI::SubmitFullscreenQuad(Ref<Pipeline> pipeline, Ref<Material> material) // Ref<RenderCommandBuffer> renderCommandBuffer, 
	{
		Ref<VulkanMaterial> vulkanMaterial = std::dynamic_pointer_cast<VulkanMaterial>(material);
		
		ETH_PROFILE_FUNCTION("VulkanRenderer::SubmitFullscreenQuad");

		uint32_t frameIndex = VulkanContext::GetSwapChain().GetCurrentFrameIndex(); // Renderer::GetCurrentFrameIndex();
		VkCommandBuffer commandBuffer = std::dynamic_pointer_cast<VulkanRenderCommandBuffer>(s_RenderCommandBuffer)->GetCommandBuffer(frameIndex);// VulkanContext::GetSwapChain().GetCurrentCommandBuffer();

		auto vulkanMeshVB = std::dynamic_pointer_cast<VulkanVertexBuffer>(s_Data->QuadVertexBuffer);
		VkBuffer vbMeshBuffer = vulkanMeshVB->GetVulkanBuffer();
		VkDeviceSize offsets[1] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vbMeshBuffer, offsets);

		auto vulkanMeshIB = std::dynamic_pointer_cast<VulkanIndexBuffer>(s_Data->QuadIndexBuffer);
		VkBuffer ibBuffer = vulkanMeshIB->GetVulkanBuffer();
		vkCmdBindIndexBuffer(commandBuffer, ibBuffer, 0, VK_INDEX_TYPE_UINT32);

		Ref<VulkanPipeline> vulkanPipeline = std::dynamic_pointer_cast<VulkanPipeline>(pipeline);
		VkPipelineLayout layout = vulkanPipeline->GetPipelineLayout();
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipeline->GetVulkanPipeline());

		if (tempfirst)
		{
			UpdateMaterialForRendering(vulkanMaterial);
			tempfirst = false;
		}

		VkDescriptorSet descriptorSet = vulkanMaterial->GetDescriptorSet(frameIndex);
		if (descriptorSet)
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &descriptorSet, 0, nullptr);

		// Buffer uniformStorageBuffer = vulkanMaterial->GetUniformStorageBuffer();
		// if (uniformStorageBuffer.Size)
		// 	vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, uniformStorageBuffer.Size, uniformStorageBuffer.Data);

		vkCmdDrawIndexed(commandBuffer, s_Data->QuadIndexBuffer->GetCount(), 1, 0, 0, 0);
	}

	void VulkanRendererAPI::DrawMesh(Ref<Pipeline> pipeline, Ref<Mesh> mesh, Ref<Material> material, const glm::mat4& transform)
	{
		ETH_CORE_ASSERT(mesh);
		
		uint32_t frameIndex = VulkanContext::GetSwapChain().GetCurrentFrameIndex();
		VkCommandBuffer commandBuffer = std::dynamic_pointer_cast<VulkanRenderCommandBuffer>(s_RenderCommandBuffer)->GetCommandBuffer(frameIndex);

		// Buffer pushConstantBuffer;
		// pushConstantBuffer.Allocate(sizeof(glm::mat4) + additionalUniforms.Size);
		// if (additionalUniforms.Size)
		// 	pushConstantBuffer.Write(additionalUniforms.Data, additionalUniforms.Size, sizeof(glm::mat4));

		Ref<VulkanMaterial> vulkanMaterial = std::dynamic_pointer_cast<VulkanMaterial>(material);
		// Renderer::Submit([renderCommandBuffer, pipeline, uniformBufferSet, storageBufferSet, mesh, vulkanMaterial, transform, pushConstantBuffer]() mutable
		// 	{
		ETH_PROFILE_FUNCTION("VulkanRenderer::RenderMeshWithMaterial");

		// Ref<MeshAsset> meshAsset = mesh->GetMeshAsset();
		auto vulkanMeshVB = std::dynamic_pointer_cast<VulkanVertexBuffer>(mesh->m_VertexBuffer);// meshAsset->GetVertexBuffer().As<VulkanVertexBuffer>();
		VkBuffer vbMeshBuffer = vulkanMeshVB->GetVulkanBuffer();
		VkDeviceSize offsets[1] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vbMeshBuffer, offsets);

		auto vulkanMeshIB = std::dynamic_pointer_cast<VulkanIndexBuffer>(mesh->m_IndexBuffer);// Ref<VulkanIndexBuffer>(meshAsset->GetIndexBuffer());
		VkBuffer ibBuffer = vulkanMeshIB->GetVulkanBuffer();
		vkCmdBindIndexBuffer(commandBuffer, ibBuffer, 0, VK_INDEX_TYPE_UINT32);

		UpdateMaterialForRendering(vulkanMaterial);

		Ref<VulkanPipeline> vulkanPipeline = std::dynamic_pointer_cast<VulkanPipeline>(pipeline);
		VkPipelineLayout layout = vulkanPipeline->GetPipelineLayout();
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipeline->GetVulkanPipeline());

		float lineWidth = 2.0;// vulkanPipeline->GetSpecification().LineWidth;
		if (lineWidth != 1.0f)
			vkCmdSetLineWidth(commandBuffer, lineWidth);

		// Bind descriptor sets describing shader binding points
		VkDescriptorSet descriptorSet = vulkanMaterial->GetDescriptorSet(frameIndex);
		if (descriptorSet)
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &descriptorSet, 0, nullptr);

		// Buffer uniformStorageBuffer = vulkanMaterial->GetUniformStorageBuffer();
		// if (uniformStorageBuffer)
		// 	vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_FRAGMENT_BIT, pushConstantBuffer.Size, uniformStorageBuffer.Size, uniformStorageBuffer.Data);

		auto& submeshes = mesh->m_Submeshes;
		for (auto& submesh : submeshes)
		{
			// // const Submesh& submesh = meshAssetSubmeshes[submeshIndex];
			// glm::mat4 worldTransform = transform * submesh.Transform;
			// pushConstantBuffer.Write(&worldTransform, sizeof(glm::mat4));
			// vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, pushConstantBuffer.Size, pushConstantBuffer.Data);
			vkCmdDrawIndexed(commandBuffer, submesh.IndexCount, 1, submesh.BaseIndex, submesh.BaseVertex, 0);
		}
		// pushConstantBuffer.Release();
		// });
	}

	// TODO: test
	void VulkanRendererAPI::SetUniform(uint32_t binding, uint32_t set , const void* data, uint32_t size, uint32_t offset)
	{
		uint32_t bufferIndex = VulkanContext::GetSwapChain().GetCurrentFrameIndex();// Renderer::GetCurrentFrameIndex();
		s_UniformBufferSet->Get(binding, set, bufferIndex)->SetData(&data, size);
	}

}