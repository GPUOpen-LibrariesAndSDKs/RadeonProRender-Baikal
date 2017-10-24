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
#include <OpenImageIO/imageio.h>
#include <map>
#include <memory>

#include "math/int2.h"
#include "SceneGraph/texture.h"
#include "SceneGraph/material.h"
#include "SceneGraph/IO/image_io.h"
#include "SceneGraph/iterator.h"
#include "WrapObject/MaterialObject.h"
#include "WrapObject/Exception.h"


using namespace RadeonRays;
using namespace Baikal;

namespace
{
    //contains pairs <rpr input name, baikal input name> of input names
    const std::map<std::string, std::string> kInputNamesDictionary = { { "color" , "albedo" },
                                                                { "normal" , "normal" },
																{ "roughness" , "roughness" },
																{ "roughness_x" , "roughness" },
                                                                { "weight" , "weight" }, 
                                                                { "ior" , "ior" },
                                                                { "color0", "base_material" },
                                                                { "color1", "top_material" }, 
                                                                { "data", "data" }, };

    std::map<uint32_t, std::string> kMaterialNodeInputStrings = {
        { RPR_MATERIAL_INPUT_COLOR, "color" },
        { RPR_MATERIAL_INPUT_COLOR0, "color0" },
        { RPR_MATERIAL_INPUT_COLOR1, "color1" },
        { RPR_MATERIAL_INPUT_NORMAL, "normal" },
        { RPR_MATERIAL_INPUT_UV, "uv" },
        { RPR_MATERIAL_INPUT_DATA, "data" },
        { RPR_MATERIAL_INPUT_ROUGHNESS, "roughness" },
        { RPR_MATERIAL_INPUT_IOR, "ior" },
        { RPR_MATERIAL_INPUT_ROUGHNESS_X, "roughness_x" },
        { RPR_MATERIAL_INPUT_ROUGHNESS_Y, "roughness_y" },
        { RPR_MATERIAL_INPUT_ROTATION, "rotation" },
        { RPR_MATERIAL_INPUT_WEIGHT, "weight" },
        { RPR_MATERIAL_INPUT_OP, "op" },
        { RPR_MATERIAL_INPUT_INVEC, "invec" },
        { RPR_MATERIAL_INPUT_UV_SCALE, "uv_scale" },
        { RPR_MATERIAL_INPUT_VALUE, "value" },
        { RPR_MATERIAL_INPUT_REFLECTANCE, "reflectance" },
        { RPR_MATERIAL_INPUT_SCALE, "bumpscale" },
        { RPR_MATERIAL_INPUT_SCATTERING, "sigmas" },
        { RPR_MATERIAL_INPUT_ABSORBTION, "sigmaa" },
        { RPR_MATERIAL_INPUT_EMISSION, "emission" },
        { RPR_MATERIAL_INPUT_G, "g" },
        { RPR_MATERIAL_INPUT_MULTISCATTER, "multiscatter" },

        { RPR_MATERIAL_STANDARD_INPUT_DIFFUSE_COLOR, "diffuse.color" },
        { RPR_MATERIAL_STANDARD_INPUT_DIFFUSE_NORMAL, "diffuse.normal" },
        { RPR_MATERIAL_STANDARD_INPUT_GLOSSY_COLOR, "glossy.color" },
        { RPR_MATERIAL_STANDARD_INPUT_GLOSSY_NORMAL, "glossy.normal" },
        { RPR_MATERIAL_STANDARD_INPUT_GLOSSY_ROUGHNESS_X, "glossy.roughness_x" },
        { RPR_MATERIAL_STANDARD_INPUT_GLOSSY_ROUGHNESS_Y, "glossy.roughness_y" },
        { RPR_MATERIAL_STANDARD_INPUT_CLEARCOAT_COLOR, "clearcoat.color" },
        { RPR_MATERIAL_STANDARD_INPUT_CLEARCOAT_NORMAL, "clearcoat.normal" },
        { RPR_MATERIAL_STANDARD_INPUT_REFRACTION_COLOR, "refraction.color" },
        { RPR_MATERIAL_STANDARD_INPUT_REFRACTION_NORMAL, "refraction.normal" },
        { RPR_MATERIAL_STANDARD_INPUT_REFRACTION_ROUGHNESS, "refraction.roughness" },   //  REFRACTION doesn't have roughness parameter.
        { RPR_MATERIAL_STANDARD_INPUT_REFRACTION_IOR, "refraction.ior" },
        { RPR_MATERIAL_STANDARD_INPUT_TRANSPARENCY_COLOR, "transparency.color" },
        { RPR_MATERIAL_STANDARD_INPUT_DIFFUSE_TO_REFRACTION_WEIGHT, "weights.diffuse2refraction" },
        { RPR_MATERIAL_STANDARD_INPUT_GLOSSY_TO_DIFFUSE_WEIGHT, "weights.glossy2diffuse" },
        { RPR_MATERIAL_STANDARD_INPUT_CLEARCOAT_TO_GLOSSY_WEIGHT, "weights.clearcoat2glossy" },
        { RPR_MATERIAL_STANDARD_INPUT_TRANSPARENCY, "weights.transparency" },

    };

