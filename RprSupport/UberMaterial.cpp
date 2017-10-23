/*****************************************************************************\
*
*  Module Name    UberMaterial.h
*  Project        RRP support library
*
*  Description    RRP uber material implentation
*
*  Copyright 2017 Advanced Micro Devices, Inc.
*
*  All rights reserved.  This notice is intended as a precaution against
*  inadvertent publication and does not imply publication or any waiver
*  of confidentiality.  The year included in the foregoing notice is the
*  year of creation of the work.
\*****************************************************************************/
#include "UberMaterial.h"

namespace RprSupport
{
    static constexpr float kMetalEmulationIor = 1000.f;

    inline void ThrowIfRprFailed(rpr_int status, std::string const& message)
    {
        if (status != RPR_SUCCESS)
            throw std::runtime_error(message);
    }

    struct UberMaterial::InputDesc
    {
        std::uint32_t id;
        std::uint32_t types;
        bool structural;

        InputDesc(std::uint32_t i = 0, std::uint32_t t = 0, bool s = false)
            : id(i)
            , types(t)
            , structural(s)
        {
        }
    };

    struct UberMaterial::BuildingBlocks
    {
        // Diffuse blocks
        struct
        {
            rpr_material_node closure;
            rpr_material_node mul;
        } diffuse;

        // Reflection blocks
        struct
        {
            rpr_material_node closure;
            rpr_material_node blend;
            rpr_material_node mul_weight;
            rpr_material_node mul_fresnel;
            rpr_material_node fresnel;

        } reflection;

        // Refraction blocks
        struct
        {
            rpr_material_node closure_transparency;
            rpr_material_node closure_refraction;
            rpr_material_node closure_current;
            rpr_material_node blend;
            rpr_material_node mul;
            rpr_material_node fresnel;
        } refraction;

        // Coating blocks
        struct
        {
            rpr_material_node closure;
            rpr_material_node blend;
            rpr_material_node mul_weight;
            rpr_material_node mul_fresnel;
            rpr_material_node fresnel;
        } coating;

        // Emission blocks
        struct
        {
            rpr_material_node closure;
            rpr_material_node blend;
        } emission;

        // Transparency blocks
        struct
        {
            rpr_material_node closure;
            rpr_material_node blend;
        } transparency;

        // Volume 
        struct
        {
            rpr_material_node closure;
            rpr_material_node closure_refract;
            rpr_material_node blend;
            rpr_material_node absorption_sub;
            rpr_material_node absorption_div;
            rpr_material_node scatter_sub;
            rpr_material_node scatter_div;
        } volume;
    };

    UberMaterial::UberMaterial(rpr_material_system material_system)
        : Material(material_system)
        , m_building_blocks(CreateBuildingBlocks())
    {
        for (auto& input : s_inputs)
        {
            RegisterInput(input.id, input.types, input.structural);
        }

        WireUp();

        SetDefaults();
    }

    UberMaterial::~UberMaterial()
    {
        auto& bb = *m_building_blocks;

        rprObjectDelete(bb.diffuse.closure);
        rprObjectDelete(bb.diffuse.mul);
        rprObjectDelete(bb.reflection.blend);
        rprObjectDelete(bb.reflection.closure);
        rprObjectDelete(bb.reflection.fresnel);
        rprObjectDelete(bb.reflection.mul_fresnel);
        rprObjectDelete(bb.reflection.mul_weight);
        rprObjectDelete(bb.refraction.blend);
        rprObjectDelete(bb.refraction.closure_refraction);
        rprObjectDelete(bb.refraction.closure_transparency);
        rprObjectDelete(bb.refraction.fresnel);
        rprObjectDelete(bb.refraction.mul);
        rprObjectDelete(bb.coating.blend);
        rprObjectDelete(bb.coating.closure);
        rprObjectDelete(bb.coating.fresnel);
        rprObjectDelete(bb.coating.mul_fresnel);
        rprObjectDelete(bb.coating.mul_weight);
        rprObjectDelete(bb.emission.blend);
        rprObjectDelete(bb.emission.closure);
        rprObjectDelete(bb.transparency.blend);
        rprObjectDelete(bb.transparency.closure);
        rprObjectDelete(bb.volume.absorption_div);
        rprObjectDelete(bb.volume.absorption_sub);
        rprObjectDelete(bb.volume.scatter_div);
        rprObjectDelete(bb.volume.scatter_sub);
        rprObjectDelete(bb.volume.blend);
        rprObjectDelete(bb.volume.closure);
        rprObjectDelete(bb.volume.closure_refract);
    }

