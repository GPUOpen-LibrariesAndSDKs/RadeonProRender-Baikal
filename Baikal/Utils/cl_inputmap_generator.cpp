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

#include <assert.h>

#include "cl_inputmap_generator.h"
#include "SceneGraph/uberv2material.h"

using namespace Baikal;

const std::string header = "#ifndef INPUTMAPS_CL\n#define INPUTMAPS_CL\n\n";
const std::string footer = "#endif\n\n";

const std::string float4_selector_header = 
    "float4 GetInputMapFloat4(uint input_id)\n{\n"
    "\tswitch(input_id)\n\t{\n";
const std::string float4_selector_footer = "\t}\n}\n";

const std::string float_selector_header = 
    "float GetInputMapFloat(uint input_id)\n{\n"
    "\tswitch(input_id)\n\t{\n";
const std::string float_selector_footer = "\t}\n}\n";

void CLInputMapGenerator::Generate(const Collector& mat_collector)
{
    m_source_code = header;
    m_read_functions.clear();
    m_float4_selector = float4_selector_header;
    m_float_selector = float_selector_header;
    
    auto mat_iter = mat_collector.CreateIterator();
    for (; mat_iter->IsValid(); mat_iter->Next())
    {
        auto material = mat_iter->ItemAs<Material>();
        const UberV2Material *uberv2_material = dynamic_cast<const UberV2Material*>(material.get());
        // We need to generate source only for UberV2 materials
        if (!uberv2_material)
        {
            continue;
        }
        GenerateSingleMaterial(uberv2_material);
    }

    m_source_code += m_read_functions;
    m_source_code += m_float4_selector + float4_selector_footer;
    m_source_code += m_float_selector + float_selector_footer;
    m_source_code += footer;
}

void CLInputMapGenerator::GenerateSingleMaterial(const UberV2Material *material)
{
    for (size_t input_id = 0; input_id < material->GetNumInputs(); ++input_id)
    {
        Material::Input input = material->GetInput(input_id);
        assert((!input.info.supported_types.empty()) && 
            ( *(input.info.supported_types.begin()) == Material::InputType::kInputMap));

        auto input_map = input.value.input_map_value;

        GenerateSingleInput(input_map);
    }
}

void CLInputMapGenerator::GenerateSingleInput(std::shared_ptr<Baikal::InputMap> input)
{

}
