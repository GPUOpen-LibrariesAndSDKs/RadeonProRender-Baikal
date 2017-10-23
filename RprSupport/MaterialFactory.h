/*****************************************************************************\
*
*  Module Name    MaterialFactory.h
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
#pragma once

#include "RprSupport.h"

namespace RprSupport
{
    class Material;

    /*
    \brief Material factory

    Factory decouple material creation from the specific place it is needed
    */
    class MaterialFactory
    {
    public:
        static MaterialFactory* CreateDefault(rpr_material_system material_system);

        MaterialFactory(rpr_material_system material_system);
        virtual ~MaterialFactory();

        // Create a material of specified type
        virtual Material* CreateMaterial(rpr_uint type) = 0;

        MaterialFactory(MaterialFactory const&) = delete;
        MaterialFactory& operator = (MaterialFactory const&) = delete;

    protected:
        rpr_material_system GetMaterialSystem() const { return m_material_system; }

    private:
        rpr_material_system m_material_system;
    };

    inline MaterialFactory::MaterialFactory(rpr_material_system material_system)
        : m_material_system(material_system)
    {
    }

    inline MaterialFactory::~MaterialFactory()
    {
    }
}