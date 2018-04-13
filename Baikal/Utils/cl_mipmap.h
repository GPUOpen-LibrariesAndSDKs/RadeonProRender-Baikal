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

#ifndef __MIPMAP_BUILDER__
#define __MIPMAP_BUILDER__

#include <CLW.h>
#include "SceneGraph/clwscene.h"

namespace Baikal
{
    class Mipmap
    {
    public:
        Mipmap(CLWContext context);

        void Build(Collector& texture_collector, CLWBuffer<char> mipmap_info, CLWBuffer<char> texture_data);

        // computes size in bytes of the mipmap pyramid for one given image specs
        // inluding original image as base level
        static std::uint32_t ComputeMipPyramidSize(std::uint32_t width, std::uint32_t height, std::uint32_t pitch, int format);

    private:
        void BuildMipPyramid(const ClwScene::Texture& texture);

        void Downscale(
            std::uint32_t dst_offset, std::uint32_t dst_width,
            std::uint32_t dst_pitch, std::uint32_t dst_height,
            std::uint32_t src_offset, std::uint32_t src_width,
            std::uint32_t src_pitch, std::uint32_t src_height);

        void ComputeWeights(CLWBuffer<float> weights, int size, bool is_rounding_necessary);

        static int PixelBytes(int format);

        // OpenCL context
        CLWContext m_context;
        CLWProgram m_program;
        // buffer for temporary image scaled in x dimension
        std::uint32_t m_tmp_buffer_size;
        CLWBuffer<char> m_tmp_buffer;
        // buffers for weights
        std::uint32_t m_x_weights_num, m_y_weights_num;
        CLWBuffer<float> m_x_weights, m_y_weights;
        // buffer to store mipmap offsets
        CLWBuffer<char> m_mipmap_offsets;
        // clw buffers to sore texture data and mip levels info
        CLWBuffer<char> m_mipmap_info;
        CLWBuffer<char> m_texture_data
    };
}
#endif //__MIPMAP_BUILDER__