    //these material inputs are ignored
    const std::set<std::string> kUnsupportedInputs = { "anisotropic" };
    
    //these materials are unsupported now:
    const std::set<int> kUnsupportedMaterials = {   RPR_MATERIAL_NODE_ADD,
                                                    RPR_MATERIAL_NODE_ARITHMETIC,
                                                    RPR_MATERIAL_NODE_BLEND_VALUE,
                                                    RPR_MATERIAL_NODE_VOLUME,
                                                    RPR_MATERIAL_NODE_INPUT_LOOKUP,
                                                    RPR_MATERIAL_NODE_MICROFACET_ANISOTROPIC_REFLECTION,
                                                    RPR_MATERIAL_NODE_MICROFACET_ANISOTROPIC_REFRACTION,
                                                    RPR_MATERIAL_NODE_TWOSIDED,
                                                    RPR_MATERIAL_NODE_UV_PROJECT,};
}

MaterialObject::MaterialObject(rpr_material_node_type in_type)
{
    m_is_tex = false;
    switch (in_type)
    {
    case RPR_MATERIAL_NODE_DIFFUSE:
    case RPR_MATERIAL_NODE_ORENNAYAR:
    {
        m_mat = SingleBxdf::Create(SingleBxdf::BxdfType::kLambert);
        break;
    }
	case RPR_MATERIAL_NODE_WARD:
    case RPR_MATERIAL_NODE_MICROFACET:
    {
        m_mat = SingleBxdf::Create(SingleBxdf::BxdfType::kMicrofacetGGX);
        break;
    }
    case RPR_MATERIAL_NODE_MICROFACET_REFRACTION:
    {
        m_mat = SingleBxdf::Create(SingleBxdf::BxdfType::kMicrofacetRefractionGGX);
        break;
    }
    case RPR_MATERIAL_NODE_REFLECTION:
    {
        m_mat = SingleBxdf::Create(SingleBxdf::BxdfType::kIdealReflect);
        break;
    }
    case RPR_MATERIAL_NODE_REFRACTION:
    {
        m_mat = SingleBxdf::Create(SingleBxdf::BxdfType::kIdealRefract);
        break;
    }
    case RPR_MATERIAL_NODE_STANDARD:
    {
        m_mat = SingleBxdf::Create(SingleBxdf::BxdfType::kLambert);
        break;
    }
    case RPR_MATERIAL_NODE_DIFFUSE_REFRACTION:
    {
        m_mat = SingleBxdf::Create(SingleBxdf::BxdfType::kTranslucent);
        break;
    }
    case RPR_MATERIAL_NODE_FRESNEL_SCHLICK:
    case RPR_MATERIAL_NODE_FRESNEL:
    {
        m_mat = SingleBxdf::Create(SingleBxdf::BxdfType::kTranslucent);
        break;
    }
    case RPR_MATERIAL_NODE_EMISSIVE:
    {
        m_mat = SingleBxdf::Create(SingleBxdf::BxdfType::kEmissive);
        break;
    }
    case RPR_MATERIAL_NODE_BLEND:
    {
        m_mat = MultiBxdf::Create(MultiBxdf::Type::kMix);
        m_mat->SetInputValue("weight", 0.5f);
        m_mat->SetThin(false);
        break;
    }
    case RPR_MATERIAL_NODE_TRANSPARENT:
    case RPR_MATERIAL_NODE_PASSTHROUGH:
    {
        m_mat = SingleBxdf::Create(SingleBxdf::BxdfType::kPassthrough);
        break;
    }
    case RPR_MATERIAL_NODE_ADD:
    case RPR_MATERIAL_NODE_ARITHMETIC:
    case RPR_MATERIAL_NODE_BLEND_VALUE:
    case RPR_MATERIAL_NODE_VOLUME:
    case RPR_MATERIAL_NODE_INPUT_LOOKUP:
    case RPR_MATERIAL_NODE_MICROFACET_ANISOTROPIC_REFLECTION:
    case RPR_MATERIAL_NODE_MICROFACET_ANISOTROPIC_REFRACTION:
    case RPR_MATERIAL_NODE_TWOSIDED:
    case RPR_MATERIAL_NODE_UV_PROJECT:

	{
		//these materials are unsupported
		if (kUnsupportedMaterials.find(in_type) == kUnsupportedMaterials.end())
		{
			throw Exception(RPR_ERROR_INTERNAL_ERROR, "MaterialObject: need to update list of unsupported materials.");
		}
        m_mat = SingleBxdf::Create(SingleBxdf::BxdfType::kLambert);
		break;
	}
    case RPR_MATERIAL_NODE_NORMAL_MAP:
    case RPR_MATERIAL_NODE_IMAGE_TEXTURE:
    case RPR_MATERIAL_NODE_NOISE2D_TEXTURE:
    case RPR_MATERIAL_NODE_DOT_TEXTURE:
    case RPR_MATERIAL_NODE_GRADIENT_TEXTURE:
    case RPR_MATERIAL_NODE_CHECKER_TEXTURE:
    case RPR_MATERIAL_NODE_CONSTANT_TEXTURE:
    case RPR_MATERIAL_NODE_BUMP_MAP:
        m_tex = Texture::Create();
        m_is_tex = true;
        break;
    default:
        throw Exception(RPR_ERROR_INVALID_PARAMETER_TYPE, "MaterialObject: unrecognized material type");
    }

    m_type = (Type)in_type;

}

