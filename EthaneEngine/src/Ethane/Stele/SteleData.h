#pragma once

// Internal to the stele implementation: include only from Ethane/Stele/*.cpp

#include "Ethane/Stele/SteleSnapshot.h"

#include <unordered_map>

namespace Ethane::Detail {

	// The data of one stele version. Consecutive versions share every element, component and child list
	// that didn't change between them.
	struct SteleData
	{
		const TypeRegistry* Registry = nullptr;
		uint64_t Version = 0;
		std::unordered_map<ElementId, SteleElementRef> Elements;
		// Children of each parent (null id: top level), sorted by (Order, Id)
		std::unordered_map<ElementId, std::shared_ptr<const std::vector<ElementId>>> Children;
	};

}
