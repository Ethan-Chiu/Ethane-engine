#pragma once

#include "Ethane/UI/UIImage.h"
#include "Ethane/NodeGraph/NodeGraph.h"

namespace Ethane::NodeGraph {

	class SCNG : public NodeGraph
	{
	public:
		virtual Node* CustomCreateNodePopup(Node* node) override;
		virtual void DrawCustomNode(Node& node) override;
		void Compile();
	private:
		void DrawSCNode(Node& node);
	private:
		// Imgui helper
		void ButtonAddAndRemove(Node& node, const std::string& pinVal, const PinType pinType);

		// DB
		Node* SpawnSCDBPlotNode();
		
		// Type 
		Node* SpawnSCIntNode();
		Node* SpawnSCStringArrayNode();
		Node* SpawnSCIntArrayNode();
		Node* SpawnSCStatesToIntArrayNode();

		// State Node
		Node* SpawnSCPlotState();
		Node* SpawnSCPuzzleState();
		Node* SpawnSCShadowCloneState();
		Node* SpawnSCDialogState();
		Node* SpawnSCRewardState();
		Node* SpawnSCLockState();
		Node* SpawnSCEventState();

		Node* SpawnSCIndexNode();
		Node* SpawnSCNode();
	};

}