MaterialObject::MaterialObject(rpr_image_format const in_format, rpr_image_desc const * in_image_desc, void const * in_data)
    : m_is_tex(true)
    , m_type(Type::kImage)
    , m_tex(nullptr)
{
    //Clean if material was already initialized
    Clear();

 //   //TODO: fix only 4 components data supported
	//if (in_format.num_components != 4 && in_format.num_components != 3)
	//{
	//	throw Exception(RPR_ERROR_UNIMPLEMENTED, "TextureObject: only 3 and 4 component texture implemented.");
	//}

    //tex size
    int2 tex_size(in_image_desc->image_width, in_image_desc->image_height);

    //texture takes ownership of its data array
    //so need to copy input data
	int pixels_count = tex_size.x * tex_size.y;

	//bytes per pixel
	int pixel_bytes = in_format.num_components;
	int component_bytes = 1;
    Texture::Format data_format = Texture::Format::kRgba8;
    switch (in_format.type)
    {
    case RPR_COMPONENT_TYPE_UINT8:
        break;
    case RPR_COMPONENT_TYPE_FLOAT16:
		component_bytes = 2;
        data_format = Texture::Format::kRgba16;
        break;
    case RPR_COMPONENT_TYPE_FLOAT32:
		component_bytes = 4;
        data_format = Texture::Format::kRgba32;
        break;
    default:
        throw Exception(RPR_ERROR_INVALID_PARAMETER, "TextureObject: invalid format type.");
    }
	pixel_bytes *= component_bytes;
	int data_size = 4 * component_bytes * pixels_count;//4 component baikal texture
    char* data = new char[data_size];
	if (in_format.num_components == 4)
	{
		//copy data
		memcpy(data, in_data, data_size);
	}
	else
	{
		//copy to 4component texture
		const char* in_data_cast = static_cast<const char*>(in_data);
		for (int i = 0; i < pixels_count; ++i)
		{
			//copy
			for (unsigned int comp_ind = 0; comp_ind < in_format.num_components; ++comp_ind)
			{
				int index = comp_ind * component_bytes;
				memcpy(&data[i * 4 * component_bytes + index], &in_data_cast[i * in_format.num_components * component_bytes + index], component_bytes);
			}
			//clean other colors
			for (unsigned int comp_ind = in_format.num_components; comp_ind < 4; ++comp_ind)
			{
				memset(&data[i * 4 + comp_ind], 0, component_bytes);
			}
		}
	}
    m_tex = Texture::Create(data, tex_size, data_format);
}

