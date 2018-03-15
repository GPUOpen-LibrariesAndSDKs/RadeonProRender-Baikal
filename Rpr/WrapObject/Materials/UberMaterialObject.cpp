#include "UberMaterialObject.h"

#include "TextureMaterialObject.h"
#include "ArithmeticMaterialObject.h"
#include "SceneGraph/uberv2material.h"
#include "SceneGraph/inputmaps.h"
#include "WrapObject/Exception.h"

using namespace RadeonRays;
using namespace Baikal;

UberMaterialObject::UberMaterialObject() :
    MaterialObject(kUberV2)
{
    m_mat = UberV2Material::Create();
}

void UberMaterialObject::SetInputF(const std::string & input_name, const RadeonRays::float4 & val)
{
    auto inputMap = Baikal::InputMap_ConstantFloat3::Create(val);
    m_mat->SetInputValue(input_name, inputMap);
}

void UberMaterialObject::SetInputU(const std::string& input_name, rpr_uint val) 
{

    // First - check if we set values that are parameters now
    if (input_name == "uberv2.layers")
    {
        m_mat->SetLayers(val);
    }
    else if (input_name == "uberv2.refraction.ior_mode")
    {
        m_mat->LinkRefractionIOR(val == RPR_UBER_MATERIAL_REFRACTION_MODE_LINKED);
    }
    else if (input_name == "uberv2.refraction.thin_surface")
    {
        m_mat->SetThin(val);
    }
    else if (input_name == "uberv2.emission.mode")
    {
        m_mat->SetDoubleSided(val == RPR_UBER_MATERIAL_EMISSION_MODE_DOUBLESIDED);
    }
    else if (input_name == "uberv2.sss.multiscatter")
    {
        m_mat->SetMultiscatter(val);
    }
    else
    {
        m_mat->SetInputValue(input_name, val);
    }
}

Baikal::Material::Ptr UberMaterialObject::GetMaterial()
{
    return m_mat;
}

void UberMaterialObject::SetInputMaterial(const std::string & input_name, MaterialObject * input)
{
    if (input->IsArithmetic())
    {
        ArithmeticMaterialObject *arithmetic = static_cast<ArithmeticMaterialObject*>(input);
        m_mat->SetInputValue(input_name, arithmetic->GetInputMap());
    }
    else
    {
        throw Exception(RPR_ERROR_INTERNAL_ERROR, "Only arithmetic nodes allowed as UberV2 inputs");
    }
}

void UberMaterialObject::SetInputTexture(const std::string & input_name, TextureMaterialObject * input)
{
    try
    {
        if (input_name == "uberv2.bump")
        {
            auto sampler = Baikal::InputMap_SamplerBumpMap::Create(input->GetTexture());
            auto remap = Baikal::InputMap_Remap::Create(
                Baikal::InputMap_ConstantFloat3::Create(float3(0.0f, 1.0f, 0.0f)),
                Baikal::InputMap_ConstantFloat3::Create(float3(-1.0f, 1.0f, 0.0f)),
                sampler);
            m_mat->SetInputValue("uberv2.shading_normal", remap);
        }
        else if (input_name == "uberv2.normal")
        {
            auto sampler = Baikal::InputMap_Sampler::Create(input->GetTexture());
            auto remap = Baikal::InputMap_Remap::Create(
                Baikal::InputMap_ConstantFloat3::Create(float3(0.0f, 1.0f, 0.0f)),
                Baikal::InputMap_ConstantFloat3::Create(float3(-1.0f, 1.0f, 0.0f)),
                sampler);
            m_mat->SetInputValue("uberv2.shading_normal", remap);
        }
        else
        {
            m_mat->SetInputValue(input_name, input->GetTexture());
        }
    }
    catch (...)
    {
        //Ignore
        return;
    }
}
