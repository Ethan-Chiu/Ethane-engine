#pragma once

#include "Ethane.h"

#include "Ethane/ImGui/ImGuiLayer.h"
// #include "Ethane/RayTracing/RayTracing.h"

namespace Ethane {

	class RayTracingLayer : public Layer
	{
	public:
		RayTracingLayer();
		virtual ~RayTracingLayer() = default;

		virtual void OnAttach() override;
		virtual void OnDetach() override;

		void OnUpdate(Timestep ts) override;
		virtual void OnImGuiRender() override;
		void OnEvent(Event& e) override;
	private:
		// RayTracing m_RT;
	};

}