MaterialObject::MaterialObject(const std::string& in_path)
    : m_is_tex(true)
    , m_type(Type::kImage)
    , m_tex(nullptr)
{
    //Clean if material was already initialized
    Clear();

    //load texture using oiio
    std::unique_ptr<ImageIo> oiio = ImageIo::CreateImageIo();
    Texture::Ptr texture = nullptr;
    try
    {
        texture = oiio->LoadImage(in_path);
    }
    catch (...) {
        throw Exception(RPR_ERROR_IO_ERROR, "TextureObject: failed to load image.");
    }

    m_tex = texture;
}

MaterialObject::~MaterialObject()
{
    Clear();
}

void MaterialObject::Clear()
{
    m_mat = nullptr;
    m_tex = nullptr;
}

std::string MaterialObject::TranslatePropName(const std::string& in, Type type)
{
    std::string result;
    if (type == Type::kBumpMap)
    {
        return "bump";
    }
    else if (type == Type::kNormalMap)
    {
        return "normal";
    }

    auto it = kInputNamesDictionary.find(in);
    if (it != kInputNamesDictionary.end())
    {
        result = it->second;
    }
    else
    {
        throw Exception(RPR_ERROR_UNIMPLEMENTED, "MaterialObject: unimplemented input.");

    }

    return result;
}


void MaterialObject::SetInputMaterial(const std::string& input_name, MaterialObject* input)
{
    //TODO: fix
    if (kUnsupportedMaterials.find(m_type) != kUnsupportedMaterials.end() /*||
        kUnsupportedMaterials.find(input->GetType()) != kUnsupportedMaterials.end()*/)
    {
        //this is unsupported material, so don't update anything
        return;
    }

    //translated input name
    std::string name;

    //TODO:fix
    //check is input name is registered
    auto it_input = std::find_if(kMaterialNodeInputStrings.begin(), kMaterialNodeInputStrings.end(), 
                            [&input_name](const std::pair<uint32_t, std::string>& input) { return input.second == input_name; });
    if (it_input == kMaterialNodeInputStrings.end())
    {
        throw Exception(RPR_ERROR_INVALID_TAG, "MaterialObject: unregistered input name.");
    }
    try
    {
        name = TranslatePropName(input_name);
    }
    catch (Exception& e)
    {
        //failed to translate valid input name annd ignore it
        //TODO:
        return;
    }
    //TODO: fix
    //can't set material to color input
    if (input_name == "color" && input->IsMaterial())
    {
        return;
    }


    //convert input name
    if (IsTexture())
    {
        if (!input->IsTexture())
        {
            throw Exception(RPR_ERROR_INVALID_TAG, "MaterialObject: can't set Material as input of Texture.");

        }
        if (name == "data" ||
            name == "albedo" && IsMap())
        {
            //copy image data
            auto tex = input->GetTexture();
            const char* data = tex->GetData();
            RadeonRays::int2 size = tex->GetSize();
            auto format = tex->GetFormat();
            char* tex_data = new char[tex->GetSizeInBytes()];
            memcpy(tex_data, data, tex->GetSizeInBytes());

            m_tex->SetData(tex_data, size, format);
        }
        else
        {
            throw Exception(RPR_ERROR_INVALID_TAG, "MaterialObject: invalid tag for texture.");
        }
    }
    else
    {
        if (input->IsTexture())
        {
            m_mat->SetInputValue(name, input->GetTexture());
            //handle blend material case
            if (m_type == kBlend && name == "weight")
            {
                auto blend_mat = std::dynamic_pointer_cast<MultiBxdf>(m_mat);
                blend_mat->SetType(MultiBxdf::Type::kMix);
            }
        }
        else
        {
            //handle blend material case
            if (m_type == kBlend && name == "weight")
            {
                //expected only fresnel materials
                if (input->m_type != Type::kFresnel && input->m_type != Type::kFresnelShlick)
                {
                    //only fresnel materials can be handled as weight of blend material.
                    //throw Exception(RPR_ERROR_INVALID_PARAMETER, "MaterialObject: expected only fresnel materials as weight of blend material.");
                    return;
                }

                auto blend_mat = std::dynamic_pointer_cast<MultiBxdf>(m_mat);
                blend_mat->SetType(MultiBxdf::Type::kFresnelBlend);
                blend_mat->SetInputValue("ior", input->m_mat->GetInputValue("ior").float_value);
            }
            else
            {
                m_mat->SetInputValue(name, input->GetMaterial());
            }
        }
    }
    input->AddOutput(this);
    SetInput(input, input_name);
    Notify();
}

