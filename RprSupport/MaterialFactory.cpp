/*****************************************************************************\
*
*  Module Name    MaterialFactory.cpp
*  Project        RRP support library
*
*  Description    RRP material factory interface
*
*  Copyright 2017 Advanced Micro Devices, Inc.
*
*  All rights reserved.  This notice is intended as a precaution against
*  inadvertent publication and does not imply publication or any waiver
*  of confidentiality.  The year included in the foregoing notice is the
*  year of creation of the work.
\*****************************************************************************/
#include "MaterialFactory.h"
#include "UberMaterial.h"

#include <stdexcept>

namespace RprSupport
{
    class DefaultMaterialFactory : public MaterialFactory
    {
    public:
        DefaultMaterialFactory(rpr_material_system material_system)
            : MaterialFactory(material_system)
        {
        }

        // Create a material of specified type
        Material* CreateMaterial(rpr_uint type) override;
    };

    Material* DefaultMaterialFactory::CreateMaterial(rpr_uint type)
    {
        switch (type)
        {
        case RPRX_MATERIAL_UBER:
            return new UberMaterial(GetMaterialSystem());
        default:
            throw std::runtime_error("Error: DefaultMaterialFactory - material type not supported");
        }

        return nullptr;
    }

    MaterialFactory* MaterialFactory::CreateDefault(rpr_material_system material_system)
    {
        return new DefaultMaterialFactory(material_system);
    }
}