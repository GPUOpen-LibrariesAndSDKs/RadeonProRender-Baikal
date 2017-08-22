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
#include <map>
#include <string>
#include <memory>

#include "math/float3.h"

#include "scene_object.h"
#include "texture.h"

namespace Baikal
{
    class Iterator;
    class Material;
    class SingleBxdf;
    class MultiBxdf;
    class DisneyBxdf;
    using MaterialPtr = std::shared_ptr<Material>;
    using MaterialCPtr = std::shared_ptr<Material const>;
    using SingleBxdfPtr = std::shared_ptr<SingleBxdf>;
    using MultiBxdfPtr = std::shared_ptr<MultiBxdf>;
    using DisneyBxdfPtr = std::shared_ptr<DisneyBxdf>;

    /**
     \brief High level material interface
     
     \details Base class for all CPU side material supported by the renderer.
     */
    class Material : public SceneObject
    {
    public:
        // Material input type
        enum class InputType
        {
            kFloat4 = 0,
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
        class InputValue
        {
        public:
            // Current type
            InputType type;
            
            // Possible values (use based on type)
            union
            {
                RadeonRays::float4 float_value;
                TextureCPtr tex_value;
                MaterialCPtr mat_value;
            };

            InputValue(const RadeonRays::float4& val) : type(InputType::kFloat4), float_value(val) {}
            InputValue(const TextureCPtr& val) : type(InputType::kTexture), tex_value(val) {}
            InputValue(const MaterialCPtr& val) : type(InputType::kMaterial), mat_value(val) {}
            InputValue() : type(InputType::kFloat4) , float_value(RadeonRays::float4()) {}
            ~InputValue();
            InputValue(const InputValue& v);
            InputValue& operator=(const InputValue& v);
        private:
            void Clean();
        };

        // Full input state
        struct Input
        {
            InputInfo info;
            InputValue value;
        };

        // Constructor
        Material();
        // Destructor
        virtual ~Material() = 0;

        // Iterator of dependent materials (plugged as inputs)
        virtual std::unique_ptr<Iterator> CreateMaterialIterator() const;
        // Iterator of textures (plugged as inputs)
        virtual std::unique_ptr<Iterator> CreateTextureIterator() const;
        // Iterator of inputs
        virtual std::unique_ptr<Iterator> CreateInputIterator() const;
        // Check if material has emissive components
        virtual bool HasEmission() const;

        // Set input value
        // If specific data type is not supported throws std::runtime_error
        void SetInputValue(std::string const& name,
                           RadeonRays::float4 const& value);
        void SetInputValue(std::string const& name, TextureCPtr texture);
        void SetInputValue(std::string const& name, MaterialCPtr material);

        InputValue GetInputValue(std::string const& name) const;

        // Check if material is thin (normal is always pointing in ray incidence
        // direction)
        bool IsThin() const;
        // Set thin flag
        void SetThin(bool thin);
        
        Material(Material const&) = delete;
        Material& operator = (Material const&) = delete;

    protected:
        // Register specific input
        void RegisterInput(std::string const& name, std::string const& desc,
                           std::set<InputType>&& supported_types);
        // Wipe out all the inputs
        void ClearInputs();

    private:
        class InputIterator;

        using InputMap = std::map<std::string, Input>;
        // Input map
        InputMap m_inputs;
        // Thin material
        bool m_thin;;
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
            kZero,
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

        SingleBxdf(BxdfType type);

        BxdfType GetBxdfType() const;
        void SetBxdfType(BxdfType type);

        // Check if material has emissive components
        bool HasEmission() const override;

    private:
        BxdfType m_type;
    };
    
    class MultiBxdf : public Material
    {
    public:
        enum class Type
        {
            kLayered,
            kFresnelBlend,
            kMix
        };

        MultiBxdf(Type type);

        Type GetType() const;
        void SetType(Type type);

        // Check if material has emissive components
        bool HasEmission() const override;
    
    private:
        Type m_type;
    };
    
    class DisneyBxdf : public Material
    {
    public:
        DisneyBxdf();
        
        // Check if material has emissive components
        bool HasEmission() const override;
    };
}
