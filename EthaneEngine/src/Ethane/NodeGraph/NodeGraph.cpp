#include "ethpch.h"
#include "NodeGraph.h"

#include "Ethane/Utils/PlatformUtils.h"

#include <imgui.h>

# define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>

#include "Ethane/Asset/SerializerYaml.h"

// TODO: test
#include <format>

namespace ed = ax::NodeEditor;

namespace Ethane::NodeGraph {

	NodeGraph::NodeGraph()
	{
        Init();
	}

    void NodeGraph::Init()
    {
        ed::Config config;

        config.SettingsFile = "Blueprints.json";
        config.UserPointer = this;
        config.LoadNodeSettings = [](ed::NodeId nodeId, char* data, void* userPointer) -> size_t
        {
            auto self = static_cast<NodeGraph*>(userPointer);

            auto node = self->FindNode(nodeId);
            if (!node)
                return 0;

            if (data != nullptr)
                memcpy(data, node->State.data(), node->State.size());
            return node->State.size();
        };

        config.SaveNodeSettings = [](ed::NodeId nodeId, const char* data, size_t size, ed::SaveReasonFlags reason, void* userPointer) -> bool
        {
            auto self = static_cast<NodeGraph*>(userPointer);

            auto node = self->FindNode(nodeId);
            if (!node)
                return false;

            node->State.assign(data, size);

            self->TouchNode(nodeId);

            return true;
        };

        m_Context = ed::CreateEditor(&config);
        ed::SetCurrentEditor(m_Context);

        // Node* node;
        // node = SpawnInputActionNode();      ed::SetNodePosition(node->ID, ImVec2(-252, 220));
        // node = SpawnBranchNode();           ed::SetNodePosition(node->ID, ImVec2(-300, 351));
        // node = SpawnDoNNode();              ed::SetNodePosition(node->ID, ImVec2(-238, 504));
        // node = SpawnOutputActionNode();     ed::SetNodePosition(node->ID, ImVec2(71, 80));
        // node = SpawnSetTimerNode();         ed::SetNodePosition(node->ID, ImVec2(168, 316));
        // 
        // node = SpawnTreeSequenceNode();     ed::SetNodePosition(node->ID, ImVec2(1000, 329));
        // node = SpawnTreeTaskNode();         ed::SetNodePosition(node->ID, ImVec2(1204, 458));
        // node = SpawnTreeTask2Node();        ed::SetNodePosition(node->ID, ImVec2(868, 538));
        // 
        // node = SpawnComment();              ed::SetNodePosition(node->ID, ImVec2(112, 576)); ed::SetGroupSize(node->ID, ImVec2(384, 154));
        // node = SpawnComment();              ed::SetNodePosition(node->ID, ImVec2(800, 224)); ed::SetGroupSize(node->ID, ImVec2(640, 400));
        // 
        // node = SpawnLessNode();             ed::SetNodePosition(node->ID, ImVec2(366, 652));
        // node = SpawnWeirdNode();            ed::SetNodePosition(node->ID, ImVec2(144, 652));
        // node = SpawnMessageNode();          ed::SetNodePosition(node->ID, ImVec2(-348, 698));
        // node = SpawnPrintStringNode();      ed::SetNodePosition(node->ID, ImVec2(-69, 652));
        // 
        // node = SpawnHoudiniTransformNode(); ed::SetNodePosition(node->ID, ImVec2(500, -70));
        // node = SpawnHoudiniGroupNode();     ed::SetNodePosition(node->ID, ImVec2(500, 42));

        ed::NavigateToContent();

        BuildNodes();

        // m_Links.push_back(Link(GetNextLinkId().Get(), m_Nodes[5].Outputs[0].ID, m_Nodes[6].Inputs[0].ID));
        // m_Links.push_back(Link(GetNextLinkId().Get(), m_Nodes[5].Outputs[0].ID, m_Nodes[7].Inputs[0].ID));
        // m_Links.push_back(Link(GetNextLinkId().Get(), m_Nodes[14].Outputs[0].ID, m_Nodes[15].Inputs[0].ID));

        // Load Textures
        m_HeaderBackground =    Texture2D::Create("resources/icons/NodeGraph/BlueprintBackground.png");
        m_SaveIcon =            Texture2D::Create("resources/icons/NodeGraph/ic_save_white_24dp.png");
        m_RestoreIcon =         Texture2D::Create("resources/icons/NodeGraph/ic_restore_white_24dp.png");

        // Load Rules
        {
            auto& rule = m_LinkRules.emplace_back();
            rule.ViolateRule = [](Pin* startPin, Pin* endPin) {return endPin == startPin; };
        }
        {
            auto& rule = m_LinkRules.emplace_back();
            rule.ViolateRule = [](Pin* startPin, Pin* endPin) {return endPin->Kind == startPin->Kind; };
            rule.ErrorMessage = "x Incompatible Pin Kind";
        }
        {
            auto& rule = m_LinkRules.emplace_back();
            rule.ViolateRule = [](Pin* startPin, Pin* endPin) {return endPin->Node == startPin->Node; };
            rule.ErrorMessage = "x Cannot connect to self";
        }
        {
            auto& rule = m_LinkRules.emplace_back();
            rule.ViolateRule = [](Pin* startPin, Pin* endPin) {return endPin->Type != startPin->Type; };
            rule.ErrorMessage = "x Incompatible Pin Type";
            rule.ErrorLinkColor = ImColor(255, 128, 128);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    static inline ImRect ImGui_GetItemRect()
    {
        return ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
    }

    static inline ImRect ImRect_Expanded(const ImRect& rect, float x, float y)
    {
        auto result = rect;
        result.Min.x -= x;
        result.Min.y -= y;
        result.Max.x += x;
        result.Max.y += y;
        return result;
    }

    static bool Splitter(bool split_vertically, float thickness, float* size1, float* size2, float min_size1, float min_size2, float splitter_long_axis_size = -1.0f)
    {
        using namespace ImGui;
        ImGuiContext& g = *GImGui;
        ImGuiWindow* window = g.CurrentWindow;
        ImGuiID id = window->GetID("##Splitter");
        ImRect bb;
        bb.Min = window->DC.CursorPos + (split_vertically ? ImVec2(*size1, 0.0f) : ImVec2(0.0f, *size1));
        bb.Max = bb.Min + CalcItemSize(split_vertically ? ImVec2(thickness, splitter_long_axis_size) : ImVec2(splitter_long_axis_size, thickness), 0.0f, 0.0f);
        return SplitterBehavior(bb, id, split_vertically ? ImGuiAxis_X : ImGuiAxis_Y, size1, size2, min_size1, min_size2, 0.0f);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    int NodeGraph::GetNextId()
    {
        return UUID();
        // return m_NextId++;
    }

    ed::LinkId NodeGraph::GetNextLinkId()
    {
        return ed::LinkId(GetNextId());
    }

    void NodeGraph::TouchNode(ed::NodeId id)
    {
        m_NodeTouchTime[id] = m_TouchTime;
    }

    float NodeGraph::GetTouchProgress(ed::NodeId id)
    {
        auto it = m_NodeTouchTime.find(id);
        if (it != m_NodeTouchTime.end() && it->second > 0.0f)
            return (m_TouchTime - it->second) / m_TouchTime;
        else
            return 0.0f;
    }

    void NodeGraph::UpdateTouch()
    {
        const auto deltaTime = ImGui::GetIO().DeltaTime;
        for (auto& entry : m_NodeTouchTime)
        {
            if (entry.second > 0.0f)
                entry.second -= deltaTime;
        }
    }

    Node* NodeGraph::FindNode(ed::NodeId id)
    {
        for (auto& node : m_Nodes)
            if (node.ID == id)
                return &node;

        return nullptr;
    }

    Link* NodeGraph::FindLink(ed::LinkId id)
    {
        for (auto& link : m_Links)
            if (link.ID == id)
                return &link;

        return nullptr;
    }

    Pin* NodeGraph::FindPin(ed::PinId id)
    {
        if (!id)
            return nullptr;

        for (auto& node : m_Nodes)
        {
            for (auto& pin : node.Inputs)
                if (pin.ID == id)
                    return &pin;

            for (auto& pin : node.Outputs)
                if (pin.ID == id)
                    return &pin;
        }

        return nullptr;
    }

    bool NodeGraph::IsPinLinked(ed::PinId id)
    {
        if (!id)
            return false;

        for (auto& link : m_Links)
            if (link.StartPinID == id || link.EndPinID == id)
                return true;

        return false;
    }

    bool NodeGraph::CanCreateLink(Pin* a, Pin* b)
    {
        if (!a || !b)
            return false;
        return IsLinkValid(a, b);
    }

    void NodeGraph::BuildNode(Node* node)
    {
        for (auto& input : node->Inputs)
        {
            input.Node = node;
            input.Kind = PinDirection::Input;
        }

        for (auto& output : node->Outputs)
        {
            output.Node = node;
            output.Kind = PinDirection::Output;
        }
    }

    void NodeGraph::BuildNodes()
    {
        for (auto& node : m_Nodes)
            BuildNode(&node);
    }

    Node* NodeGraph::SpawnInputActionNode()
    {
        m_Nodes.emplace_back(GetNextId(), "InputAction Fire", ImColor(255, 128, 128));
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Delegate);
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "Pressed", PinType::Flow);
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "Released", PinType::Flow);

        BuildNode(&m_Nodes.back());

        return &m_Nodes.back();
    }

