/**********************************************************************
Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved.

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

#include "material_explorer.h"
#include <memory>

using namespace Baikal;

////////////////////////////////////////////////////////
//// InputMapExplorer::Node implementation
////////////////////////////////////////////////////////

InputMapExplorer::Node::Node(
    int id_, const std::string& name_,
    const ImVec2& pos_, float value_,
    const ImVec4& color_, int inputs_count_, int outputs_count_) :
    // initialize list
    id(id_), name(name_), pos(pos_), value(value_), color(color_), inputs_count(inputs_count_), outputs_count(outputs_count_)
{   }

ImVec2 InputMapExplorer::Node::GetInputSlotPos(int slot_no) const
{ return ImVec2(pos.x, pos.y + size.y * ((float)slot_no + 1) / ((float)inputs_count + 1)); }

ImVec2 InputMapExplorer::Node::GetOutputSlotPos(int slot_no) const
{ return ImVec2(pos.x + size.x, pos.y + size.y * ((float)slot_no + 1) / ((float)outputs_count + 1)); }


////////////////////////////////////////////////////////
//// InputMapExplorer::NodeLink implementation
////////////////////////////////////////////////////////

InputMapExplorer::NodeLink::NodeLink(int input_idx_, int input_slot_, int output_idx_, int output_slot_) :
    input_idx(input_idx_), input_slot(input_slot_), output_idx(output_idx_), output_slot(output_slot_)
{   }

////////////////////////////////////////////////////////
//// InputMapExplorer implementation
////////////////////////////////////////////////////////

namespace
{
    struct InputMapExplorerConcrete : public InputMapExplorer
    {
        InputMapExplorerConcrete(InputMap::Ptr input_map) :
            InputMapExplorer(input_map)
        {   }
    };
}

InputMapExplorer::Ptr InputMapExplorer::Create(InputMap::Ptr input_map)
{
    return std::make_shared<InputMapExplorer>(InputMapExplorerConcrete(input_map));
}

InputMapExplorer::InputMapExplorer(InputMap::Ptr input_map) :
    m_uber_graph(UberGraph::Create(input_map))
{   }

////////////////////////////////////////////////////////
//// MaterialExplorer implementation
////////////////////////////////////////////////////////

std::vector<MaterialExplorer::LayerDesc> MaterialExplorer::GetUberLayersDesc()
{
    return std::vector<LayerDesc>
    {
        {
            UberV2Material::Layers::kEmissionLayer,
            { "uberv2.emission.color" }
        },
        {
            UberV2Material::Layers::kCoatingLayer,
            { "uberv2.coating.color", "uberv2.coating.ior" }
        },
        {
            UberV2Material::Layers::kReflectionLayer,
            {
                "uberv2.reflection.color",
                "uberv2.reflection.roughness",
                "uberv2.reflection.anisotropy",
                "uberv2.reflection.anisotropy_rotation",
                "uberv2.reflection.ior",
                "uberv2.reflection.metalness",
            }
        },
        {
            UberV2Material::Layers::kDiffuseLayer,
            { "uberv2.diffuse.color" }
        },
        {
            UberV2Material::Layers::kRefractionLayer,
            {
                "uberv2.refraction.color",
                "uberv2.refraction.roughness",
                "uberv2.refraction.ior"
            }
        },
        {
            UberV2Material::Layers::kTransparencyLayer,
            { "uberv2.transparency" }
        },
        {
            UberV2Material::Layers::kShadingNormalLayer,
            { "uberv2.shading_normal" }
        },
        {
            UberV2Material::Layers::kSSSLayer,
            {
                "uberv2.sss.absorption_color",
                "uberv2.sss.scatter_color",
                "uberv2.sss.subsurface_color",
                "uberv2.sss.absorption_distance",
                "uberv2.sss.scatter_distance",
                "uberv2.sss.scatter_direction"
            }
        },
    };
}