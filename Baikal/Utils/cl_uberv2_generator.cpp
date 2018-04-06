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
    static const std::string reader_function_arguments("material_attributes[offset++], dg, input_map_values, TEXTURE_ARGS");
    struct LayerReader
    {
        std::string variable;
        std::string reader;
    };
    // Should have same order as in ClwSceneController::WriteMaterial
    static const std::vector<std::pair<UberV2Material::Layers, std::vector<LayerReader>>> readers =
    {
        {
            UberV2Material::Layers::kEmissionLayer,
            {
                {"shader_data.emission_color", "GetInputMapFloat4"}
            }
        },
        {
            UberV2Material::Layers::kCoatingLayer,
            {
                {"shader_data.coating_color", "GetInputMapFloat4"},
                {"shader_data.coating_ior", "GetInputMapFloat"}
            }
        },
        {
            UberV2Material::Layers::kReflectionLayer,
            {
                {"shader_data.coating_color", "GetInputMapFloat4"},
                {"shader_data.coating_ior", "GetInputMapFloat"},
                {"shader_data.reflection_color", "GetInputMapFloat4"},
                {"shader_data.reflection_roughness", "GetInputMapFloat"},
                {"shader_data.reflection_anisotropy", "GetInputMapFloat"},
                {"shader_data.reflection_anisotropy_rotation", "GetInputMapFloat"},
                {"shader_data.reflection_ior", "GetInputMapFloat"},
                {"shader_data.reflection_metalness", "GetInputMapFloat"}
            }
        },
        {
            UberV2Material::Layers::kDiffuseLayer,
            {
                {"shader_data.diffuse_color", "GetInputMapFloat4"}
            }
        },
        {
            UberV2Material::Layers::kRefractionLayer,
            {
                {"shader_data.refraction_color", "GetInputMapFloat4"},
                {"shader_data.refraction_roughness", "GetInputMapFloat"},
                {"shader_data.refraction_ior", "GetInputMapFloat"}
            }
        },
        {
            UberV2Material::Layers::kTransparencyLayer,
            {
                {"shader_data.transparency", "GetInputMapFloat"}
            }
        },
        {
            UberV2Material::Layers::kShadingNormalLayer,
            {
                {"shader_data.shading_normal", "GetInputMapFloat4"},
            }
        },
        {
            UberV2Material::Layers::kSSSLayer,
            {
                {"shader_data.sss_absorption_color", "GetInputMapFloat4"},
                {"shader_data.sss_scatter_color", "GetInputMapFloat4"},
                {"shader_data.sss_subsurface_color", "GetInputMapFloat4"},
                {"shader_data.sss_absorption_distance", "GetInputMapFloat"},
                {"shader_data.sss_scatter_distance", "GetInputMapFloat"},
                {"shader_data.sss_scatter_direction", "GetInputMapFloat"},
            }
        }
    };


    // Reserve 4k. This should be enought for maximum configuration
    sources->m_prepare_inputs.reserve(4096);
    // Write base code
    uint32_t layers = material->GetLayers();
    sources->m_prepare_inputs =
        "UberV2ShaderData UberV2PrepareInputs" + std::to_string(layers) + "("
        "DifferentialGeometry const* dg, GLOBAL InputMapData const* restrict input_map_values,"
        "GLOBAL int const* restrict material_attributes, TEXTURE_ARG_LIST)\n"
        "{\n"
        "\tUberV2ShaderData shader_data;\n"
        "\tint offset = dg->mat.offset + 1;\n";

    // Write code for each layer
    for (auto &reader: readers)
    {
        // If current layer exist in material - write it
        if ((layers & reader.first) == reader.first)
        {
            for (auto &r : reader.second)
            {
                sources->m_prepare_inputs += std::string("\t") + r.variable + " = " + r.reader + "(" + reader_function_arguments + ");\n";
            }
        }
    }

    sources->m_prepare_inputs += "\treturn shader_data;\n}";
}

void CLUberV2Generator::MaterialGenerateGetPdf(UberV2Material::Ptr material, UberV2Sources *sources)
{

}

