#pragma once

#include "Ethane/Stele/ComponentValue.h"
#include "Ethane/Schema/FieldAccess.h"

#include <string>
#include <variant>

namespace Ethane {

	// The edits a stele understands. Each changes one small thing, so ops from different editors
	// combine predictably and each one has a simple inverse.
	namespace SteleOp {

		struct CreateElement
		{
			ElementId Id;
			std::string Name;
			ElementId Parent;   // null: top level
			std::string Order; // empty in a transaction: after the last sibling (resolved at commit)
		};

		// The element must have no children (destroy them first; Transaction::DestroyTree does that)
		struct DestroyElement { ElementId Id; };

		struct RenameElement { ElementId Id; std::string Name; };

		struct MoveElement
		{
			ElementId Id;
			ElementId Parent;
			std::string Order; // empty in a transaction: after the last sibling (resolved at commit)
		};

		struct AddComponent
		{
			ElementId Id;
			TypeId Type;
			ComponentRef Value; // null in a transaction: default-constructed (resolved at commit)
		};

		struct RemoveComponent { ElementId Id; TypeId Type; };

		struct SetField
		{
			ElementId Id;
			TypeId Type;
			FieldPath Path;
			FieldValue Value;
		};

	}

	using SteleOperation = std::variant<
		SteleOp::CreateElement, SteleOp::DestroyElement, SteleOp::RenameElement, SteleOp::MoveElement,
		SteleOp::AddComponent, SteleOp::RemoveComponent, SteleOp::SetField>;

}