    Node* NodeGraph::SpawnBranchNode()
    {
        m_Nodes.emplace_back(GetNextId(), "Branch");
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "Condition", PinType::Bool);
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "True", PinType::Flow);
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "False", PinType::Flow);

        BuildNode(&m_Nodes.back());

        return &m_Nodes.back();
    }

    Node* NodeGraph::SpawnDoNNode()
    {
        m_Nodes.emplace_back(GetNextId(), "Do N");
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "Enter", PinType::Flow);
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "N", PinType::Int);
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "Reset", PinType::Flow);
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "Exit", PinType::Flow);
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "Counter", PinType::Int);

        BuildNode(&m_Nodes.back());

        return &m_Nodes.back();
    }

    Node* NodeGraph::SpawnOutputActionNode()
    {
        m_Nodes.emplace_back(GetNextId(), "OutputAction");
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "Sample", PinType::Float);
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "Condition", PinType::Bool);
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "Event", PinType::Delegate);

        BuildNode(&m_Nodes.back());

        return &m_Nodes.back();
    }

    Node* NodeGraph::SpawnPrintStringNode()
    {
        m_Nodes.emplace_back(GetNextId(), "Print String");
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "In String", PinType::String);
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Flow);

        BuildNode(&m_Nodes.back());

        return &m_Nodes.back();
    }

    Node* NodeGraph::SpawnMessageNode()
    {
        m_Nodes.emplace_back(GetNextId(), "", ImColor(128, 195, 248));
        m_Nodes.back().Type = NodeType::Simple;
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "Message", PinType::String);

        BuildNode(&m_Nodes.back());

        return &m_Nodes.back();
    }

    Node* NodeGraph::SpawnSetTimerNode()
    {
        m_Nodes.emplace_back(GetNextId(), "Set Timer", ImColor(128, 195, 248));
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "Object", PinType::Object);
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "Function Name", PinType::Function);
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "Time", PinType::Float);
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "Looping", PinType::Bool);
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Flow);

        BuildNode(&m_Nodes.back());

        return &m_Nodes.back();
    }

    Node* NodeGraph::SpawnLessNode()
    {
        m_Nodes.emplace_back(GetNextId(), "<", ImColor(128, 195, 248));
        m_Nodes.back().Type = NodeType::Simple;
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Float);
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Float);
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Float);

        BuildNode(&m_Nodes.back());

        return &m_Nodes.back();
    }

    Node* NodeGraph::SpawnWeirdNode()
    {
        m_Nodes.emplace_back(GetNextId(), "o.O", ImColor(128, 195, 248));
        m_Nodes.back().Type = NodeType::Simple;
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Float);
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Float);
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Float);

        BuildNode(&m_Nodes.back());

        return &m_Nodes.back();
    }

    Node* NodeGraph::SpawnTraceByChannelNode()
    {
        m_Nodes.emplace_back(GetNextId(), "Single Line Trace by Channel", ImColor(255, 128, 64));
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "Start", PinType::Flow);
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "End", PinType::Int);
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "Trace Channel", PinType::Float);
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "Trace Complex", PinType::Bool);
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "Actors to Ignore", PinType::Int);
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "Draw Debug Type", PinType::Bool);
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "Ignore Self", PinType::Bool);
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Flow);
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "Out Hit", PinType::Float);
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "Return Value", PinType::Bool);

        BuildNode(&m_Nodes.back());

        return &m_Nodes.back();
    }

    Node* NodeGraph::SpawnTreeSequenceNode()
    {
        m_Nodes.emplace_back(GetNextId(), "Sequence");
        m_Nodes.back().Type = NodeType::Tree;
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Flow);

        BuildNode(&m_Nodes.back());

        return &m_Nodes.back();
    }

    Node* NodeGraph::SpawnTreeTaskNode()
    {
        m_Nodes.emplace_back(GetNextId(), "Move To");
        m_Nodes.back().Type = NodeType::Tree;
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);

        BuildNode(&m_Nodes.back());

        return &m_Nodes.back();
    }

    Node* NodeGraph::SpawnTreeTask2Node()
    {
        m_Nodes.emplace_back(GetNextId(), "Random Wait");
        m_Nodes.back().Type = NodeType::Tree;
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);

        BuildNode(&m_Nodes.back());

        return &m_Nodes.back();
    }

    Node* NodeGraph::SpawnComment()
    {
        m_Nodes.emplace_back(GetNextId(), "Test Comment");
        m_Nodes.back().Type = NodeType::Comment;
        m_Nodes.back().Size = ImVec2(300, 200);

        return &m_Nodes.back();
    }

    Node* NodeGraph::SpawnHoudiniTransformNode()
    {
        m_Nodes.emplace_back(GetNextId(), "Transform");
        m_Nodes.back().Type = NodeType::Houdini;
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Flow);

        BuildNode(&m_Nodes.back());

        return &m_Nodes.back();
    }

    Node* NodeGraph::SpawnHoudiniGroupNode()
    {
        m_Nodes.emplace_back(GetNextId(), "Group");
        m_Nodes.back().Type = NodeType::Houdini;
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Flow);

        BuildNode(&m_Nodes.back());

        return &m_Nodes.back();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    ImColor NodeGraph::GetIconColor(PinType type)
    {
        switch (type)
        {
        default:
        case PinType::Flow:     return ImColor(255, 255, 255);
        case PinType::Bool:     return ImColor(220, 48, 48);
        case PinType::Int:      return ImColor(68, 201, 156);
        case PinType::Float:    return ImColor(147, 226, 74);
        case PinType::String:   return ImColor(124, 21, 153);
        case PinType::Object:   return ImColor(51, 150, 215);
        case PinType::Function: return ImColor(218, 0, 183);
        case PinType::Delegate: return ImColor(255, 48, 48);
        }
    };

    void NodeGraph::DrawPinIcon(const Pin& pin, bool connected, int alpha)
    {
        ax::Drawing::IconType iconType;
        ImColor  color = GetIconColor(pin.Type);
        color.Value.w = alpha / 255.0f;
        switch (pin.Type)
        {
        case PinType::Flow:     iconType = ax::Drawing::IconType::Flow;   break;
        case PinType::Bool:     iconType = ax::Drawing::IconType::Circle; break;
        case PinType::Int:      iconType = ax::Drawing::IconType::Circle; break;
        case PinType::Float:    iconType = ax::Drawing::IconType::Circle; break;
        case PinType::String:   iconType = ax::Drawing::IconType::Circle; break;
        case PinType::Object:   iconType = ax::Drawing::IconType::Circle; break;
        case PinType::Function: iconType = ax::Drawing::IconType::Circle; break;
        case PinType::Delegate: iconType = ax::Drawing::IconType::Square; break;
        default:
            return;
        }

        ax::Widgets::Icon(ImVec2(static_cast<float>(m_CPinIconSize), static_cast<float>(m_CPinIconSize)), iconType, connected, color, ImColor(32, 32, 32, alpha));
    };

    void NodeGraph::ShowStyleEditor(bool* show)
    {
        if (!ImGui::Begin("Style", show))
        {
            ImGui::End();
            return;
        }

        auto paneWidth = ImGui::GetContentRegionAvail().x;

        auto& editorStyle = ed::GetStyle();
        ImGui::BeginHorizontal("Style buttons", ImVec2(paneWidth, 0), 1.0f);
        ImGui::TextUnformatted("Values");
        ImGui::Spring();
        if (ImGui::Button("Reset to defaults"))
            editorStyle = ed::Style();
        ImGui::EndHorizontal();
        ImGui::Spacing();

        ImGui::DragFloat4("Node Padding", &editorStyle.NodePadding.x, 0.1f, 0.0f, 40.0f);
        ImGui::DragFloat("Node Rounding", &editorStyle.NodeRounding, 0.1f, 0.0f, 40.0f);
        ImGui::DragFloat("Node Border Width", &editorStyle.NodeBorderWidth, 0.1f, 0.0f, 15.0f);
        ImGui::DragFloat("Hovered Node Border Width", &editorStyle.HoveredNodeBorderWidth, 0.1f, 0.0f, 15.0f);
        ImGui::DragFloat("Selected Node Border Width", &editorStyle.SelectedNodeBorderWidth, 0.1f, 0.0f, 15.0f);
        ImGui::DragFloat("Pin Rounding", &editorStyle.PinRounding, 0.1f, 0.0f, 40.0f);
        ImGui::DragFloat("Pin Border Width", &editorStyle.PinBorderWidth, 0.1f, 0.0f, 15.0f);
        ImGui::DragFloat("Link Strength", &editorStyle.LinkStrength, 1.0f, 0.0f, 500.0f);
        ImGui::DragFloat("Scroll Duration", &editorStyle.ScrollDuration, 0.001f, 0.0f, 2.0f);
        ImGui::DragFloat("Flow Marker Distance", &editorStyle.FlowMarkerDistance, 1.0f, 1.0f, 200.0f);
        ImGui::DragFloat("Flow Speed", &editorStyle.FlowSpeed, 1.0f, 1.0f, 2000.0f);
        ImGui::DragFloat("Flow Duration", &editorStyle.FlowDuration, 0.001f, 0.0f, 5.0f);
        ImGui::DragFloat("Group Border Width", &editorStyle.GroupBorderWidth, 0.1f, 0.0f, 15.0f);

        ImGui::Separator();

        static ImGuiColorEditFlags edit_mode = ImGuiColorEditFlags_DisplayRGB;
        ImGui::BeginHorizontal("Color Mode", ImVec2(paneWidth, 0), 1.0f);
        ImGui::TextUnformatted("Filter Colors");
        ImGui::Spring();
        ImGui::RadioButton("RGB", &edit_mode, ImGuiColorEditFlags_DisplayRGB);
        ImGui::Spring(0);
        ImGui::RadioButton("HSV", &edit_mode, ImGuiColorEditFlags_DisplayHSV);
        ImGui::Spring(0);
        ImGui::RadioButton("HEX", &edit_mode, ImGuiColorEditFlags_DisplayHex);
        ImGui::EndHorizontal();

        static ImGuiTextFilter filter;
        filter.Draw("", paneWidth);

        ImGui::Spacing();

        ImGui::PushItemWidth(-160);
        for (int i = 0; i < ed::StyleColor_Count; ++i)
        {
            auto name = ed::GetStyleColorName((ed::StyleColor)i);
            if (!filter.PassFilter(name))
                continue;

            ImGui::ColorEdit4(name, &editorStyle.Colors[i].x, edit_mode);
        }
        ImGui::PopItemWidth();

        ImGui::End();
    }


