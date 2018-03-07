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

#include "inputmap.h"

#include <assert.h>

#include "math/float3.h"
#include "math/matrix.h"


namespace Baikal
{
    class InputMap_ConstantFloat3 : public InputMap
    {
    public:
        using Ptr = std::shared_ptr<InputMap_ConstantFloat3>;
        static Ptr Create(RadeonRays::float3 value)
        {
            return Ptr(new InputMap_ConstantFloat3(value));
        }

        void CollectTextures(std::set<Texture::Ptr> &textures) override
        {
            return;
        }

        void SetValue(RadeonRays::float3 value)
        {
            m_value = value;
            SetDirty(true);
        }

        RadeonRays::float3 GetValue() const
        {
            return m_value;
        }

    private:
        RadeonRays::float3 m_value;

        explicit InputMap_ConstantFloat3(RadeonRays::float3 v) :
        InputMap(InputMapType::kConstantFloat3),
        m_value(v)
        {
            SetDirty(true);
        }
    };

    class InputMap_ConstantFloat : public InputMap
    {
    public:
        using Ptr = std::shared_ptr<InputMap_ConstantFloat>;
        static Ptr Create(float value)
        {
            return Ptr(new InputMap_ConstantFloat(value));
        }

        void CollectTextures(std::set<Texture::Ptr> &textures) override
        {
            return;
        }

        void SetValue(float value)
        {
            m_value = value;
            SetDirty(true);
        }

        float GetValue() const
        {
            return m_value;
        }
    private:
        float m_value;

        explicit InputMap_ConstantFloat(float v) :
        InputMap(InputMapType::kConstantFloat),
        m_value(v)
        {
            SetDirty(true);
        }
    };

    class InputMap_Sampler : public InputMap
    {
    public:
        using Ptr = std::shared_ptr<InputMap_Sampler>;
        static Ptr Create(Texture::Ptr texture)
        {
            return Ptr(new InputMap_Sampler(texture));
        }

        void CollectTextures(std::set<Texture::Ptr> &textures) override
        {
            textures.insert(m_texture);
            return;
        }

        void SetTexture(Texture::Ptr texture)
        {
            m_texture = texture;
            assert(m_texture);
            SetDirty(true);
        }

        Texture::Ptr GetTexture() const
        {
            return m_texture;
        }

    private:
        Texture::Ptr m_texture;

        explicit InputMap_Sampler(Texture::Ptr texture) :
        InputMap(InputMapType::kSampler),
        m_texture(texture)
        {
            assert(m_texture);
            SetDirty(true);
        }
    };


    template<InputMap::InputMapType type>
    class InputMap_TwoArg : public InputMap
    {
    public:
        using Ptr = std::shared_ptr<InputMap_TwoArg<type>>;
        static Ptr Create(InputMap::Ptr a, InputMap::Ptr b)
        {
            return Ptr(new InputMap_TwoArg(a, b));
        }

        void CollectTextures(std::set<Texture::Ptr> &textures) override
        {
            m_a->CollectTextures(textures);
            m_b->CollectTextures(textures);
            return;
        }

        void SetA(InputMap::Ptr a)
        {
            m_a = a;
            assert(m_a);
            SetDirty(true);
        }

        void SetB(InputMap::Ptr b)
        {
            m_b = b;
            assert(m_b);
            SetDirty(true);
        }

        InputMap::Ptr GetA() const
        {
            return m_a;
        }

        InputMap::Ptr GetB() const
        {
            return m_b;
        }

    private:
        InputMap::Ptr m_a;
        InputMap::Ptr m_b;

        InputMap_TwoArg(InputMap::Ptr a, InputMap::Ptr b) :
            InputMap(type),
            m_a(a),
            m_b(b)
        {
            assert(m_a && m_b);
            SetDirty(true);
        }
    };

    typedef InputMap_TwoArg<InputMap::InputMapType::kAdd> InputMap_Add;
    typedef InputMap_TwoArg<InputMap::InputMapType::kSub> InputMap_Sub;
    typedef InputMap_TwoArg<InputMap::InputMapType::kMul> InputMap_Mul;
    typedef InputMap_TwoArg<InputMap::InputMapType::kDiv> InputMap_Div;
    typedef InputMap_TwoArg<InputMap::InputMapType::kMin> InputMap_Min;
    typedef InputMap_TwoArg<InputMap::InputMapType::kMax> InputMap_Max;
    typedef InputMap_TwoArg<InputMap::InputMapType::kDot3> InputMap_Dot3;
    typedef InputMap_TwoArg<InputMap::InputMapType::kCross3> InputMap_Cross3;
    typedef InputMap_TwoArg<InputMap::InputMapType::kDot4> InputMap_Dot4;
    typedef InputMap_TwoArg<InputMap::InputMapType::kCross4> InputMap_Cross4;
    typedef InputMap_TwoArg<InputMap::InputMapType::kPow> InputMap_Pow;
    typedef InputMap_TwoArg<InputMap::InputMapType::kMod> InputMap_Mod;

