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
    "float4 GetInputMapFloat4(uint input_id, DifferentialGeometry const* dg, GLOBAL InputMapData const* restrict input_map_values, TEXTURE_ARG_LIST)\n{\n"
    "\tswitch(input_id)\n\t{\n";
const std::string float4_selector_footer = "\t}\n\treturn 0.0f;\n}\n";

const std::string float_selector_header =
    "float GetInputMapFloat(uint input_id, DifferentialGeometry const* dg, GLOBAL InputMapData const* restrict input_map_values, TEXTURE_ARG_LIST)\n{\n"
    "\tswitch(input_id)\n\t{\n";
const std::string float_selector_footer = "\t}\n\treturn 0.0f;\n}\n";

void CLInputMapGenerator::Generate(const Collector& mat_collector, const Collector& tex_collector)
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
        GenerateSingleMaterial(uberv2_material, tex_collector);
    }

    m_source_code += m_read_functions;
    m_source_code += m_float4_selector + float4_selector_footer;
    m_source_code += m_float_selector + float_selector_footer;
    m_source_code += footer;
}

void CLInputMapGenerator::GenerateSingleMaterial(const UberV2Material *material, const Collector& tex_collector)
{
    for (size_t input_id = 0; input_id < material->GetNumInputs(); ++input_id)
    {
        Material::Input input = material->GetInput(input_id);
        assert((!input.info.supported_types.empty()) &&
            ( *(input.info.supported_types.begin()) == Material::InputType::kInputMap));

        auto input_map = input.value.input_map_value;

        GenerateSingleInput(input_map, tex_collector);
    }
}

void CLInputMapGenerator::GenerateSingleInput(std::shared_ptr<Baikal::InputMap> input, const Collector& tex_collector)
{
    if (m_generated_inputs.find(input->GetId()) != m_generated_inputs.end()) return;

    std::string input_id = std::to_string(input->GetId());

    //Generate code for selectors
    m_float4_selector += "\t\tcase " + input_id + ": return ReadInputMap" + input_id + "(dg, input_map_values, TEXTURE_ARGS);\n";
    m_float_selector += "\t\tcase " + input_id + ": return ReadInputMap" + input_id + "(dg, input_map_values, TEXTURE_ARGS).x;\n";

    m_read_functions += "float4 ReadInputMap" + input_id + "(DifferentialGeometry const* dg, GLOBAL InputMapData const* restrict input_map_values, TEXTURE_ARG_LIST)\n{\n"
        "\treturn (float4)(\n\t";

    GenerateInputSource(input, tex_collector);

    m_read_functions += "\t);\n}\n";

    m_generated_inputs.insert(input->GetId());
}

