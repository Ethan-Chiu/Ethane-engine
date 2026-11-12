#pragma once

#include "Ethane/Stele/Stele.h"

#include <filesystem>
#include <string>
#include <vector>

namespace Ethane {

	// Saves and loads steles as YAML (files use the .stele extension), using the schema for every
	// component (no per-type code):
	//
	//     Stele: 1                     # file format version
	//     Types:                       # schema version of each component type used
	//       Transform: 1
	//     Elements:                    # parents before children, siblings in order
	//       - Id: 3f2a9c0d1e4b5a67
	//         Name: Lamp
	//         Parent: 0000000000000000  # omitted for top-level elements
	//         Order: V
	//         Components:
	//           Transform:
	//             Position: [0, 2, 0]
	//
	// Runtime-only fields are not saved. Loading is forgiving: unknown component types and fields, and
	// values of the wrong type, are skipped with a warning and the rest of the file still loads.
	namespace SteleSerializer {

		constexpr int kFormatVersion = 1;

		std::string ToYaml(const SteleSnapshot& snapshot);

		struct LoadResult
		{
			bool Ok = false;
			std::string Error;                 // set when nothing was loaded
			std::vector<std::string> Warnings; // things that were skipped
			explicit operator bool() const { return Ok; }
		};

		// Adds the file's elements to `stele` in one commit (labelled `label`)
		LoadResult FromYaml(std::string_view yaml, Stele& stele, std::string label = "Load");

		bool Save(const SteleSnapshot& snapshot, const std::filesystem::path& path);
		LoadResult Load(const std::filesystem::path& path, Stele& stele);

	}

}