void MaterialObject::SetInputValue(const std::string& input_name, const RadeonRays::float4& val)
{
    //TODO:
    if (kUnsupportedMaterials.find(m_type) != kUnsupportedMaterials.end())
    {
        //this is unsupported material, so don't update anything
        return;
    }
    if (kUnsupportedInputs.find(input_name) != kUnsupportedInputs.end())
    {
        //ignore input
        return;
    }

    std::string name;

    //TODO:fix
    //check is input name is registered
    auto it_input = std::find_if(kMaterialNodeInputStrings.begin(), kMaterialNodeInputStrings.end(),
                                [&input_name](const std::pair<uint32_t, std::string>& input) { return input.second == input_name; });
    if (it_input == kMaterialNodeInputStrings.end())
    {
        throw Exception(RPR_ERROR_INVALID_TAG, "MaterialObject: unregistered input name.");
    }
    try
    {
        name = TranslatePropName(input_name);
    }
    catch (Exception& e)
    {
        //failed to translate valid input name and ignore it
        //TODO:
        return;
    }

    //convert input name
    if (m_type == Type::kImage)
    {
        throw Exception(RPR_ERROR_INVALID_PARAMETER, "MaterialObject: trying SetInputValue for rpr_image object.");
    }

    if (IsTexture())
    {
		//TODO: handle texture materials nodes
		return;
        //throw Exception(RPR_ERROR_UNIMPLEMENTED, "MaterialObject: SetInputValue not implemented for Texture materials.");
    }

    //translate material name
    m_mat->SetInputValue(name, val);

    //if rougness is to small replace microfacet by ideal reflect
    if (m_type == Type::kMicrofacet && name == "roughness")
    {
        if (val.sqnorm() < 1e-6f)
        {
            auto bxdf_mat = std::dynamic_pointer_cast<SingleBxdf>(m_mat);
            bxdf_mat->SetBxdfType(Baikal::SingleBxdf::BxdfType::kIdealReflect);
        }
        else
        {
            auto bxdf_mat = std::dynamic_pointer_cast<SingleBxdf>(m_mat);
            bxdf_mat->SetBxdfType(Baikal::SingleBxdf::BxdfType::kMicrofacetGGX);
        }
    }
    //handle blend material case
    if (m_type == kBlend && name == "weight")
    {
        auto blend_mat = std::dynamic_pointer_cast<MultiBxdf>(m_mat);
        blend_mat->SetType(MultiBxdf::Type::kMix);
    }
    SetInput(nullptr, input_name);
    Notify();
}

void MaterialObject::SetInput(MaterialObject* input_mat, const std::string& input_name)
{
    //remove link with previous input
    MaterialObject* old_input = m_inputs[input_name];
    if (old_input)
    {
        old_input->RemoveOutput(this);
    }

    m_inputs[input_name] = input_mat;
}


void MaterialObject::AddOutput(MaterialObject* mat)
{
    m_out_mats.insert(mat);
}

void MaterialObject::RemoveOutput(MaterialObject* mat)
{
    m_out_mats.erase(mat);
}