    template<InputMap::InputMapType type>
    class InputMap_OneArg : public InputMap
    {
    public:
        using Ptr = std::shared_ptr<InputMap_OneArg<type>>;
        static Ptr Create(InputMap::Ptr arg)
        {
            return Ptr(new InputMap_OneArg(arg));
        }

        void CollectTextures(std::set<Texture::Ptr> &textures) override
        {
            m_arg->CollectTextures(textures);
            return;
        }

        void SetArg(InputMap::Ptr arg)
        {
            m_arg = arg;
            assert(m_arg);
            SetDirty(true);
        }

        InputMap::Ptr GetArg() const
        {
            return m_arg;
        }

    private:
        InputMap::Ptr m_arg;

        InputMap_OneArg(InputMap::Ptr arg) :
        InputMap(type),
        m_arg(arg)
        {
            assert(m_arg);
            SetDirty(true);
        }
    };

    typedef InputMap_OneArg<InputMap::InputMapType::kSin> InputMap_Sin;
    typedef InputMap_OneArg<InputMap::InputMapType::kCos> InputMap_Cos;
    typedef InputMap_OneArg<InputMap::InputMapType::kTan> InputMap_Tan;
    typedef InputMap_OneArg<InputMap::InputMapType::kAsin> InputMap_Asin;
    typedef InputMap_OneArg<InputMap::InputMapType::kAcos> InputMap_Acos;
    typedef InputMap_OneArg<InputMap::InputMapType::kAtan> InputMap_Atan;
    typedef InputMap_OneArg<InputMap::InputMapType::kLength3> InputMap_Length3;
    typedef InputMap_OneArg<InputMap::InputMapType::kNormalize3> InputMap_Normalize3;
    typedef InputMap_OneArg<InputMap::InputMapType::kFloor> InputMap_Floor;
    typedef InputMap_OneArg<InputMap::InputMapType::kAbs> InputMap_Abs;

    class InputMap_Lerp : public InputMap
    {
    public:
        using Ptr = std::shared_ptr<InputMap_Lerp>;
        static Ptr Create(InputMap::Ptr a, InputMap::Ptr b, InputMap::Ptr control)
        {
            return Ptr(new InputMap_Lerp(a, b, control));
        }

        void SetA(InputMap::Ptr a)
        {
            m_a = a;
            assert(m_a);
            SetDirty(true);
        }

        void SetB(InputMap::Ptr b)
        {
            m_b = b;
            assert(m_b);
            SetDirty(true);
        }

        InputMap::Ptr GetA() const
        {
            return m_a;
        }

        InputMap::Ptr GetB() const
        {
            return m_b;
        }

        void SetControl(InputMap::Ptr control)
        {
            m_control = control;
            assert(m_control);
            SetDirty(true);
        }

        InputMap::Ptr GetControl() const
        {
            return m_control;
        }

        void CollectTextures(std::set<Texture::Ptr> &textures) override
        {
            m_a->CollectTextures(textures);
            m_b->CollectTextures(textures);
            m_control->CollectTextures(textures);
            return;
        }


    private:
        InputMap::Ptr m_a;
        InputMap::Ptr m_b;
        InputMap::Ptr m_control;

        InputMap_Lerp(InputMap::Ptr a, InputMap::Ptr b, InputMap::Ptr control) :
        InputMap(InputMap::InputMapType::kLerp),
        m_a(a),
        m_b(b),
        m_control(control)
        {
            assert(m_a && m_b && m_control);
            SetDirty(true);
        }
    };

    class InputMap_Select : public InputMap
    {
    public:
        using Ptr = std::shared_ptr<InputMap_Select>;
        enum class Selection
        {
            kX = 0,
            kY = 1,
            kZ = 2,
            kW = 3
        };
        static Ptr Create(InputMap::Ptr arg, Selection selection)
        {
            return Ptr(new InputMap_Select(arg, selection));
        }

        void CollectTextures(std::set<Texture::Ptr> &textures) override
        {
            m_arg->CollectTextures(textures);
            return;
        }

        void SetArg(InputMap::Ptr arg)
        {
            m_arg = arg;
            assert(m_arg);
            SetDirty(true);
        }

        InputMap::Ptr GetArg() const
        {
            return m_arg;
        }

        void SetSelection(Selection selection)
        {
            m_selection = selection;
            SetDirty(true);
        }

