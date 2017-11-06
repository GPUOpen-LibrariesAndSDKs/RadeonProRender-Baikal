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
#pragma once

#include "MaterialObject.h"

//materials represented as Baikal::SingleBxdf
class SingleBxdfMaterialObject
    : public MaterialObject
{
public:
    SingleBxdfMaterialObject(MaterialObject::Type mat_type, Baikal::SingleBxdf::BxdfType type);
    void SetInputF(const std::string& input_name, const RadeonRays::float4& val) override;

    Baikal::Material::Ptr GetMaterial() override;

protected:
    void SetInputMaterial(const std::string& input_name, MaterialObject* input) override;
    void SetInputTexture(const std::string& input_name, TextureMaterialObject* input) override;
private:

    bool is_base; //to know which one mmat input is active now
    Baikal::Material::Ptr m_mmat; //multibxdf material
    Baikal::Material::Ptr m_base_mat; //singlebxdf material 
    Baikal::Material::Ptr m_top_baterial;

};