    UberMaterial::BuildingBlocks* UberMaterial::CreateBuildingBlocks() const
    {
        BuildingBlocks* bb = new BuildingBlocks;

        auto status = RPR_SUCCESS;

        auto material_system = GetMaterialSystem();

        status = rprMaterialSystemCreateNode(material_system, RPR_MATERIAL_NODE_DIFFUSE, &bb->diffuse.closure);
        ThrowIfRprFailed(status, "RPR node creation failed");

        status = rprMaterialSystemCreateNode(material_system, RPR_MATERIAL_NODE_ARITHMETIC, &bb->diffuse.mul);
        ThrowIfRprFailed(status, "RPR node creation failed");

        status = rprMaterialSystemCreateNode(material_system, RPR_MATERIAL_NODE_MICROFACET_ANISOTROPIC_REFLECTION, &bb->reflection.closure);
        ThrowIfRprFailed(status, "RPR node creation failed");

        status = rprMaterialSystemCreateNode(material_system, RPR_MATERIAL_NODE_BLEND, &bb->reflection.blend);
        ThrowIfRprFailed(status, "RPR node creation failed");

        status = rprMaterialSystemCreateNode(material_system, RPR_MATERIAL_NODE_ARITHMETIC, &bb->reflection.mul_weight);
        ThrowIfRprFailed(status, "RPR node creation failed");

        status = rprMaterialSystemCreateNode(material_system, RPR_MATERIAL_NODE_ARITHMETIC, &bb->reflection.mul_fresnel);
        ThrowIfRprFailed(status, "RPR node creation failed");

        status = rprMaterialSystemCreateNode(material_system, RPR_MATERIAL_NODE_FRESNEL, &bb->reflection.fresnel);
        ThrowIfRprFailed(status, "RPR node creation failed");

        status = rprMaterialSystemCreateNode(material_system, RPR_MATERIAL_NODE_MICROFACET_REFRACTION, &bb->refraction.closure_refraction);
        ThrowIfRprFailed(status, "RPR node creation failed");

        bb->refraction.closure_current = bb->refraction.closure_refraction;

        status = rprMaterialSystemCreateNode(material_system, RPR_MATERIAL_NODE_TRANSPARENT, &bb->refraction.closure_transparency);
        ThrowIfRprFailed(status, "RPR node creation failed");

        status = rprMaterialSystemCreateNode(material_system, RPR_MATERIAL_NODE_ARITHMETIC, &bb->refraction.mul);
        ThrowIfRprFailed(status, "RPR node creation failed");

        status = rprMaterialSystemCreateNode(material_system, RPR_MATERIAL_NODE_FRESNEL, &bb->refraction.fresnel);
        ThrowIfRprFailed(status, "RPR node creation failed");

        status = rprMaterialSystemCreateNode(material_system, RPR_MATERIAL_NODE_BLEND, &bb->refraction.blend);
        ThrowIfRprFailed(status, "RPR node creation failed");

        status = rprMaterialSystemCreateNode(material_system, RPR_MATERIAL_NODE_MICROFACET, &bb->coating.closure);
        ThrowIfRprFailed(status, "RPR node creation failed");

        status = rprMaterialSystemCreateNode(material_system, RPR_MATERIAL_NODE_BLEND, &bb->coating.blend);
        ThrowIfRprFailed(status, "RPR node creation failed");

        status = rprMaterialSystemCreateNode(material_system, RPR_MATERIAL_NODE_ARITHMETIC, &bb->coating.mul_weight);
        ThrowIfRprFailed(status, "RPR node creation failed");

        status = rprMaterialSystemCreateNode(material_system, RPR_MATERIAL_NODE_ARITHMETIC, &bb->coating.mul_fresnel);
        ThrowIfRprFailed(status, "RPR node creation failed");

        status = rprMaterialSystemCreateNode(material_system, RPR_MATERIAL_NODE_FRESNEL, &bb->coating.fresnel);
        ThrowIfRprFailed(status, "RPR node creation failed");

        status = rprMaterialSystemCreateNode(material_system, RPR_MATERIAL_NODE_EMISSIVE, &bb->emission.closure);
        ThrowIfRprFailed(status, "RPR node creation failed");

        status = rprMaterialSystemCreateNode(material_system, RPR_MATERIAL_NODE_BLEND, &bb->emission.blend);
        ThrowIfRprFailed(status, "RPR node creation failed");

        status = rprMaterialSystemCreateNode(material_system, RPR_MATERIAL_NODE_TRANSPARENT, &bb->transparency.closure);
        ThrowIfRprFailed(status, "RPR node creation failed");

        status = rprMaterialSystemCreateNode(material_system, RPR_MATERIAL_NODE_BLEND, &bb->transparency.blend);
        ThrowIfRprFailed(status, "RPR node creation failed");

        status = rprMaterialSystemCreateNode(material_system, RPR_MATERIAL_NODE_BLEND, &bb->volume.blend);
        ThrowIfRprFailed(status, "RPR node creation failed");

        status = rprMaterialSystemCreateNode(material_system, RPR_MATERIAL_NODE_ARITHMETIC, &bb->volume.absorption_sub);
        ThrowIfRprFailed(status, "RPR node creation failed");

        status = rprMaterialSystemCreateNode(material_system, RPR_MATERIAL_NODE_ARITHMETIC, &bb->volume.absorption_div);
        ThrowIfRprFailed(status, "RPR node creation failed");

        status = rprMaterialSystemCreateNode(material_system, RPR_MATERIAL_NODE_ARITHMETIC, &bb->volume.scatter_sub);
        ThrowIfRprFailed(status, "RPR node creation failed");

        status = rprMaterialSystemCreateNode(material_system, RPR_MATERIAL_NODE_ARITHMETIC, &bb->volume.scatter_div);
        ThrowIfRprFailed(status, "RPR node creation failed");


        // Volume
        status = rprMaterialSystemCreateNode(material_system, RPR_MATERIAL_NODE_VOLUME, &bb->volume.closure);
        ThrowIfRprFailed(status, "RPR node creation failed");

        status = rprMaterialSystemCreateNode(material_system, RPR_MATERIAL_NODE_DIFFUSE_REFRACTION, &bb->volume.closure_refract);
        ThrowIfRprFailed(status, "RPR node creation failed");

        return bb;
    }

