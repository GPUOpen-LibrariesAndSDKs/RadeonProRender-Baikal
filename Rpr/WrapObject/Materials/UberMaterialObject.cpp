#include "UberMaterialObject.h"

#include "TextureMaterialObject.h"

using namespace RadeonRays;
using namespace Baikal;

UberMaterialObject::UberMaterialObject() :
    MaterialObject(kUberV2)
{
    m_mat = UberV2Material::Create();
}

void UberMaterialObject::SetInputF(const std::string & input_name, const RadeonRays::float4 & val)
{
    m_mat->SetInputValue(input_name, val);
}

void UberMaterialObject::SetInputU(const std::string& input_name, rpr_uint val) 
{
    m_mat->SetInputValue(input_name, val);
}

Baikal::Material::Ptr UberMaterialObject::GetMaterial()
{
    return m_mat;
}

void UberMaterialObject::SetInputMaterial(const std::string & input_name, MaterialObject * input)
{
    m_mat->SetInputValue(input_name, input->GetMaterial());
}

void UberMaterialObject::SetInputTexture(const std::string & input_name, TextureMaterialObject * input)
{
    try
    {
        m_mat->SetInputValue(input_name, input->GetTexture());
    }
    catch (...)
    {
        //Ignore
        return;
    }
}
