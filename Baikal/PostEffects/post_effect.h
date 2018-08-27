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
#pragma once

#include "Renderers/renderer.h"
#include "Output/output.h"

#include <map>
#include <set>
#include <string>
#include <stdexcept>
#include <cassert>


namespace Baikal
{
    class Camera;

    /**
    \brief Interface for post-processing effects.

    \details Post processing effects are operating on 2D renderer outputs and 
    do not know anything on rendreing methonds or renderer internal implementation.
    However not all Outputs are compatible with a given post-processing effect, 
    it is done to optimize the implementation and forbid running say OpenCL 
    post-processing effects on Vulkan output. Post processing effects may have 
    scalar parameters and may rely on the presense of certain content type (like 
    normals or diffuse albedo) in a given input set.
    */
    class PostEffect
    {
    public:

        enum class ParamType
        {
            kFloat = 0,
            kUint,
            kFloat2,
            kFloat4,
            kString,
        };

        class Param
        {
        public:
            ParamType GetType() const;

            float GetFloat() const;
            std::uint32_t GetUint() const;
            const RadeonRays::float2& GetFloat2() const;
            const RadeonRays::float4& GetFloat4() const;
            const std::string& GetString() const;

            Param(float value);
            Param(std::uint32_t value);
            Param(RadeonRays::float2 const& value);
            Param(RadeonRays::float4 const& value);
            Param(std::string const& value);

            operator float() const { return GetFloat(); }
            operator std::uint32_t() const { return GetUint(); }
            operator const RadeonRays::float2&() const { return GetFloat2(); }
            operator const RadeonRays::float4&() const { return GetFloat4(); }
            operator const std::string&() const { return GetString(); }

        private:
            void AssertType(ParamType type) const;

            ParamType m_type;
            union {
                std::uint32_t m_uint_value;
                float m_float_value;
                RadeonRays::float2 m_float2_value;
                RadeonRays::float4 m_float4_value;
            };
            std::string m_str_value;
        };

        // Data type to pass all necessary content into the post effect.
        using InputSet = std::map<Renderer::OutputType, Output*>;

        // Specification of the input set types
        using InputTypes = std::set<Renderer::OutputType>;

        // Default constructor & destructor
        PostEffect() = default;
        virtual ~PostEffect() = default;

        virtual InputTypes GetInputTypes() const = 0;

        // Apply post effect and use output for the result
        virtual void Apply(InputSet const& input_set, Output& output) = 0;

        virtual void SetParameter(std::string const& name, Param value);

        const Param& GetParameter(std::string const& name) const;

    protected:

        void RegisterParameter(std::string const& name, Param init_value);

    private:

        // Parameter map
        std::map<std::string, Param> m_parameters;
    };
}
