#pragma once

#include "Ethane/UI/UIImage.h"
#include "Pin.h"
#include "Node.h"

#include <imgui_node_editor.h>
#include <map>

namespace ax::NodeEditor
{
	struct EditorContext;
}

namespace Ethane {

	struct Link
	{
		ax::NodeEditor::LinkId ID;
		ax::NodeEditor::PinId StartPinID;
		ax::NodeEditor::PinId EndPinID;

		ImColor Color;

		Link(ed::LinkId id, ed::PinId startPinId, ed::PinId endPinId) :
			ID(id), StartPinID(startPinId), EndPinID(endPinId), Color(255, 255, 255)
		{
		}
	};

	struct LinkRule
	{
		std::function<bool(Pin* a, Pin* b)> ViolateRule;
		std::string ErrorMessage = "Can't create link";
		ImColor ErrorLabelColor = { 45, 32, 32, 180 };
		ImColor ErrorLinkColor = { 255, 0, 0 };
		float LinkThickness = 2.0f;
	};

	class NodeGraph
	{
	public:
		NodeGraph();

		void Init();
		void OnImGuiRender();
		void OnClose();

	private:
		bool IsLinkValid(Pin* startPin, Pin* endPin, LinkRule* violateRule = nullptr);

		void M_DrawLink();
		void M_HandleDelete();

		// UI stuff
		void M_CreateNodePopup();
		void M_DrawCommentNode(Node& node);
		void M_DrawHoudiniNode(Node& node); // remove maybe?
		void M_DrawTreeNode(Node& node); // remove maybe?
		// test
		void ShowLeftPane(float paneWidth);
		void ShowStyleEditor(bool* show = nullptr);

	// Utilities: Basic functionalities
	private:
		int GetNextId();
		ed::LinkId GetNextLinkId();

		void TouchNode(ed::NodeId id);
		float GetTouchProgress(ed::NodeId id);
		void UpdateTouch();

		// Find
		Node* FindNode(ed::NodeId id);
		Link* FindLink(ed::LinkId id);
		Pin* FindPin(ed::PinId id);
        
		bool IsPinLinked(ed::PinId id);
		bool CanCreateLink(Pin* a, Pin* b);

		void BuildNode(Node* node);
		void  BuildNodes();

		// SpawnNode
		Node* SpawnInputActionNode();
		Node* SpawnBranchNode();
		Node* SpawnDoNNode();
		Node* SpawnOutputActionNode();
		Node* SpawnPrintStringNode();
		Node* SpawnMessageNode();
		Node* SpawnSetTimerNode();
		Node* SpawnLessNode();
		Node* SpawnWeirdNode();
		Node* SpawnTraceByChannelNode();
		Node* SpawnTreeSequenceNode();
		Node* SpawnTreeTaskNode();
		Node* SpawnTreeTask2Node();
		Node* SpawnComment();
		Node* SpawnHoudiniTransformNode();
		Node* SpawnHoudiniGroupNode();
       
		// style
		ImColor GetIconColor(PinType type);
		void DrawPinIcon(const Pin& pin, bool connected, int alpha);
		
	private:
		// Context and Config constants
		ax::NodeEditor::EditorContext* m_Context = nullptr;
		const float			 m_CCommentAlpha = 0.75f;
		const uint32_t       m_CPinIconSize = 24;

		// State
		bool m_CreateNewNode = false;
		Pin* m_NewNodeLinkPin = nullptr;
		Pin* m_NewLinkPin = nullptr;

		uint32_t             m_NextId = 1;
		std::vector<Node>    m_Nodes;
		std::vector<Link>    m_Links;

		std::vector<LinkRule> m_LinkRules;

		const float          m_TouchTime = 1.0f;
		std::map<ed::NodeId, float, NodeIdLess> m_NodeTouchTime;
		bool                 m_ShowOrdinals = false;

		Ref<Texture2D> m_HeaderBackground;
		Ref<Texture2D> m_SaveIcon;
		Ref<Texture2D> m_RestoreIcon;
	};
}