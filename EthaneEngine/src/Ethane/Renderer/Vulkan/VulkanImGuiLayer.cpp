#include "ethpch.h"
#include "VulkanImGuiLayer.h"

#include "imgui.h"
#include "ImGuizmo.h"
#ifndef IMGUI_IMPL_API
#define IMGUI_IMPL_API
#endif

#include "backends/imgui_impl_glfw.h"
#include "examples/imgui_impl_vulkan_with_textures.h"

#include "Ethane/Core/Application.h"
#include "GLFW/glfw3.h"


namespace Ethane {

	VulkanImGuiLayer::VulkanImGuiLayer()
	{

	}

	VulkanImGuiLayer::~VulkanImGuiLayer()
	{
	}

	void VulkanImGuiLayer::OnAttach()
	{
		// Setup Dear ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
		//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
		//io.ConfigViewportsNoDecoration = false;
		//io.ConfigViewportsNoAutoMerge = true;
		//io.ConfigViewportsNoTaskBarIcon = true;

		io.Fonts->AddFontFromFileTTF("assets/fonts/opensans/OpenSans-Bold.ttf", 18.0f);
		io.FontDefault = io.Fonts->AddFontFromFileTTF("assets/fonts/opensans/OpenSans-Regular.ttf", 18.0f);
		// io.Fonts->AddFontFromFileTTF("Resources/Fonts/opensans/OpenSans-Regular.ttf", 24.0f);

		// Setup Dear ImGui style
		ImGui::StyleColorsDark();
		// SetDarkThemeColors();
		//ImGui::StyleColorsClassic();

		// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}
		style.Colors[ImGuiCol_WindowBg] = ImVec4(0.15f, 0.15f, 0.15f, style.Colors[ImGuiCol_WindowBg].w);

		// VulkanImGuiLayer* instance = this;
		// Renderer::Submit([instance]()
		// 	{
		Application& app = Application::Get();
		GLFWwindow* window = static_cast<GLFWwindow*>(app.GetWindow().GetNativeWindow());

		// auto vulkanContext = VulkanContext::Get();
		auto device = VulkanContext::GetDevice()->GetVulkanDevice();

		VkDescriptorPool descriptorPool;

		// Create Descriptor Pool
		VkDescriptorPoolSize pool_sizes[] =
		{
			{ VK_DESCRIPTOR_TYPE_SAMPLER, 100 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100 },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 100 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 100 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 100 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 100 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 100 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 100 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 100 },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 100 }
		};
		VkDescriptorPoolCreateInfo pool_info = {};
		pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		pool_info.maxSets = 100 * IM_ARRAYSIZE(pool_sizes);
		pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
		pool_info.pPoolSizes = pool_sizes;
		VK_CHECK_RESULT(vkCreateDescriptorPool(device, &pool_info, nullptr, &descriptorPool));

		// Setup Platform/Renderer bindings
		ImGui_ImplGlfw_InitForVulkan(window, true);
		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.Instance = VulkanContext::GetInstance();
		init_info.PhysicalDevice = VulkanContext::GetPhysicalDevice()->GetVulkanPhysicalDevice();
		init_info.Device = device;
		init_info.QueueFamily = VulkanContext::GetPhysicalDevice()->GetQueueFamilyIndices().Graphics.value();
		init_info.Queue = VulkanContext::GetDevice()->GetGraphicsQueue();
		init_info.PipelineCache = nullptr;
		init_info.DescriptorPool = descriptorPool;
		init_info.Allocator = nullptr;
		init_info.MinImageCount = 2;
		const Ref<VulkanSwapChain> swapChain = VulkanContext::GetSwapChain();// Application::Get().GetWindow().GetSwapChain();
		init_info.ImageCount = swapChain->GetImageCount();
		init_info.CheckVkResultFn = Utils::VulkanCheckResult;
		ImGui_ImplVulkan_Init(&init_info, swapChain->GetRenderPass());

		// Load Fonts
		// - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
		// - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
		// - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
		// - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
		// - Read 'docs/FONTS.md' for more instructions and details.
		// - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
		//io.Fonts->AddFontDefault();
		//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
		//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
		//io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
		//io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
		//ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
		//IM_ASSERT(font != NULL);

		// Upload Fonts
		{
			// Use any command queue

			VkCommandBuffer commandBuffer = VulkanContext::GetDevice()->CreateCommandBuffer(QueueFamilyTypes::Graphics, false, true);
			ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
			VulkanContext::GetDevice()->SubmitCommandBuffer(commandBuffer);

			VK_CHECK_RESULT(vkDeviceWaitIdle(device));
			ImGui_ImplVulkan_DestroyFontUploadObjects();
		}

		SetDarkThemeColors();

		uint32_t framesInFlight = swapChain->GetMaxFramesInFlight();// Renderer::GetConfig().FramesInFlight;
		m_ImGuiCommandBuffers.resize(framesInFlight);
		for (uint32_t i = 0; i < framesInFlight; i++)
			m_ImGuiCommandBuffers[i] = VulkanContext::GetDevice()->CreateSecondaryCommandBuffer();

	}

	void VulkanImGuiLayer::Cleanup()
	{
		auto device = VulkanContext::GetDevice()->GetVulkanDevice();
		
		if (m_ImGuiCommandBuffers.size() > 0)
		{
			vkFreeCommandBuffers(device, VulkanContext::GetDevice()->GetGraphicsCommandPool(), static_cast<uint32_t>(m_ImGuiCommandBuffers.size()), m_ImGuiCommandBuffers.data());
		}
	}

	void VulkanImGuiLayer::OnDetach()
	{
		auto device = VulkanContext::GetDevice()->GetVulkanDevice();

		VK_CHECK_RESULT(vkDeviceWaitIdle(device));
		Cleanup();
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}

	void VulkanImGuiLayer::OnEvent(Event& e)
	{
	}

	void VulkanImGuiLayer::Begin()
	{
		ETH_PROFILE_FUNCTION();

		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		ImGuizmo::BeginFrame();
	}

	void VulkanImGuiLayer::End()
	{
		ETH_PROFILE_FUNCTION();

		ImGui::Render();
		
		const Ref<VulkanSwapChain> swapChain = VulkanContext::GetSwapChain();
		uint32_t commandBufferIndex = swapChain->GetCurrentFrameIndex();

		uint32_t width = swapChain->GetWidth();
		uint32_t height = swapChain->GetHeight();
		
		VkCommandBufferInheritanceInfo inheritanceInfo = {};
		inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
		inheritanceInfo.renderPass = swapChain->GetRenderPass();
		inheritanceInfo.framebuffer = swapChain->GetCurrentFramebuffer();
		
		VkCommandBufferBeginInfo cmdBufInfo = {};
		cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmdBufInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
		cmdBufInfo.pInheritanceInfo = &inheritanceInfo;
		
		VK_CHECK_RESULT(vkBeginCommandBuffer(m_ImGuiCommandBuffers[commandBufferIndex], &cmdBufInfo));
		
		ImDrawData* main_draw_data = ImGui::GetDrawData();
		ImGui_ImplVulkan_RenderDrawData(main_draw_data, m_ImGuiCommandBuffers[commandBufferIndex]);
		
		VK_CHECK_RESULT(vkEndCommandBuffer(m_ImGuiCommandBuffers[commandBufferIndex]));

		swapChain->RegisterSecondaryCmdBuffer(m_ImGuiCommandBuffers[commandBufferIndex]);

		ImGuiIO& io = ImGui::GetIO(); (void)io;
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}
	}

}