#if 1

    void NodeGraph::ShowLeftPane(float paneWidth)
    {
        auto& io = ImGui::GetIO();

        ImGui::BeginChild("Selection", ImVec2(paneWidth, 0));

        paneWidth = ImGui::GetContentRegionAvail().x;

        static bool showStyleEditor = false;
        ImGui::BeginHorizontal("Style Editor", ImVec2(paneWidth, 0));
        ImGui::Spring(0.0f, 0.0f);
        if (ImGui::Button("Zoom to Content"))
            ed::NavigateToContent();
        ImGui::Spring(0.0f);
        if (ImGui::Button("Show Flow"))
        {
            for (auto& link : m_Links)
                ed::Flow(link.ID);
        }
        ImGui::Spring();
        if (ImGui::Button("Open"))
            OnOpenGraphFile();
        ImGui::Spring();
        if (ImGui::Button("Save"))
            OnSave();
        ImGui::Spring();
        if (ImGui::Button("Compile"))
            OnCompile();
        ImGui::Spring(0.0f);
        if (ImGui::Button("Edit Style"))
            showStyleEditor = true;
        ImGui::EndHorizontal();
        ImGui::Checkbox("Show Ordinals", &m_ShowOrdinals);

        if (showStyleEditor)
            ShowStyleEditor(&showStyleEditor);

        std::vector<ed::NodeId> selectedNodes;
        std::vector<ed::LinkId> selectedLinks;

        selectedNodes.resize(ed::GetSelectedObjectCount());
        selectedLinks.resize(ed::GetSelectedObjectCount());

        int nodeCount = ed::GetSelectedNodes(selectedNodes.data(), static_cast<int>(selectedNodes.size()));
        int linkCount = ed::GetSelectedLinks(selectedLinks.data(), static_cast<int>(selectedLinks.size()));

        selectedNodes.resize(nodeCount);
        selectedLinks.resize(linkCount);

        int saveIconWidth = m_SaveIcon->GetWidth();
        int saveIconHeight = m_SaveIcon->GetWidth();
        int restoreIconWidth = m_RestoreIcon->GetWidth();
        int restoreIconHeight = m_RestoreIcon->GetWidth();

        ImGui::GetWindowDrawList()->AddRectFilled(
            ImGui::GetCursorScreenPos(),
            ImGui::GetCursorScreenPos() + ImVec2(paneWidth, ImGui::GetTextLineHeight()),
            ImColor(ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]), ImGui::GetTextLineHeight() * 0.25f);
        ImGui::Spacing(); ImGui::SameLine();
        ImGui::TextUnformatted("Nodes");
        ImGui::Indent();
        for (auto& node : m_Nodes)
        {
            ImGui::PushID(node.ID.AsPointer());
            auto start = ImGui::GetCursorScreenPos();

            if (const auto progress = GetTouchProgress(node.ID))
            {
                ImGui::GetWindowDrawList()->AddLine(
                    start + ImVec2(-8, 0),
                    start + ImVec2(-8, ImGui::GetTextLineHeight()),
                    IM_COL32(255, 0, 0, 255 - (int)(255 * progress)), 4.0f);
            }

            bool isSelected = std::find(selectedNodes.begin(), selectedNodes.end(), node.ID) != selectedNodes.end();
            if (ImGui::Selectable((node.Name + "##" + std::to_string(reinterpret_cast<uintptr_t>(node.ID.AsPointer()))).c_str(), &isSelected))
            {
                if (io.KeyCtrl)
                {
                    if (isSelected)
                        ed::SelectNode(node.ID, true);
                    else
                        ed::DeselectNode(node.ID);
                }
                else
                    ed::SelectNode(node.ID, false);

                ed::NavigateToSelection();
            }
            if (ImGui::IsItemHovered() && !node.State.empty())
                ImGui::SetTooltip("State: %s", node.State.c_str());

            auto id = std::string("(") + std::to_string(reinterpret_cast<uintptr_t>(node.ID.AsPointer())) + ")";
            auto textSize = ImGui::CalcTextSize(id.c_str(), nullptr);
            auto iconPanelPos = start + ImVec2(
                paneWidth - ImGui::GetStyle().FramePadding.x - ImGui::GetStyle().IndentSpacing - saveIconWidth - restoreIconWidth - ImGui::GetStyle().ItemInnerSpacing.x * 1,
                (ImGui::GetTextLineHeight() - saveIconHeight) / 2);
            ImGui::GetWindowDrawList()->AddText(
                ImVec2(iconPanelPos.x - textSize.x - ImGui::GetStyle().ItemInnerSpacing.x, start.y),
                IM_COL32(255, 255, 255, 255), id.c_str(), nullptr);

            auto drawList = ImGui::GetWindowDrawList();
            ImGui::SetCursorScreenPos(iconPanelPos);
            ImGui::SetItemAllowOverlap();
            if (node.SavedState.empty())
            {
                if (ImGui::InvisibleButton("save", ImVec2((float)saveIconWidth, (float)saveIconHeight)))
                    node.SavedState = node.State;

                if (ImGui::IsItemActive())
                    drawList->AddImage(UIImage(m_SaveIcon).GetTextureID(), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 96));
                else if (ImGui::IsItemHovered())
                    drawList->AddImage(UIImage(m_SaveIcon).GetTextureID(), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 255));
                else
                    drawList->AddImage(UIImage(m_SaveIcon).GetTextureID(), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 160));
            }
            else
            {
                ImGui::Dummy(ImVec2((float)saveIconWidth, (float)saveIconHeight));
                drawList->AddImage(UIImage(m_SaveIcon).GetTextureID(), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 32));
            }

            ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
            ImGui::SetItemAllowOverlap();
            if (!node.SavedState.empty())
            {
                if (ImGui::InvisibleButton("restore", ImVec2((float)restoreIconWidth, (float)restoreIconHeight)))
                {
                    node.State = node.SavedState;
                    ed::RestoreNodeState(node.ID);
                    node.SavedState.clear();
                }

                if (ImGui::IsItemActive())
                    drawList->AddImage(UIImage(m_RestoreIcon).GetTextureID(), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 96));
                else if (ImGui::IsItemHovered())
                    drawList->AddImage(UIImage(m_RestoreIcon).GetTextureID(), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 255));
                else
                    drawList->AddImage(UIImage(m_RestoreIcon).GetTextureID(), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 160));
            }
            else
            {
                ImGui::Dummy(ImVec2((float)restoreIconWidth, (float)restoreIconHeight));
                drawList->AddImage(UIImage(m_RestoreIcon).GetTextureID(), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 32));
            }

            ImGui::SameLine(0, 0);
            ImGui::SetItemAllowOverlap();
            ImGui::Dummy(ImVec2(0, (float)restoreIconHeight));

            ImGui::PopID();
        }
        ImGui::Unindent();

        static int changeCount = 0;

        ImGui::GetWindowDrawList()->AddRectFilled(
            ImGui::GetCursorScreenPos(),
            ImGui::GetCursorScreenPos() + ImVec2(paneWidth, ImGui::GetTextLineHeight()),
            ImColor(ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]), ImGui::GetTextLineHeight() * 0.25f);
        ImGui::Spacing(); ImGui::SameLine();
        ImGui::TextUnformatted("Selection");

        ImGui::BeginHorizontal("Selection Stats", ImVec2(paneWidth, 0));
        ImGui::Text("Changed %d time%s", changeCount, changeCount > 1 ? "s" : "");
        ImGui::Spring();
        if (ImGui::Button("Deselect All"))
            ed::ClearSelection();
        ImGui::EndHorizontal();
        ImGui::Indent();
        for (int i = 0; i < nodeCount; ++i) ImGui::Text("Node (%p)", selectedNodes[i].AsPointer());
        for (int i = 0; i < linkCount; ++i) ImGui::Text("Link (%p)", selectedLinks[i].AsPointer());
        ImGui::Unindent();

        if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Z)))
            for (auto& link : m_Links)
                ed::Flow(link.ID);

        if (ed::HasSelectionChanged())
            ++changeCount;

        ImGui::EndChild();
    }


