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
#pragma once

#include "Material.h"
#include <array>
#include <memory>

namespace RprSupport
{
    /*
    \brief RPR Uber shader

    RPR uber shader is spec'd here:[INSERT LINK]
    and has the following parameters:
    ...
    */
    class UberMaterial : public Material
    {
    public:
        UberMaterial(rpr_material_system material_system);
        ~UberMaterial();

    private:
        // Customization API
        // Compile graph after structural change
        RPRMaterialBundle CompileGraph() override;
        // Map parameters into graph node parameters
        void MapParameters() override;

        // Private API
        struct BuildingBlocks;
        // Allocate building blocks and create RPR nodes in it
        BuildingBlocks* CreateBuildingBlocks() const;

        // Do initial graph wiring
        void WireUp();
        // Set defaults
        void SetDefaults();
        // Rewires graph for metalness or ior reflection workflows
        void RewireReflection(rpr_uint mode);
        // Rewires graph for linked vs free IOR workflows
        void RewireRefraction(rpr_bool thin_surface);
        // Rewires graph for metalness or ior reflection workflows
        void RewireCoating(rpr_uint mode);
        // Check whether diffuse component should be enabled
        bool HasDiffuse() const;
        // Check whether reflection component should be enabled
        bool HasReflection() const;
        // Check whether refraction component should be enabled
        bool HasRefraction() const;
        // Check whether emission component should be enabled
        bool HasCoating() const;
        // Check whether emission component should be enabled
        bool HasEmission() const;
        // Check whether material has transparency
        bool HasTransparency() const;
        // Check whether material has SSS
        bool HasSSS() const;
        // Apply parameter change, return true if applied
        bool ApplyInputChange(rpr_material_node node, std::string const& name, Input const& input);

        // Structure containing RPR nodes which are being 
        // used as building blocks for the graph
        std::unique_ptr<BuildingBlocks> m_building_blocks;

        // Number of inputs for Uber material
        static constexpr std::uint32_t kNumInputs = 36;
        // Input descriptor
        struct InputDesc;
        // Input array
        using  InputInitializerArray = std::array <InputDesc, kNumInputs>;
        static InputInitializerArray s_inputs;
    };
}