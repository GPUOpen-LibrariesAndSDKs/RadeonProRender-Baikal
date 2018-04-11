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

#include <algorithm>
#include "cl_mipmap.h"

namespace Baikal
{
    Mipmap::Mipmap(CLWContext context) :
        m_context(context)
    {
        m_program = CLWProgram::CreateFromFile("LevelScaler.cl", nullptr, m_context);
    }

    void Mipmap::ComputeWeights(CLWBuffer<float> weights, int size, bool is_rounding_necessary)
    {
        auto compute_weights_kernel = (is_rounding_necessary) ?
            (m_program.GetKernel("ComputeWeights_RoundingUp")) : (m_program.GetKernel("ComputeWeights_NoRounding"));

        int argc = 0;
        compute_weights_kernel.SetArg(argc++, weights);
        compute_weights_kernel.SetArg(argc++, size);

        int thread_num = (int)(std::ceilf(size / 64.f) * 64.f);
        m_context.Launch1D(0, thread_num, 64, compute_weights_kernel).Wait();
    }

    void Mipmap::Downscale(
        std::uint32_t dst_offset, std::uint32_t dst_width,
        std::uint32_t dst_pitch, std::uint32_t dst_height,
        std::uint32_t src_offset, std::uint32_t src_width,
        std::uint32_t src_pitch, std::uint32_t src_height)
    {

        m_tmp_buffer_size = dst_pitch * src_height;
        if (m_tmp_buffer.GetElementCount() < m_tmp_buffer_size)
        {
            m_tmp_buffer = m_context.CreateBuffer<char>(m_tmp_buffer_size, CL_MEM_READ_WRITE);
        }

        // downscale in x direction
        if (src_width != 1)
        {
            bool is_rounding_necessary = (src_width % 2 != 0);

            m_x_weights_num = 3 * dst_width;
            if (m_x_weights.GetElementCount() < m_x_weights_num)
            {
                m_x_weights = m_context.CreateBuffer<float>(m_x_weights_num, CL_MEM_READ_ONLY);
            }

            ComputeWeights(m_x_weights, m_x_weights_num, is_rounding_necessary);

            {
                int argc = 0;

                auto scale_x = m_program.GetKernel("ScaleX_4C");
                scale_x.SetArg(argc++, m_tmp_buffer);
                scale_x.SetArg(argc++, m_x_weights);
                scale_x.SetArg(argc++, 0);
                scale_x.SetArg(argc++, dst_width);
                scale_x.SetArg(argc++, src_height);
                scale_x.SetArg(argc++, dst_pitch);
                scale_x.SetArg(argc++, m_texture_data);
                scale_x.SetArg(argc++, src_offset);
                scale_x.SetArg(argc++, src_width);
                scale_x.SetArg(argc++, src_height);
                scale_x.SetArg(argc++, src_pitch);

                int thread_num = (int)(std::ceilf(m_tmp_buffer_size / 64.f) * 64.f);
                m_context.Launch1D(0, thread_num, 64, scale_x).Wait();
            }
        }
        else
        {
            m_context.CopyBuffer<char>(
                0,
                m_texture_data,
                m_tmp_buffer,
                src_offset,
                0,
                src_pitch * src_height);
        }

        // downscale in y direction
        if (src_height != 1)
        {
            bool is_rounding_necessary = (src_height % 2 != 0);

            m_y_weights_num = 3 * dst_height;
            if (m_y_weights.GetElementCount() < m_y_weights_num)
            {
                m_y_weights = m_context.CreateBuffer<float>(m_y_weights_num, CL_MEM_READ_ONLY);
            }

            ComputeWeights(m_y_weights, m_y_weights_num, is_rounding_necessary);

            {
                int argc = 0;

                auto scale_y = m_program.GetKernel("ScaleY_4C");
                scale_y.SetArg(argc++, m_texture_data);
                scale_y.SetArg(argc++, m_y_weights);
                scale_y.SetArg(argc++, dst_offset);
                scale_y.SetArg(argc++, dst_width);
                scale_y.SetArg(argc++, dst_height);
                scale_y.SetArg(argc++, dst_pitch);
                scale_y.SetArg(argc++, m_tmp_buffer);
                scale_y.SetArg(argc++, 0);
                scale_y.SetArg(argc++, dst_width);
                scale_y.SetArg(argc++, src_height);
                scale_y.SetArg(argc++, dst_pitch);

                int thread_num = (int)(std::ceilf(dst_pitch * dst_height / 64.f) * 64.f);
                m_context.Launch1D(0, thread_num, 64, scale_y).Wait();
            }
        }
    }

    void Mipmap::BuildMipPyramid(
        const ClwScene::Texture& texture_info,
        std::uint32_t img_width,
        std::uint32_t img_height,
        std::uint32_t img_pitch)
    {
        int level_num = (int)std::floor(std::log(std::max(img_width, img_height))) + 1;
        if (level_num > MAX_LEVEL_NUM)
        {
            throw std::exception(
                "Mipmap::BuildMipPyramid(...): too big resolution for mipmapping");
        }

        // initialize mip
        m_cpu_level_offsets.offsets[0] = texture_info.dataoffset;
        for (auto i = 0u; i < MAX_LEVEL_NUM; i++)
        {
            m_cpu_level_offsets.offsets[i] = -1;
        }

        int level_counter = 0;
        int level_data_offset = m_cpu_level_offsets.offsets[0];

        for (int i = 1u; i < level_num; i++)
        {
            auto dst_width = (std::uint32_t)ceilf(img_width / 2.f);
            auto dst_height = (std::uint32_t)ceilf(img_height / 2.f);

            auto dst_pitch = (std::uint32_t)PixelBytes(texture_info) * dst_width;

            Downscale(
                level_data_offset + img_pitch * img_height, dst_width,
                dst_pitch, dst_height,
                level_data_offset, img_width,
                img_pitch, img_height);

            level_data_offset += img_pitch * img_height;
            m_cpu_level_offsets.offsets[i] = level_data_offset;
            level_counter++;
        }

        m_context.WriteBuffer<char>(
            0,
            m_mipmap_info,
            (char*)&m_cpu_level_offsets,
            sizeof(ClwScene::MipmapOffsets) * texture_info.mipmap_index,
            sizeof(ClwScene::MipmapOffsets));
    }

    void Mipmap::Build(CLWBuffer<char> texture_info, CLWBuffer<char> mipmap_info, CLWBuffer<char> texture_data)
    {
        // reset buffers
        m_texture_info = texture_info;
        m_mipmap_info = mipmap_info;
        m_texture_data = texture_data;

        m_cpu_texture_info.resize(m_texture_data.GetElementCount());

        m_context.WriteBuffer<char>(
            0,
            m_texture_info,
            (char*)m_cpu_texture_info.data(),
            sizeof(Texture) * m_texture_data.GetElementCount());

        for (const auto& texture : m_cpu_texture_info)
        {
            if (texture.mipmap_enabled)
            {
                BuildMipPyramid(texture, texture.w, texture.h, PixelBytes(texture) * texture.w);
            }
        }
    }

    int Mipmap::PixelBytes(const ClwScene::Texture& texture) const
    {
        int pixel_in_bytes = 0;
        if (texture.fmt == ClwScene::RGBA32)
        {
            pixel_in_bytes = 16;
        }
        else if (texture.fmt == ClwScene::RGBA16)
        {
            pixel_in_bytes = 8;
        }
        else // RGBA8
        {
            pixel_in_bytes = 4;
        }

        return pixel_in_bytes;
    }
}