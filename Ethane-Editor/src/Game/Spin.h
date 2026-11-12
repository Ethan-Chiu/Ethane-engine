#pragma once

namespace Ethane {

	// Game code living outside the engine: a component type and a script, registered at startup like
	// any game's would be.
	//
	// A "Spin" component sets how fast its element turns; a NativeScript component with class "Spin"
	// turns it around its Y axis while playing.
	struct SpinComponent
	{
		float Speed = 0.5f; // radians per second
	};

	// Registers SpinComponent in the global type registry and the Spin script in the global script
	// registry (again is a no-op)
	void RegisterGameTypes();

}
