/**********************************************************************
 Copyright (c) 2019 Advanced Micro Devices, Inc. All rights reserved.

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


#include "PostEffects/ML/denoiser_preprocessor.h"
#include "PostEffects/ML/error_handler.h"

#include "CLWBuffer.h"
#include "Output/clwoutput.h"
#include "math/mathutils.h"

#include "RadeonProML.h"

namespace Baikal
{
    namespace PostEffects
    {
        using float3 =  RadeonRays::float3;
        using OutputType = Renderer::OutputType;

        DenoiserPreprocessor::DenoiserPreprocessor(CLWContext context,
                                               CLProgramManager const* program_manager,
                                               std::uint32_t start_spp)
        : DataPreprocessor(context, program_manager, start_spp)
        , m_primitives(context)
        , m_model(Model::kColorAlbedoDepthNormal9)
        , m_context(mlCreateContext(), mlReleaseContext)
        {
            switch (m_model)
            {
                case Model::kColorDepthNormalGloss7:
                    m_layout.emplace_back(OutputType::kColor, 3);
                    m_layout.emplace_back(OutputType::kDepth, 1);
                    m_layout.emplace_back(OutputType::kViewShadingNormal, 2);
                    m_layout.emplace_back(OutputType::kGloss, 1);
                    break;
                case Model::kColorAlbedoNormal8:
                    m_layout.emplace_back(OutputType::kColor, 3 );
                    m_layout.emplace_back(OutputType::kAlbedo, 3);
                    m_layout.emplace_back(OutputType::kViewShadingNormal, 2);
                    break;
                case Model::kColorAlbedoDepthNormal9:
                    m_layout.emplace_back(OutputType::kColor, 3 );
                    m_layout.emplace_back(OutputType::kAlbedo, 3);
                    m_layout.emplace_back(OutputType::kDepth, 1);
                    m_layout.emplace_back(OutputType::kViewShadingNormal, 2);
                    break;
            }

            for (const auto& layer: m_layout)
            {
                m_channels += layer.second;
            }
        }

        void DenoiserPreprocessor::Init(std::uint32_t width, std::uint32_t height)
        {
            auto context = GetContext();

            m_cache = CLWBuffer<float>::Create(context,
                                               CL_MEM_READ_WRITE,
                                               4 * width * height);

            m_input = CLWBuffer<float>::Create(context,
                                               CL_MEM_READ_WRITE,
                                               m_channels * width * height);

            ml_image_info image_info = {ML_FLOAT32, m_width, m_height, m_channels};
            m_image = mlCreateImage(m_context.get(), &image_info);


            if (!m_image)
            {
                ContextError(m_context.get());
            }

            m_is_initialized = true;
        }

        Image DenoiserPreprocessor::MakeInput(PostEffect::InputSet const& inputs)
        {
            auto context = GetContext();
            unsigned channels_count = 0u;
            unsigned real_sample_count = 0u;

            if (!m_is_initialized)
            {
                auto color = inputs.at(Renderer::OutputType::kColor);
                m_width = color->width();
                m_height = color->height();
                Init(m_width, m_height);
                m_is_initialized = true;
            }

            for (const auto& desc : m_layout)
            {
                auto type = desc.first;
                auto input = inputs.at(type);

                auto clw_output = dynamic_cast<ClwOutput*>(input);
                auto device_mem = clw_output->data();

                switch (type)
                {
                    case OutputType::kColor:
                        DivideBySampleCount(CLWBuffer<float3>::CreateFromClBuffer(m_cache),
                                            device_mem);

                        WriteToInputs(m_input,
                                      m_cache,
                                      m_width,
                                      m_height,
                                      channels_count /* dst channels offset */,
                                      m_channels     /* dst channels num */,
                                      0              /* src channels offset */,
                                      4              /* src channels num */,
                                      3              /* channels to copy */);

                        channels_count += 3;

                        real_sample_count = ReadSpp(CLWBuffer<float3>::CreateFromClBuffer(m_cache));

                        if (real_sample_count < m_start_spp)
                        {
                            return Image(real_sample_count, nullptr);
                        }
                        break;

                    case OutputType::kDepth:
                        m_primitives.Normalize(0,
                                               CLWBuffer<cl_float3>::CreateFromClBuffer(device_mem),
                                               CLWBuffer<cl_float3>::CreateFromClBuffer(m_cache),
                                               (int)device_mem.GetElementCount() / sizeof(cl_float3));

                        WriteToInputs(m_input,
                                      m_cache,
                                      m_width,
                                      m_height,
                                      channels_count, // dst channels offset
                                      m_channels,     // dst channels num
                                      0,              // src channels offset
                                      4,              // src channels num
                                      1).Wait();      // channels to copy

                        channels_count += 1;
                        break;

                    case OutputType::kViewShadingNormal:
                    case OutputType::kGloss:
                    case OutputType::kAlbedo:
                        DivideBySampleCount(CLWBuffer<float3>::CreateFromClBuffer(m_cache),
                                            CLWBuffer<float3>::CreateFromClBuffer(device_mem));

                        WriteToInputs(m_input,
                                      m_cache,
                                      m_width,
                                      m_height,
                                      channels_count,       // dst channels offset
                                      m_channels,           // dst channels num
                                      0,                    // src channels offset
                                      4,                    // src channels num
                                      desc.second).Wait();  // channels to copy

                        channels_count += desc.second;

                    default:
                        break;
                }
            }

            size_t image_size = 0;
            auto host_buffer = mlMapImage(m_image, &image_size);

            if (!host_buffer || image_size == 0)
            {
                throw std::runtime_error("map operation failed");
            }

            context.ReadBuffer(0,
                    m_input,
                    static_cast<float*>(host_buffer),
                    m_input.GetElementCount()).Wait();

            if (mlUnmapImage(m_image, host_buffer) != ML_OK)
            {
                throw std::runtime_error("unmap operation failed");
            }

            return Image(static_cast<std::uint32_t>(real_sample_count), m_image);
        }

        std::tuple<std::uint32_t, std::uint32_t> DenoiserPreprocessor::ChannelsNum() const
        {
            return {m_channels, 3};
        }

        std::set<Renderer::OutputType> DenoiserPreprocessor::GetInputTypes() const
        {
            std::set<Renderer::OutputType> out_set;

            for (const auto& type: m_layout)
            {
                out_set.insert(type.first);
            }

            return out_set;
        }

        void DenoiserPreprocessor::DivideBySampleCount(CLWBuffer<float3> const& dst,
                                                     CLWBuffer<float3> const& src)
        {
            assert(dst.GetElementCount() >= src.GetElementCount());

            auto division_kernel = GetKernel("DivideBySampleCount");

            // Set kernel parameters
            unsigned argc = 0;
            division_kernel.SetArg(argc++, dst);
            division_kernel.SetArg(argc++, src);
            division_kernel.SetArg(argc++, (int)src.GetElementCount());

            // run DivideBySampleCount kernel
            auto thread_num = ((src.GetElementCount() + 63) / 64) * 64;
            GetContext().Launch1D(0,
                                  thread_num,
                                  64,
                                  division_kernel);
        }
    }
}
