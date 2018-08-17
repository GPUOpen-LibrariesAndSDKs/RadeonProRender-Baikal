/**********************************************************************
Copyright (c) 2018 Advanced Micro Devices, Inc. All rights reserved.

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

#include <CLW.h>
#include "SceneGraph/clwscene.h"
#include "Utils/clw_class.h"
#include "Utils/cl_program_manager.h"

namespace Baikal
{
    class CLMipmapGenerator : protected ClwClass
    {
    public:
        using Ptr = std::shared_ptr<CLMipmapGenerator>;

        static Ptr Create(CLWContext context, const CLProgramManager *program_manager);

        // Generates images in mipmap levels
        // The function saves result in reserved areas of texturedata buffer
        void BuildMipPyramid(
            Texture::Ptr texture,
            std::size_t texture_index,
            CLWBuffer<ClwScene::Texture> textures,
            CLWBuffer<ClwScene::MipLevel> mip_levels,
            CLWBuffer<char> texturedata);

    protected:
        CLMipmapGenerator(CLWContext context, const CLProgramManager *program_manager);

    private:
        void Downsample(
            Texture::Ptr texture,
            std::size_t texture_index,
            std::size_t mip_level,
            CLWBuffer<ClwScene::Texture> textures,
            CLWBuffer<ClwScene::MipLevel> mip_levels,
            CLWBuffer<char> texturedata);

        void ComputeWeights(CLWBuffer<RadeonRays::float4> weights, bool is_rounding_necessary);

        CLWKernel GetDownsampleXKernel(Texture::Format format);
        CLWKernel GetDownsampleYKernel(Texture::Format format);

        // Buffer for temporary image scaled in x dimension
        CLWBuffer<char> m_tmp_buffer;
        // Weight buffers
        CLWBuffer<RadeonRays::float4> m_x_weights, m_y_weights;
    };
}
