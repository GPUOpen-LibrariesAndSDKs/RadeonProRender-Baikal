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

#include "uberv2material.h"
#include "inputmaps.h"

using namespace Baikal;
using namespace RadeonRays;

namespace
{
    struct UberV2MaterialConcrete : public UberV2Material {
    };
}

UberV2Material::Ptr UberV2Material::Create() {
    return std::make_shared<UberV2MaterialConcrete>();
}

UberV2Material::UberV2Material()
{
    using namespace RadeonRays;

    // Create several input maps that needed for default material parameters
    auto f4_one = InputMap_ConstantFloat3::Create(float3(1.0f, 1.0f, 1.0f, 1.0f));
    auto f4_zero = InputMap_ConstantFloat3::Create(float3(0.0f, 0.0f, 0.0f, 0.0f));
    auto f_zero = InputMap_ConstantFloat::Create(0.0f);
    auto def_ior = InputMap_ConstantFloat::Create(1.5f);
    auto def_roughness = InputMap_ConstantFloat::Create(0.5f);

    //Diffuse
    RegisterInput("uberv2.diffuse.color", "base diffuse albedo", { InputType::kInputMap });
    SetInputValue("uberv2.diffuse.color", f4_one);

    //Reflection
    RegisterInput("uberv2.reflection.color", "base reflection albedo", { InputType::kInputMap });
    SetInputValue("uberv2.reflection.color", f4_one);
    RegisterInput("uberv2.reflection.roughness", "reflection roughness", { InputType::kInputMap });
    SetInputValue("uberv2.reflection.roughness", def_roughness);
    RegisterInput("uberv2.reflection.anisotropy", "level of anisotropy", { InputType::kInputMap });
    SetInputValue("uberv2.reflection.anisotropy", f_zero);
    RegisterInput("uberv2.reflection.anisotropy_rotation", "orientation of anisotropic component", { InputType::kInputMap });
    SetInputValue("uberv2.reflection.anisotropy_rotation", f_zero);
    RegisterInput("uberv2.reflection.ior", "index of refraction", { InputType::kInputMap });
    SetInputValue("uberv2.reflection.ior", def_ior);
    RegisterInput("uberv2.reflection.metalness", "metalness of the material", { InputType::kInputMap });
    SetInputValue("uberv2.reflection.metalness", f_zero);

    //Coating
    RegisterInput("uberv2.coating.color", "base coating albedo", { InputType::kInputMap });
    SetInputValue("uberv2.coating.color", f4_one);
    RegisterInput("uberv2.coating.ior", "index of refraction", { InputType::kInputMap });
    SetInputValue("uberv2.coating.ior", def_ior);

    //Refraction
    RegisterInput("uberv2.refraction.color", "base refraction albedo", { InputType::kInputMap });
    SetInputValue("uberv2.refraction.color", f4_one);
    RegisterInput("uberv2.refraction.roughness", "refraction roughness", { InputType::kInputMap });
    SetInputValue("uberv2.refraction.roughness", def_roughness);
    RegisterInput("uberv2.refraction.ior", "index of refraction", { InputType::kInputMap });
    SetInputValue("uberv2.refraction.ior", def_ior);

    //Emission
    RegisterInput("uberv2.emission.color", "emission albedo", { InputType::kInputMap });
    SetInputValue("uberv2.emission.color", f4_one);

    //SSS
    RegisterInput("uberv2.sss.absorption_color", "volume absorption color", { InputType::kInputMap });
    SetInputValue("uberv2.sss.absorption_color", f4_zero);
    RegisterInput("uberv2.sss.scatter_color", "volume scattering color", { InputType::kInputMap });
    SetInputValue("uberv2.sss.scatter_color", f4_zero);
    RegisterInput("uberv2.sss.absorption_distance", "maximum distance the light can travel before absorbed in meters", { InputType::kInputMap });
    SetInputValue("uberv2.sss.absorption_distance", f_zero);
    RegisterInput("uberv2.sss.scatter_distance", "maximum distance the light can travel before scattered", { InputType::kInputMap });
    SetInputValue("uberv2.sss.scatter_distance", f_zero);
    RegisterInput("uberv2.sss.scatter_direction", "scattering direction (G parameter of Henyey-Grenstein scattering function)", { InputType::kInputMap });
    SetInputValue("uberv2.sss.scatter_direction", f_zero);
    RegisterInput("uberv2.sss.subsurface_color", "color of diffuse refraction BRDF", { InputType::kInputMap });
    SetInputValue("uberv2.sss.subsurface_color", f4_one);

    //Transparency
    RegisterInput("uberv2.transparency", "level of transparency", { InputType::kInputMap });
    SetInputValue("uberv2.transparency", f_zero);

    //Normal mapping
    RegisterInput("uberv2.shading_normal", "Shading normal", { InputType::kInputMap });
    SetInputValue("uberv2.shading_normal", f_zero);
}

bool UberV2Material::HasEmission() const
{
    return (layers_ & Layers::kEmissionLayer) == Layers::kEmissionLayer;
}
