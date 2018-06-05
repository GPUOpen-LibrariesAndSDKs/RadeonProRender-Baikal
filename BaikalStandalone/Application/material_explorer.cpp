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

static inline ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs)
{ return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y); }

static inline ImVec2 operator-(const ImVec2& lhs, const ImVec2& rhs)
{ return ImVec2(lhs.x - rhs.x, lhs.y - rhs.y); }

////////////////////////////////////////////////////////
//// MaterialExplorer implementation
////////////////////////////////////////////////////////

namespace
{
    struct MaterialExplorerConcrete : public MaterialExplorer
    {
        MaterialExplorerConcrete(UberV2Material::Ptr material) :
            MaterialExplorer(material)
        {   }
    };
}

MaterialExplorer::Ptr MaterialExplorer::Create(UberV2Material::Ptr material)
{
    return std::make_shared<MaterialExplorer>(MaterialExplorerConcrete(material));
}

MaterialExplorer::MaterialExplorer(UberV2Material::Ptr material) :
    m_material(material)
{
    auto layers = m_material->GetLayers();
    auto layers_desc = GetUberLayersDesc();

    auto find_layer =
        [&layers_desc](UberV2Material::Layers layers)
        {
            return *(std::find_if(
                layers_desc.begin(), layers_desc.end(),
                [layers](LayerDesc desc)
                {
                    if (static_cast<std::uint32_t>(desc.first) == layers)
                        return true;
                    return false;
                }));
        };

    // collect all supported layers
    if (layers & UberV2Material::kEmissionLayer)
        m_layers.push_back(find_layer(UberV2Material::kEmissionLayer));
    if (layers & UberV2Material::kTransparencyLayer)
        m_layers.push_back(find_layer(UberV2Material::kTransparencyLayer));
    if (layers & UberV2Material::kCoatingLayer)
        m_layers.push_back(find_layer(UberV2Material::kCoatingLayer));
    if (layers & UberV2Material::kReflectionLayer)
        m_layers.push_back(find_layer(UberV2Material::kReflectionLayer));
    if (layers & UberV2Material::kDiffuseLayer)
        m_layers.push_back(find_layer(UberV2Material::kDiffuseLayer));
    if (layers & UberV2Material::kRefractionLayer)
        m_layers.push_back(find_layer(UberV2Material::kRefractionLayer));
    if (layers & UberV2Material::kSSSLayer)
        m_layers.push_back(find_layer(UberV2Material::kSSSLayer));
    if (layers & UberV2Material::kShadingNormalLayer)
        m_layers.push_back(find_layer(UberV2Material::kShadingNormalLayer));
}

void MaterialExplorer::ChangeLayer()
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));

    ImGui::OpenPopup("Layers_desc");
    if (ImGui::BeginPopup("Layers_desc"))
    {
        ImGui::Text("Layers desc");
        ImGui::Separator();

        for (const auto &item : m_layers)
        {
            for (const auto& input : item.second)
            {
                if (ImGui::MenuItem(input.c_str(), NULL, false, false))
                {   }
            }
        }
    }
    ImGui::EndPopup();
    ImGui::PopStyleVar();
}

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