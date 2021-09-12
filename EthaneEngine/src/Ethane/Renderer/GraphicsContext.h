#pragma once

namespace Ethane {

	class GraphicsContext
	{
	public:
		virtual ~GraphicsContext() = default;

		virtual void Init() = 0;

		virtual void BeginFrame() = 0; //TODO: test
		virtual void SwapBuffers() = 0;

		virtual void OnResize(uint32_t width, uint32_t height) = 0;

		// TODO: test
		virtual void ShutDown() = 0;

		static Scope<GraphicsContext> Create(void* window);
	};
}