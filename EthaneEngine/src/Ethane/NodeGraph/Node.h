#pragma once

#include <imgui_node_editor.h>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>

#include "Pin.h"

namespace Ethane {

	enum class NodeType
	{
		Blueprint,
		Simple,
		Tree,
		Comment,
		Houdini
	};

	struct Node
	{
		ed::NodeId ID;
		std::string Name;
		std::vector<Pin> Inputs;
		std::vector<Pin> Outputs;
		ImColor Color;
		NodeType Type;
		ImVec2 Size;

		std::string State;
		std::string SavedState;

		Node(int id, const char* name, ImColor color = ImColor(255, 255, 255)) :
			ID(id), Name(name), Color(color), Type(NodeType::Blueprint), Size(0, 0)
		{
		}
	};

	struct NodeIdLess
	{
		bool operator()(const ed::NodeId& lhs, const ed::NodeId& rhs) const
		{
			return lhs.AsPointer() < rhs.AsPointer();
		}
	};

	class MyNode
	{
	public:


	};

}