    void UberMaterial::WireUp()
    {
        auto& bb = *m_building_blocks;

        // Set up multiplicators
        auto status = rprMaterialNodeSetInputU(bb.diffuse.mul, "op", RPR_MATERIAL_NODE_OP_MUL);
        ThrowIfRprFailed(status, "RPR node wiring failed");

        status = rprMaterialNodeSetInputU(bb.reflection.mul_weight, "op", RPR_MATERIAL_NODE_OP_MUL);
        ThrowIfRprFailed(status, "RPR node wiring failed");

        status = rprMaterialNodeSetInputU(bb.reflection.mul_fresnel, "op", RPR_MATERIAL_NODE_OP_MUL);
        ThrowIfRprFailed(status, "RPR node wiring failed");

        status = rprMaterialNodeSetInputU(bb.refraction.mul, "op", RPR_MATERIAL_NODE_OP_MUL);
        ThrowIfRprFailed(status, "RPR node wiring failed");

        status = rprMaterialNodeSetInputU(bb.coating.mul_weight, "op", RPR_MATERIAL_NODE_OP_MUL);
        ThrowIfRprFailed(status, "RPR node wiring failed");

        status = rprMaterialNodeSetInputU(bb.coating.mul_fresnel, "op", RPR_MATERIAL_NODE_OP_MUL);
        ThrowIfRprFailed(status, "RPR node wiring failed");

        status = rprMaterialNodeSetInputU(bb.volume.absorption_sub, "op", RPR_MATERIAL_NODE_OP_SUB);
        ThrowIfRprFailed(status, "RPR node wiring failed");

        status = rprMaterialNodeSetInputU(bb.volume.absorption_div, "op", RPR_MATERIAL_NODE_OP_DIV);
        ThrowIfRprFailed(status, "RPR node wiring failed");

        status = rprMaterialNodeSetInputU(bb.volume.scatter_sub, "op", RPR_MATERIAL_NODE_OP_SUB);
        ThrowIfRprFailed(status, "RPR node wiring failed");

        status = rprMaterialNodeSetInputU(bb.volume.scatter_div, "op", RPR_MATERIAL_NODE_OP_DIV);
        ThrowIfRprFailed(status, "RPR node wiring failed");

        // Wire volume mapping
        // res = (1.f - color)/dist
        status = rprMaterialNodeSetInputF(bb.volume.absorption_sub, "color0", 1.f, 1.f, 1.f, 1.f);
        ThrowIfRprFailed(status, "RPR node wiring failed");

        status = rprMaterialNodeSetInputF(bb.volume.scatter_sub, "color0", 1.f, 1.f, 1.f, 1.f);
        ThrowIfRprFailed(status, "RPR node wiring failed");

        status = rprMaterialNodeSetInputN(bb.volume.absorption_div, "color0", bb.volume.absorption_sub);
        ThrowIfRprFailed(status, "RPR node wiring failed");

        status = rprMaterialNodeSetInputN(bb.volume.scatter_div, "color0", bb.volume.scatter_sub);
        ThrowIfRprFailed(status, "RPR node wiring failed");


        // Connect diffuse weight multiplicator into diffuse albedo
        status = rprMaterialNodeSetInputN(bb.diffuse.closure, "color", bb.diffuse.mul);
        ThrowIfRprFailed(status, "RPR node wiring failed");

        // Connect refraction mul output into refraction closure
        status = rprMaterialNodeSetInputN(bb.refraction.closure_refraction, "color", bb.refraction.mul);
        ThrowIfRprFailed(status, "RPR node wiring failed");

        // Connect refraction mul output into transparency closure
        status = rprMaterialNodeSetInputN(bb.refraction.closure_transparency, "color", bb.refraction.mul);
        ThrowIfRprFailed(status, "RPR node wiring failed");

        // Connect diffuse subgraph output into the blend with refraction
        status = rprMaterialNodeSetInputN(bb.refraction.blend, "color1", bb.diffuse.closure);
        ThrowIfRprFailed(status, "RPR node wiring failed");

        // Connect refraction into the blend with reflection
        status = rprMaterialNodeSetInputN(bb.refraction.blend, "color0", bb.refraction.closure_refraction);
        ThrowIfRprFailed(status, "RPR node wiring failed");

        // Connect Fresnel as a weight
        status = rprMaterialNodeSetInputN(bb.refraction.blend, "weight", bb.refraction.fresnel);
        ThrowIfRprFailed(status, "RPR node wiring failed");

        // Connect reflection weight multiplicator into reflection albedo
        status = rprMaterialNodeSetInputN(bb.reflection.closure, "color", bb.reflection.mul_weight);
        ThrowIfRprFailed(status, "RPR node wiring failed");

        // Connect diffuse-refraction subgraph output into the blend with reflection
        status = rprMaterialNodeSetInputN(bb.reflection.blend, "color0", bb.refraction.blend);
        ThrowIfRprFailed(status, "RPR node wiring failed");

        // Connect reflection closure into the blend with diffuse
        status = rprMaterialNodeSetInputN(bb.reflection.blend, "color1", bb.reflection.closure);
        ThrowIfRprFailed(status, "RPR node wiring failed");

        // Connect fresnel multiplicator into the weight for diffuse-reflection blend
        status = rprMaterialNodeSetInputN(bb.reflection.blend, "weight", bb.reflection.mul_fresnel);
        ThrowIfRprFailed(status, "RPR node wiring failed");

        // Connect Fresnel node into Fresnel multiplicator
        status = rprMaterialNodeSetInputN(bb.reflection.mul_fresnel, "color0", bb.reflection.fresnel);
        ThrowIfRprFailed(status, "RPR node wiring failed");

        // Connect coating weight multiplicator into coating albedo
        status = rprMaterialNodeSetInputN(bb.coating.closure, "color", bb.coating.mul_weight);
        ThrowIfRprFailed(status, "RPR node wiring failed");

        // Connect refraction subgraph output into the blend with coating
        status = rprMaterialNodeSetInputN(bb.coating.blend, "color0", bb.reflection.blend);
        ThrowIfRprFailed(status, "RPR node wiring failed");

        // Connect reflection closure into the blend with diffuse
        status = rprMaterialNodeSetInputN(bb.coating.blend, "color1", bb.coating.closure);
        ThrowIfRprFailed(status, "RPR node wiring failed");

        // Connect fresnel multiplicator into the weight for diffuse-reflection blend
        status = rprMaterialNodeSetInputN(bb.coating.blend, "weight", bb.coating.mul_fresnel);
        ThrowIfRprFailed(status, "RPR node wiring failed");

        // Connect Fresnel node into Fresnel multiplicator
        status = rprMaterialNodeSetInputN(bb.coating.mul_fresnel, "color0", bb.coating.fresnel);
        ThrowIfRprFailed(status, "RPR node wiring failed");

        // Connect emission blend into transparent blend
        status = rprMaterialNodeSetInputN(bb.transparency.blend, "color0", bb.coating.blend);
        ThrowIfRprFailed(status, "RPR node wiring failed");

        // Connect tranparency closure into transparent blend
        status = rprMaterialNodeSetInputN(bb.transparency.blend, "color1", bb.transparency.closure);
        ThrowIfRprFailed(status, "RPR node wiring failed");

        // Connect coating blend into emission blend
        status = rprMaterialNodeSetInputN(bb.emission.blend, "color0", bb.transparency.blend);
        ThrowIfRprFailed(status, "RPR node wiring failed");

        // Connect emission closure into emission blend
        status = rprMaterialNodeSetInputN(bb.emission.blend, "color1", bb.emission.closure);
        ThrowIfRprFailed(status, "RPR node wiring failed");

        // Connect volume params
        status = rprMaterialNodeSetInputN(bb.volume.closure, "sigmas", bb.volume.scatter_div);
        ThrowIfRprFailed(status, "RPR node wiring failed");

        status = rprMaterialNodeSetInputN(bb.volume.closure, "sigmaa", bb.volume.absorption_div);
        ThrowIfRprFailed(status, "RPR node wiring failed");

        // Connect volume closure
        status = rprMaterialNodeSetInputN(bb.volume.blend, "color1", bb.volume.closure_refract);
        ThrowIfRprFailed(status, "RPR node wiring failed");
    }