void CLUberV2Generator::MaterialGenerateSample(UberV2Material::Ptr material, UberV2Sources *sources)
{
    uint32_t layers = material->GetLayers();
    sources->m_sample =
        "float3 UberV2_Sample(DifferentialGeometry const* dg, float3 wi, TEXTURE_ARG_LIST, float2 sample, float3* wo, float* pdf,
        "UberV2ShaderData const* shader_data)\n"
        "{\n"
        "\tconst int sampledComponent = Bxdf_UberV2_GetSampledComponent(dg);\n";








    soruces->m_sample +=
        "\treturn 0.f;\n"
        "}\n";

}
void CLUberV2Generator::MaterialGenerateGetBxDFType(UberV2Material::Ptr material, UberV2Sources *sources)
{

    uint32_t layers = material->GetLayers();
    sources->m_get_bxdf_type = "void GetMaterialBxDFType" + std::to_string(layers) + "("
        "float3 wi, Sampler* sampler, SAMPLER_ARG_LIST, DifferentialGeometry* dg, UberV2ShaderData const* shader_data)\n"
        "{\n";
        "\tint bxdf_flags = 0;\n";

    // If we have layer that requires fresnel blend (reflection/refraction/coating)
    if ((layers &
        (UberV2Material::Layers::kReflectionLayer | UberV2Material::Layers::kRefractionLayer | UberV2Material::Layers::kCoatingLayer ))
        != 0)
    {

        sources->m_get_bxdf_type += "\tconst float ndotwi = dot(dg->n, wi);\n";
    }

    if ((layers & UberV2Material::Layers::kEmissionLayer) == UberV2Material::Layers::kEmissionLayer)
    {
        sources->m_get_bxdf_type += "\tbxdf_flags |= kBxdfFlagsEmissive;\n";
    }

    if ((layers & UberV2Material::Layers::kTransparencyLayer) == UberV2Material::Layers::kTransparencyLayer)
    {
        // If layer have transparency check in OpenCL if we sample it and if we sample it - return
        sources->m_get_bxdf_type +=
            "\tfloat sample = Sampler_Sample1D(sampler, SAMPLER_ARGS);\n"
            "\tif (sample < shader_data->transparency)\n"
            "\t{\n"
            "\t\tbxdf_flags |= (kBxdfFlagsTransparency | kBxdfFlagsSingular);\n"
            "\t\tBxdf_SetFlags(dg, bxdf_flags);\n"
            "\t\tBxdf_UberV2_SetSampledComponent(dg, kBxdfUberV2SampleTransparency);\n"
            "\t\treturn;\n"
            "\t}\n";
    }

    // Check refraction layer. If we have it and plan to sample it - set flags and sampled component
    if ((layers & UberV2Material::Layers::kRefractionLayer) == UberV2Material::Layers::kRefractionLayer)
    {
        sources->m_get_bxdf_type +=
            "\tfloat sample = Sampler_Sample1D(sampler, SAMPLER_ARGS);\n"
            "\tconst float fresnel = CalculateFresnel(1.0f, shader_data->refraction_ior, ndotwi);\n"
            "\tif (sample >= fresnel)\n"
            "\t{\n"
            "\t\tBxdf_UberV2_SetSampledComponent(dg, kBxdfUberV2SampleRefraction);\n"
            "\t\tif (shader_data->refraction_roughness < ROUGHNESS_EPS)\n"
            "\t\t{\n"
            "\t\t\tbxdf_flags |= kBxdfFlagsSingular;\n"
            "\t\t}\n"
            "\t\tBxdf_SetFlags(dg, bxdf_flags);"
            "\t\treturn;\n"
            "\t}\n";
    }

    sources->m_get_bxdf_type +=
        "\tbxdf_flags |= kBxdfFlagsBrdf;\n"
        "\tfloat top_ior = 1.0f;\n";

    if ((layers & UberV2Material::Layers::kCoatingLayer) == UberV2Material::Layers::kCoatingLayer)
    {
        sources->m_get_bxdf_type +=
            "\tfloat sample = Sampler_Sample1D(sampler, SAMPLER_ARGS);\n"
            "\tconst float fresnel = CalculateFresnel(top_ior, shader_data->coating_ior, ndotwi);\n"
            "\tif (sample < fresnel)\n"
            "\t{\n"
            "\t\tbxdf_flags |= kBxdfFlagsSingular;\n"
            "\t\tBxdf_SetFlags(dg, bxdf_flags);\n"
            "\t\tBxdf_UberV2_SetSampledComponent(dg, kBxdfUberV2SampleCoating);\n"
            "\t\treturn;\n"
            "\t}\n"
            "\ttop_ior = shader_data->coating_ior;\n";
    }

    if ((layers & UberV2Material::Layers::kReflectionLayer) == UberV2Material::Layers::kReflectionLayer)
    {
        sources->m_get_bxdf_type +=
            "\tconst float fresnel = CalculateFresnel(top_ior, shader_data->reflection_ior, ndotwi);\n"
            "\tfloat sample = Sampler_Sample1D(sampler, SAMPLER_ARGS);\n"
            "\tif (sample < fresnel)\n"
            "\t{\n"
            "\t\tif (shader_data->reflection_roughness < ROUGHNESS_EPS)\n"
            "\t\t{\n"
            "\t\t\tbxdf_flags |= kBxdfFlagsSingular;\n"
            "\t\t}\n"
            "\t\tBxdf_UberV2_SetSampledComponent(dg, kBxdfUberV2SampleReflection);\n"
            "\t\tBxdf_SetFlags(dg, bxdf_flags);\n"
            "\t\treturn;\n"
            "\t}\n";

    }

    sources->m_get_bxdf_type +=
        "\tBxdf_UberV2_SetSampledComponent(dg, kBxdfUberV2SampleDiffuse);\n"
        "\tBxdf_SetFlags(dg, bxdf_flags);\n"
        "return;\n"
        "}\n";
}

void CLUberV2Generator::MaterialGenerateEvaluate(UberV2Material::Ptr material, UberV2Sources *sources)
{

}