#endif

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	void NodeGraph::OnImGuiRender()
	{
        UpdateTouch();

        ImGui::Begin("test");
        static bool firstframe = true; // Used to position the nodes on startup
        auto& io = ImGui::GetIO();

        // FPS Counter Ribbon
        ImGui::Text("FPS: %.2f (%.2gms)", io.Framerate, io.Framerate ? 1000.0f / io.Framerate : 0.0f);
        ImGui::Separator();

        static ed::NodeId contextNodeId = 0;
        static ed::LinkId contextLinkId = 0;
        static ed::PinId  contextPinId = 0;
        // m_CreateNewNode = false;
        // m_NewNodeLinkPin = nullptr;
        // static Pin* newLinkPin = nullptr;

        static float leftPaneWidth = 400.0f;
        static float rightPaneWidth = 800.0f;
        Splitter(true, 4.0f, &leftPaneWidth, &rightPaneWidth, 50.0f, 50.0f);

        ShowLeftPane(leftPaneWidth - 4.0f);
        ImGui::SameLine(0.0f, 12.0f);

        // Node Editor Widget
        ed::SetCurrentEditor(m_Context);
        ed::Begin("My Editor"); //, ImVec2(0.0, 0.0f)
        uint32_t uniqueId = 1;

        {
            auto cursorTopLeft = ImGui::GetCursorScreenPos();
            m_Builder = ed::Utilities::BlueprintNodeBuilder(UIImage(m_HeaderBackground).GetTextureID(), m_HeaderBackground->GetWidth(), m_HeaderBackground->GetHeight());

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Draw Nodes
            for (auto& node : m_Nodes)
            {
                if (node.Type == NodeType::Blueprint || node.Type == NodeType::Simple)
                    M_DrawSimpleBlueprintNode(node);
                else if (node.Type == NodeType::Comment)
                    M_DrawCommentNode(node);
                else if (node.Type == NodeType::Houdini)
                    M_DrawHoudiniNode(node);
                else if (node.Type == NodeType::Tree)
                    M_DrawTreeNode(node);
                else
                    DrawCustomNode(node);
            }
        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Create Links/Nodes
            for (auto& link : m_Links)
                ed::Link(link.ID, link.StartPinID, link.EndPinID, link.Color, 2.0f);

            if (!m_CreateNewNode)
            {
                if (ed::BeginCreate(ImColor(255, 255, 255), 2.0f))
                {
                    auto showLabel = [](const char* label, ImColor color)
                    {
                        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - ImGui::GetTextLineHeight());
                        auto size = ImGui::CalcTextSize(label);

                        auto padding = ImGui::GetStyle().FramePadding;
                        auto spacing = ImGui::GetStyle().ItemSpacing;

                        ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(spacing.x, -spacing.y));

                        auto rectMin = ImGui::GetCursorScreenPos() - padding;
                        auto rectMax = ImGui::GetCursorScreenPos() + size + padding;

                        auto drawList = ImGui::GetWindowDrawList();
                        drawList->AddRectFilled(rectMin, rectMax, color, size.y * 0.15f);
                        ImGui::TextUnformatted(label);
                    };

                    ed::PinId startPinId = 0, endPinId = 0;
                    if (ed::QueryNewLink(&startPinId, &endPinId))
                    {
                        auto startPin = FindPin(startPinId);
                        auto endPin = FindPin(endPinId);

                        m_NewLinkPin = startPin ? startPin : endPin;

                        if (startPin->Kind == PinDirection::Input)
                        {
                            std::swap(startPin, endPin);
                            std::swap(startPinId, endPinId);
                        }

                        if (startPin && endPin)
                        {
                            LinkRule vrule;
                            if (!IsLinkValid(startPin, endPin, &vrule))
                            {
                                showLabel(vrule.ErrorMessage.c_str(), vrule.ErrorLabelColor);
                                ed::RejectNewItem(vrule.ErrorLinkColor, vrule.LinkThickness);
                            }
                            else
                            {
                                showLabel("+ Create Link", ImColor(32, 45, 32, 180));
                                if (ed::AcceptNewItem(ImColor(128, 255, 128), 4.0f))
                                {
                                    m_Links.emplace_back(Link(GetNextId(), startPinId, endPinId));
                                    m_Links.back().Color = GetIconColor(startPin->Type);
                                }
                            }
                        }
                    }

                    ed::PinId pinId = 0;
                    if (ed::QueryNewNode(&pinId))
                    {
                        m_NewLinkPin = FindPin(pinId);
                        if (m_NewLinkPin)
                            showLabel("+ Create Node", ImColor(32, 45, 32, 180));

                        if (ed::AcceptNewItem())
                        {
                            m_CreateNewNode = true;
                            m_NewNodeLinkPin = FindPin(pinId);
                            m_NewLinkPin = nullptr;
                            ed::Suspend();
                            ImGui::OpenPopup("Create New Node");
                            ed::Resume();
                        }
                    }
                }
                else
                    m_NewLinkPin = nullptr;

                ed::EndCreate();

                // Delete Node/Link
                M_HandleDelete();
                
            }

            ImGui::SetCursorScreenPos(cursorTopLeft);
        }

        // Create Links/Nodes
        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Popup
        ed::Suspend();
        if (ed::ShowNodeContextMenu(&contextNodeId))
            ImGui::OpenPopup("Node Context Menu");
        else if (ed::ShowPinContextMenu(&contextPinId))
            ImGui::OpenPopup("Pin Context Menu");
        else if (ed::ShowLinkContextMenu(&contextLinkId))
            ImGui::OpenPopup("Link Context Menu");
        else if (ed::ShowBackgroundContextMenu())
        {
            ImGui::OpenPopup("Create New Node");
            m_NewNodeLinkPin = nullptr;
        }
        ed::Resume();

        ed::Suspend();
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
        if (ImGui::BeginPopup("Node Context Menu"))
        {
            auto node = FindNode(contextNodeId);

            ImGui::TextUnformatted("Node Context Menu");
            ImGui::Separator();
            if (node)
            {
                ImGui::Text("ID: %d", (uint32_t)(node->ID.Get()));
                ImGui::Text("ID(pointer): %p", node->ID.AsPointer());
                ImGui::Text("Type: %s", node->Type == NodeType::Blueprint ? "Blueprint" : (node->Type == NodeType::Tree ? "Tree" : "Comment"));
                ImGui::Text("Inputs: %d", (int)node->Inputs.size());
                ImGui::Text("Outputs: %d", (int)node->Outputs.size());
            }
            else
                ImGui::Text("Unknown node: %p", contextNodeId.AsPointer());
            ImGui::Separator();
            if (ImGui::MenuItem("Delete"))
                ed::DeleteNode(contextNodeId);
            ImGui::EndPopup();
        }

        if (ImGui::BeginPopup("Pin Context Menu"))
        {
            auto pin = FindPin(contextPinId);

            ImGui::TextUnformatted("Pin Context Menu");
            ImGui::Separator();
            if (pin)
            {
                ImGui::Text("ID: %d", (uint32_t)(pin->ID.Get()));
                ImGui::Text("ID(pointer): %p", pin->ID.AsPointer());
                if (pin->Node)
                    ImGui::Text("Node: %p", pin->Node->ID.AsPointer());
                else
                    ImGui::Text("Node: %s", "<none>");
            }
            else
                ImGui::Text("Unknown pin: %p", contextPinId.AsPointer());

            ImGui::EndPopup();
        }

        if (ImGui::BeginPopup("Link Context Menu"))
        {
            auto link = FindLink(contextLinkId);

            ImGui::TextUnformatted("Link Context Menu");
            ImGui::Separator();
            if (link)
            {
                ImGui::Text("ID: %d", (uint32_t)(link->ID.Get()));
                ImGui::Text("ID(pointer): %p", link->ID.AsPointer());
                ImGui::Text("From: %p", link->StartPinID.AsPointer());
                ImGui::Text("To: %p", link->EndPinID.AsPointer());
            }
            else
                ImGui::Text("Unknown link: %p", contextLinkId.AsPointer());
            ImGui::Separator();
            if (ImGui::MenuItem("Delete"))
                ed::DeleteLink(contextLinkId);
            ImGui::EndPopup();
        }

        M_CreateNodePopup();

        ImGui::PopStyleVar();
        ed::Resume();
        // Popup end
        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ed::End();
        // ed::SetCurrentEditor(nullptr);
        firstframe = false;

        ImGui::End();
	}

    void NodeGraph::OnClose()
    {
        ed::DestroyEditor(m_Context);
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Helper Functions

    void NodeGraph::M_HandleDelete()
    {
        if (ed::BeginDelete())
        {
            ed::LinkId linkId = 0;
            while (ed::QueryDeletedLink(&linkId))
            {
                if (ed::AcceptDeletedItem())
                {
                    auto id = std::find_if(m_Links.begin(), m_Links.end(), [linkId](auto& link) { return link.ID == linkId; });
                    if (id != m_Links.end())
                        m_Links.erase(id);
                }
            }

            ed::NodeId nodeId = 0;
            while (ed::QueryDeletedNode(&nodeId))
            {
                if (ed::AcceptDeletedItem())
                {
                    auto id = std::find_if(m_Nodes.begin(), m_Nodes.end(), [nodeId](auto& node) { return node.ID == nodeId; });
                    if (id != m_Nodes.end())
                        m_Nodes.erase(id);
                }
            }
        }
        ed::EndDelete();
    }

    bool NodeGraph::IsLinkValid(Pin* startPin, Pin* endPin, LinkRule* violateRule)
    {
        for (auto& rule : m_LinkRules)
        {
            if ((rule.ViolateRule)(startPin, endPin))
            {
                if (violateRule)
                    violateRule = &rule;
                return false;
            }

        }
        return true;
    }

    void NodeGraph::M_DrawLink()
    {
        
    }

    void NodeGraph::M_CreateNodePopup()
    {
        if (ImGui::BeginPopup("Create New Node"))
        {
            auto openPopupPosition = ImGui::GetMousePos();

            auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
            auto viewportMaxRegion = ImGui::GetWindowContentRegionMax();
            auto viewportOffset = ImGui::GetWindowPos();
            auto newNodePostion = ImVec2{ openPopupPosition.x - viewportMinRegion.x, openPopupPosition.y - viewportMinRegion.y };
            // ETH_CORE_INFO("{0} {1}", );
            newNodePostion = ImVec2{ 50, 0};
            // newNodePostion = openPopupPosition;
            // ETH_CORE_INFO("{0} {1}", newNodePostion.x, newNodePostion.y);
            // ImGui::SetCursorScreenPos(ImGui::GetMousePosOnOpeningCurrentPopup());

            Node* node = nullptr;
            if (m_ShowDefaultCreateNodePopup)
            {
                if (ImGui::MenuItem("Input Action"))
                    node = SpawnInputActionNode();
                if (ImGui::MenuItem("Output Action"))
                    node = SpawnOutputActionNode();
                if (ImGui::MenuItem("Branch"))
                    node = SpawnBranchNode();
                if (ImGui::MenuItem("Do N"))
                    node = SpawnDoNNode();
                if (ImGui::MenuItem("Set Timer"))
                    node = SpawnSetTimerNode();
                if (ImGui::MenuItem("Less"))
                    node = SpawnLessNode();
                if (ImGui::MenuItem("Weird"))
                    node = SpawnWeirdNode();
                if (ImGui::MenuItem("Trace by Channel"))
                    node = SpawnTraceByChannelNode();
                if (ImGui::MenuItem("Print String"))
                    node = SpawnPrintStringNode();
                ImGui::Separator();
                if (ImGui::MenuItem("Comment"))
                    node = SpawnComment();
                ImGui::Separator();
                if (ImGui::MenuItem("Sequence"))
                    node = SpawnTreeSequenceNode();
                if (ImGui::MenuItem("Move To"))
                    node = SpawnTreeTaskNode();
                if (ImGui::MenuItem("Random Wait"))
                    node = SpawnTreeTask2Node();
                ImGui::Separator();
                if (ImGui::MenuItem("Message"))
                    node = SpawnMessageNode();
                ImGui::Separator();
                if (ImGui::MenuItem("Transform"))
                    node = SpawnHoudiniTransformNode();
                if (ImGui::MenuItem("Group"))
                    node = SpawnHoudiniGroupNode();
            }
            ImGui::Separator();
            node = CustomCreateNodePopup(node);

            if (node)
            {
                BuildNodes();

                m_CreateNewNode = false;

                ed::SetNodePosition(node->ID, newNodePostion);

                // Link node if it is able to do so
                if (auto startPin = m_NewNodeLinkPin)
                {
                    auto& pins = startPin->Kind == PinDirection::Input ? node->Outputs : node->Inputs;
                
                    for (auto& pin : pins)
                    {
                        if (CanCreateLink(startPin, &pin))
                        {
                            auto endPin = &pin;
                            if (startPin->Kind == PinDirection::Input)
                                std::swap(startPin, endPin);
                
                            m_Links.emplace_back(Link(GetNextId(), startPin->ID, endPin->ID));
                            m_Links.back().Color = GetIconColor(startPin->Type);
                
                            break;
                        }
                    }
                }
            }

            ImGui::EndPopup();
        }
        else
            m_CreateNewNode = false;
    }

    void NodeGraph::M_DrawSimpleBlueprintNode(Node& node)
    {
        const auto isSimple = node.Type == NodeType::Simple;

        bool hasOutputDelegates = false;
        for (auto& output : node.Outputs)
            if (output.Type == PinType::Delegate)
                hasOutputDelegates = true;

        m_Builder.Begin(node.ID);
        if (!isSimple)
        {
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
        }

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
            if (!input.Name.empty())
            {
                ImGui::TextUnformatted(input.Name.c_str());
                ImGui::Spring(0);
            }
            if (input.Type == PinType::Bool)
            {
                ImGui::Button("Hello");
                ImGui::Spring(0);
            }
            ImGui::PopStyleVar();
            m_Builder.EndInput();
        }

        // Draw simple main body
        if (isSimple)
        {
            m_Builder.Middle();

            ImGui::Spring(1, 0);
            ImGui::TextUnformatted(node.Name.c_str());
            ImGui::Spring(1, 0);
        }

        // Draw Output
        for (auto& output : node.Outputs)
        {
            if (!isSimple && output.Type == PinType::Delegate)
                continue;

            auto alpha = ImGui::GetStyle().Alpha;
            if (m_NewLinkPin && !CanCreateLink(m_NewLinkPin, &output) && &output != m_NewLinkPin)
                alpha = alpha * (48.0f / 255.0f);

            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
            m_Builder.Output(output.ID);
            if (output.Type == PinType::String)
            {
                static char buffer[128] = "Edit Me\nMultiline!";
                static bool wasActive = false;

                ImGui::PushItemWidth(100.0f);
                ImGui::InputText("##edit", buffer, 127);
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
                ImGui::TextUnformatted(output.Name.c_str());
            }
            ImGui::Spring(0);
            DrawPinIcon(output, IsPinLinked(output.ID), (int)(alpha * 255));
            ImGui::PopStyleVar();
            m_Builder.EndOutput();
        }

        m_Builder.End();
    }

    void NodeGraph::M_DrawCommentNode(Node& node)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, m_CCommentAlpha);
        ed::PushStyleColor(ed::StyleColor_NodeBg, ImColor(255, 255, 255, 64));
        ed::PushStyleColor(ed::StyleColor_NodeBorder, ImColor(255, 255, 255, 64));
        ed::BeginNode(node.ID);
        ImGui::PushID(node.ID.AsPointer());
        ImGui::BeginVertical("content");
        ImGui::BeginHorizontal("horizontal");
        ImGui::Spring(1);
        ImGui::TextUnformatted(node.Name.c_str());
        ImGui::Spring(1);
        ImGui::EndHorizontal();
        ed::Group(node.Size);
        ImGui::EndVertical();
        ImGui::PopID();
        ed::EndNode();
        ed::PopStyleColor(2);
        ImGui::PopStyleVar();

        if (ed::BeginGroupHint(node.ID))
        {
            //auto alpha   = static_cast<int>(commentAlpha * ImGui::GetStyle().Alpha * 255);
            auto bgAlpha = static_cast<int>(ImGui::GetStyle().Alpha * 255);

            //ImGui::PushStyleVar(ImGuiStyleVar_Alpha, commentAlpha * ImGui::GetStyle().Alpha);

            auto min = ed::GetGroupMin();
            //auto max = ed::GetGroupMax();

            ImGui::SetCursorScreenPos(min - ImVec2(-8, ImGui::GetTextLineHeightWithSpacing() + 4));
            ImGui::BeginGroup();
            ImGui::TextUnformatted(node.Name.c_str());
            ImGui::EndGroup();

            auto drawList = ed::GetHintBackgroundDrawList();

            auto hintBounds = ImGui_GetItemRect();
            auto hintFrameBounds = ImRect_Expanded(hintBounds, 8, 4);

            drawList->AddRectFilled(
                hintFrameBounds.GetTL(),
                hintFrameBounds.GetBR(),
                IM_COL32(255, 255, 255, 64 * bgAlpha / 255), 4.0f);

            drawList->AddRect(
                hintFrameBounds.GetTL(),
                hintFrameBounds.GetBR(),
                IM_COL32(255, 255, 255, 128 * bgAlpha / 255), 4.0f);

            //ImGui::PopStyleVar();
        }
        ed::EndGroupHint();
    }

    void NodeGraph::M_DrawHoudiniNode(Node& node)
    {
        const float rounding = 10.0f;
        const float padding = 12.0f;


        ed::PushStyleColor(ed::StyleColor_NodeBg, ImColor(229, 229, 229, 200));
        ed::PushStyleColor(ed::StyleColor_NodeBorder, ImColor(125, 125, 125, 200));
        ed::PushStyleColor(ed::StyleColor_PinRect, ImColor(229, 229, 229, 60));
        ed::PushStyleColor(ed::StyleColor_PinRectBorder, ImColor(125, 125, 125, 60));

        const auto pinBackground = ed::GetStyle().Colors[ed::StyleColor_NodeBg];

        ed::PushStyleVar(ed::StyleVar_NodePadding, ImVec4(0, 0, 0, 0));
        ed::PushStyleVar(ed::StyleVar_NodeRounding, rounding);
        ed::PushStyleVar(ed::StyleVar_SourceDirection, ImVec2(0.0f, 1.0f));
        ed::PushStyleVar(ed::StyleVar_TargetDirection, ImVec2(0.0f, -1.0f));
        ed::PushStyleVar(ed::StyleVar_LinkStrength, 0.0f);
        ed::PushStyleVar(ed::StyleVar_PinBorderWidth, 1.0f);
        ed::PushStyleVar(ed::StyleVar_PinRadius, 6.0f);
        ed::BeginNode(node.ID);

        ImGui::BeginVertical(node.ID.AsPointer());
        if (!node.Inputs.empty())
        {
            ImGui::BeginHorizontal("inputs");
            ImGui::Spring(1, 0);

            ImRect inputsRect;
            int inputAlpha = 200;
            for (auto& pin : node.Inputs)
            {
                ImGui::Dummy(ImVec2(padding, padding));
                inputsRect = ImGui_GetItemRect();
                ImGui::Spring(1, 0);
                inputsRect.Min.y -= padding;
                inputsRect.Max.y -= padding;

                const auto allRoundCornersFlags = ImDrawFlags_RoundCornersAll;

                //ed::PushStyleVar(ed::StyleVar_PinArrowSize, 10.0f);
                //ed::PushStyleVar(ed::StyleVar_PinArrowWidth, 10.0f);
                ed::PushStyleVar(ed::StyleVar_PinCorners, allRoundCornersFlags);

                ed::BeginPin(pin.ID, ed::PinKind::Input);
                ed::PinPivotRect(inputsRect.GetCenter(), inputsRect.GetCenter());
                ed::PinRect(inputsRect.GetTL(), inputsRect.GetBR());
                ed::EndPin();
                //ed::PopStyleVar(3);
                ed::PopStyleVar(1);

                auto drawList = ImGui::GetWindowDrawList();
                drawList->AddRectFilled(inputsRect.GetTL(), inputsRect.GetBR(),
                    IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), inputAlpha), 4.0f, allRoundCornersFlags);
                drawList->AddRect(inputsRect.GetTL(), inputsRect.GetBR(),
                    IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), inputAlpha), 4.0f, allRoundCornersFlags);

                if (m_NewLinkPin && !CanCreateLink(m_NewLinkPin, &pin) && &pin != m_NewLinkPin)
                    inputAlpha = (int)(255 * ImGui::GetStyle().Alpha * (48.0f / 255.0f));
            }

            //ImGui::Spring(1, 0);
            ImGui::EndHorizontal();
        }

        ImGui::BeginHorizontal("content_frame");
        ImGui::Spring(1, padding);

        ImGui::BeginVertical("content", ImVec2(0.0f, 0.0f));
        ImGui::Dummy(ImVec2(160, 0));
        ImGui::Spring(1);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
        ImGui::TextUnformatted(node.Name.c_str());
        ImGui::PopStyleColor();
        ImGui::Spring(1);
        ImGui::EndVertical();
        auto contentRect = ImGui_GetItemRect();

        ImGui::Spring(1, padding);
        ImGui::EndHorizontal();

        if (!node.Outputs.empty())
        {
            ImGui::BeginHorizontal("outputs");
            ImGui::Spring(1, 0);

            ImRect outputsRect;
            int outputAlpha = 200;
            for (auto& pin : node.Outputs)
            {
                ImGui::Dummy(ImVec2(padding, padding));
                outputsRect = ImGui_GetItemRect();
                ImGui::Spring(1, 0);
                outputsRect.Min.y += padding;
                outputsRect.Max.y += padding;

                const auto allRoundCornersFlags = ImDrawFlags_RoundCornersAll;
                const auto topRoundCornersFlags = ImDrawFlags_RoundCornersTop;

                ed::PushStyleVar(ed::StyleVar_PinCorners, topRoundCornersFlags);
                ed::BeginPin(pin.ID, ed::PinKind::Output);
                ed::PinPivotRect(outputsRect.GetCenter(), outputsRect.GetCenter());
                ed::PinRect(outputsRect.GetTL(), outputsRect.GetBR());
                ed::EndPin();
                ed::PopStyleVar();


                auto drawList = ImGui::GetWindowDrawList();
                drawList->AddRectFilled(outputsRect.GetTL(), outputsRect.GetBR(),
                    IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), outputAlpha), 4.0f, allRoundCornersFlags);
                drawList->AddRect(outputsRect.GetTL(), outputsRect.GetBR(),
                    IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), outputAlpha), 4.0f, allRoundCornersFlags);


                if (m_NewLinkPin && !CanCreateLink(m_NewLinkPin, &pin) && &pin != m_NewLinkPin)
                    outputAlpha = (int)(255 * ImGui::GetStyle().Alpha * (48.0f / 255.0f));
            }

            ImGui::EndHorizontal();
        }

        ImGui::EndVertical();

        ed::EndNode();
        ed::PopStyleVar(7);
        ed::PopStyleColor(4);
    }

    void NodeGraph::M_DrawTreeNode(Node& node)
    {
        const float rounding = 5.0f;
        const float padding = 12.0f;

        const auto pinBackground = ed::GetStyle().Colors[ed::StyleColor_NodeBg];

        ed::PushStyleColor(ed::StyleColor_NodeBg, ImColor(128, 128, 128, 200));
        ed::PushStyleColor(ed::StyleColor_NodeBorder, ImColor(32, 32, 32, 200));
        ed::PushStyleColor(ed::StyleColor_PinRect, ImColor(60, 180, 255, 150));
        ed::PushStyleColor(ed::StyleColor_PinRectBorder, ImColor(60, 180, 255, 150));

        ed::PushStyleVar(ed::StyleVar_NodePadding, ImVec4(0, 0, 0, 0));
        ed::PushStyleVar(ed::StyleVar_NodeRounding, rounding);
        ed::PushStyleVar(ed::StyleVar_SourceDirection, ImVec2(0.0f, 1.0f));
        ed::PushStyleVar(ed::StyleVar_TargetDirection, ImVec2(0.0f, -1.0f));
        ed::PushStyleVar(ed::StyleVar_LinkStrength, 0.0f);
        ed::PushStyleVar(ed::StyleVar_PinBorderWidth, 1.0f);
        ed::PushStyleVar(ed::StyleVar_PinRadius, 5.0f);
        ed::BeginNode(node.ID);

        ImGui::BeginVertical(node.ID.AsPointer());
        ImGui::BeginHorizontal("inputs");
        ImGui::Spring(0, padding * 2);

        ImRect inputsRect;
        int inputAlpha = 200;
        if (!node.Inputs.empty())
        {
            auto& pin = node.Inputs[0];
            ImGui::Dummy(ImVec2(0, padding));
            ImGui::Spring(1, 0);
            inputsRect = ImGui_GetItemRect();

            ed::PushStyleVar(ed::StyleVar_PinArrowSize, 10.0f);
            ed::PushStyleVar(ed::StyleVar_PinArrowWidth, 10.0f);
#if IMGUI_VERSION_NUM > 18101
            ed::PushStyleVar(ed::StyleVar_PinCorners, ImDrawFlags_RoundCornersBottom);
#else
            ed::PushStyleVar(ed::StyleVar_PinCorners, 12);
#endif
            ed::BeginPin(pin.ID, ed::PinKind::Input);
            ed::PinPivotRect(inputsRect.GetTL() + ImVec2(1.0, 0), inputsRect.GetBR());
            ed::PinRect(inputsRect.GetTL() + ImVec2(1.0, 0), inputsRect.GetBR());
            ed::EndPin();
            ed::PopStyleVar(3);

            if (m_NewLinkPin && !CanCreateLink(m_NewLinkPin, &pin) && &pin != m_NewLinkPin)
                inputAlpha = (int)(255 * ImGui::GetStyle().Alpha * (48.0f / 255.0f));
        }
        else
            ImGui::Dummy(ImVec2(0, padding));

        ImGui::Spring(0, padding * 2);
        ImGui::EndHorizontal();

        ImGui::BeginHorizontal("content_frame");
        ImGui::Spring(1, padding);

        ImGui::BeginVertical("content", ImVec2(0.0f, 0.0f));
        ImGui::Dummy(ImVec2(160, 0));
        ImGui::Spring(1);
        ImGui::TextUnformatted(node.Name.c_str());
        ImGui::Spring(1);
        ImGui::EndVertical();
        auto contentRect = ImGui_GetItemRect();

        ImGui::Spring(1, padding);
        ImGui::EndHorizontal();

        ImGui::BeginHorizontal("outputs");
        ImGui::Spring(0, padding * 2);

        ImRect outputsRect;
        int outputAlpha = 200;
        if (!node.Outputs.empty())
        {
            auto& pin = node.Outputs[0];
            ImGui::Dummy(ImVec2(0, padding));
            ImGui::Spring(1, 0);
            outputsRect = ImGui_GetItemRect();

#if IMGUI_VERSION_NUM > 18101
            ed::PushStyleVar(ed::StyleVar_PinCorners, ImDrawFlags_RoundCornersTop);
#else
            ed::PushStyleVar(ed::StyleVar_PinCorners, 3);
#endif
            ed::BeginPin(pin.ID, ed::PinKind::Output);
            ed::PinPivotRect(outputsRect.GetTL() + ImVec2(1.0, 0), outputsRect.GetBR());
            ed::PinRect(outputsRect.GetTL() + ImVec2(1.0, 0), outputsRect.GetBR());
            ed::EndPin();
            ed::PopStyleVar();

            if (m_NewLinkPin && !CanCreateLink(m_NewLinkPin, &pin) && &pin != m_NewLinkPin)
                outputAlpha = (int)(255 * ImGui::GetStyle().Alpha * (48.0f / 255.0f));
        }
        else
            ImGui::Dummy(ImVec2(0, padding));

        ImGui::Spring(0, padding * 2);
        ImGui::EndHorizontal();

        ImGui::EndVertical();

        ed::EndNode();
        ed::PopStyleVar(7);
        ed::PopStyleColor(4);

        auto drawList = ed::GetNodeBackgroundDrawList(node.ID);

        // const auto fringeScale = ImGui::GetStyle().AntiAliasFringeScale;
        // const auto unitSize    = 1.0f / fringeScale;
        
        // const auto ImDrawList_AddRect = [](ImDrawList* drawList, const ImVec2& a, const ImVec2& b, ImU32 col, float rounding, int rounding_corners, float thickness)
        // {
        //     if ((col >> 24) == 0)
        //         return;
        //     drawList->PathRect(a, b, rounding, rounding_corners);
        //     drawList->PathStroke(col, true, thickness);
        // };

#if IMGUI_VERSION_NUM > 18101
        const auto    topRoundCornersFlags = ImDrawFlags_RoundCornersTop;
        const auto bottomRoundCornersFlags = ImDrawFlags_RoundCornersBottom;
#else
        const auto    topRoundCornersFlags = 1 | 2;
        const auto bottomRoundCornersFlags = 4 | 8;
#endif

        drawList->AddRectFilled(inputsRect.GetTL() + ImVec2(1.0, 1.0), inputsRect.GetBR(),
            IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), inputAlpha), 4.0f, bottomRoundCornersFlags);
        //ImGui::PushStyleVar(ImGuiStyleVar_AntiAliasFringeScale, 1.0f);
        drawList->AddRect(inputsRect.GetTL() + ImVec2(1.0, 1.0), inputsRect.GetBR(),
            IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), inputAlpha), 4.0f, bottomRoundCornersFlags);
        //ImGui::PopStyleVar();
        drawList->AddRectFilled(outputsRect.GetTL(), outputsRect.GetBR() - ImVec2(0, 1),
            IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), outputAlpha), 4.0f, topRoundCornersFlags);
        //ImGui::PushStyleVar(ImGuiStyleVar_AntiAliasFringeScale, 1.0f);
        drawList->AddRect(outputsRect.GetTL(), outputsRect.GetBR() - ImVec2(0, 1),
            IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), outputAlpha), 4.0f, topRoundCornersFlags);
        //ImGui::PopStyleVar();
        drawList->AddRectFilled(contentRect.GetTL(), contentRect.GetBR(), IM_COL32(24, 64, 128, 200), 0.0f);
        //ImGui::PushStyleVar(ImGuiStyleVar_AntiAliasFringeScale, 1.0f);
        drawList->AddRect(
            contentRect.GetTL(),
            contentRect.GetBR(),
            IM_COL32(48, 128, 255, 100), 0.0f);
        //ImGui::PopStyleVar();
    }






