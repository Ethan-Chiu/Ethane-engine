#pragma once

#include "Ethane/Core/UUID.h"

#include <imgui_node_editor.h>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>

namespace ed = ax::NodeEditor;

namespace Ethane {

	enum class PinType
	{
		Flow,
		Bool,
		Int,
		Float,
		String,
		Object,
		Function,
		Delegate,
	};

	enum PinDirection
	{
		Input,
		Output
	};

	struct Node;

	struct Pin
	{
		ed::PinId       ID;
		Node*		    Node;
		std::string     Name;
		PinType         Type;
		PinDirection    Kind;

		Pin(int id, const char* name, PinType type) :
			ID(id), Node(nullptr), Name(name), Type(type), Kind(PinDirection::Input)
		{
		}
	};

	class MyPin 
	{

	public:
		UUID m_ID;
		std::string m_Name;
		PinDirection m_Direction;


	private:
		Node* OwningNode;
	};

}