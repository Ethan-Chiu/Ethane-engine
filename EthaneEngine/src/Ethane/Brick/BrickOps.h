#pragma once

#include "Ethane/Brick/ComponentValue.h"
#include "Ethane/Schema/FieldAccess.h"

#include <string>
#include <variant>

namespace Ethane {

	// The edits a brick understands. Each changes one small thing, so ops from different editors
	// combine predictably and each one has a simple inverse.
	namespace BrickOp {

		struct CreateObject
		{
			ObjectId Id;
			std::string Name;
			ObjectId Parent;   // null: top level
			std::string Order; // empty in a transaction: after the last sibling (resolved at commit)
		};

		// The object must have no children (destroy them first; Transaction::DestroyTree does that)
		struct DestroyObject { ObjectId Id; };

		struct RenameObject { ObjectId Id; std::string Name; };

		struct MoveObject
		{
			ObjectId Id;
			ObjectId Parent;
			std::string Order; // empty in a transaction: after the last sibling (resolved at commit)
		};

		struct AddComponent
		{
			ObjectId Id;
			TypeId Type;
			ComponentRef Value; // null in a transaction: default-constructed (resolved at commit)
		};

		struct RemoveComponent { ObjectId Id; TypeId Type; };

		struct SetField
		{
			ObjectId Id;
			TypeId Type;
			FieldPath Path;
			FieldValue Value;
		};

	}

	using BrickOperation = std::variant<
		BrickOp::CreateObject, BrickOp::DestroyObject, BrickOp::RenameObject, BrickOp::MoveObject,
		BrickOp::AddComponent, BrickOp::RemoveComponent, BrickOp::SetField>;

}
