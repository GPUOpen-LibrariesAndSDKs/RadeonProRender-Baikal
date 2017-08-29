#include "material.h"
#include "iterator.h"

#include <cassert>
#include <memory>

namespace Baikal
{
    Material::InputValue::~InputValue()
    {
        Clean();
    }
    Material::InputValue::InputValue(const InputValue& v)
    {
        switch (v.type)
        {
        case InputType::kFloat4:
            float_value = v.float_value;
            break;
        case InputType::kTexture:
            //initiate empty shared_ptr
            new (&tex_value)TextureCPtr();
            tex_value = v.tex_value;
            break;
        case InputType::kMaterial:
            //initiate empty shared_ptr
            new (&mat_value)MaterialCPtr;
            mat_value = v.mat_value;
            break;
        default:
            throw("unhandled material input type");
        }

        type = v.type;
    }
    Material::InputValue& Material::InputValue::operator=(const Material::InputValue& v)
    {
        //clean current shared_ptrs
        Clean();
        switch (v.type)
        {
        case InputType::kFloat4:
            float_value = v.float_value;
            break;
        case InputType::kTexture:
            //initiate empty shared_ptr
            new (&tex_value)TextureCPtr();
            tex_value = v.tex_value;
            break;
        case InputType::kMaterial:
            //initiate empty shared_ptr
            new (&mat_value)MaterialCPtr;
            mat_value = v.mat_value;
            break;
        default:
            throw("unhandled material input type");
        }

        type = v.type;
        return *this;
    }

    void Material::InputValue::Clean()
    {
        switch (type)
        {
        case InputType::kFloat4:
            break;
        case InputType::kTexture:
            tex_value.~shared_ptr();
            break;
        case InputType::kMaterial:
            mat_value.~shared_ptr();
            break;
        default:
            break;
        }
        type = InputType::kFloat4;
        float_value = RadeonRays::float4();
    }


    class Material::InputIterator : public Iterator
    {
    public:
        
        InputIterator(InputMap::const_iterator begin,
                      InputMap::const_iterator end)
        : m_begin(begin)
        , m_end(end)
        {
            Reset();
        }
        
        // Check if we reached end
        bool IsValid() const override
        {
            return m_cur != m_end;
        }
        
        // Advance by 1
        void Next() override
        {
            ++m_cur;
        }
        
        // Set to starting iterator
        void Reset() override
        {
            m_cur = m_begin;
        }
        
        // Get underlying item
        std::weak_ptr<void const> Item() const override
        {
            std::weak_ptr<void const> res;
            switch (m_cur->second.value.type)
            {
            case Material::InputType::kFloat4:
                res = std::make_shared<RadeonRays::float4>(m_cur->second.value.float_value);
                break;
            case Material::InputType::kMaterial:
                res = m_cur->second.value.mat_value;
                break;
            case Material::InputType::kTexture:
                res = m_cur->second.value.tex_value;
                break;
            }
            return res;
        }
        
    private:
        InputMap::const_iterator m_begin;
        InputMap::const_iterator m_end;
        InputMap::const_iterator m_cur;
    };
    
   
    
    
    Material::Material()
    : m_thin(false)
    {
    }
    
    void Material::RegisterInput(std::string const& name,
                                 std::string const& desc,
                                 std::set<InputType>&& supported_types)
    {
        Input input = {{name, desc, std::move(supported_types)}};
        
        assert(input.info.supported_types.size() > 0);
        
        InputType type = *input.info.supported_types.begin();
        
        switch (type)
        {
            case InputType::kFloat4:
                input.value.float_value = RadeonRays::float4();
                break;
            case InputType::kTexture:
                input.value = InputValue(TextureCPtr(nullptr));
                break;
            case InputType::kMaterial:
                input.value = InputValue(MaterialCPtr(nullptr));
                break;
            default:
                break;
        }
        
        m_inputs.emplace(std::make_pair(name, input));
    }
    
