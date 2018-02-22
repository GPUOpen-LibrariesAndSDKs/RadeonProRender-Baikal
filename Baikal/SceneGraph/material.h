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

/**
 \file material.h
 \author Dmitry Kozlov
 \version 1.0
 \brief Contains a class representing material
 */
#pragma once

#include <set>
#include <unordered_map>
#include <string>
#include <memory>

#include "math/float3.h"

#include "scene_object.h"
#include "texture.h"

namespace Baikal
{
    class Iterator;
    class Texture;

    /**
     \brief High level material interface
     
     \details Base class for all CPU side material supported by the renderer.
     */
    class Material : public SceneObject
    {
    public:
        using Ptr = std::shared_ptr<Material>;

        friend class MaterialAccessor;

        // Material input type
        enum class InputType
        {
            kUint = 0,
            kFloat4,
            kTexture,
            kMaterial
        };

        // Input description
        struct InputInfo
        {
            // Short name
            std::string name;
            // Desctiption
            std::string desc;
            // Set of supported types
            std::set<InputType> supported_types;
        };

        // Input value description
        struct InputValue {
            // Current type
            InputType type;
            
            // Possible values (use based on type)
            uint32_t uint_value;
            RadeonRays::float4 float_value;
            Texture::Ptr tex_value;
            Material::Ptr mat_value;
            
            InputValue()
            : type(InputType::kMaterial)
            , mat_value(nullptr)
            , tex_value(nullptr)
            , float_value()
            {
            }
        };

        // Full input state
        struct Input
        {
            InputInfo info;
            InputValue value;
        };

        using InputMap = std::unordered_map<std::string, Input>;

        // Destructor
        virtual ~Material() = 0;

        // Iterator of dependent materials (plugged as inputs)
        virtual std::unique_ptr<Iterator> CreateMaterialIterator() const;
        // Iterator of textures (plugged as inputs)
        virtual std::unique_ptr<Iterator> CreateTextureIterator() const;
        // Check if material has emissive components
        virtual bool HasEmission() const;

        // Set input value
        // If specific data type is not supported throws std::runtime_error
        void SetInputValue(std::string const& name, uint32_t value);
        void SetInputValue(std::string const& name,
                           RadeonRays::float4 const& value);
        void SetInputValue(std::string const& name, Texture::Ptr texture);
        void SetInputValue(std::string const& name, Material::Ptr material);

        InputValue GetInputValue(std::string const& name) const;

        // Check if material is thin (normal is always pointing in ray incidence
        // direction)
        bool IsThin() const;
        // Set thin flag
        void SetThin(bool thin);

        size_t GetNumInputs() const;
        Input GetInput(std::uint32_t idx) const;

        Material(Material const&) = delete;
        Material& operator = (Material const&) = delete;

    protected:
        Input& GetInput(const std::string& name, InputType type);

        Material();
    
        // Register specific input
        void RegisterInput(std::string const& name, std::string const& desc,
                           std::set<InputType>&& supported_types);
        
        // Wipe out all the inputs
        void ClearInputs();

    private:
        // Input map
        InputMap m_inputs;
        // Thin material
        bool m_thin;
    };

    inline Material::~Material()
    {
    }

    inline bool Material::HasEmission() const
    {
        return false;
    }

    class SingleBxdf : public Material
    {
    public:
        enum class BxdfType
        {
            kZero = 0,
            kLambert,
            kIdealReflect,
            kIdealRefract,
            kMicrofacetBeckmann,
            kMicrofacetGGX,
            kEmissive,
            kPassthrough,
            kTranslucent,
            kMicrofacetRefractionGGX,
            kMicrofacetRefractionBeckmann
        };
        
        using Ptr = std::shared_ptr<SingleBxdf>;
        static Ptr Create(BxdfType type);

        BxdfType GetBxdfType() const;
        void SetBxdfType(BxdfType type);

        // Check if material has emissive components
        bool HasEmission() const override;

    protected:
        SingleBxdf(BxdfType type);

    private:
        BxdfType m_type;
    };
    
    class MultiBxdf : public Material
    {
    public:
        enum class Type
        {
            kLayered = 0,
            kFresnelBlend,
            kMix
        };
        
        using Ptr = std::shared_ptr<MultiBxdf>;
        static Ptr Create(Type type);

        Type GetType() const;
        void SetType(Type type);

        // Check if material has emissive components
        bool HasEmission() const override;

    protected:
        MultiBxdf(Type type);

    private:
        Type m_type;
    };
    
    class DisneyBxdf : public Material
    {
    public:
        using Ptr = std::shared_ptr<DisneyBxdf>;
        static Ptr Create();
        
        // Check if material has emissive components
        bool HasEmission() const override;

    protected:
        DisneyBxdf();
    };

    class VolumeMaterial : public Material
    {
    public:
        enum class PhaseFunction
        {
            kUniform = 0,
            kRayleigh,
            kMieMurky,
            kMieHazy
        };

        using Ptr = std::shared_ptr<VolumeMaterial>;
        static Ptr Create();

        // Check if material has emissive components
        bool HasEmission() const override;

    protected:
        VolumeMaterial();
    };

    class UberV2Material : public Material
    {
    public:
        enum RefractionMode
        {
            kRefractionSeparate = 1U,
            kRefractionLinked = 2U
        };
        enum EmissionMode
        {
            kEmissionSinglesided = 1U,
            kEmissionDoublesided = 2U
        };

        enum Layers
        {
            kEmissionLayer = 0x1,
            kTransparencyLayer = 0x2,
            kCoatingLayer = 0x4,
            kReflectionLayer = 0x8,
            kDiffuseLayer = 0x10,
            kRefractionLayer = 0x20,
            kSSSLayer = 0x40
        };

        using Ptr = std::shared_ptr<UberV2Material>;
        static Ptr Create();

        // Check if material has emissive components
        bool HasEmission() const override;

    protected:
        UberV2Material();
    };
    class MaterialAccessor
    {
    public:
        MaterialAccessor(Material::Ptr material);

        std::vector<std::string> GetTypeInfo() const;
        void SetType(std::uint32_t type);
        int GetType() const;

        ~MaterialAccessor() = default;

        MaterialAccessor(const MaterialAccessor&) = delete;
        MaterialAccessor& operator = (const MaterialAccessor&) = delete;
    private:
        Material::Ptr m_material;
    };

}
