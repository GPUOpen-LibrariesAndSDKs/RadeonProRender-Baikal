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

#include "post_effect.h"

namespace Baikal
{

    PostEffect::Param::Param(std::uint32_t value)
                      : m_type(ParamType::kUint),
                        m_uint_value(value)
    {   }

    PostEffect::Param::Param(float value)
                      : m_type(ParamType::kFloat),
                        m_float_value(value)
    {   }

    PostEffect::Param::Param(RadeonRays::float2 const& value)
                      : m_type(ParamType::kFloat2),
                        m_float2_value(value)
    {   }

    PostEffect::Param::Param(RadeonRays::float4 const& value)
                      : m_type(ParamType::kFloat4),
                        m_float4_value(value)
    {   }

    PostEffect::Param::Param(std::string const& value)
                      : m_type(ParamType::kString),
                        m_str_value(value)
    {   }

    PostEffect::ParamType PostEffect::Param::GetType() const
    {
        return m_type;
    }

    void PostEffect::Param::AssertType(ParamType type) const
    {
        if (m_type != type)
        {
            throw std::runtime_error("Attempt to get incorrect param type value");
        }
    }

    float PostEffect::Param::GetFloat() const
    {
        AssertType(ParamType::kFloat);
        return m_float_value;
    }

    std::uint32_t PostEffect::Param::GetUint() const
    {
        AssertType(ParamType::kUint);
        return m_uint_value;
    }

    const RadeonRays::float4& PostEffect::Param::GetFloat4() const
    {
        AssertType(ParamType::kFloat4);
        return m_float4_value;
    }

    const RadeonRays::float2& PostEffect::Param::GetFloat2() const
    {
        AssertType(ParamType::kFloat2);
        return m_float2_value;
    }

    const std::string& PostEffect::Param::GetString() const
    {
        AssertType(ParamType::kString);
        return m_str_value;
    }

    const PostEffect::Param& PostEffect::GetParameter(std::string const& name) const
    {
        auto iter = m_parameters.find(name);

        if (iter == m_parameters.cend())
        {
            throw std::runtime_error("PostEffect: no such parameter " + name);
        }

        return iter->second;
    }

    void PostEffect::SetParameter(std::string const& name, Param value)
    {
        auto iter = m_parameters.find(name);

        if (iter == m_parameters.cend())
        {
            throw std::runtime_error("PostEffect: no such parameter " + name);
        }

        if (value.GetType() != iter->second.GetType())
        {
            throw std::runtime_error("PostEffect: attemp to change type of registred parameter " + name);
        }

        iter->second = std::move(value);
    }

    void PostEffect::RegisterParameter(std::string const &name, Param initial_value)
    {
        if (m_parameters.find(name) != m_parameters.cend())
        {
            throw std::runtime_error("Attempt to register already existing name");
        }

        m_parameters.emplace(name, initial_value);
    }
}