    void Material::ClearInputs()
    {
        m_inputs.clear();
    }

    
    // Iterator of dependent materials (plugged as inputs)
    std::unique_ptr<Iterator> Material::CreateMaterialIterator() const
    {
        std::set<MaterialCPtr> materials;
        
        std::for_each(m_inputs.cbegin(), m_inputs.cend(),
                      [&materials](std::pair<std::string, Input> const& map_entry)
                      {
                          if (map_entry.second.value.type == InputType::kMaterial &&
                              map_entry.second.value.mat_value != nullptr)
                          {
                              materials.insert(map_entry.second.value.mat_value);
                          }
                      }
                      );
        
        return std::unique_ptr<Iterator>(
            new ContainerIterator<std::set<MaterialCPtr>>(std::move(materials)));
    }
    
    // Iterator of textures (plugged as inputs)
    std::unique_ptr<Iterator> Material::CreateTextureIterator() const
    {
        std::set<TextureCPtr> textures;
        
        std::for_each(m_inputs.cbegin(), m_inputs.cend(),
                      [&textures](std::pair<std::string, Input> const& map_entry)
                      {
                          if (map_entry.second.value.type == InputType::kTexture &&
                              map_entry.second.value.tex_value != nullptr)
                          {
                              textures.insert(map_entry.second.value.tex_value);
                          }
                      }
                      );
        
        return std::unique_ptr<Iterator>(
            new ContainerIterator<std::set<TextureCPtr>>(std::move(textures)));
    }
    
    // Iterator of inputs
    std::unique_ptr<Iterator> Material::CreateInputIterator() const
    {
        return std::unique_ptr<Iterator>(new InputIterator(m_inputs.cbegin(), m_inputs.cend()));
    }
    
    // Set input value
    // If specific data type is not supported throws std::runtime_error
    void Material::SetInputValue(std::string const& name, RadeonRays::float4 const& value)
    {
        auto input_iter = m_inputs.find(name);
        
        if (input_iter != m_inputs.cend())
        {
            auto& input = input_iter->second;
            
            if (input.info.supported_types.find(InputType::kFloat4) != input.info.supported_types.cend())
            {
                input.value = InputValue(value);
                SetDirty(true);
            }
            else
            {
                throw std::runtime_error("Input type not supported");
            }
        }
        else
        {
            throw std::runtime_error("No such input");
        }
    }
    
    void Material::SetInputValue(std::string const& name, TextureCPtr texture)
    {
        auto input_iter = m_inputs.find(name);
        
        if (input_iter != m_inputs.cend())
        {
            auto& input = input_iter->second;
            
            if (input.info.supported_types.find(InputType::kTexture) != input.info.supported_types.cend())
            {
                input.value = InputValue(texture);
                SetDirty(true);
            }
            else
            {
                throw std::runtime_error("Input type not supported");
            }
        }
        else
        {
            throw std::runtime_error("No such input");
        }
    }
    
    void Material::SetInputValue(std::string const& name, MaterialCPtr material)
    {
        auto input_iter = m_inputs.find(name);
        
        if (input_iter != m_inputs.cend())
        {
            auto& input = input_iter->second;
            
            if (input.info.supported_types.find(InputType::kMaterial) != input.info.supported_types.cend())
            {
                input.value = InputValue(material);
                SetDirty(true);
            }
            else
            {
                throw std::runtime_error("Input type not supported");
            }
        }
        else
        {
            throw std::runtime_error("No such input");
        }
    }
    
    Material::InputValue Material::GetInputValue(std::string const& name) const
    {
        auto input_iter = m_inputs.find(name);
        
        if (input_iter != m_inputs.cend())
        {
            auto& input = input_iter->second;
            
            return input.value;
        }
        else
        {
            throw std::runtime_error("No such input");
        }
    }

    bool Material::IsThin() const
    {
        return m_thin;
    }
    
    void Material::SetThin(bool thin)
    {
        m_thin = thin;
        SetDirty(true);
    }

