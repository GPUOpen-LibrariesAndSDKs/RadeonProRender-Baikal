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

static inline ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y); }
static inline ImVec2 operator-(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x - rhs.x, lhs.y - rhs.y); }

////////////////////////////////////////////////////////
//// MaterialExplorer implementation
////////////////////////////////////////////////////////

namespace
{
    struct MaterialExplorerConcrete : public MaterialExplorer
    {
        MaterialExplorerConcrete(InputMap::Ptr input_map) :
            MaterialExplorer(input_map)
        {   }
    };
}

MaterialExplorer::Ptr MaterialExplorer::Create(InputMap::Ptr input_map)
{
    return std::make_shared<MaterialExplorer>(MaterialExplorerConcrete(input_map));
}

MaterialExplorer::MaterialExplorer(InputMap::Ptr input_map) :
    m_uber_graph(UberGraph::Create(input_map))
{   }

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