void CLInputMapGenerator::GenerateInputSource(std::shared_ptr<Baikal::InputMap> input, const Collector& tex_collector)
{
    switch (input->m_type)
    {

        case InputMap::InputMapType::kConstantFloat:
        {
            InputMap_ConstantFloat *i = static_cast<InputMap_ConstantFloat*>(input.get());

            ClwScene::InputMapData dta = {0};
            dta.float_value.value.x = i->m_value;
            dta.float_value.value.y = i->m_value;
            dta.float_value.value.z = i->m_value;
            dta.int_values.type = ClwScene::InputMapDataType::kFloat;
            m_input_map.push_back(dta);

            m_read_functions += "((float4)(input_map_values[" + std::to_string(m_input_map.size() - 1) + "].float_value.value, 0.0f))\n";
            break;
        }
        case InputMap::InputMapType::kConstantFloat3:
        {
            InputMap_ConstantFloat3 *i = static_cast<InputMap_ConstantFloat3*>(input.get());

            ClwScene::InputMapData dta = {0};
            dta.float_value.value = i->m_value;
            dta.int_values.type = ClwScene::InputMapDataType::kFloat3;
            m_input_map.push_back(dta);

            m_read_functions += "((float4)(input_map_values[" + std::to_string(m_input_map.size() - 1) + "].float_value.value, 0.0f))\n";
            break;
        }
        case InputMap::InputMapType::kSampler:
        {
            InputMap_Sampler *i = static_cast<InputMap_Sampler*>(input.get());

            ClwScene::InputMapData dta = {0};
            dta.int_values.idx = tex_collector.GetItemIndex(i->m_texture);
            dta.int_values.type = ClwScene::InputMapDataType::kFloat3;
            m_input_map.push_back(dta);
            m_read_functions += "(Texture_Sample2D(dg->uv, TEXTURE_ARGS_IDX(input_map_values[" + std::to_string(m_input_map.size() - 1) + "].int_values.idx)))\n";
            break;
        }
        // Two inputs
        case InputMap::InputMapType::kAdd:
        {
            InputMap_Add *i = static_cast<InputMap_Add*>(input.get());

            m_read_functions += "(\n\t\t";
            GenerateInputSource(i->m_a, tex_collector);
            m_read_functions +=  "\t)\n\t + \n\t(\n\t\t";
            GenerateInputSource(i->m_b, tex_collector);
            m_read_functions += "\t)\n";
            break;
        }
        case InputMap::InputMapType::kSub:
        {
            InputMap_Sub *i = static_cast<InputMap_Sub*>(input.get());

            m_read_functions += "(\n\t\t";
            GenerateInputSource(i->m_a, tex_collector);
            m_read_functions +=  "\t)\n\t - \n\t(\n\t\t";
            GenerateInputSource(i->m_b, tex_collector);
            m_read_functions += "\t)\n";
            break;
        }
        case InputMap::InputMapType::kMul:
        {
            InputMap_Mul *i = static_cast<InputMap_Mul*>(input.get());

            m_read_functions += "(\n\t\t";
            GenerateInputSource(i->m_a, tex_collector);
            m_read_functions +=  "\t)\n\t * \n\t(\n\t\t";
            GenerateInputSource(i->m_b, tex_collector);
            m_read_functions += "\t)\n";
            break;
        }
        case InputMap::InputMapType::kDiv:
        {
            InputMap_Div *i = static_cast<InputMap_Div*>(input.get());

            m_read_functions += "(\n\t\t";
            GenerateInputSource(i->m_a, tex_collector);
            m_read_functions +=  "\t)\n\t / \n\t(\n\t\t";
            GenerateInputSource(i->m_b, tex_collector);
            m_read_functions += "\t)\n";
            break;
        }
        case InputMap::InputMapType::kMin:
        {
            InputMap_Min *i = static_cast<InputMap_Min*>(input.get());

            m_read_functions += "min(\n\t\t";
            GenerateInputSource(i->m_a, tex_collector);
            m_read_functions +=  "\t, \n\t\t";
            GenerateInputSource(i->m_b, tex_collector);
            m_read_functions += "\t)\n";
            break;
        }
        case InputMap::InputMapType::kMax:
        {
            InputMap_Max *i = static_cast<InputMap_Max*>(input.get());

            m_read_functions += "max(\n\t\t";
            GenerateInputSource(i->m_a, tex_collector);
            m_read_functions +=  "\t, \n\t\t";
            GenerateInputSource(i->m_b, tex_collector);
            m_read_functions += "\t)\n";
            break;
        }
        case InputMap::InputMapType::kDot3:
        {
            InputMap_Dot3 *i = static_cast<InputMap_Dot3*>(input.get());

            m_read_functions += "((float4)(dot(\n\t\t";
            GenerateInputSource(i->m_a, tex_collector);
            m_read_functions +=  ".xyz\t, \n\t\t";
            GenerateInputSource(i->m_b, tex_collector);
            m_read_functions += ".xyz\t), 0.0f, 0.0f, 0.0f))\n";
            break;
        }
        case InputMap::InputMapType::kDot4:
        {
            InputMap_Dot4 *i = static_cast<InputMap_Dot4*>(input.get());

            m_read_functions += "((float4)(dot(\n\t\t";
            GenerateInputSource(i->m_a, tex_collector);
            m_read_functions +=  "\t, \n\t\t";
            GenerateInputSource(i->m_b, tex_collector);
            m_read_functions += "\t), 0.0f, 0.0f, 0.0f))\n";
            break;
        }
        case InputMap::InputMapType::kCross3:
        {
            InputMap_Cross3 *i = static_cast<InputMap_Cross3*>(input.get());

            m_read_functions += "((float4)(cross(\n\t\t";
            GenerateInputSource(i->m_a, tex_collector);
            m_read_functions +=  ".xyz\t, \n\t\t";
            GenerateInputSource(i->m_b, tex_collector);
            m_read_functions += ".xyz\t), 0.0f))\n";
            break;
        }
        case InputMap::InputMapType::kCross4:
        {
            InputMap_Cross4 *i = static_cast<InputMap_Cross4*>(input.get());

            m_read_functions += "cross(\n\t\t";
            GenerateInputSource(i->m_a, tex_collector);
            m_read_functions +=  "\t, \n\t\t";
            GenerateInputSource(i->m_b, tex_collector);
            m_read_functions += "\t)\n";
            break;
        }
        case InputMap::InputMapType::kPow:
        {
            InputMap_Pow *i = static_cast<InputMap_Pow*>(input.get());

            m_read_functions += "pow(\n\t\t";
            GenerateInputSource(i->m_a, tex_collector);
            m_read_functions +=  "\t, \n\t\t";
            GenerateInputSource(i->m_b, tex_collector);
            m_read_functions += ".x\t)\n";
            break;
        }
        case InputMap::InputMapType::kMod:
        {
            InputMap_Mod *i = static_cast<InputMap_Mod*>(input.get());

            m_read_functions += "fmod(\n\t\t";
            GenerateInputSource(i->m_a, tex_collector);
            m_read_functions +=  "\t, \n\t\t";
            GenerateInputSource(i->m_b, tex_collector);
            m_read_functions += "\t)\n";
            break;
        }
        //Single input
        case InputMap::InputMapType::kSin:
        {
            InputMap_Sin *i = static_cast<InputMap_Sin*>(input.get());

            m_read_functions += "sin(\n\t\t";
            GenerateInputSource(i->m_arg, tex_collector);
            m_read_functions += "\t)\n";
            break;
        }
        case InputMap::InputMapType::kCos:
        {
            InputMap_Cos *i = static_cast<InputMap_Cos*>(input.get());

            m_read_functions += "cos(\n\t\t";
            GenerateInputSource(i->m_arg, tex_collector);
            m_read_functions += "\t)\n";
            break;
        }
        case InputMap::InputMapType::kTan:
        {
            InputMap_Tan *i = static_cast<InputMap_Tan*>(input.get());

            m_read_functions += "tan(\n\t\t";
            GenerateInputSource(i->m_arg, tex_collector);
            m_read_functions += "\t)\n";
            break;
        }
        case InputMap::InputMapType::kAsin:
        {
            InputMap_Asin *i = static_cast<InputMap_Asin*>(input.get());

            m_read_functions += "asin(\n\t\t";
            GenerateInputSource(i->m_arg, tex_collector);
            m_read_functions += "\t)\n";
            break;
        }
        case InputMap::InputMapType::kAcos:
        {
            InputMap_Acos *i = static_cast<InputMap_Acos*>(input.get());

            m_read_functions += "acos(\n\t\t";
            GenerateInputSource(i->m_arg, tex_collector);
            m_read_functions += "\t)\n";
            break;
        }
        case InputMap::InputMapType::kAtan:
        {
            InputMap_Atan *i = static_cast<InputMap_Atan*>(input.get());

            m_read_functions += "atan(\n\t\t";
            GenerateInputSource(i->m_arg, tex_collector);
            m_read_functions += "\t)\n";
            break;
        }
        case InputMap::InputMapType::kLength3:
        {
            InputMap_Length3 *i = static_cast<InputMap_Length3*>(input.get());

            m_read_functions += "(float4)(length(\n\t\t";
            GenerateInputSource(i->m_arg, tex_collector);
            m_read_functions += ".xyz\t), 0.0f, 0.0f, 0.0f)\n";
            break;
        }
        case InputMap::InputMapType::kNormalize3:
        {
            InputMap_Normalize3 *i = static_cast<InputMap_Normalize3*>(input.get());

            m_read_functions += "(float4)(normalize(\n\t\t";
            GenerateInputSource(i->m_arg, tex_collector);
            m_read_functions += ".xyz\t), 0.0f)\n";
            break;
        }
        case InputMap::InputMapType::kFloor:
        {
            InputMap_Floor *i = static_cast<InputMap_Floor*>(input.get());

            m_read_functions += "floor(\n\t\t";
            GenerateInputSource(i->m_arg, tex_collector);
            m_read_functions += "\t)\n";
            break;
        }
        case InputMap::InputMapType::kAbs:
        {
            InputMap_Abs *i = static_cast<InputMap_Abs*>(input.get());

            m_read_functions += "fabs(\n\t\t";
            GenerateInputSource(i->m_arg, tex_collector);
            m_read_functions += "\t)\n";
            break;
        }
        // Specials
        case InputMap::InputMapType::kLerp:
        {
            InputMap_Lerp *i = static_cast<InputMap_Lerp*>(input.get());

            m_read_functions += "mix(\n\t\t";
            GenerateInputSource(i->m_a, tex_collector);
            m_read_functions +=  "\t, \n\t\t";
            GenerateInputSource(i->m_b, tex_collector);
            m_read_functions +=  "\t, \n\t\t";
            GenerateInputSource(i->m_control, tex_collector);
            m_read_functions += "\t)\n";
            break;
        }
        case InputMap::InputMapType::kSelect:
        {
            InputMap_Select *i = static_cast<InputMap_Select*>(input.get());
            static const std::vector<std::string> selection_to_text = { ".x", ".y", ".z", ".w" };
            assert(static_cast<uint32_t>(i->m_selection) < selection_to_text.size());

            m_read_functions += "(\n\t\t";
            GenerateInputSource(i->m_arg, tex_collector);
            m_read_functions +=  selection_to_text[static_cast<uint32_t>(i->m_selection)];
            m_read_functions += "\n\t)\n";
            break;
        }
        case InputMap::InputMapType::kShuffle:
        {
            InputMap_Shuffle *i = static_cast<InputMap_Shuffle*>(input.get());

            m_read_functions += "shuffle(\n\t\t";
            GenerateInputSource(i->m_arg, tex_collector);
            m_read_functions += "\t, \n\t\t";
            m_read_functions += "(uint4)(" + std::to_string(i->m_mask[0]) + ", " + std::to_string(i->m_mask[1]) + ", " + std::to_string(i->m_mask[2]) + ", " + std::to_string(i->m_mask[3]) + ")\n";
            m_read_functions += "\t)\n";
            break;
        }
        case InputMap::InputMapType::kShuffle2:
        {
            InputMap_Shuffle2 *i = static_cast<InputMap_Shuffle2*>(input.get());

            m_read_functions += "shuffle2(\n\t\t";
            GenerateInputSource(i->m_a, tex_collector);
            m_read_functions += "\t, \n\t\t";
            GenerateInputSource(i->m_b, tex_collector);
            m_read_functions += "\t, \n\t\t";
            m_read_functions += "(uint4)(" + std::to_string(i->m_mask[0]) + ", " + std::to_string(i->m_mask[1]) + ", " + std::to_string(i->m_mask[2]) + ", " + std::to_string(i->m_mask[3]) + ")\n";
            m_read_functions += "\t)\n";
            break;
        }
        case InputMap::InputMapType::kMatMul:
        {
            InputMap_MatMul *i = static_cast<InputMap_MatMul*>(input.get());

            m_read_functions += "matrix_mul_vector4(\n\t\t";
            //Generate matrix
            m_read_functions += "matrix_from_rows(\n\t\t\t";
            m_read_functions += "make_float4(" +
                std::to_string(i->m_mat4.m00) + ", " +
                std::to_string(i->m_mat4.m01) + ", " +
                std::to_string(i->m_mat4.m02) + ", " +
                std::to_string(i->m_mat4.m03) + "), \n\t\t\t";
            m_read_functions += "make_float4(" +
                std::to_string(i->m_mat4.m10) + ", " +
                std::to_string(i->m_mat4.m11) + ", " +
                std::to_string(i->m_mat4.m12) + ", " +
                std::to_string(i->m_mat4.m13) + "), \n\t\t\t";
            m_read_functions += "make_float4(" +
                std::to_string(i->m_mat4.m20) + ", " +
                std::to_string(i->m_mat4.m21) + ", " +
                std::to_string(i->m_mat4.m22) + ", " +
                std::to_string(i->m_mat4.m23) + "), \n\t\t\t";
            m_read_functions += "make_float4(" +
                std::to_string(i->m_mat4.m30) + ", " +
                std::to_string(i->m_mat4.m31) + ", " +
                std::to_string(i->m_mat4.m32) + ", " +
                std::to_string(i->m_mat4.m33) + ")),\n\t\t(";
            GenerateInputSource(i->m_arg, tex_collector);
            m_read_functions +=  "\t)\n\t)";
            break;
        }


    }
}
