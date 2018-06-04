
/**********************************************************************
 Copyright (c) 2017 Advanced Micro Devices, Inc. All rights reserved.

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 ********************************************************************/

#include "ImGUI/imgui.h"
#include "SceneGraph/uberv2material.h"
#include "uber_graph.h"

class InputMapExplorer
{
public:
    using Ptr = std::shared_ptr<InputMapExplorer>;

    static Ptr Create(Baikal::InputMap::Ptr input_map);

    struct Node
    {
        int     id;
        std::string name;
        ImVec2  pos, size;
        float   value;
        ImVec4  color;
        int     inputs_count, outputs_count;

        Node(int id_, const std::string& name_, const ImVec2& pos_, float value_, const ImVec4& color_, int inputs_count_, int outputs_count_);

        ImVec2 GetInputSlotPos(int slot_no) const;
        ImVec2 GetOutputSlotPos(int slot_no) const;
    };

    struct NodeLink
    {
        int input_idx, input_slot, output_idx, output_slot;

        NodeLink(int input_idx_, int input_slot_, int output_idx_, int output_slot_);
    };

    static void DrawInput(const ImVec2 &win_size);

protected:
    InputMapExplorer(Baikal::InputMap::Ptr input_map);

private:
    UberGraph::Ptr m_uber_graph;
    ImVector<Node> nodes;
    ImVector<NodeLink> links;
};

class MaterialExplorer
{
public:
    using LayerDesc = std::pair<Baikal::UberV2Material::Layers, std::vector<std::string>>;

    static std::vector<LayerDesc> GetUberLayersDesc();

private:
    InputMapExplorer::Ptr m_input;
};