    void UberMaterial::RewireReflection(rpr_uint mode)
    {
        auto& bb = *m_building_blocks;

        switch (mode)
        {
        case RPRX_UBER_MATERIAL_REFLECTION_MODE_PBR:
        {
            // Connect Fresnel weight directly into reflection blend
            auto status = rprMaterialNodeSetInputN(bb.reflection.blend, "weight", bb.reflection.fresnel);
            ThrowIfRprFailed(status, "RPR node wiring failed");
            break;
        }

        case RPRX_UBER_MATERIAL_REFLECTION_MODE_METALNESS:
            // Connect Fresnel weight through metalness multiplicator into reflection blend
            auto status = rprMaterialNodeSetInputN(bb.reflection.blend, "weight", bb.reflection.mul_fresnel);
            ThrowIfRprFailed(status, "RPR node wiring failed");
            // Set very high metal-like IOR
            status = rprMaterialNodeSetInputF(bb.reflection.fresnel, "ior", kMetalEmulationIor, kMetalEmulationIor, kMetalEmulationIor, kMetalEmulationIor);
            ThrowIfRprFailed(status, "RPR node wiring failed");
            break;
        }
    }

    void UberMaterial::RewireCoating(rpr_uint mode)
    {
        auto& bb = *m_building_blocks;

        switch (mode)
        {
        case RPRX_UBER_MATERIAL_COATING_MODE_PBR:
        {
            // Connect Fresnel weight directly into reflection blend
            auto status = rprMaterialNodeSetInputN(bb.coating.blend, "weight", bb.coating.fresnel);
            ThrowIfRprFailed(status, "RPR node wiring failed");
            break;
        }

        case RPRX_UBER_MATERIAL_COATING_MODE_METALNESS:
            // Connect Fresnel weight through metalness multiplicator into reflection blend
            auto status = rprMaterialNodeSetInputN(bb.coating.blend, "weight", bb.coating.mul_fresnel);
            ThrowIfRprFailed(status, "RPR node wiring failed");
            // Set very high metal-like IOR
            status = rprMaterialNodeSetInputF(bb.coating.fresnel, "ior", kMetalEmulationIor, kMetalEmulationIor, kMetalEmulationIor, kMetalEmulationIor);
            ThrowIfRprFailed(status, "RPR node wiring failed");
            break;
        }
    }

    void UberMaterial::RewireRefraction(rpr_bool thin_surface)
    {
        auto& bb = *m_building_blocks;

        // Depending on thin_surface input we either use refraction or transparency
        if (!thin_surface)
        {
            // Connect refraction into the blend with reflection
            auto status = rprMaterialNodeSetInputN(bb.refraction.blend, "color0", bb.refraction.closure_refraction);
            ThrowIfRprFailed(status, "RPR node wiring failed");

            bb.refraction.closure_current = bb.refraction.closure_refraction;
        }
        else
        {
            // Connect refraction into the blend with reflection
            auto status = rprMaterialNodeSetInputN(bb.refraction.blend, "color0", bb.refraction.closure_transparency);
            ThrowIfRprFailed(status, "RPR node wiring failed");

            bb.refraction.closure_current = bb.refraction.closure_transparency;
        }
    }

    bool UberMaterial::HasDiffuse() const
    {
        auto input_weight = GetInput(RPRX_UBER_MATERIAL_DIFFUSE_WEIGHT);
        auto input_color = GetInput(RPRX_UBER_MATERIAL_DIFFUSE_COLOR);
        return !(input_weight.IsZero() || input_color.IsZero());
    }

    bool UberMaterial::HasReflection() const
    {
        auto input_weight = GetInput(RPRX_UBER_MATERIAL_REFLECTION_WEIGHT);
        auto input_color = GetInput(RPRX_UBER_MATERIAL_REFLECTION_COLOR);
        return !(input_weight.IsZero() || input_color.IsZero());
    }


    bool UberMaterial::HasCoating() const
    {
        auto input_weight = GetInput(RPRX_UBER_MATERIAL_COATING_WEIGHT);
        auto input_color = GetInput(RPRX_UBER_MATERIAL_COATING_COLOR);
        return !(input_weight.IsZero() || input_color.IsZero());
    }

    bool UberMaterial::HasRefraction() const
    {
        auto input_weight = GetInput(RPRX_UBER_MATERIAL_REFRACTION_WEIGHT);
        auto input_color = GetInput(RPRX_UBER_MATERIAL_REFRACTION_COLOR);
        return !(input_weight.IsZero() || input_color.IsZero());
    }

    bool UberMaterial::HasEmission() const
    {
        auto input_weight = GetInput(RPRX_UBER_MATERIAL_EMISSION_WEIGHT);
        auto input_color = GetInput(RPRX_UBER_MATERIAL_EMISSION_COLOR);
        return !(input_weight.IsZero() || input_color.IsZero());
    }

    bool UberMaterial::HasTransparency() const
    {
        auto input = GetInput(RPRX_UBER_MATERIAL_TRANSPARENCY);
        return !(input.IsZero() || input.IsZero());
    }

    bool UberMaterial::HasSSS() const
    {
        auto input_weight = GetInput(RPRX_UBER_MATERIAL_SSS_WEIGHT);
        auto input_absorption_color = GetInput(RPRX_UBER_MATERIAL_SSS_ABSORPTION_COLOR);
        auto input_scatter_color = GetInput(RPRX_UBER_MATERIAL_SSS_SCATTER_COLOR);

        return !(input_weight.IsZero() || input_absorption_color.IsZero() && input_scatter_color.IsZero());
    }

