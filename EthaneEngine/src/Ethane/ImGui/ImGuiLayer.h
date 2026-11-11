#pragma once

#include "Ethane/Core/Layer.h"

#include <imgui.h>

namespace Ethane {

	class GfxBackendAPI;
	class IWindowTarget;
	class IRenderTarget;

	// Dear ImGui on top of the window, pushed as an overlay by Application. Each frame:
	//   Begin() -> every layer's OnImGuiRender() -> End()   builds the UI (before the frame is recorded)
	//   Render(commandList)                               records it into the window's render pass, after
	//                                                       every layer's OnRender (so it's drawn on top)
	// While it's blocking events (the default), mouse / keyboard events ImGui wants (e.g. the cursor is
	// over an ImGui window) are marked handled, so layers below don't see them.
	class ImGuiLayer : public Layer
	{
	public:
		ImGuiLayer() : Layer("ImGuiLayer") {}

		virtual void Begin() = 0;
		virtual void End() = 0;
		virtual void Render(ICommandList& commandList) = 0;

		// The color image of `target`, for ImGui::Image. Valid for the current UI frame: ask again every
		// frame (resizing the target replaces its image). The target must be rendered into before the
		// window's render pass of every frame that shows it, e.g. in Layer::OnRenderOffscreen.
		virtual ImTextureID GetTextureID(const IRenderTarget& target) = 0;

		virtual void OnEvent(Event& e) override;
		void BlockEvents(bool block) { m_BlockEvents = block; }

		void SetDarkThemeColors();

		static ImGuiLayer* Create(GfxBackendAPI& backend, IWindowTarget& windowTarget, void* nativeWindow);

	protected:
		bool m_BlockEvents = true;
	};
}
