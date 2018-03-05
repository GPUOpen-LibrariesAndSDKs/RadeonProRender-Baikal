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

#include <assert.h>

#include "cl_inputmap_generator.h"
#include "SceneGraph/uberv2material.h"
#include "SceneGraph/inputmaps.h"

using namespace Baikal;

const std::string header = "#ifndef INPUTMAPS_CL\n#define INPUTMAPS_CL\n\n";
const std::string footer = "#endif\n\n";

const std::string float4_selector_header =
    "float4 GetInputMapFloat4(uint input_id, DifferentialGeometry const* dg, GLOBAL InputMapData const* restrict input_map_values)\n{\n"
    "\tswitch(input_id)\n\t{\n";
const std::string float4_selector_footer = "\t}\n\treturn 0.0f;\n}\n";

const std::string float_selector_header =
    "float GetInputMapFloat(uint input_id, DifferentialGeometry const* dg, GLOBAL InputMapData const* restrict input_map_values)\n{\n"
    "\tswitch(input_id)\n\t{\n";
const std::string float_selector_footer = "\t}\n\treturn 0.0f;\n}\n";

void CLInputMapGenerator::Generate(const Collector& mat_collector)
{
    m_source_code = header;
    m_read_functions.clear();
    m_float4_selector = float4_selector_header;
    m_float_selector = float_selector_header;
    m_input_map.clear();

    m_generated_inputs.clear();
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
    if (m_generated_inputs.find(input->GetId()) != m_generated_inputs.end()) return;

    std::string input_id = std::to_string(input->GetId());

    //Generate code for selectors
    m_float4_selector += "\t\tcase " + input_id + ": return ReadInputMap" + input_id + "(dg, input_map_values);\n";
    m_float_selector += "\t\tcase " + input_id + ": return ReadInputMap" + input_id + "(dg, input_map_values).x;\n";

    m_read_functions += "float4 ReadInputMap" + input_id + "(DifferentialGeometry const* dg, GLOBAL InputMapData const* restrict input_map_values)\n{\n"
        "\treturn (float4)(\n\t";

    GenerateInputSource(input);

    m_read_functions += "\t);\n}\n";

    m_generated_inputs.insert(input->GetId());
}

void CLInputMapGenerator::GenerateInputSource(std::shared_ptr<Baikal::InputMap> input)
{
    if (input->m_type == InputMap::InputMapType::kConstantFloat)
    {
        InputMap_ConstantFloat *i = static_cast<InputMap_ConstantFloat*>(input.get());

        ClwScene::InputMapData dta = {0};
        dta.float_value.value.x = i->m_value;
        dta.int_values.type = ClwScene::InputMapDataType::kFloat;
        m_input_map.push_back(dta);

        m_read_functions += "(float4)(input_map_values[" + std::to_string(m_input_map.size() - 1) + "].float_value.value, 0.0f)\n";
    }
    else if(input->m_type == InputMap::InputMapType::kConstantFloat3)
    {
        InputMap_ConstantFloat3 *i = static_cast<InputMap_ConstantFloat3*>(input.get());

        ClwScene::InputMapData dta = {0};
        dta.float_value.value = i->m_value;
        dta.int_values.type = ClwScene::InputMapDataType::kFloat3;
        m_input_map.push_back(dta);

        m_read_functions += "(float4)(input_map_values[" + std::to_string(m_input_map.size() - 1) + "].float_value.value, 0.0f)\n";
    }
    else if(input->m_type == InputMap::InputMapType::kAdd)
    {
        InputMap_Add *i = static_cast<InputMap_Add*>(input.get());


        m_read_functions += "(\n\t\t";
        GenerateInputSource(i->m_a);
        m_read_functions +=  "\t)\n\t + \n\t(\n\t\t";
        GenerateInputSource(i->m_b);
        m_read_functions += "\t)\n";
    }
    else if(input->m_type == InputMap::InputMapType::kSub)
    {
        InputMap_Sub *i = static_cast<InputMap_Sub*>(input.get());


        m_read_functions += "(\n\t\t";
        GenerateInputSource(i->m_a);
        m_read_functions +=  "\t)\n\t - \n\t(\n\t\t";
        GenerateInputSource(i->m_b);
        m_read_functions += "\t)\n";
    }
    else if(input->m_type == InputMap::InputMapType::kMul)
    {
        InputMap_Mul *i = static_cast<InputMap_Mul*>(input.get());


        m_read_functions += "(\n\t\t";
        GenerateInputSource(i->m_a);
        m_read_functions +=  "\t)\n\t * \n\t(\n\t\t";
        GenerateInputSource(i->m_b);
        m_read_functions += "\t)\n";
    }
    else if(input->m_type == InputMap::InputMapType::kDiv)
    {
        InputMap_Div *i = static_cast<InputMap_Div*>(input.get());


        m_read_functions += "(\n\t\t";
        GenerateInputSource(i->m_a);
        m_read_functions +=  "\t)\n\t / \n\t(\n\t\t";
        GenerateInputSource(i->m_b);
        m_read_functions += "\t)\n";
    }


}
