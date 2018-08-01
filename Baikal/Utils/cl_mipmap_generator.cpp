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

#include "cl_mipmap_generator.h"
#include "SceneGraph/texture.h"
#include <algorithm>

namespace Baikal
{
    CLMipmapGenerator::CLMipmapGenerator(CLWContext context, const CLProgramManager *program_manager)
        : ClwClass(context, program_manager, "../Baikal/Kernels/CL/mipmap_level_scaler.cl", "")
    {  }

    void CLMipmapGenerator::ComputeWeights(CLWBuffer<RadeonRays::float4> weights, bool is_rounding_necessary)
    {
        auto compute_weights_kernel = (is_rounding_necessary) ?
            GetKernel("ComputeWeights_RoundingUp") : GetKernel("ComputeWeights_NoRounding");

        int argc = 0;
        compute_weights_kernel.SetArg(argc++, weights);
        compute_weights_kernel.SetArg(argc++, (int)weights.GetElementCount());

        GetContext().Launch1D(0, ((weights.GetElementCount() + 63) / 64) * 64, 64, compute_weights_kernel).Wait();
    }


    CLWKernel CLMipmapGenerator::GetDownsampleXKernel(Texture::Format format)
    {
        CLWKernel kernel;
        switch (format)
        {
        case Texture::Format::kRgba8:
            kernel = GetKernel("ScaleX_uchar4");
            break;
        case Texture::Format::kRgba16:
            kernel = GetKernel("ScaleX_half4");
            break;
        case Texture::Format::kRgba32:
            kernel = GetKernel("ScaleX_float4");
            break;
        default:
            throw std::runtime_error("CLMipmapGenerator::GetScaleXKernel(...): unsupported format");
        }

        return kernel;
    }

    CLWKernel CLMipmapGenerator::GetDownsampleYKernel(Texture::Format format)
    {
        CLWKernel kernel;
        switch (format)
        {
        case Texture::Format::kRgba8:
            kernel = GetKernel("ScaleY_uchar4");
            break;
        case Texture::Format::kRgba16:
            kernel = GetKernel("ScaleY_half4");
            break;
        case Texture::Format::kRgba32:
            kernel = GetKernel("ScaleY_float4");
            break;
        default:
            throw std::runtime_error("CLMipmapGenerator::GetScaleYKernel(...): unsupported format");
        }

        return kernel;
    }

    void CLMipmapGenerator::Downsample(
        Texture::Ptr texture,
        std::size_t texture_index,
        std::size_t mip_level,
        CLWBuffer<ClwScene::Texture> textures,
        CLWBuffer<ClwScene::MipLevel> mip_levels,
        CLWBuffer<char> texturedata)
    {
        auto src_size = texture->GetSize(mip_level);
        auto dst_size = texture->GetSize(mip_level + 1);
        auto texel_size = Texture::GetPixelSize(texture->GetFormat());

        // Temp buffer that contains texture data resampled by x component
        std::size_t new_tmp_buffer_size = texel_size * dst_size.x * src_size.y;

        if (m_tmp_buffer.GetElementCount() < new_tmp_buffer_size)
        {
            m_tmp_buffer = GetContext().CreateBuffer<char>(new_tmp_buffer_size, CL_MEM_READ_WRITE);
        }

        // Downsample in x direction
        {
            bool is_rounding_necessary = (src_size.x % 2 != 0);

            if (static_cast<int>(m_x_weights.GetElementCount()) < dst_size.x)
            {
                m_x_weights = GetContext().CreateBuffer<float4>(dst_size.x, CL_MEM_READ_WRITE);
            }

            ComputeWeights(m_x_weights, is_rounding_necessary);

            {
                CLWKernel downsample_kernel = GetDownsampleXKernel(texture->GetFormat());

                int argc = 0;
                downsample_kernel.SetArg(argc++, (int)texture_index);
                downsample_kernel.SetArg(argc++, (int)mip_level);
                downsample_kernel.SetArg(argc++, m_x_weights);
                downsample_kernel.SetArg(argc++, textures);
                downsample_kernel.SetArg(argc++, mip_levels);
                downsample_kernel.SetArg(argc++, texturedata);
                downsample_kernel.SetArg(argc++, m_tmp_buffer);

                std::size_t work_size = dst_size.x * src_size.y;
                GetContext().Launch1D(0, ((work_size + 63) / 64) * 64, 64, downsample_kernel).Wait();
            }
        }

        // downscale in y direction
        {
            bool is_rounding_necessary = (src_size.y % 2 != 0);

            if (static_cast<int>(m_y_weights.GetElementCount()) < dst_size.y)
            {
                m_y_weights = GetContext().CreateBuffer<RadeonRays::float4>(dst_size.y, CL_MEM_READ_WRITE);
            }

            ComputeWeights(m_y_weights, is_rounding_necessary);

            {
                CLWKernel downsample_kernel = GetDownsampleYKernel(texture->GetFormat());

                int argc = 0;
                downsample_kernel.SetArg(argc++, (int)texture_index);
                downsample_kernel.SetArg(argc++, (int)mip_level);
                downsample_kernel.SetArg(argc++, m_y_weights);
                downsample_kernel.SetArg(argc++, textures);
                downsample_kernel.SetArg(argc++, mip_levels);
                downsample_kernel.SetArg(argc++, texturedata);
                downsample_kernel.SetArg(argc++, m_tmp_buffer);

                std::size_t work_size = dst_size.x * dst_size.y;
                GetContext().Launch1D(0, ((work_size + 63) / 64) * 64, 64, downsample_kernel).Wait();
            }
        }

        GetContext().Flush(0);
    }

    void CLMipmapGenerator::BuildMipPyramid(
        Texture::Ptr texture,
        std::size_t texture_index,
        CLWBuffer<ClwScene::Texture> textures,
        CLWBuffer<ClwScene::MipLevel> mip_levels,
        CLWBuffer<char> texturedata
    )
    {
        for (std::size_t i = 0; i < texture->GetMipLevelCount() - 1; i++)
        {
            // Downsample current level in x and y dimensions
            Downsample(texture, texture_index, i, textures, mip_levels, texturedata);
        }
    }

    namespace
    {
        struct CLMipmapGeneratorConcrete : public CLMipmapGenerator {
            CLMipmapGeneratorConcrete(CLWContext context, const CLProgramManager *program_manager) :
                CLMipmapGenerator(context, program_manager) {}
        };
    }

    CLMipmapGenerator::Ptr CLMipmapGenerator::Create(CLWContext context, const CLProgramManager *program_manager)
    {
        return std::make_shared<CLMipmapGeneratorConcrete>(context, program_manager);
    }
}