    SingleBxdf::SingleBxdf(BxdfType type)
    : m_type(type)
    {
        RegisterInput("albedo", "Diffuse color", {InputType::kFloat4, InputType::kTexture});
        RegisterInput("normal", "Normal map", {InputType::kTexture});
        RegisterInput("bump", "Bump map", { InputType::kTexture });
        RegisterInput("ior", "Index of refraction", {InputType::kFloat4});
        RegisterInput("fresnel", "Fresnel flag", {InputType::kFloat4});
        RegisterInput("roughness", "Roughness", {InputType::kFloat4, InputType::kTexture});
        
        SetInputValue("albedo", RadeonRays::float4(0.7f, 0.7f, 0.7f, 1.f));
        SetInputValue("normal", TexturePtr());
        SetInputValue("bump", TexturePtr());
    }
    
    SingleBxdf::BxdfType SingleBxdf::GetBxdfType() const
    {
        return m_type;
    }
    
    void SingleBxdf::SetBxdfType(BxdfType type)
    {
        m_type = type;
        SetDirty(true);
    }

    bool SingleBxdf::HasEmission() const
    {
        return m_type == BxdfType::kEmissive;
    }
    
    MultiBxdf::MultiBxdf(Type type)
    : m_type(type)
    {
        RegisterInput("base_material", "Base material", {InputType::kMaterial});
        RegisterInput("top_material", "Top material", {InputType::kMaterial});
        RegisterInput("ior", "Index of refraction", {InputType::kFloat4});
        RegisterInput("weight", "Blend weight", {InputType::kFloat4, InputType::kTexture});
    }
    
    MultiBxdf::Type MultiBxdf::GetType() const
    {
        return m_type;
    }
    
    void MultiBxdf::SetType(Type type)
    {
        m_type = type;
        SetDirty(true);
    }

    bool MultiBxdf::HasEmission() const
    {
        InputValue base = GetInputValue("base_material");
        InputValue top = GetInputValue("base_material");

        if (base.mat_value && base.mat_value->HasEmission())
            return true;
        if (top.mat_value && top.mat_value->HasEmission())
            return true;

        return false;
    }
    
    DisneyBxdf::DisneyBxdf()
    {
        RegisterInput("albedo", "Base color", {InputType::kFloat4, InputType::kTexture});
        RegisterInput("metallic", "Metallicity", {InputType::kFloat4, InputType::kTexture});
        RegisterInput("subsurface", "Subsurface look of diffuse base", {InputType::kFloat4});
        RegisterInput("specular", "Specular exponent", {InputType::kFloat4, InputType::kTexture});
        RegisterInput("specular_tint", "Specular color to base", {InputType::kFloat4, InputType::kTexture});
        RegisterInput("anisotropy", "Anisotropy of specular layer", {InputType::kFloat4, InputType::kTexture});
        RegisterInput("sheen", "Sheen for cloth", {InputType::kFloat4, InputType::kTexture});
        RegisterInput("sheen_tint", "Sheen to base color", {InputType::kFloat4, InputType::kTexture});
        RegisterInput("clearcoat", "Clearcoat layer", {InputType::kFloat4, InputType::kTexture});
        RegisterInput("clearcoat_gloss", "Clearcoat roughness", {InputType::kFloat4, InputType::kTexture});
        RegisterInput("roughness", "Roughness of specular & diffuse layers", {InputType::kFloat4, InputType::kTexture});
        RegisterInput("normal", "Normal map", {InputType::kTexture});
        RegisterInput("bump", "Bump map", { InputType::kTexture });
        
        SetInputValue("albedo", RadeonRays::float4(0.7f, 0.7f, 0.7f, 1.f));
        SetInputValue("metallic", RadeonRays::float4(0.25f, 0.25f, 0.25f, 0.25f));
        SetInputValue("specular", RadeonRays::float4(0.25f, 0.25f, 0.25f, 0.25f));
        SetInputValue("normal", TexturePtr());
        SetInputValue("bump", TexturePtr());
    }
    
    // Check if material has emissive components
    bool DisneyBxdf::HasEmission() const
    {
        return false;
    }
}
