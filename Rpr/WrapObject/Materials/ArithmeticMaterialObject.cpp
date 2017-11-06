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
#include "ArithmeticMaterialObject.h"
#include "TextureMaterialObject.h"
#include "WrapObject/Exception.h"
#include "SceneGraph/material.h"

using namespace Baikal;
using namespace RadeonRays;

ArithmeticMaterialObject::ArithmeticMaterialObject(MaterialObject::Type mat_type)
    : MaterialObject(mat_type)
{
    auto albedo = SingleBxdf::Create(SingleBxdf::BxdfType::kLambert);
    albedo->SetInputValue("albedo", { 0.f, 0.f , 0.f , 0.f });
    m_mat = MultiBxdf::Create(MultiBxdf::Type::kMix);
    m_mat->SetInputValue("weight", { 0.5f, 0.5f, 0.5f, 0.5f });
    m_mat->SetInputValue("base_material", albedo);
    m_mat->SetThin(false);
}

void ArithmeticMaterialObject::SetInputMaterial(const std::string& input_name, MaterialObject* input)
{
    if (input_name == "base_material")
    {
        m_mat->SetInputValue("top_material", input->GetMaterial());
    }
    else
    {
        //ignore
        //throw Exception(RPR_ERROR_INTERNAL_ERROR, "Unhandled");
    }
}

void ArithmeticMaterialObject::SetInputTexture(const std::string& input_name, TextureMaterialObject* input)
{
    if (input_name == "base_material")
    {
        auto m = SingleBxdf::Create(SingleBxdf::BxdfType::kLambert);
        m->SetInputValue("albedo", input->GetTexture());
        //m->SetInputValue("albedo", float4(1.f, 0.f, 0.f, 1.f));
        m_mat->SetInputValue("top_material", m);
    }
    else if (input_name == "top_material")
    {
        m_mat->SetInputValue("weight", input->GetTexture());
    }
    else
    {
        //ignore
    }
}


void ArithmeticMaterialObject::SetInputF(const std::string& input_name, const RadeonRays::float4& val)
{
    if (input_name == "top_material")
    {
        float4 w = float4(1.f, 1.f, 1.f, 1.f) - val;
        w = val;
        m_mat->SetInputValue("weight", w);
    }
    else if (input_name == "base_material")
    {
        auto m = SingleBxdf::Create(SingleBxdf::BxdfType::kLambert);
        m->SetInputValue("albedo", val);
        m_mat->SetInputValue("top_material", m);
    }
    else
    {
        throw Exception(RPR_ERROR_INTERNAL_ERROR, "Unhandled");
    }
}

void ArithmeticMaterialObject::Update(MaterialObject* mat)
{

}

Baikal::Material::Ptr ArithmeticMaterialObject::GetMaterial()
{
    return m_mat;
}
