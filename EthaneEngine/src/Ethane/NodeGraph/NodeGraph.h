#pragma once

#include "Ethane/UI/UIImage.h"
#include "Pin.h"
#include "Node.h"

#include "builders.h"
#include "widgets.h"

#include <imgui_node_editor.h>
#include <map>

#include <filesystem>

namespace ax::NodeEditor
{
	struct EditorContext;
}

namespace Ethane::NodeGraph {

	struct Link
	{
		ax::NodeEditor::LinkId ID;
		ax::NodeEditor::PinId StartPinID;
		ax::NodeEditor::PinId EndPinID;

		ImColor Color;

		Link(uint32_t id, ed::PinId startPinId, ed::PinId endPinId) :
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

		// TODO: test
		void OnCompile();

		virtual Node* CustomCreateNodePopup(Node* node) { return node; };
		virtual void DrawCustomNode(Node& node) {};

	private:
		bool IsLinkValid(Pin* startPin, Pin* endPin, LinkRule* violateRule = nullptr);

		void M_DrawLink();
		void M_HandleDelete();

		// UI stuff
		void M_CreateNodePopup();
		void M_DrawSimpleBlueprintNode(Node& node);
		void M_DrawCommentNode(Node& node);
		void M_DrawHoudiniNode(Node& node); // remove maybe?
		void M_DrawTreeNode(Node& node); // remove maybe?

		// test
		void ShowLeftPane(float paneWidth);
		void ShowStyleEditor(bool* show = nullptr);

		// Compile stuff
		void Compile(std::ofstream& out);

		// Save and load stuff
		void OnSave();
		void OnOpenGraphFile();
		bool Deserialize(const std::filesystem::path& filepath);
		void Serialize(const std::filesystem::path& filepath);

	// Utilities: Basic functionalities
	protected:
		int GetNextId();
		ed::LinkId GetNextLinkId();

		void TouchNode(ed::NodeId id);
		float GetTouchProgress(ed::NodeId id);
		void UpdateTouch();

		// Find
		Node* FindNode(ed::NodeId id);
		Link* FindLink(ed::LinkId id);
		Pin* FindPin(ed::PinId id);
		//TODO: test
		Pin* NodeGraph::FindOtherLinkedPin(ed::PinId pinID);
        
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
		
	protected:
		// Context and Config constants
		ax::NodeEditor::EditorContext* m_Context = nullptr;
		const float			 m_CCommentAlpha = 0.75f;
		const uint32_t       m_CPinIconSize = 24;

		ed::Utilities::BlueprintNodeBuilder m_Builder;

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

		// resources
		Ref<Texture2D> m_HeaderBackground;
		Ref<Texture2D> m_SaveIcon;
		Ref<Texture2D> m_RestoreIcon;

		// TODO: Test
		bool m_ShowDefaultCreateNodePopup = true;
	};
}