// -----------------------------------------------------------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------------------------------------------------------

#define ETH_SERIALIZE_KAY_VALUE(propName, propVal, outputNode) outputNode << YAML::Key << #propName << YAML::Value << propVal
#define ETH_DESERIALIZE_KAY_VALUE(propName, destination, node, defaultValue) destination = node[#propName] ? node[#propName].as<decltype(defaultValue)>() : defaultValue

    YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec2& v)
    {
        out << YAML::Flow;
        out << YAML::BeginSeq << v.x << v.y << YAML::EndSeq;
        return out;
    }

    YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec3& v)
    {
        out << YAML::Flow;
        out << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
        return out;
    }

    YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec4& v)
    {
        out << YAML::Flow;
        out << YAML::BeginSeq << v.x << v.y << v.z << v.w << YAML::EndSeq;
        return out;
    }

    static std::string NodeTypeToString(NodeType nodeType)
    {
        switch (nodeType)
        {
        default:
        case NodeType::Blueprint: return "Blueprint";
        case NodeType::Simple:    return "Simple";
        case NodeType::Tree:      return "Tree";
        case NodeType::Comment:   return "Comment";
        case NodeType::Houdini:   return "Houdini";

            // SC
        case NodeType::SC:        return "SC";
        }
    }

    static NodeType NodeTypeFromString(const std::string_view& nodeTypeStr)
    {
        if (nodeTypeStr == "Blueprint") return NodeType::Blueprint;
        if (nodeTypeStr == "Simple")    return NodeType::Simple;
        if (nodeTypeStr == "Tree")      return NodeType::Tree;
        if (nodeTypeStr == "Comment")   return NodeType::Comment;
        if (nodeTypeStr == "Houdini")   return NodeType::Houdini;

        // SC
        if (nodeTypeStr == "SC")        return NodeType::SC;
    }

    static std::string PinTypeToString(PinType pinType)
    {
        switch (pinType)
        {
        default:
        case PinType::Flow:     return "Flow";
        case PinType::Bool:     return "Bool";
        case PinType::Int:      return "Int";
        case PinType::Float:    return "Float";
        case PinType::String:   return "String";
        case PinType::Object:   return "Object";
        case PinType::Function: return "Function";
        case PinType::Delegate: return "Delegate";
            
        // SC
        case PinType::StringArray: return "StringArray";
        case PinType::IntArray:    return "IntArray";
        }
    }

    static PinType PinTypeFromString(const std::string_view& pinTypeStr)
    {
        if (pinTypeStr == "Flow")     return PinType::Flow;
        if (pinTypeStr == "Bool")     return PinType::Bool;
        if (pinTypeStr == "Int")      return PinType::Int;
        if (pinTypeStr == "Float")    return PinType::Float;
        if (pinTypeStr == "String")   return PinType::String;
        if (pinTypeStr == "Object")   return PinType::Object;
        if (pinTypeStr == "Function") return PinType::Function;
        if (pinTypeStr == "Delegate") return PinType::Delegate;
        
        // SC
        if (pinTypeStr == "StringArray") return PinType::StringArray;
        if (pinTypeStr == "IntArray")    return PinType::IntArray;
    }