void MaterialObject::Notify()
{
    for (auto mat : m_out_mats)
    {
        mat->Update(this);
    }
}

void MaterialObject::Update(MaterialObject* mat)
{
    auto input_it = std::find_if(m_inputs.begin(), m_inputs.end(), [mat](const std::pair<std::string, MaterialObject*>& i) { return i.second == mat; });
    if (input_it == m_inputs.end())
    {
        throw Exception(RPR_ERROR_INTERNAL_ERROR, "MaterialObject: failed to find requested input node.");
    }
    const std::string& input_name = TranslatePropName(input_it->first, mat->GetType());
    if (m_type == kBlend && input_name == "weight")
    {
        //expected only fresnel materials
        if (mat->m_type == Type::kFresnel || mat->m_type == Type::kFresnelShlick)
        {
            auto blend_mat = std::dynamic_pointer_cast<MultiBxdf>(m_mat);
            blend_mat->SetType(MultiBxdf::Type::kFresnelBlend);
            blend_mat->SetInputValue("ior", mat->m_mat->GetInputValue("ior").float_value);
        }
    }
}

uint64_t MaterialObject::GetInputCount()
{
    return m_inputs.size();
}

rpr_uint MaterialObject::GetInputType(int i)
{
    rpr_uint result = RPR_MATERIAL_NODE_INPUT_TYPE_NODE;
    //get i input
    auto it = m_inputs.begin();
    for (int index = 0; it != m_inputs.end(); ++it, ++index)
        if (index == i)
        {
            break;
        }

    //means no MaterialObject connected
    if (!it->second)
    {
        result = RPR_MATERIAL_NODE_INPUT_TYPE_FLOAT4;
    }
    //connected input represent rpr_image
    else if(it->second->IsImg())
    {
        result = RPR_MATERIAL_NODE_INPUT_TYPE_IMAGE;
    }

    return result;
}

std::string MaterialObject::GetInputName(int i)
{
    //get i input
    auto it = m_inputs.begin();
    for (int index = 0; it != m_inputs.end(); ++it, ++index)
        if (index == i)
        {
            break;
        }

    return it->first;
}


void MaterialObject::GetInput(int i, void* out, size_t* out_size)
{
    //get i input
    auto it = m_inputs.begin();
    for (int index = 0; it != m_inputs.end(); ++it, ++index)
        if (index == i)
        {
            break;
        }

    //translated name
    std::string trans_name = TranslatePropName(it->first);
    //means no MaterialObject connected
    if (!it->second)
    {
        *out_size = sizeof(RadeonRays::float4);
        Baikal::Material::InputValue value = m_mat->GetInputValue(trans_name);
        if (value.type != Baikal::Material::InputType::kFloat4)
        {
            throw Exception(RPR_ERROR_INTERNAL_ERROR, "MaterialObject: material input type is unexpected.");
        }

        memcpy(out, &value.float_value, *out_size);
    }
    //images, textures and materials
    else
    {
        *out_size = sizeof(rpr_material_node);
        memcpy(out, &it->second, *out_size);
    }
}


rpr_image_desc MaterialObject::GetTextureDesc() const
{
    RadeonRays::int2 size = m_tex->GetSize();
    rpr_uint depth = (rpr_uint)m_tex->GetSizeInBytes() / size.x / size.y;
    return {(rpr_uint)size.x, (rpr_uint)size.y, depth, 0, 0};
}
char const* MaterialObject::GetTextureData() const
{
    return m_tex->GetData();
}
rpr_image_format MaterialObject::GetTextureFormat() const
{
    rpr_component_type type;
    switch (m_tex->GetFormat())
    {
    case Baikal::Texture::Format::kRgba8:
        type = RPR_COMPONENT_TYPE_UINT8;
        break;
    case Baikal::Texture::Format::kRgba16:
        type = RPR_COMPONENT_TYPE_FLOAT16;
        break;
    case Baikal::Texture::Format::kRgba32:
        type = RPR_COMPONENT_TYPE_FLOAT32;
        break;
    default:
        throw Exception(RPR_ERROR_INTERNAL_ERROR, "MaterialObject: invalid image format.");
    }
    //only 4component textures used
    return{ 4, type };
}
