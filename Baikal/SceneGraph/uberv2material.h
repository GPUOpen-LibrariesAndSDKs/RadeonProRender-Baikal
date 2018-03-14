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

#include "material.h"

namespace Baikal
{
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
            kSSSLayer = 0x40,
            kShadingNormalLayer = 0x80
        };

        using Ptr = std::shared_ptr<UberV2Material>;
        static Ptr Create();

        // Check if material has emissive components
        bool HasEmission() const override;

        void LinkRefractionIOR(bool link_to_reflection)
        {
            is_link_to_reflection_ = link_to_reflection;
        }
        bool IsLinkRefractionIOR() const
        {
            return is_link_to_reflection_;
        }

        void SetDoubleSided(bool is_double_sided)
        {
            is_double_sided_ = is_double_sided;
        }
        bool isDoubleSided() const
        {
            return is_double_sided_;
        }

        void SetMultiscatter(bool is_multiscatter)
        {
            is_multiscatter_ = is_multiscatter;
        }
        bool IsMultiscatter() const
        {
            return is_multiscatter_;
        }

        void SetLayers(uint32_t layers)
        {
            layers_ = layers;
        }
        uint32_t GetLayers() const
        {
            return layers_;
        }

    protected:
        UberV2Material();
        bool is_link_to_reflection_;
        bool is_double_sided_;
        bool is_multiscatter_;
        uint32_t layers_;

    };

}