#if 1
    void NodeGraph::Serialize(const std::filesystem::path& filepath)
    {
        YAML::Emitter out;

        //============================================================
        /// Nodes

        out << YAML::BeginMap; // Nodes & Links

        out << YAML::Key << "Nodes" << YAML::Value;
        out << YAML::BeginSeq;
        for (auto& node : m_Nodes)
        {
            out << YAML::BeginMap; // node

            const ImVec4& nodeCol = node.Color.Value;
            const ImVec2& nodeSize = node.Size;
            const glm::vec4 nodeColOut(nodeCol.x, nodeCol.y, nodeCol.z, nodeCol.w);
            const glm::vec2 nodeSizeOut(nodeSize.x, nodeSize.y);

            ETH_SERIALIZE_KAY_VALUE(ID, node.ID.Get(), out);
            ETH_SERIALIZE_KAY_VALUE(Name, node.Name, out);
            ETH_SERIALIZE_KAY_VALUE(Color, nodeColOut, out);
            ETH_SERIALIZE_KAY_VALUE(Type, NodeTypeToString(node.Type), out);
            ETH_SERIALIZE_KAY_VALUE(Size, nodeSizeOut, out);
            ETH_SERIALIZE_KAY_VALUE(Location, node.State, out);

            out << YAML::Key << "Inputs" << YAML::BeginSeq;
            for (auto& in : node.Inputs)
            {
                out << YAML::BeginMap; // in
                ETH_SERIALIZE_KAY_VALUE(ID, in.ID.Get(), out);
                ETH_SERIALIZE_KAY_VALUE(Name, in.Name, out);
                ETH_SERIALIZE_KAY_VALUE(Type, PinTypeToString(in.Type), out);
                // TODO: test
                if (in.Value.has_value())
                {
                    std::string& str = std::any_cast<std::string>(in.Value);
                    ETH_SERIALIZE_KAY_VALUE(ValueType, in.Value.type().name(), out);
                    ETH_SERIALIZE_KAY_VALUE(Value, str, out);
                }
                else
                {
                    ETH_SERIALIZE_KAY_VALUE(ValueType, "", out);
                    ETH_SERIALIZE_KAY_VALUE(Value, "", out);
                }
                // ETH_SERIALIZE_KAY_VALUE(Storage, StorageKindToString(in.Storage), out);
                // ETH_SERIALIZE_KAY_VALUE(Value, choc::json::toString(in.Value), out);
                out << YAML::EndMap; // in
            }
            out << YAML::EndSeq; // Inputs

            out << YAML::Key << "Outputs" << YAML::BeginSeq;
            for (auto& outp : node.Outputs)
            {
                out << YAML::BeginMap; // outp
                ETH_SERIALIZE_KAY_VALUE(ID, outp.ID.Get(), out);
                ETH_SERIALIZE_KAY_VALUE(Name, outp.Name, out);
                ETH_SERIALIZE_KAY_VALUE(Type, PinTypeToString(outp.Type), out);
                //TODO: test
                if (outp.Value.has_value())
                {
                    std::string& str = std::any_cast<std::string>(outp.Value);
                    ETH_SERIALIZE_KAY_VALUE(ValueType, outp.Value.type().name(), out);
                    ETH_SERIALIZE_KAY_VALUE(Value, str, out);
                }
                else
                {
                    ETH_SERIALIZE_KAY_VALUE(ValueType, "", out);
                    ETH_SERIALIZE_KAY_VALUE(Value, "", out);
                }
                // ETH_SERIALIZE_KAY_VALUE(Storage, StorageKindToString(outp.Storage), out);
                // ETH_SERIALIZE_KAY_VALUE(Value, choc::json::toString(outp.Value), out);
                out << YAML::EndMap; // outp
            }
            out << YAML::EndSeq; // Outputs

            out << YAML::EndMap; // node
        }
        out << YAML::EndSeq; // Nodes

        //============================================================
        /// Links

        out << YAML::Key << "Links" << YAML::Value;
        out << YAML::BeginSeq;
        for (auto& link : m_Links)
        {
            out << YAML::BeginMap; // link

            const auto& col = link.Color.Value;
            const glm::vec4 colOut(col.x, col.y, col.z, col.w);

            ETH_SERIALIZE_KAY_VALUE(ID, link.ID.Get(), out);
            ETH_SERIALIZE_KAY_VALUE(StartPinID, link.StartPinID.Get(), out);
            ETH_SERIALIZE_KAY_VALUE(EndPinID, link.EndPinID.Get(), out);
            ETH_SERIALIZE_KAY_VALUE(Color, colOut, out);

            out << YAML::EndMap; // link
        }
        out << YAML::EndSeq; // Links

        out << YAML::EndMap; // Nodes & Links

        // Out
        std::ofstream fout(filepath.string());
        fout << out.c_str();
        fout.close();
    }
