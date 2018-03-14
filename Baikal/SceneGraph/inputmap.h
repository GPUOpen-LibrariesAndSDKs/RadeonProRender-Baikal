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
            kConstantFloat3 = 0,
            kConstantFloat,
            kSampler,
            kAdd, // a + b
            kSub, // a - b
            kMul, // a * b
            kDiv, // a / b
            kSin, // sin(arg)
            kCos, // cos(arg)
            kTan, // tan(arg)
            kSelect, // Selects single component from input arg
            kDot3, // dot(a.xyz, b.xyz)
            kCross3, // cross(a.xyz, b.xyz)
            kLength3, // length(a.xyz)
            kNormalize3, // normalize(a.xyz)
            kPow, // pow(a, b)
            kAcos, // acos(arg)
            kAsin, // asin(arg)
            kAtan, // atan(arg)
            kLerp, // mix (a, b, control)
            kMin, // min(a, b)
            kMax, // max(a, b)
            kFloor, // floor(arg)
            kMod, // fmod(a, b)
            kAbs, // fabs(arg)
            kShuffle, // shuffle(arg, mask)
            kShuffle2, // shuffle2(a, b, mask)
            kDot4, // dot(a, b)
            kCross4, // cross(a, b)
            kMatMul, // arg * mat4
            kRemap, // remaps from source range (x->y) to destination range (x->y)
                    // calculated as mix(float3(dest.x), float3(dest.y), (val - src.x)/(src.y - src.x))
            kSamplerBumpmap // samples normal from bump map texture
        };
        InputMapType m_type;

        InputMap(InputMapType t) : SceneObject(), m_type(t) {}
        virtual void CollectTextures(std::set<Texture::Ptr> &textures) = 0;
        virtual void GetLeafs(std::set<Ptr> &leafs) {};
        virtual bool IsLeaf() const { return false;}
    };
}
