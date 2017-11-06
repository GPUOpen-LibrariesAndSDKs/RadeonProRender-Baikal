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
#include <OpenImageIO/imageio.h>
#include <map>
#include <memory>

#include "SingleBxdfMaterialObject.h"
#include "TextureMaterialObject.h"
#include "WrapObject/Exception.h"


using namespace RadeonRays;
using namespace Baikal;

SingleBxdfMaterialObject::SingleBxdfMaterialObject(MaterialObject::Type mat_type, Baikal::SingleBxdf::BxdfType type)
    :MaterialObject(mat_type)
    , is_base(true)
{
    m_base_mat = SingleBxdf::Create(type);
    
    m_mmat =  MultiBxdf::Create(MultiBxdf::Type::kMix);
    float4 w = { 0.f, 0.f, 0.f, 0.f };
    m_mmat->SetInputValue("weight", w);
    m_mmat->SetInputValue("base_material", m_base_mat);
    m_mmat->SetInputValue("top_material", m_base_mat);

}

void SingleBxdfMaterialObject::SetInputMaterial(const std::string& input_name, MaterialObject* input)
{
    if (input_name != "albedo")
    {
        throw Exception(RPR_ERROR_INVALID_TAG, "Invalid SingleBxdfMaterialObject input");
    }
    //SingleBxdf not supporting materials as input, so using MultiBxdf instead
    float4 w = { 1.f, 1.f, 1.f, 1.f };
    m_mmat->SetInputValue("weight", w);
    m_mmat->SetInputValue("top_material", input->GetMaterial());
    is_base = false;
}

void SingleBxdfMaterialObject::SetInputTexture(const std::string& input_name, TextureMaterialObject* input)
{
    if (input_name == "albedo")
    {
        float4 w = { 0.f, 0.f, 0.f, 0.f };
        m_mmat->SetInputValue("weight", w);
        is_base = true;
    }
    Material::Ptr active_mat = is_base ? m_base_mat : m_mmat->GetInputValue("top_material").mat_value;
    try
    {
        active_mat->SetInputValue(input_name, input->GetTexture());
    }
    catch (...)
    {
        //ignore
        return;
    }
}

void SingleBxdfMaterialObject::SetInputF(const std::string& input_name, const RadeonRays::float4& val)
{
    if (input_name == "albedo")
    {
        float4 w = { 0.f, 0.f, 0.f, 0.f };
        m_mmat->SetInputValue("weight", w);
        is_base = true;
    }
    Material::Ptr active_mat = is_base ? m_base_mat : m_mmat->GetInputValue("top_material").mat_value;
    m_base_mat->SetInputValue(input_name, val);

    //if roughness is to small replace microfacet by ideal reflect
    if (GetType() && input_name == "roughness")
    {
        if (val.sqnorm() < 1e-6f)
        {
            auto bxdf_mat = std::dynamic_pointer_cast<SingleBxdf>(m_base_mat);
            bxdf_mat->SetBxdfType(Baikal::SingleBxdf::BxdfType::kIdealReflect);
        }
        else
        {
            auto bxdf_mat = std::dynamic_pointer_cast<SingleBxdf>(m_base_mat);
            bxdf_mat->SetBxdfType(Baikal::SingleBxdf::BxdfType::kMicrofacetGGX);
        }
    }
}

Baikal::Material::Ptr SingleBxdfMaterialObject::GetMaterial()
{
    return m_mmat;
}
