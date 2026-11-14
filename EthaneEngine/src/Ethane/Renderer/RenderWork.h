#pragma once

#include <functional>
#include <vector>

namespace Ethane {

	class ICommandList;

	// The GPU work of one frame, handed over from the frame's update (layers, UI) to whatever records it.
	//
	// The contract: a pass owns everything it reads (captured by value: frame packets, matrices,
	// RefCountPtrs to GPU objects) or uses objects that only recording touches (a Renderer, a render
	// target being drawn into). It never reads layer state, which the next frame's update may already be
	// changing. Recording can then happen later, or on another thread, without locks.
	class RenderWork
	{
	public:
		using Pass = std::function<void(ICommandList&)>;

		// Before the window's render pass, outside any pass: render to textures (IRenderTarget) here
		void Offscreen(Pass pass) { m_Offscreen.push_back(std::move(pass)); }
		// Inside the window's render pass, in the order added (the UI goes last)
		void Window(Pass pass) { m_Window.push_back(std::move(pass)); }

		void RecordOffscreen(ICommandList& commandList)
		{
			for (Pass& pass : m_Offscreen)
				pass(commandList);
		}
		void RecordWindow(ICommandList& commandList)
		{
			for (Pass& pass : m_Window)
				pass(commandList);
		}

		bool Empty() const { return m_Offscreen.empty() && m_Window.empty(); }

	private:
		std::vector<Pass> m_Offscreen;
		std::vector<Pass> m_Window;
	};

}
