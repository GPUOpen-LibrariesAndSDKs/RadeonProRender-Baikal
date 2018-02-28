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

    //Diffuse
    RegisterInput("uberv2.diffuse.color", "base diffuse albedo", { InputType::kInputMap });
    SetInputValue("uberv2.diffuse.color", InputMap_ConstantFloat4::Create(float4(1.0f, 1.0f, 1.0f, 1.0f)));

    //Reflection
    RegisterInput("uberv2.reflection.color", "base reflection albedo", { InputType::kInputMap });
    SetInputValue("uberv2.reflection.color", InputMap_ConstantFloat4::Create(float4(1.0f, 1.0f, 1.0f)));
    RegisterInput("uberv2.reflection.roughness", "reflection roughness", { InputType::kInputMap });
    SetInputValue("uberv2.reflection.roughness", InputMap_ConstantFloat::Create(0.5f));
    RegisterInput("uberv2.reflection.anisotropy", "level of anisotropy", { InputType::kInputMap });
    SetInputValue("uberv2.reflection.anisotropy", InputMap_ConstantFloat::Create(0.0f));
    RegisterInput("uberv2.reflection.anisotropy_rotation", "orientation of anisotropic component", { InputType::kInputMap });
    SetInputValue("uberv2.reflection.anisotropy_rotation", InputMap_ConstantFloat::Create(0.0f));
    RegisterInput("uberv2.reflection.ior", "index of refraction", { InputType::kInputMap });
    SetInputValue("uberv2.reflection.ior", InputMap_ConstantFloat::Create(1.5f));
    RegisterInput("uberv2.reflection.metalness", "metalness of the material", { InputType::kInputMap });
    SetInputValue("uberv2.reflection.metalness", InputMap_ConstantFloat::Create(0.0f));

    //Coating
    RegisterInput("uberv2.coating.color", "base coating albedo", { InputType::kInputMap });
    SetInputValue("uberv2.coating.color", InputMap_ConstantFloat4::Create(float4(1.0f, 1.0f, 1.0f, 1.0f)));
    RegisterInput("uberv2.coating.ior", "index of refraction", { InputType::kInputMap });
    SetInputValue("uberv2.coating.ior", InputMap_ConstantFloat::Create(1.5f));

    //Refraction
    RegisterInput("uberv2.refraction.color", "base refraction albedo", { InputType::kInputMap });
    SetInputValue("uberv2.refraction.color", InputMap_ConstantFloat4::Create(float4(1.0f, 1.0f, 1.0f, 1.0f)));
    RegisterInput("uberv2.refraction.roughness", "refraction roughness", { InputType::kInputMap });
    SetInputValue("uberv2.refraction.roughness", InputMap_ConstantFloat::Create(0.5f));
    RegisterInput("uberv2.refraction.ior", "index of refraction", { InputType::kInputMap });
    SetInputValue("uberv2.refraction.ior", InputMap_ConstantFloat::Create(1.5f));

    //Emission
    RegisterInput("uberv2.emission.color", "emission albedo", { InputType::kInputMap });
    SetInputValue("uberv2.emission.color", InputMap_ConstantFloat4::Create(float4(1.0f, 1.0f, 1.0f, 1.0f)));

    //SSS
    RegisterInput("uberv2.sss.absorption_color", "volume absorption color", { InputType::kInputMap });
    SetInputValue("uberv2.sss.absorption_color", InputMap_ConstantFloat4::Create(float4(0.0f, 0.0f, 0.0f, 0.0f)));
    RegisterInput("uberv2.sss.scatter_color", "volume scattering color", { InputType::kInputMap });
    SetInputValue("uberv2.sss.scatter_color", InputMap_ConstantFloat4::Create(float4(0.0f, 0.0f, 0.0f, 0.0f)));
    RegisterInput("uberv2.sss.absorption_distance", "maximum distance the light can travel before absorbed in meters", { InputType::kInputMap });
    SetInputValue("uberv2.sss.absorption_distance", InputMap_ConstantFloat::Create(0.0f));
    RegisterInput("uberv2.sss.scatter_distance", "maximum distance the light can travel before scattered", { InputType::kInputMap });
    SetInputValue("uberv2.sss.scatter_distance", InputMap_ConstantFloat::Create(0.0f));
    RegisterInput("uberv2.sss.scatter_direction", "scattering direction (G parameter of Henyey-Grenstein scattering function)", { InputType::kInputMap });
    SetInputValue("uberv2.sss.scatter_direction", InputMap_ConstantFloat::Create(0.0f));
    RegisterInput("uberv2.sss.subsurface_color", "color of diffuse refraction BRDF", { InputType::kInputMap });
    SetInputValue("uberv2.sss.subsurface_color", InputMap_ConstantFloat4::Create(float4(1.0f, 1.0f, 1.0f, 1.0f)));

    //Transparency
    RegisterInput("uberv2.transparency", "level of transparency", { InputType::kInputMap });
    SetInputValue("uberv2.transparency", InputMap_ConstantFloat::Create(0.0f));


    /*RegisterInput("uberv2.normal", "normal map texture", { InputType::kInputMap });
    SetInputValue("uberv2.normal", Texture::Ptr{ nullptr });
    RegisterInput("uberv2.bump", "bump map texture", { InputType::kInputMap });
    SetInputValue("uberv2.bump", Texture::Ptr{ nullptr });*/
    /*        RegisterInput("uberv2.displacement", "uberv2.displacement", );
    SetInputValue("uberv2.displacement", );*/
}

bool UberV2Material::HasEmission() const
{
    return false;// (GetInputValue("uberv2.emission.weight").float_value.sqnorm() != 0);
}