    RPRMaterialBundle UberMaterial::CompileGraph()
    {
        auto& bb = *m_building_blocks;

        // Reflection
        {
            // We might need to rewire here
            auto mode = GetInput(RPRX_UBER_MATERIAL_REFLECTION_MODE);

            // If wiring has changed
            if (mode.dirty)
            {
                // Wire up reflection part
                RewireReflection(mode.value_ui);
            }
        }

        // Refraction
        {
            // We might need to rewire here
            auto thin_surface = GetInput(RPRX_UBER_MATERIAL_REFRACTION_THIN_SURFACE);

            // If wiring has changed
            if (thin_surface.dirty)
            {
                // Wire up reflection part
                RewireRefraction(thin_surface.value_ui);
            }
        }

        // Coating
        {
            // We might need to rewire here
            auto mode = GetInput(RPRX_UBER_MATERIAL_COATING_MODE);

            // If wiring has changed
            if (mode.dirty)
            {
                // Wire up reflection part
                RewireCoating(mode.value_ui);
            }
        }

        // Start with diffuse component
        rpr_material_node running_out = nullptr;
        rpr_material_node refraction_blend = nullptr;


        //status = rprMaterialNodeSetInputN(bb.volume.blend, "color1", bb.transparency.blend);
        //ThrowIfRprFailed(status, "RPR node wiring failed");


        // If it has SSS
        rpr_material_node sss = nullptr;
        if (HasSSS())
        {
            running_out = bb.volume.closure_refract;
            refraction_blend = bb.volume.blend;
            sss = bb.volume.closure;

            auto status = rprMaterialNodeSetInputN(bb.volume.blend, "color0", bb.diffuse.closure);
            ThrowIfRprFailed(status, "RPR node wiring failed");
        }

        // If it has reflection, output is refraction blend
        if (HasRefraction() && !sss)
        {
            running_out = bb.refraction.closure_current;
            refraction_blend = bb.refraction.blend;

            auto status = rprMaterialNodeSetInputN(bb.refraction.blend, "color1", bb.diffuse.closure);
            ThrowIfRprFailed(status, "RPR node wiring failed");
        }

        // If it has reflection, output is reflection blend
        if (HasDiffuse())
        {
            running_out = running_out ? refraction_blend : bb.diffuse.closure;
        }

        // If it has reflection, output is reflection blend
        if (HasReflection())
        {
            auto status = rprMaterialNodeSetInputN(bb.reflection.blend, "color0", running_out);
            ThrowIfRprFailed(status, "RPR node wiring failed");

            running_out = running_out ? bb.reflection.blend : bb.reflection.closure;
        }

        // If it has coating, output is refraction blend
        if (HasCoating())
        {
            auto status = rprMaterialNodeSetInputN(bb.coating.blend, "color0", running_out);
            ThrowIfRprFailed(status, "RPR node wiring failed");

            running_out = running_out ? bb.coating.blend : bb.coating.closure;
        }

        // Check transparency
        if (HasTransparency())
        {
            auto status = rprMaterialNodeSetInputN(bb.transparency.blend, "color0", running_out);
            ThrowIfRprFailed(status, "RPR node wiring failed");

            running_out = running_out ? bb.transparency.blend : bb.transparency.closure;
        }


        // Check emission
        if (HasEmission())
        {
            auto status = rprMaterialNodeSetInputN(bb.emission.blend, "color0", running_out);
            ThrowIfRprFailed(status, "RPR node wiring failed");

            running_out = running_out ? bb.emission.blend : bb.emission.closure;
        }

        auto displacement = GetInput(RPRX_UBER_MATERIAL_DISPLACEMENT);
        return RPRMaterialBundle{ running_out, sss, displacement.value_ptr  };
    }

    bool UberMaterial::ApplyInputChange(rpr_material_node node, std::string const& name, Input const& input)
    {
        if (input.dirty)
        {
            switch (input.type)
            {
            case InputType::kFloat4:
            {
                auto status = rprMaterialNodeSetInputF(node, name.c_str(), input.value_f4.x, input.value_f4.y, input.value_f4.z, input.value_f4.w);
                ThrowIfRprFailed(status, "RPR input value setter failed");
                break;
            }
            case InputType::kUint:
            {
                auto status = rprMaterialNodeSetInputU(node, name.c_str(), input.value_ui);
                ThrowIfRprFailed(status, "RPR input value setter failed");
                break;
            }
            case InputType::kPointer:
            {
                auto status = rprMaterialNodeSetInputN(node, name.c_str(), input.value_ptr);
                ThrowIfRprFailed(status, "RPR input value setter failed");
                break;
            }
            }

            return true;
        }

        return false;
    }

