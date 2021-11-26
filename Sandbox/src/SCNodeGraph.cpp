#include "SCNodeGraph.h"

namespace Ethane::NodeGraph {

	Node* SCNG::CustomCreateNodePopup(Node* node)
	{
		// if (ImGui::MenuItem("State"))
		// 	node = SpawnSCNode();
        // if (ImGui::MenuItem("Switch"))
        //     node = SpawnSCSwitchNode();
        // if (ImGui::MenuItem("Input"))
        //     node = SpawnSCInputNode();
        if (ImGui::MenuItem("Plot"))
            node = SpawnSCPlotState();
        if (ImGui::MenuItem("Puzzle"))
            node = SpawnSCPuzzleState();
        if (ImGui::MenuItem("ShadowClone"))
            node = SpawnSCShadowCloneState();
        if (ImGui::MenuItem("Dialog"))
            node = SpawnSCDialogState();
        if (ImGui::MenuItem("Reward"))
            node = SpawnSCRewardState();
        if (ImGui::MenuItem("Lock"))
            node = SpawnSCLockState();
        if (ImGui::MenuItem("Event"))
            node = SpawnSCEventState();
        ImGui::Separator();
        if (ImGui::MenuItem("Int"))
            node = SpawnSCIntNode();
        if (ImGui::MenuItem("IntArray"))
            node = SpawnSCIntArrayNode();
        if (ImGui::MenuItem("StringArray"))
            node = SpawnSCStringArrayNode();
        if (ImGui::MenuItem("ToIntArrayNode"))
            node = SpawnSCStatesToIntArrayNode();
        return node;
	}

	void SCNG::DrawCustomNode(Node& node)
	{
        switch (node.Type)
        {
        case NodeType::SC:
            DrawSCNode(node);
            break;
		}
	}


    void SCNG::ButtonAddAndRemove(Node& node, const std::string& pinVal, const PinType pinType)
    {
        if (ImGui::Button("+"))
        {
            node.Inputs.emplace_back(GetNextId(), pinVal.c_str(), pinType);
            BuildNode(&node);
        }
        ImGui::SameLine();
        if (ImGui::Button("-"))
        {
            node.Inputs.pop_back();
        }
    }
    /////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////

    // DB ---------------------------------------------------------------------------
    Node* SCNG::SpawnSCDBPlotNode()
    {
        m_Nodes.emplace_back(GetNextId(), "DB_Plot");
        m_Nodes.back().Type = NodeType::SC;
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "plot id", PinType::Flow);

