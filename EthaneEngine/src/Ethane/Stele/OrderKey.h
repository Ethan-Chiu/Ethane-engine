#pragma once

#include <string>
#include <string_view>

namespace Ethane {

	// Fractional order keys: strings of base-62 digits ('0'-'9', 'A'-'Z', 'a'-'z') read as the digits
	// after a radix point, so a key can always be made between any two others without renumbering.
	// Plain string comparison orders them. Keys never end in '0'.
	//
	// Siblings in a stele are ordered by key (ties broken by ElementId), so two editors inserting at the
	// same place concurrently never invalidate each other's positions.
	namespace OrderKey {

		// A key strictly between `before` and `after`; an empty `before` means "the very start", an empty
		// `after` means "the very end". Requires before < after when both are given.
		std::string Between(std::string_view before, std::string_view after);

		bool IsValid(std::string_view key);

	}

}