    void UberMaterial::MapParameters()
    {
        auto& bb = *m_building_blocks;

		const Material::Input& matNormal = GetInput(RPRX_UBER_MATERIAL_NORMAL);
		const Material::Input& matBump = GetInput(RPRX_UBER_MATERIAL_BUMP);

        // Map diffuse parameters
        // Diffuse color and weight go into mul node inputs
        ApplyInputChange(bb.diffuse.mul, "color0", GetInput(RPRX_UBER_MATERIAL_DIFFUSE_COLOR));
        ApplyInputChange(bb.diffuse.mul, "color1", GetInput(RPRX_UBER_MATERIAL_DIFFUSE_WEIGHT));
        // Diffuse roughness and normal got into diffuse node directly
        ApplyInputChange(bb.diffuse.closure, "roughness", GetInput(RPRX_UBER_MATERIAL_DIFFUSE_ROUGHNESS));
        ApplyInputChange(bb.diffuse.closure, "normal", matNormal.IsZero() ? matBump : matNormal);

        // Reflection color and weight go into mul node inputs
        ApplyInputChange(bb.reflection.mul_weight, "color0", GetInput(RPRX_UBER_MATERIAL_REFLECTION_COLOR));
        ApplyInputChange(bb.reflection.mul_weight, "color1", GetInput(RPRX_UBER_MATERIAL_REFLECTION_WEIGHT));


        // Anisotropy rotation & normal go into reflection node directly 
        ApplyInputChange(bb.reflection.closure, "rotation", GetInput(RPRX_UBER_MATERIAL_REFLECTION_ANISOTROPY_ROTATION));
		ApplyInputChange(bb.reflection.closure, "roughness", GetInput(RPRX_UBER_MATERIAL_REFLECTION_ROUGHNESS));
		ApplyInputChange(bb.reflection.closure, "anisotropic", GetInput(RPRX_UBER_MATERIAL_REFLECTION_ANISOTROPY));
        ApplyInputChange(bb.reflection.closure, "normal", matNormal.IsZero() ? matBump : matNormal);

        // Pick up IOR or metalness (they have same id) depending on configured workflow
        if (GetInput(RPRX_UBER_MATERIAL_REFLECTION_MODE).value_ui == RPRX_UBER_MATERIAL_REFLECTION_MODE_PBR)
            ApplyInputChange(bb.reflection.fresnel, "ior", GetInput(RPRX_UBER_MATERIAL_REFLECTION_IOR));
        else
            ApplyInputChange(bb.reflection.mul_fresnel, "color1", GetInput(RPRX_UBER_MATERIAL_REFLECTION_METALNESS));

        // Map refraction parameters
        // Refraction color and weight go into mul node inputs
        ApplyInputChange(bb.refraction.mul, "color0", GetInput(RPRX_UBER_MATERIAL_REFRACTION_COLOR));
        ApplyInputChange(bb.refraction.mul, "color1", GetInput(RPRX_UBER_MATERIAL_REFRACTION_WEIGHT));
        ApplyInputChange(bb.refraction.closure_refraction, "roughness", GetInput(RPRX_UBER_MATERIAL_REFRACTION_ROUGHNESS));

        // Take the value from reflection IOR for linked mode and use separate one for separate mode
        if (GetInput(RPRX_UBER_MATERIAL_REFRACTION_IOR_MODE).value_ui == RPRX_UBER_MATERIAL_REFRACTION_MODE_SEPARATE)
        {
            ApplyInputChange(bb.refraction.fresnel, "ior", GetInput(RPRX_UBER_MATERIAL_REFRACTION_IOR));
            ApplyInputChange(bb.refraction.closure_refraction, "ior", GetInput(RPRX_UBER_MATERIAL_REFRACTION_IOR));
        }
        else
        {
            // Only allow linked mode if reflection workflow is set to PBR, otherwise it does not make sense
            if (GetInput(RPRX_UBER_MATERIAL_REFLECTION_MODE).value_ui == RPRX_UBER_MATERIAL_REFLECTION_MODE_PBR)
            {
                ApplyInputChange(bb.refraction.fresnel, "ior", GetInput(RPRX_UBER_MATERIAL_REFLECTION_IOR));
                ApplyInputChange(bb.refraction.closure_refraction, "ior", GetInput(RPRX_UBER_MATERIAL_REFLECTION_IOR));
            }
            else
                throw std::runtime_error("Error: attemp to link refraction IOR to reflection set to metalness workflow (use PBR instead).");
        }


        // Coating color and weight go into mul node inputs
        ApplyInputChange(bb.coating.mul_weight, "color0", GetInput(RPRX_UBER_MATERIAL_COATING_COLOR));
        ApplyInputChange(bb.coating.mul_weight, "color1", GetInput(RPRX_UBER_MATERIAL_COATING_WEIGHT));

        // Anisotropy rotation & normal go into reflection node directly 
        ApplyInputChange(bb.coating.closure, "roughness", GetInput(RPRX_UBER_MATERIAL_COATING_ROUGHNESS));
        //ApplyInputChange(bb.coating.closure, "normal", GetInput(RPRX_UBER_COATING_NORMAL));

        // Pick up IOR or metalness (they have same id) depending on configured workflow
        if (GetInput(RPRX_UBER_MATERIAL_COATING_MODE).value_ui == RPRX_UBER_MATERIAL_COATING_MODE_PBR)
            ApplyInputChange(bb.coating.fresnel, "ior", GetInput(RPRX_UBER_MATERIAL_COATING_IOR));
        else
            ApplyInputChange(bb.coating.mul_fresnel, "color1", GetInput(RPRX_UBER_MATERIAL_COATING_METALNESS));

        // Diffuse refraction
        ApplyInputChange(bb.volume.closure_refract, "color", GetInput(RPRX_UBER_MATERIAL_SSS_SUBSURFACE_COLOR));
        ApplyInputChange(bb.volume.blend, "weight", GetInput(RPRX_UBER_MATERIAL_SSS_WEIGHT));

        // Emission
        ApplyInputChange(bb.emission.closure, "color", GetInput(RPRX_UBER_MATERIAL_EMISSION_COLOR));
        ApplyInputChange(bb.emission.blend, "weight", GetInput(RPRX_UBER_MATERIAL_EMISSION_WEIGHT));

        // Volume
        ApplyInputChange(bb.volume.absorption_sub, "color1", GetInput(RPRX_UBER_MATERIAL_SSS_ABSORPTION_COLOR));
        ApplyInputChange(bb.volume.absorption_div, "color1", GetInput(RPRX_UBER_MATERIAL_SSS_ABSORPTION_DISTANCE));

        ApplyInputChange(bb.volume.scatter_sub, "color1", GetInput(RPRX_UBER_MATERIAL_SSS_SCATTER_COLOR));
        ApplyInputChange(bb.volume.scatter_div, "color1", GetInput(RPRX_UBER_MATERIAL_SSS_SCATTER_DISTANCE));

        ApplyInputChange(bb.volume.closure, "g", GetInput(RPRX_UBER_MATERIAL_SSS_SCATTER_DIRECTION));

        if (GetInput(RPRX_UBER_MATERIAL_SSS_MULTISCATTER).value_ui == RPR_TRUE)
        {
            auto status = rprMaterialNodeSetInputF(bb.volume.closure, "multiscatter", 1.f, 1.f, 1.f, 1.f);
            ThrowIfRprFailed(status, "RPR input value setter failed");
        }
        else
        {
            auto status = rprMaterialNodeSetInputF(bb.volume.closure, "multiscatter", 0.f, 0.f, 0.f, 0.f);
            ThrowIfRprFailed(status, "RPR input value setter failed");
        }

        // Transparency
        ApplyInputChange(bb.transparency.blend, "weight", GetInput(RPRX_UBER_MATERIAL_TRANSPARENCY));
    }

