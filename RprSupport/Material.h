/*****************************************************************************\
*
*  Module Name    Material.h
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
#pragma once

#include "RprSupport.h"

#include <cstdint>
#include <set>
#include <map>
#include <cassert>
#include <stdexcept>
#include <cmath>

namespace RprSupport
{
    static constexpr float kEps = 0.00001f;

    // float4 class to simplify storage
    struct float4
    {
        float x, y, z, w;
        bool is_zero() const
        {
            return std::fabs(x * x + y * y + z * z + w * w) < kEps;
        }
    };

    /*
        \brief Object appearance descriptor.

        Every object might have surface, volume and displacement slots. RPRMaterialBundle groups them together.
    */
    struct RPRMaterialBundle
    {
        rpr_material_node surface_material;
        rpr_material_node volume_material;
		rpr_material_node displacement_material;
        RPRMaterialBundle(rpr_material_node sm = nullptr,
            rpr_material_node vm = nullptr,
			rpr_material_node dm = nullptr
			)
            : surface_material(sm)
            , volume_material(vm)
			, displacement_material(dm)
        {
        }
    };

    /*
        \brief Material interface.

        Material handles input storage and compilation into RPR graphs. Material has 2 classes of parameters:
        - Structural
        - Non-structural
        Structural parameters change toggles graph re-structuring in derived classes.
        Non-structural parameter changes only toggle scalar parameter changes in an underlying graph.
    */
    class Material
    {
    public:
        enum class InputType;
        using InputTypes = std::uint32_t;
        struct Input;

        Material(rpr_material_system material_system);
        virtual ~Material() = default;

        // Client API for inputs handling
        void SetInput(std::uint32_t id, float4 const& value);
        void SetInput(std::uint32_t id, std::uint32_t value);
        void SetInput(std::uint32_t id, void* value);
        Input GetInput(std::uint32_t id) const;

        // Compile shading graph
        RPRMaterialBundle Compile();
        RPRMaterialBundle GetBundle() const { return m_current_bundle; }

        // Subclassing API
    protected:
        // Register an input, so that it becomes accessible for client API
        void RegisterInput(std::uint32_t id, InputTypes types, bool structural = false);

        rpr_material_system GetMaterialSystem() const;

    private:
        // Detect if an underlying graph requires structural change
        bool HasStructuralChange() const;

        // Customization API
        // Compile graph structure
        virtual RPRMaterialBundle CompileGraph() = 0;
        // Map non-structural parameters
        virtual void MapParameters() = 0;

    private:
        // Inputs
        std::map<std::uint32_t, Input> m_inputs;
        // Inputs potentially affecting graph structure
        std::set<std::uint32_t> m_structural_inputs;
        // RPR material system
        rpr_material_system m_material_system;
        // Current state of the shading graph
        RPRMaterialBundle m_current_bundle;
    };

    /*
    \brief Material input value type

    Material inputs can support maximum of 3 types: float4, uint and node pointer.
    */
    enum class Material::InputType
    {
        kFloat4 = 0x1,
        kUint = 0x2,
        kPointer = 0x4
    };

    /*
    \brief Material input class

    Material input keeps the value along with its type, it also tracks value cahnges via a dirty flag interface.
    */
    struct Material::Input
    {
        // Supported types bitmask
        InputTypes supported_types;
        // Currenlty contained type
        InputType type;
        // Indicates wether input has been changed
        // since last SetDirty(false) operation
        bool dirty;

        // Actual value
        union
        {
            float4 value_f4;
            std::uint32_t value_ui;
            void* value_ptr;
        };

        // Constructor
        Input(InputTypes types)
            : supported_types(types)
            , value_f4{ 0.f, 0.f, 0.f, 0.f }
            , dirty(true)
        {
            type = (supported_types & static_cast<std::uint32_t>(InputType::kFloat4)) ? InputType::kFloat4 :
                (supported_types & static_cast<std::uint32_t>(InputType::kUint)) ? InputType::kUint : InputType::kPointer;
        }

        // Dirty interface
        void SetDirty(bool d) { dirty = d; }

        // Checks wether input is zero (nullptr of float4(0))
        bool IsZero() const
        {
            return (type == InputType::kFloat4 && value_f4.is_zero()) ||
                (type == InputType::kPointer && !value_ptr);
        }
    };

    inline Material::Material(rpr_material_system material_system)
        : m_material_system(material_system)
    {
    }

    inline rpr_material_system Material::GetMaterialSystem() const
    {
        return m_material_system;
    }
}