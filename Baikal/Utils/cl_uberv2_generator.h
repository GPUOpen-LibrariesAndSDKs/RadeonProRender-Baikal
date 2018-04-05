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

#include <map>

#include "SceneGraph/uberv2material.h"

namespace Baikal
{
    class CLUberV2Generator
    {
    public:
        CLUberV2Generator();
        ~CLUberV2Generator();
        void AddMaterial(UberV2Material::Ptr material);

    private:
        struct UberV2Sources
        {
            std::string m_prepare_inputs;
            std::string m_get_pdf;
            std::string m_sample;
            std::string m_get_bxdf_type;
            std::string m_evaluate;
        };

        void MaterialGeneratePrepareInputs(UberV2Material::Ptr material, UberV2Sources *sources);
        void MaterialGenerateGetPdf(UberV2Material::Ptr material, UberV2Sources *sources);
        void MaterialGenerateSample(UberV2Material::Ptr material, UberV2Sources *sources);
        void MaterialGenerateGetBxDFType(UberV2Material::Ptr material, UberV2Sources *sources);
        void MaterialGenerateEvaluate(UberV2Material::Ptr material, UberV2Sources *sources);

        std::map<uint32_t, UberV2Sources> m_materials;
    };

}