        BuildNode(&m_Nodes.back());
        return &m_Nodes.back();
    }

    // Type -------------------------------------------------------------------------
    Node* SCNG::SpawnSCStringArrayNode()
    {
        m_Nodes.emplace_back(GetNextId(), "StringArray");
        m_Nodes.back().Type = NodeType::SC;
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "string array", PinType::StringArray);

        BuildNode(&m_Nodes.back());
        return &m_Nodes.back();
    }

    Node* SCNG::SpawnSCIntNode()
    {
        m_Nodes.emplace_back(GetNextId(), "Int");
        m_Nodes.back().Type = NodeType::SC;
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "int", PinType::Int);

        BuildNode(&m_Nodes.back());
        return &m_Nodes.back();
    }

    Node* SCNG::SpawnSCIntArrayNode()
    {
        m_Nodes.emplace_back(GetNextId(), "IntArray");
        m_Nodes.back().Type = NodeType::SC;
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "int array", PinType::IntArray);

        BuildNode(&m_Nodes.back());
        return &m_Nodes.back();
    }

    Node* SCNG::SpawnSCStatesToIntArrayNode()
    {
        m_Nodes.emplace_back(GetNextId(), "CastToIntArray");
        m_Nodes.back().Type = NodeType::SC;
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "int array", PinType::IntArray);

        BuildNode(&m_Nodes.back());
        return &m_Nodes.back();
    }

    // State ------------------------------------------------------------------------
    Node* SCNG::SpawnSCPlotState()
    {
        m_Nodes.emplace_back(GetNextId(), "GameStatePlot");
        m_Nodes.back().Type = NodeType::SC;
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "plot", PinType::Int);
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "next state", PinType::Flow);

        BuildNode(&m_Nodes.back());
        return &m_Nodes.back();
    }

    Node* SCNG::SpawnSCPuzzleState()
    {
        m_Nodes.emplace_back(GetNextId(), "GameStatePuzzle");
        m_Nodes.back().Type = NodeType::SC;
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "puzzle", PinType::Int);
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "next state", PinType::Flow);

        BuildNode(&m_Nodes.back());
        return &m_Nodes.back();
    }

    Node* SCNG::SpawnSCShadowCloneState()
    {
        m_Nodes.emplace_back(GetNextId(), "GameStateShadowClone");
        m_Nodes.back().Type = NodeType::SC;
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "nbKeys", PinType::StringArray);
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "closingStates", PinType::IntArray);
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "next state", PinType::Flow);
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "branch", PinType::Flow);

        BuildNode(&m_Nodes.back());
        return &m_Nodes.back();
    }

    Node* SCNG::SpawnSCDialogState()
    {
        m_Nodes.emplace_back(GetNextId(), "GameStateDialog");
        m_Nodes.back().Type = NodeType::SC;
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "dialog", PinType::Int);
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "next state", PinType::Flow);

        BuildNode(&m_Nodes.back());
        return &m_Nodes.back();
    }

    Node* SCNG::SpawnSCRewardState()
    {
        m_Nodes.emplace_back(GetNextId(), "GameStateReward");
        m_Nodes.back().Type = NodeType::SC;
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "reward state", PinType::IntArray);
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "reward props", PinType::IntArray);
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "reward abilities", PinType::IntArray);
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "dialog", PinType::Int);
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "next state", PinType::Flow);

        BuildNode(&m_Nodes.back());
        return &m_Nodes.back();
    }

    Node* SCNG::SpawnSCLockState()
    {
        m_Nodes.emplace_back(GetNextId(), "GameStateLock");
        m_Nodes.back().Type = NodeType::SC;
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "require state", PinType::IntArray);
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "require props", PinType::IntArray);
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "require abilities", PinType::IntArray);
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "fallback state", PinType::Int);
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "dialog", PinType::Int);
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "next state", PinType::Flow);

        BuildNode(&m_Nodes.back());
        return &m_Nodes.back();
    }

    Node* SCNG::SpawnSCEventState()
    {
        m_Nodes.emplace_back(GetNextId(), "GameStateLock");
        m_Nodes.back().Type = NodeType::SC;
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "require state", PinType::IntArray);
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "require props", PinType::IntArray);
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "require abilities", PinType::IntArray);
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "closing state", PinType::Int);
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "next state", PinType::Flow);

        BuildNode(&m_Nodes.back());
        return &m_Nodes.back();
    }



    // Function ---------------------------------------------------------------------
    Node* SCNG::SpawnSCIndexNode()
    {
        m_Nodes.emplace_back(GetNextId(), "Switch");
        m_Nodes.back().Type = NodeType::SC;
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "value", PinType::String);
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "keys", PinType::StringArray);
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "index", PinType::Int);

        BuildNode(&m_Nodes.back());

        return &m_Nodes.back();
    }

    Node* SCNG::SpawnSCNode()
    {
        m_Nodes.emplace_back(GetNextId(), "GameState");
        m_Nodes.back().Type = NodeType::SC;
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "dialog", PinType::String);
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "index", PinType::Int);
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "next state", PinType::Flow);

        BuildNode(&m_Nodes.back());

        return &m_Nodes.back();
    }


    /////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////

	void SCNG::DrawSCNode(Node& node)
	{
        bool hasOutputDelegates = false;
        for (auto& output : node.Outputs)
            if (output.Type == PinType::Delegate)
                hasOutputDelegates = true;

        m_Builder.Begin(node.ID);

        m_Builder.Header(node.Color);
        ImGui::Spring(0);
        ImGui::TextUnformatted(node.Name.c_str());
        ImGui::Spring(1);
        ImGui::Dummy(ImVec2(0, 28));
        if (hasOutputDelegates)
        {
            ImGui::BeginVertical("delegates", ImVec2(0, 28));
            ImGui::Spring(1, 0);
            for (auto& output : node.Outputs)
            {
                if (output.Type != PinType::Delegate)
                    continue;

                auto alpha = ImGui::GetStyle().Alpha;
                if (m_NewLinkPin && !CanCreateLink(m_NewLinkPin, &output) && &output != m_NewLinkPin)
                    alpha = alpha * (48.0f / 255.0f);

                ed::BeginPin(output.ID, ed::PinKind::Output);
                ed::PinPivotAlignment(ImVec2(1.0f, 0.5f));
                ed::PinPivotSize(ImVec2(0, 0));
                ImGui::BeginHorizontal(output.ID.AsPointer());
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
                if (!output.Name.empty())
                {
                    ImGui::TextUnformatted(output.Name.c_str());
                    ImGui::Spring(0);
                }
                DrawPinIcon(output, IsPinLinked(output.ID), (int)(alpha * 255));
                ImGui::Spring(0, ImGui::GetStyle().ItemSpacing.x / 2);
                ImGui::EndHorizontal();
                ImGui::PopStyleVar();
                ed::EndPin();

                //DrawItemRect(ImColor(255, 0, 0));
            }
            ImGui::Spring(1, 0);
            ImGui::EndVertical();
            ImGui::Spring(0, ImGui::GetStyle().ItemSpacing.x / 2);
        }
        else
            ImGui::Spring(0);
        m_Builder.EndHeader();

        // Draw Inputs
        for (auto& input : node.Inputs)
        {
            auto alpha = ImGui::GetStyle().Alpha;
            if (m_NewLinkPin && !CanCreateLink(m_NewLinkPin, &input) && &input != m_NewLinkPin)
                alpha = alpha * (48.0f / 255.0f);

            m_Builder.Input(input.ID);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
            DrawPinIcon(input, IsPinLinked(input.ID), (int)(alpha * 255));
            ImGui::Spring(0);

            // types
            if ((node.Name == "IntArray" || node.Name == "StringArray") && !IsPinLinked(input.ID))
            {
                static bool wasActive = false;
                ImGui::PushItemWidth(100.0f);
                if (input.Value.has_value())
                {
                    std::string& str = std::any_cast<std::string>(input.Value);
                    char buffer[128];
                    memset(buffer, 0, sizeof(buffer));
                    strcpy_s(buffer, sizeof(buffer), str.c_str());
                    try {
                        if (ImGui::InputText("##edit", buffer, sizeof(buffer)))
                        {
                            input.Value = std::string(buffer);
                        }
                    }
                    catch (const std::bad_any_cast& e) {
                        ETH_CORE_INFO("{0}", e.what());
                    }
                }
                else
                {
                    input.Value = std::string("0");
                }
                ImGui::PopItemWidth();
                if (ImGui::IsItemActive() && !wasActive)
                {
                    ed::EnableShortcuts(false);
                    wasActive = true;
                }
                else if (!ImGui::IsItemActive() && wasActive)
                {
                    ed::EnableShortcuts(true);
                    wasActive = false;
                }
                ImGui::Spring(0);
            }


            if (!input.Name.empty())
            {
                ImGui::TextUnformatted(input.Name.c_str());
                ImGui::Spring(0);
            }
            ImGui::PopStyleVar();
            m_Builder.EndInput();
        }

        // Draw simple main body
        m_Builder.Middle();
        ImGui::Spring(1, 0);
        if (node.Name == "GameStateShadowClone")
        {
            if (ImGui::Button("+"))
            {
                node.Outputs.emplace_back(GetNextId(), "branch", PinType::Flow);
                BuildNode(&node);
            }
            ImGui::SameLine();
            if (ImGui::Button("-"))
            {
                node.Outputs.pop_back();
            }
            ImGui::TextUnformatted("Add/Remove State");
        }
        else if (node.Name == "Switch")
        {
            if (ImGui::Button("+"))
            {
                node.Inputs.emplace_back(GetNextId(), "compare", PinType::String);
                BuildNode(&node);
            }
            ImGui::SameLine();
            if (ImGui::Button("-"))
            {
                node.Inputs.pop_back();
            }
            ImGui::TextUnformatted("Compare val");
        }

        // types
        std::string pinVal;
        PinType pinType;
        if (node.Name == "IntArray")
        {
            ButtonAddAndRemove(node, "int", PinType::Int);
        }
        else if (node.Name == "StringArray")
        {
            ButtonAddAndRemove(node, "string", PinType::String);
        }
        else if (node.Name == "CastToIntArray")
        {
            ButtonAddAndRemove(node, "flow", PinType::Flow);
        }

        ImGui::Spring(1, 0);

        // Draw Output
        for (uint32_t i = 0; i < node.Outputs.size(); ++i)
        {
            auto& output = node.Outputs[i];
            if (output.Type == PinType::Delegate)
                continue;

            auto alpha = ImGui::GetStyle().Alpha;
            if (m_NewLinkPin && !CanCreateLink(m_NewLinkPin, &output) && &output != m_NewLinkPin)
                alpha = alpha * (48.0f / 255.0f);

            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
            m_Builder.Output(output.ID);

            // types
            if (node.Name == "Int")
            {
                static bool wasActive = false;
                ImGui::PushItemWidth(100.0f);
                if (output.Value.has_value())
                {
                    std::string& str = std::any_cast<std::string>(output.Value);
                    char buffer[128];
                    memset(buffer, 0, sizeof(buffer));
                    strcpy_s(buffer, sizeof(buffer), str.c_str());
                    try {
                        if (ImGui::InputText("##edit", buffer, sizeof(buffer)))
                        {
                            output.Value = std::string(buffer);
                        }
                    }
                    catch (const std::bad_any_cast& e) {
                        ETH_CORE_INFO("{0}", e.what());
                    }
                }
                else
                {
                    output.Value = std::string("0");
                }
                ImGui::PopItemWidth();
                if (ImGui::IsItemActive() && !wasActive)
                {
                    ed::EnableShortcuts(false);
                    wasActive = true;
                }
                else if (!ImGui::IsItemActive() && wasActive)
                {
                    ed::EnableShortcuts(true);
                    wasActive = false;
                }
                ImGui::Spring(0);
            }


            if (!output.Name.empty())
            {
                ImGui::Spring(0);
                ImGui::TextUnformatted((output.Name + std::to_string(i)).c_str());
            }
            ImGui::Spring(0);
            DrawPinIcon(output, IsPinLinked(output.ID), (int)(alpha * 255));
            ImGui::PopStyleVar();
            m_Builder.EndOutput();
        }

        m_Builder.End();
	}
}