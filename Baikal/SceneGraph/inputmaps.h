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


namespace Baikal
{
    class InputMap_ConstantFloat3 : public InputMap
    {
    public:
        static InputMap::Ptr Create(RadeonRays::float3 value)
        {
            return InputMap::Ptr(new InputMap_ConstantFloat3(value));
        }

        RadeonRays::float3 m_value;

    private:
        explicit InputMap_ConstantFloat3(RadeonRays::float3 v) : InputMap(InputMapType::kConstantFloat3), m_value(v) {}
    };

    class InputMap_ConstantFloat : public InputMap
    {
    public:
        static InputMap::Ptr Create(float value)
        {
            return InputMap::Ptr(new InputMap_ConstantFloat(value));
        }

        float m_value;

    private:
        explicit InputMap_ConstantFloat(float v) : InputMap(InputMapType::kConstantFloat), m_value(v) {}
    };

    template<InputMap::InputMapType type>
    class InputMap_TwoArg : public InputMap
    {
    public:
        static InputMap::Ptr Create(InputMap::Ptr a, InputMap::Ptr b)
        {
            return InputMap::Ptr(new InputMap_TwoArg(a, b));
        }

        InputMap::Ptr m_a;
        InputMap::Ptr m_b;

    private:
        InputMap_TwoArg(InputMap::Ptr a, InputMap::Ptr b) :
            InputMap(type),
            m_a(a),
            m_b(b)
        {
            assert(m_a && m_b);
        }
    };

    typedef InputMap_TwoArg<InputMap::InputMapType::kAdd> InputMap_Add;
    typedef InputMap_TwoArg<InputMap::InputMapType::kSub> InputMap_Sub;
    typedef InputMap_TwoArg<InputMap::InputMapType::kMul> InputMap_Mul;
    typedef InputMap_TwoArg<InputMap::InputMapType::kDiv> InputMap_Div;
/*

    kSampler,
    kAdd,
    kSub,
    kMul,
    kDiv,
    kSin,
    kCos,
    kTan,
    kSelect, //component as parameter
    kCombine,
    kDot3,
    kCross3,
    kLength3,
    kNormalize3,
    kPow,
    kAcos,
    kAsin,
    kAtan,
    kAverage, //Components as parameter
    kMin,
    kMax,
    kFloor,
    kMod,
    kAbs,
    kShuffle, //Step count as parameter (1 - WXYZ, 2- ZWXY, 3 - YZWX)
    kDot4
*/


}
