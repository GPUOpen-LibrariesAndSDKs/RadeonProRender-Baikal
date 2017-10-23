/*****************************************************************************\
*
*  Module Name    Material.cpp
*  Project        RRP support library
*
*  Description    RRP material interface
*
*  Copyright 2017 Advanced Micro Devices, Inc.
*
*  All rights reserved.  This notice is intended as a precaution against
*  inadvertent publication and does not imply publication or any waiver
*  of confidentiality.  The year included in the foregoing notice is the
*  year of creation of the work.
\*****************************************************************************/
#include "Material.h"

namespace RprSupport
{

    RPRMaterialBundle Material::Compile()
    {
        if (HasStructuralChange())
        {
            m_current_bundle = CompileGraph();
        }

        MapParameters();

        for (auto& input : m_inputs)
        {
            input.second.SetDirty(false);
        }

        return m_current_bundle;
    }

    void Material::RegisterInput(std::uint32_t id, InputTypes types, bool structural)
    {
        auto iter = m_inputs.find(id);
        assert(iter == m_inputs.cend());
        m_inputs.insert(std::make_pair(id, Input(types)));

        if (structural)
        {
            m_structural_inputs.emplace(id);
        }
    }

    void Material::SetInput(uint32_t id, float4 const& value)
    {
        auto iter = m_inputs.find(id);

        if (iter == m_inputs.cend())
        {
            throw std::runtime_error("Input not found");
        }

        auto& input = iter->second;

        if (input.supported_types & static_cast<std::uint32_t>(InputType::kFloat4))
        {
            input.type = InputType::kFloat4;
            input.value_f4 = value;
            input.SetDirty(true);
        }
        else
        {
            throw std::runtime_error("Input type not supported");
        }
    }

    void Material::SetInput(uint32_t id, uint32_t value)
    {
        auto iter = m_inputs.find(id);

        if (iter == m_inputs.cend())
        {
            throw std::runtime_error("Input not found");
        }

        auto& input = iter->second;

        if (input.supported_types & static_cast<std::uint32_t>(InputType::kUint))
        {
            input.type = InputType::kUint;
            input.value_ui = value;
            input.SetDirty(true);
        }
        else
        {
            throw std::runtime_error("Input type not supported");
        }
    }

    void Material::SetInput(uint32_t id, void* value)
    {
        auto iter = m_inputs.find(id);

        if (iter == m_inputs.cend())
        {
            throw std::runtime_error("Input not found");
        }

        auto& input = iter->second;

        if (input.supported_types & static_cast<std::uint32_t>(InputType::kPointer))
        {
            input.type = InputType::kPointer;
            input.value_ptr = value;
            input.SetDirty(true);
        }
        else
        {
            throw std::runtime_error("Input type not supported");
        }
    }

    Material::Input Material::GetInput(std::uint32_t id) const
    {
        auto iter = m_inputs.find(id);

        if (iter == m_inputs.cend())
        {
            throw std::runtime_error("Input not found");
        }

        return iter->second;
    }

    bool Material::HasStructuralChange() const
    {
        for (auto& id : m_structural_inputs)
        {
            auto input = m_inputs.find(id);

            // This can't fail 
            assert(input != m_inputs.cend());

            if (input->second.dirty)
            {
                return true;
            }
        }

        return false;
    }

}