#endif

    bool NodeGraph::Deserialize(const std::filesystem::path& filepath)
    {
        YAML::Node data = YAML::LoadFile(filepath.string());

        for (auto& node : data["Nodes"])
        {
            uint32_t nodeID;
            // UUID nodeID;
            std::string nodeName;
            std::string location;
            std::string nodeType;
            glm::vec4 nodeCol;
            glm::vec2 nodeSize;

            ETH_DESERIALIZE_KAY_VALUE(ID, nodeID, node, uint64_t(0));
            ETH_DESERIALIZE_KAY_VALUE(Name, nodeName, node, std::string());
            ETH_DESERIALIZE_KAY_VALUE(Color, nodeCol, node, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
            ETH_DESERIALIZE_KAY_VALUE(Type, nodeType, node, std::string());
            ETH_DESERIALIZE_KAY_VALUE(Size, nodeSize, node, glm::vec2());
            ETH_DESERIALIZE_KAY_VALUE(Location, location, node, std::string());

            auto& newNode = m_Nodes.emplace_back(nodeID, nodeName.c_str());
            newNode.State = location;
            newNode.Color = ImColor(nodeCol.x, nodeCol.y, nodeCol.z, nodeCol.w);
            newNode.Type = NodeTypeFromString(nodeType);
            newNode.Size = ImVec2(nodeSize.x, nodeSize.y);

            if (node["Inputs"])
            {
                for (auto& in : node["Inputs"])
                {
                    uint32_t ID;
                    // UUID ID;
                    std::string pinName;
                    std::string pinType;
                    // TODO: test
                    std::string pinValueType;
                    std::string pinValue;
                    // std::string valueStr;
                    // std::string pinStorage;

                    ETH_DESERIALIZE_KAY_VALUE(ID, ID, in, uint64_t(0));
                    ETH_DESERIALIZE_KAY_VALUE(Name,         pinName,       in, std::string());
                    ETH_DESERIALIZE_KAY_VALUE(Type,         pinType,       in, std::string());
                    // TODO: test
                    ETH_DESERIALIZE_KAY_VALUE(ValueType,    pinValueType,  in, std::string());
                    ETH_DESERIALIZE_KAY_VALUE(Value,        pinValue,      in, std::string());
                    // ETH_DESERIALIZE_KAY_VALUE(Storage, pinStorage, in, std::string());
                    // ETH_DESERIALIZE_KAY_VALUE(Value, valueStr, in, std::string());
#if 0
                    bool isCustomValueType = choc::text::contains(valueStr, "Value");

                    auto parseCustomValueType = [](const std::string& valueString) -> choc::value::Value
                    {
                        choc::value::Value value = choc::json::parse(valueString);

                        if (value["TypeName"].isVoid())
                        {
                            ETH_CORE_ASSERT(false, "Failed to deserialize custom value type, missing \"TypeName\" property.");
                            return {};
                        }

                        choc::value::Value customObject = choc::value::createObject(value["TypeName"].get<std::string>());
                        if (value.isObject())
                        {
                            for (uint32_t i = 0; i < value.size(); i++)
                            {
                                choc::value::MemberNameAndValue nameValue = value.getObjectMemberAt(i);
                                customObject.addMember(nameValue.name, nameValue.value);
                            }
                        }
                        else
                        {
                            ETH_CORE_ASSERT("Failed to load custom value type. It must be serialized as object.")
                        }

                        return customObject;
                    };
#endif

                    auto& newInput = newNode.Inputs.emplace_back(
                        ID,
                        pinName.c_str(),
                        PinTypeFromString(pinType)
                        // StorageKindFromString(pinStorage),
                        // isCustomValueType ? parseCustomValueType(valueStr) : choc::json::parseValue(valueStr)
                        );
                    newInput.Kind = PinDirection::Input;
                    if (pinValue != "")
                    {
                        newInput.Value = pinValue;
                    }
                }
            }

            if (node["Outputs"])
            {
                for (auto& out : node["Outputs"])
                {
                    uint32_t ID;
                    //UUID ID;
                    std::string pinName;
                    std::string pinType;
                    // TODO: test
                    std::string pinValueType;
                    std::string pinValue;
                    // std::string valueStr;
                    // std::string pinStorage;

                    ETH_DESERIALIZE_KAY_VALUE(ID,   ID,      out, uint64_t(0));
                    ETH_DESERIALIZE_KAY_VALUE(Name, pinName, out, std::string());
                    ETH_DESERIALIZE_KAY_VALUE(Type, pinType, out, std::string());
                    // TODO: test
                    ETH_DESERIALIZE_KAY_VALUE(ValueType, pinValueType, out, std::string());
                    ETH_DESERIALIZE_KAY_VALUE(Value, pinValue, out, std::string());
                    // ETH_DESERIALIZE_KAY_VALUE(Storage, pinStorage, out, std::string());
                    // ETH_DESERIALIZE_KAY_VALUE(Value, valueStr, out, std::string());

                    auto& newOutput = newNode.Outputs.emplace_back(
                        ID,
                        pinName.c_str(),
                        PinTypeFromString(pinType)
                        // StorageKindFromString(pinStorage),
                        // choc::json::parseValue(valueStr)
                    );
                    newOutput.Kind = PinDirection::Output;
                    // TODO: test
                    if (pinValue != "")
                    {
                        newOutput.Value = pinValue;
                    }
                }
            }
        }

        for (auto& link : data["Links"])
        {
            uint32_t ID;
            uint32_t StartPinID;
            uint32_t EndPinID;
            // UUID ID;
            // UUID StartPinID;
            // UUID EndPinID;
            glm::vec4 color;

            ETH_DESERIALIZE_KAY_VALUE(ID, ID, link, uint64_t(0));
            ETH_DESERIALIZE_KAY_VALUE(StartPinID, StartPinID, link, uint64_t(0));
            ETH_DESERIALIZE_KAY_VALUE(EndPinID, EndPinID, link, uint64_t(0));
            ETH_DESERIALIZE_KAY_VALUE(Color, color, link, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

            m_Links.emplace_back(ID, StartPinID, EndPinID)
                .Color = ImColor(color.x, color.y, color.z, color.w);
        }

        return true;
    }

    void NodeGraph::OnOpenGraphFile()
    {
        std::string filepath = FileDialogs::OpenFile("Ethane Graph (*.yaml)\0*.yaml\0");
        if (!filepath.empty())
        {
            Deserialize(filepath);
            ed::NavigateToContent();
            BuildNodes();
        }
    }

    void NodeGraph::OnSave()
    {
        std::string filepath = FileDialogs::SaveFile("Ethane Graph (*.yaml)\0*.yaml\0");
        if (!filepath.empty())
        {
            Serialize(filepath);
        }
    }

    void NodeGraph::OnCompile()
    {
        std::string filepath = FileDialogs::SaveFile("Ethane State (*.ts)\0*.ts\0");
        if (!filepath.empty())
        {
            std::ofstream fout(filepath);
            std::string codeBase = "";
            fout << codeBase.c_str();
            Compile(fout);
        }
    }

    // helper
    template<typename T>
    std::ostream& operator<<(std::ostream& os, const std::vector<T> vec)
    {
        if (!vec.empty())
        {
            os << vec[0];
            for (auto i = vec.cbegin() + 1; i != vec.cend(); ++i) {
                os << ", " << *i;
            }
        }
        return os;
    }

    std::ostream& operator<<(std::ostream& os, const std::vector<std::string> vec)
    {
        if (!vec.empty())
        {
            os << "\"" << vec[0] << "\"";
            for (auto i = vec.cbegin() + 1; i != vec.cend(); ++i) {
                os << ", \"" << *i << "\"";
            }
        }
        return os;
    }

    Pin* NodeGraph::FindOtherLinkedPin(ed::PinId pinID)
    {
        for (auto& link : m_Links)
        {
            if (link.EndPinID == pinID)
            {
                return FindPin(link.StartPinID);
                break;
            }
        }
    }

    void NodeGraph::Compile(std::ofstream& out)
    {
        for (auto& node : m_Nodes)
        {
            // std::map<uint32_t, uint32_t> pin
            if (node.Type == NodeType::SC)
            {
                if (node.Name == "IntArray")
                {
                    std::vector<uint32_t> output;
                    for (auto& in : node.Inputs)
                    {
                        output.push_back(std::stoi(std::any_cast<std::string>(in.Value)));
                    }
                    // node.Outputs[0].Value.reset();
                    node.Outputs[0].Value = output;
                }
                else if (node.Name == "StringArray")
                {
                    std::vector<std::string> output;
                    for (auto& in : node.Inputs)
                    {
                        output.push_back(std::any_cast<std::string>(in.Value));
                    }
                    // node.Outputs[0].Value.reset();
                    node.Outputs[0].Value = output;
                }
                else if (node.Name == "CastToIntArray")
                {
                    std::vector<uint32_t> output;
                    for (auto& in : node.Inputs)
                    {
                        auto pin = FindOtherLinkedPin(in.ID);
                        output.push_back(pin->Node->ID.Get());
                    }
                    // node.Outputs[0].Value.reset();
                    node.Outputs[0].Value = output;
                }
            }
        }
        for (auto& node : m_Nodes)
        {
            if (node.Type == NodeType::SC)
            {
                uint32_t next_state = 0;
                for (auto& link : m_Links)
                {
                    if (link.StartPinID == node.Outputs[0].ID)
                    {
                        next_state = FindPin(link.EndPinID)->Node->ID.Get();
                        break;
                    }
                }

                if (node.Name == "GameStatePlot")
                {
                    uint32_t plot_id;
                    auto pin = FindOtherLinkedPin(node.Inputs[1].ID);
                    auto& val = pin->Value;
                    plot_id = std::stoi(std::any_cast<std::string>(val));
                    // TODO: future: out << std::format("s = new PlotState({}, {}, [{}], [{}], {})", "this", node.ID.Get(), next_state, "", dialog_id);
                    out << "s = new PlotState("
                        << "this" << ", "
                        << node.ID.Get() << ", "
                        << "[" << next_state << "]" << ", "
                        << "[]" << ", "

                        << plot_id
                        << ");";
                    out << std::endl;
                    out << "this.stateGraph.set(s.id, s);";
                    out << std::endl;
                }
                else if (node.Name == "GameStatePuzzle")
                {
                    uint32_t puzzle_id;
                    auto& val = FindOtherLinkedPin(node.Inputs[1].ID)->Value;
                    puzzle_id = std::stoi(std::any_cast<std::string>(val));
                    out << "s = new PuzzleState("
                        << "this" << ", "
                        << node.ID.Get() << ", "
                        << "[" << next_state << "]" << ", "
                        << "[]" << ", "

                        << puzzle_id
                        << ");";
                    out << std::endl;
                    out << "this.stateGraph.set(s.id, s);";
                    out << std::endl;
                }
                else if (node.Name == "GameStateShadowClone")
                {
                    std::vector<std::string> nbsKey;
                    std::vector<uint32_t> closingStates;
                    std::vector<uint32_t> branchStates;

                    nbsKey = std::any_cast<std::vector<std::string>>(FindOtherLinkedPin(node.Inputs[1].ID)->Value);
                    auto pin = FindOtherLinkedPin(node.Inputs[2].ID);
                    closingStates = std::any_cast<std::vector<uint32_t>>(pin->Value);

                    for (uint32_t i = 1; i < node.Outputs.size(); ++i)
                    {
                        uint32_t branch = 0;
                        for (auto& link : m_Links)
                        {
                            if (link.StartPinID == node.Outputs[i].ID)
                            {
                                branch = FindPin(link.EndPinID)->Node->ID.Get();
                                break;
                            }
                        }
                        branchStates.push_back(branch);
                    }

                    out << "s = new ShadowCloneState("
                        << "this" << ", "
                        << node.ID.Get() << ", "
                        << "[" << next_state << "]" << ", "
                        << "[" << nbsKey << "]" << ", " // temp test
                        
                        // TODO
                        << "[" << nbsKey << "]" << ", "
                        << "[" << branchStates << "]" << ", "
                        << "[" << closingStates << "]"
                        << ");";
                    out << std::endl;
                    out << "this.stateGraph.set(s.id, s);";
                    out << std::endl;
                }
                else if (node.Name == "GameStateDialog")
                {
                    uint32_t dialog_id;
                    auto& val = FindOtherLinkedPin(node.Inputs[1].ID)->Value;
                    dialog_id = std::stoi(std::any_cast<std::string>(val));
                    // TODO: future: out << std::format("s = new PlotState({}, {}, [{}], [{}], {})", "this", node.ID.Get(), next_state, "", dialog_id);
                    out << "s = new PlotState("
                        << "this" << ", "
                        << node.ID.Get() << ", "
                        << "[" << next_state << "]" << ", "
                        << "[]" << ", "

                        << dialog_id
                        << ");";
                    out << std::endl;
                    out << "this.stateGraph.set(s.id, s);";
                    out << std::endl;
                }
                else if (node.Name == "GameStateReward")
                {

                }
                else if (node.Name == "GameStateLock")
                {
                    //  s = new LockState(this, 130, [131], [], [], [], [], 3);
                    // this.stateGraph.set(s.id, s);
                    std::vector<uint32_t> requireState;
                    std::vector<uint32_t> requireProps;
                    std::vector<uint32_t> requireAbiliities;
                    uint32_t fallbackState = 0;

                    auto pin = FindOtherLinkedPin(node.Inputs[1].ID);
                    requireState = std::any_cast<std::vector<uint32_t>>(pin->Value);
                    pin = FindOtherLinkedPin(node.Inputs[2].ID);
                    requireProps = std::any_cast<std::vector<uint32_t>>(pin->Value);
                    pin = FindOtherLinkedPin(node.Inputs[3].ID);
                    requireAbiliities = std::any_cast<std::vector<uint32_t>>(pin->Value);

                    fallbackState = FindOtherLinkedPin(node.Inputs[0].ID)->Node->ID.Get();

                    out << "s = new LockState("
                        << "this" << ", "
                        << node.ID.Get() << ", "
                        << "[" << next_state << "]" << ", "
                        << "[]" << ", "

                        << "[" << requireState << "]" << ", "
                        << "[" << requireProps << "]" << ", "
                        << "[" << requireAbiliities << "]" << ", "
                        << fallbackState
                        << ");";
                    out << std::endl;
                    out << "this.stateGraph.set(s.id, s);";
                    out << std::endl;
                }
                // std::vector<std::string> ns;
                // for (auto& output : node.Outputs)
                // {
                //     for (auto& link : m_Links)
                //     {
                //         if (link.StartPinID == output.ID)
                //             ns.push_back();
                //     }
                // }
                // out << "let state : State = new State(" << node.ID.AsPointer() << ", [ 456, 789]);" << std::endl;
                // out << "stateMap.set(" << node.ID.AsPointer() << ", state);" << std::endl;
            }
        }
    }
}


