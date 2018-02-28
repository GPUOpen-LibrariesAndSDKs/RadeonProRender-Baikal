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

#include "scene_object.h"

namespace Baikal
{
    class InputMap : public SceneObject
    {
    public:
        using Ptr = std::shared_ptr<InputMap>;
        enum class InputMapType
        {
            kConstantFloat4 = 0,
            kConstantFloat,
            kConstantInt,
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
        };
        InputMapType type;

        InputMap(InputMapType t) : SceneObject(), type(t) {}
    };
}
