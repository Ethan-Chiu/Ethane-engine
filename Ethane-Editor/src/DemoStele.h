#pragma once

namespace Ethane {

	class AssetLibrary;
	class Stele;

	// Adds the demo content to `stele` in one commit: the default meshes (res/meshes/default/*.fbx) under a
	// "Shapes" element, each fitted into a unit box with its own material and spinning in play mode, on a
	// textured, normal-mapped floor (assets/models/CobblestoneFloor), lit by a sun, seen by a camera.
	// Needs RegisterGameTypes() (Game/Spin.h). Registers the files it uses in the asset
	// library's registry; loads the meshes to fit them.
	void BuildDemoStele(Stele& stele, AssetLibrary& assets);

}