    void UberMaterial::SetDefaults()
    {
        SetInput(RPRX_UBER_MATERIAL_DIFFUSE_COLOR, { 1.f, 1.f, 1.f, 1.f });
        SetInput(RPRX_UBER_MATERIAL_DIFFUSE_WEIGHT, { 1.f, 1.f, 1.f, 1.f });
        SetInput(RPRX_UBER_MATERIAL_DIFFUSE_ROUGHNESS, { 1.f, 1.f, 1.f, 1.f });
        SetInput(RPRX_UBER_MATERIAL_REFLECTION_COLOR, { 1.f, 1.f, 1.f, 1.f });
        SetInput(RPRX_UBER_MATERIAL_REFLECTION_WEIGHT, { 0.f, 0.f, 0.f, 0.f });
        SetInput(RPRX_UBER_MATERIAL_REFLECTION_ROUGHNESS, { 0.5f, 0.5f, 0.5f, 0.5f });
        SetInput(RPRX_UBER_MATERIAL_REFLECTION_ANISOTROPY, { 0.f, 0.f, 0.f, 0.f });
        SetInput(RPRX_UBER_MATERIAL_REFLECTION_ANISOTROPY_ROTATION, { 0.f, 0.f, 0.f, 0.f });
        SetInput(RPRX_UBER_MATERIAL_REFLECTION_MODE, RPRX_UBER_MATERIAL_REFLECTION_MODE_PBR);
        SetInput(RPRX_UBER_MATERIAL_REFLECTION_IOR, { 1.5f, 1.5f, 1.5f, 1.5f });
        SetInput(RPRX_UBER_MATERIAL_REFRACTION_COLOR, { 1.f, 1.f, 1.f, 1.f });
        SetInput(RPRX_UBER_MATERIAL_REFRACTION_WEIGHT, { 0.f, 0.f, 0.f, 0.f });
        SetInput(RPRX_UBER_MATERIAL_REFRACTION_ROUGHNESS, { 0.f, 0.f, 0.f, 0.f });
        SetInput(RPRX_UBER_MATERIAL_REFRACTION_IOR, { 1.5f, 1.5f, 1.5f, 1.5f });
        SetInput(RPRX_UBER_MATERIAL_REFRACTION_IOR_MODE, RPRX_UBER_MATERIAL_REFRACTION_MODE_SEPARATE);
        SetInput(RPRX_UBER_MATERIAL_REFRACTION_THIN_SURFACE, (rpr_uint)RPR_FALSE);
        SetInput(RPRX_UBER_MATERIAL_REFRACTION_IOR_MODE, RPRX_UBER_MATERIAL_REFRACTION_MODE_SEPARATE);
        SetInput(RPRX_UBER_MATERIAL_REFRACTION_IOR_MODE, RPRX_UBER_MATERIAL_REFRACTION_MODE_SEPARATE);
        SetInput(RPRX_UBER_MATERIAL_COATING_COLOR, { 1.f, 1.f, 1.f, 1.f });
        SetInput(RPRX_UBER_MATERIAL_COATING_WEIGHT, { 0.f, 0.f, 0.f, 0.f });
        SetInput(RPRX_UBER_MATERIAL_COATING_ROUGHNESS, { 0.f, 0.f, 0.f, 0.f });
        SetInput(RPRX_UBER_MATERIAL_COATING_MODE, RPRX_UBER_MATERIAL_COATING_MODE_PBR);
        SetInput(RPRX_UBER_MATERIAL_COATING_IOR, { 3.f, 3.f, 3.f, 3.f });
        SetInput(RPRX_UBER_MATERIAL_EMISSION_COLOR, { 1.f, 1.f, 1.f, 1.f });
        SetInput(RPRX_UBER_MATERIAL_EMISSION_WEIGHT, { 0.f, 0.f, 0.f, 0.f });
        SetInput(RPRX_UBER_MATERIAL_EMISSION_MODE, RPRX_UBER_MATERIAL_EMISSION_MODE_SINGLESIDED);
        SetInput(RPRX_UBER_MATERIAL_TRANSPARENCY, { 0.f, 0.f, 0.f, 0.f });
        SetInput(RPRX_UBER_MATERIAL_SSS_WEIGHT, { 0.f, 0.f, 0.f, 0.f });
        SetInput(RPRX_UBER_MATERIAL_SSS_ABSORPTION_COLOR, { 0.f, 0.f, 0.f, 0.f });
        SetInput(RPRX_UBER_MATERIAL_SSS_SCATTER_COLOR, { 0.f, 0.f, 0.f, 0.f });
        SetInput(RPRX_UBER_MATERIAL_SSS_ABSORPTION_DISTANCE, { 0.f, 0.f, 0.f, 0.f });
        SetInput(RPRX_UBER_MATERIAL_SSS_SCATTER_DISTANCE, { 0.f, 0.f, 0.f, 0.f });
        SetInput(RPRX_UBER_MATERIAL_SSS_SCATTER_DIRECTION, { 0.f, 0.f, 0.f, 0.f });
        SetInput(RPRX_UBER_MATERIAL_SSS_SUBSURFACE_COLOR, { 1.f, 1.f, 1.f, 1.f });
        SetInput(RPRX_UBER_MATERIAL_SSS_MULTISCATTER, (rpr_uint)RPR_FALSE);
    }


