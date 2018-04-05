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

#include "cl_uberv2_generator.h"

using namespace Baikal;

CLUberV2Generator::CLUberV2Generator()
{

}

CLUberV2Generator::~CLUberV2Generator()
{
}

void CLUberV2Generator::AddMaterial(UberV2Material::Ptr material)
{
    uint32_t layers = material->GetLayers();
    // Check if we already have such material type processed
    if (m_materials.find(layers) != m_materials.end())
    {
        return;
    }
    
    UberV2Sources src;
    MaterialGeneratePrepareInputs(material, &src);
    MaterialGenerateGetPdf(material, &src);
    MaterialGenerateSample(material, &src);
    MaterialGenerateGetBxDFType(material, &src);
    MaterialGenerateEvaluate(material, &src);
    m_materials[layers] = src;
}

void CLUberV2Generator::MaterialGeneratePrepareInputs(UberV2Material::Ptr material, UberV2Sources *sources)
{

}

void CLUberV2Generator::MaterialGenerateGetPdf(UberV2Material::Ptr material, UberV2Sources *sources)
{

}

void CLUberV2Generator::MaterialGenerateSample(UberV2Material::Ptr material, UberV2Sources *sources)
{

}

void CLUberV2Generator::MaterialGenerateGetBxDFType(UberV2Material::Ptr material, UberV2Sources *sources)
{

}

void CLUberV2Generator::MaterialGenerateEvaluate(UberV2Material::Ptr material, UberV2Sources *sources)
{

}
