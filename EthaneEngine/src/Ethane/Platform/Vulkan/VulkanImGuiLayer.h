#pragma once

#include "Ethane/ImGui/ImGuiLayer.h"
#include "Ethane/Platform/Vulkan/VulkanContext.h"

namespace Ethane{

	class VulkanImGuiLayer : public ImGuiLayer
	{
	public:
		VulkanImGuiLayer();
		~VulkanImGuiLayer();

		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnEvent(Event& e) override;

		virtual void Begin() override;
		virtual void End() override;

		virtual void BlockEvents(bool block) override { m_BlockEvents = block; }

		// TODO: test
		static void Cleanup();
		static std::vector<VkCommandBuffer>& GetImGuiCommandBuffer() { return s_ImGuiCommandBuffers; };
	private:
		bool m_BlockEvents = true;
		float m_Time = 0.0f;

		static std::vector<VkCommandBuffer> s_ImGuiCommandBuffers;
	};

}