        Selection GetSelection() const
        {
            return m_selection;
        }

    private:
        InputMap::Ptr m_arg;
        Selection m_selection;

        InputMap_Select(InputMap::Ptr arg, Selection selection) :
        InputMap(InputMap::InputMapType::kSelect),
        m_arg(arg),
        m_selection(selection)
        {
            assert(m_arg);
            SetDirty(true);
        }
    };

    class InputMap_Shuffle : public InputMap
    {
    public:
        using Ptr = std::shared_ptr<InputMap_Shuffle>;
        static Ptr Create(InputMap::Ptr arg, const std::array<uint32_t, 4> &mask)
        {
            return Ptr(new InputMap_Shuffle(arg, mask));
        }

        void CollectTextures(std::set<Texture::Ptr> &textures) override
        {
            m_arg->CollectTextures(textures);
            return;
        }

        void SetArg(InputMap::Ptr arg)
        {
            m_arg = arg;
            assert(m_arg);
            SetDirty(true);
        }

        InputMap::Ptr GetArg() const
        {
            return m_arg;
        }

        void SetMask(const std::array<uint32_t, 4> &mask)
        {
            m_mask = mask;
            SetDirty(true);
        }

        std::array<uint32_t, 4> GetMask() const
        {
            return m_mask;
        }

    private:
        InputMap::Ptr m_arg;
        std::array<uint32_t, 4> m_mask;

        InputMap_Shuffle(InputMap::Ptr arg, const std::array<uint32_t, 4> &mask) :
        InputMap(InputMap::InputMapType::kShuffle),
        m_arg(arg),
        m_mask(mask)
        {
            assert(m_arg);
            SetDirty(true);
        }
    };

    class InputMap_Shuffle2 : public InputMap
    {
    public:
        using Ptr = std::shared_ptr<InputMap_Shuffle2>;
        static Ptr Create(InputMap::Ptr a, InputMap::Ptr b, const std::array<uint32_t, 4> &mask)
        {
            return Ptr(new InputMap_Shuffle2(a, b, mask));
        }

        void CollectTextures(std::set<Texture::Ptr> &textures) override
        {
            m_a->CollectTextures(textures);
            m_b->CollectTextures(textures);
            return;
        }

        void SetA(InputMap::Ptr a)
        {
            m_a = a;
            assert(m_a);
            SetDirty(true);
        }

        InputMap::Ptr GetA() const
        {
            return m_a;
        }

        void SetB(InputMap::Ptr b)
        {
            m_b = b;
            assert(m_b);
            SetDirty(true);
        }

        InputMap::Ptr GetB() const
        {
            return m_b;
        }

        void SetMask(const std::array<uint32_t, 4> &mask)
        {
            m_mask = mask;
            SetDirty(true);
        }

        std::array<uint32_t, 4> GetMask() const
        {
            return m_mask;
        }

    private:
        InputMap::Ptr m_a;
        InputMap::Ptr m_b;
        std::array<uint32_t, 4> m_mask;

        InputMap_Shuffle2(InputMap::Ptr a, InputMap::Ptr b, const std::array<uint32_t, 4> &mask) :
        InputMap(InputMap::InputMapType::kShuffle2),
        m_a(a),
        m_b(b),
        m_mask(mask)
        {
            assert(m_a && m_b);
            SetDirty(true);
        }
    };

    class InputMap_MatMul : public InputMap
    {
    public:
        using Ptr = std::shared_ptr<InputMap_MatMul>;
        static Ptr Create(InputMap::Ptr arg, const RadeonRays::matrix &mat4)
        {
            return Ptr(new InputMap_MatMul(arg, mat4));
        }

        void CollectTextures(std::set<Texture::Ptr> &textures) override
        {
            m_arg->CollectTextures(textures);
            return;
        }

        void SetArg(InputMap::Ptr arg)
        {
            m_arg = arg;
            assert(m_arg);
            SetDirty(true);
        }

        InputMap::Ptr GetArg() const
        {
            return m_arg;
        }

        void SetMatrix(const RadeonRays::matrix &mat4)
        {
            m_mat4 = mat4;
            SetDirty(true);
        }

        RadeonRays::matrix GetMatrix() const
        {
            return m_mat4;
        }

    private:
        InputMap::Ptr m_arg;
        RadeonRays::matrix m_mat4;

        InputMap_MatMul(InputMap::Ptr arg, const RadeonRays::matrix &mat4) :
        InputMap(InputMap::InputMapType::kMatMul),
        m_arg(arg),
        m_mat4(mat4)
        {
            assert(m_arg);
            SetDirty(true);
        }
    };
}
