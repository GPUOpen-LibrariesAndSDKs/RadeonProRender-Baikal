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
#include "math/float3.h"

namespace Baikal
{
    class InputMap_ConstantFloat4 : public InputMap
    {
    public:
        static InputMap::Ptr Create(RadeonRays::float4 value)
        {
            return InputMap::Ptr(new InputMap_ConstantFloat4(value));
        }

        InputMap_ConstantFloat4(RadeonRays::float4 v) : InputMap(InputMapType::kConstantFloat4), value(v) {}

        RadeonRays::float4 value;
    };

    class InputMap_ConstantFloat : public InputMap
    {
    public:
        static InputMap::Ptr Create(float value)
        {
            return InputMap::Ptr(new InputMap_ConstantFloat(value));
        }

        InputMap_ConstantFloat(float v) : InputMap(InputMapType::kConstantFloat), value(v) {}

        float value;
    };


}