    UberMaterial::InputInitializerArray UberMaterial::s_inputs =
    {
        InputDesc
        {
            RPRX_UBER_MATERIAL_DIFFUSE_COLOR,
            (static_cast<std::uint32_t>(InputType::kFloat4) |
                static_cast<std::uint32_t>(InputType::kPointer)),
            true
        },

        InputDesc
        {
            RPRX_UBER_MATERIAL_DIFFUSE_WEIGHT,
            (static_cast<std::uint32_t>(InputType::kFloat4) |
                static_cast<std::uint32_t>(InputType::kPointer)),
            true
        },

        InputDesc
        {
            RPRX_UBER_MATERIAL_DIFFUSE_ROUGHNESS,
            (static_cast<std::uint32_t>(InputType::kFloat4) |
                static_cast<std::uint32_t>(InputType::kPointer))
        },

        InputDesc
        {
            RPRX_UBER_MATERIAL_REFLECTION_COLOR,
            (static_cast<std::uint32_t>(InputType::kFloat4) |
                static_cast<std::uint32_t>(InputType::kPointer)),
            true
        },

        InputDesc
        {
            RPRX_UBER_MATERIAL_REFLECTION_WEIGHT,
            (static_cast<std::uint32_t>(InputType::kFloat4) |
                static_cast<std::uint32_t>(InputType::kPointer)),
            true
        },

        InputDesc
        {
            RPRX_UBER_MATERIAL_REFLECTION_ROUGHNESS,
            (static_cast<std::uint32_t>(InputType::kFloat4) |
                static_cast<std::uint32_t>(InputType::kPointer))
        },

        InputDesc
        {
            RPRX_UBER_MATERIAL_REFLECTION_ANISOTROPY,
            (static_cast<std::uint32_t>(InputType::kFloat4) |
                static_cast<std::uint32_t>(InputType::kPointer))
        },

        InputDesc
        {
            RPRX_UBER_MATERIAL_REFLECTION_ANISOTROPY_ROTATION,
            (static_cast<std::uint32_t>(InputType::kFloat4) |
                static_cast<std::uint32_t>(InputType::kPointer))
        },

        InputDesc
        {
            RPRX_UBER_MATERIAL_REFLECTION_MODE,
            static_cast<std::uint32_t>(InputType::kUint),
            true
        },

        InputDesc
        {
            RPRX_UBER_MATERIAL_REFLECTION_IOR,
            (static_cast<std::uint32_t>(InputType::kFloat4) |
                static_cast<std::uint32_t>(InputType::kPointer))
        },

        InputDesc
        {
            RPRX_UBER_MATERIAL_REFRACTION_COLOR,
            (static_cast<std::uint32_t>(InputType::kFloat4) |
            static_cast<std::uint32_t>(InputType::kPointer)),
            true
        },

        InputDesc
        {
            RPRX_UBER_MATERIAL_REFRACTION_WEIGHT,
            (static_cast<std::uint32_t>(InputType::kFloat4) |
                static_cast<std::uint32_t>(InputType::kPointer)),
            true
        },

        InputDesc
        {
            RPRX_UBER_MATERIAL_REFRACTION_ROUGHNESS,
            (static_cast<std::uint32_t>(InputType::kFloat4) |
                static_cast<std::uint32_t>(InputType::kPointer))
        },

        InputDesc
        {
            RPRX_UBER_MATERIAL_REFRACTION_IOR,
            (static_cast<std::uint32_t>(InputType::kFloat4) |
            static_cast<std::uint32_t>(InputType::kPointer))
        },

        InputDesc
        {
            RPRX_UBER_MATERIAL_REFRACTION_IOR_MODE,
            static_cast<std::uint32_t>(InputType::kUint),
            true
        },

        InputDesc
        {
            RPRX_UBER_MATERIAL_REFRACTION_THIN_SURFACE,
            static_cast<std::uint32_t>(InputType::kUint),
            true
        },

        InputDesc
        {
            RPRX_UBER_MATERIAL_COATING_COLOR,
            (static_cast<std::uint32_t>(InputType::kFloat4) |
                static_cast<std::uint32_t>(InputType::kPointer)),
            true
        },

        InputDesc
        {
            RPRX_UBER_MATERIAL_COATING_WEIGHT,
            (static_cast<std::uint32_t>(InputType::kFloat4) |
                static_cast<std::uint32_t>(InputType::kPointer)),
            true
        },

        InputDesc
        {
            RPRX_UBER_MATERIAL_COATING_ROUGHNESS,
            (static_cast<std::uint32_t>(InputType::kFloat4) |
                static_cast<std::uint32_t>(InputType::kPointer))
        },

        InputDesc
        {
            RPRX_UBER_MATERIAL_COATING_MODE,
            static_cast<std::uint32_t>(InputType::kUint),
            true
        },


        InputDesc
        {
            RPRX_UBER_MATERIAL_COATING_IOR,
            (static_cast<std::uint32_t>(InputType::kFloat4) |
                static_cast<std::uint32_t>(InputType::kPointer))
        },

        InputDesc
        {
            RPRX_UBER_MATERIAL_EMISSION_COLOR,
            (static_cast<std::uint32_t>(InputType::kFloat4) |
                static_cast<std::uint32_t>(InputType::kPointer)),
            true
        },

        InputDesc
        {
            RPRX_UBER_MATERIAL_EMISSION_WEIGHT,
            (static_cast<std::uint32_t>(InputType::kFloat4) |
                static_cast<std::uint32_t>(InputType::kPointer)),
            true
        },

        InputDesc
        {
            RPRX_UBER_MATERIAL_EMISSION_MODE,
            static_cast<std::uint32_t>(InputType::kUint),
            true
        },

        InputDesc
        {
            RPRX_UBER_MATERIAL_TRANSPARENCY,
            (static_cast<std::uint32_t>(InputType::kFloat4) |
                static_cast<std::uint32_t>(InputType::kPointer)),
            true
        },

        InputDesc
        {
            RPRX_UBER_MATERIAL_NORMAL,
            static_cast<std::uint32_t>(InputType::kPointer)
        },

        InputDesc
        {
            RPRX_UBER_MATERIAL_BUMP,
            static_cast<std::uint32_t>(InputType::kPointer)
        },

        InputDesc
        {
            RPRX_UBER_MATERIAL_DISPLACEMENT,
            static_cast<std::uint32_t>(InputType::kPointer)
        },

        InputDesc
        {
            RPRX_UBER_MATERIAL_SSS_WEIGHT,
            (static_cast<std::uint32_t>(InputType::kFloat4) |
            static_cast<std::uint32_t>(InputType::kPointer)),
            true
        },

        InputDesc
        {
            RPRX_UBER_MATERIAL_SSS_ABSORPTION_COLOR,
            (static_cast<std::uint32_t>(InputType::kFloat4) |
            static_cast<std::uint32_t>(InputType::kPointer)),
            true
        },

        InputDesc
        {
            RPRX_UBER_MATERIAL_SSS_SCATTER_COLOR,
            (static_cast<std::uint32_t>(InputType::kFloat4) |
            static_cast<std::uint32_t>(InputType::kPointer)),
            true
        },

        InputDesc
        {
            RPRX_UBER_MATERIAL_SSS_ABSORPTION_DISTANCE,
             (static_cast<std::uint32_t>(InputType::kFloat4) |
             static_cast<std::uint32_t>(InputType::kPointer))
        },

        InputDesc
        {
            RPRX_UBER_MATERIAL_SSS_SCATTER_DISTANCE,
            (static_cast<std::uint32_t>(InputType::kFloat4) |
            static_cast<std::uint32_t>(InputType::kPointer))
        },

        InputDesc
        {
            RPRX_UBER_MATERIAL_SSS_SCATTER_DIRECTION,
            (static_cast<std::uint32_t>(InputType::kFloat4) |
            static_cast<std::uint32_t>(InputType::kPointer))
        },

        InputDesc
        {
            RPRX_UBER_MATERIAL_SSS_SUBSURFACE_COLOR,
            (static_cast<std::uint32_t>(InputType::kFloat4) |
            static_cast<std::uint32_t>(InputType::kPointer))
        },

        InputDesc
        {
            RPRX_UBER_MATERIAL_SSS_MULTISCATTER,
            (static_cast<std::uint32_t>(InputType::kUint))
        }
    };
}