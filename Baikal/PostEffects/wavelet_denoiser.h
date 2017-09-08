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
#include "clw_post_effect.h"

namespace Baikal
{
    /**
    \brief Simple wavelet denoiser.

    \details WaveletDenoiser implements edge-avoiding A-Trous filter, taking into
    account color, position and normal information.
    Filter performs multiple passes, inserting pow(2, pass_index - 1) holes in 
    kernel on each pass.
    Parameters:
    * color_sensitivity - Higher the sensitivity the more it smoothes out depending on color difference.
    * normal_sensitivity - Higher the sensitivity the more it smoothes out depending on normal difference.
    * position_sensitivity - Higher the sensitivity the more it smoothes out depending on position difference.
    Required AOVs in input set:
    * kColor
    * kWorldShadingNormal
    * kWorldPosition
    */
    class WaveletDenoiser : public ClwPostEffect
    {
    public:
        // Constructor
        WaveletDenoiser(CLWContext context);
        virtual ~WaveletDenoiser();
        // Apply filter
        void Apply(InputSet const& input_set, Output& output) override;

    private: 
        // Find required output
        ClwOutput* FindOutput(InputSet const& input_set, Renderer::OutputType type);

        CLWProgram  m_program;

        // Ping-pong buffers for wavelet pass
        const static uint32_t m_num_tmp_buffers = 2;

        ClwOutput*  m_tmp_buffers[m_num_tmp_buffers];
        uint32_t    m_tmp_buffer_width;
        uint32_t    m_tmp_buffer_height;

        // Number of wavelet passes
        uint32_t    m_max_wavelet_passes;
    };

    inline WaveletDenoiser::WaveletDenoiser(CLWContext context)
        : ClwPostEffect(context, "../Baikal/Kernels/CL/wavelet_denoise.cl")
        , m_max_wavelet_passes(5)
    {
        // Add necessary params
        RegisterParameter("color_sensitivity", RadeonRays::float4(0.07f, 0.f, 0.f, 0.f));
        RegisterParameter("position_sensitivity", RadeonRays::float4(0.03f, 0.f, 0.f, 0.f));
        RegisterParameter("normal_sensitivity", RadeonRays::float4(0.01f, 0.f, 0.f, 0.f));

        m_tmp_buffers[0] = nullptr;
        m_tmp_buffers[1] = nullptr;
    }

    inline WaveletDenoiser::~WaveletDenoiser()
    {
        for (uint32_t i = 0; i < m_num_tmp_buffers; i++)
        {
            delete m_tmp_buffers[i];
        }
    }

    inline ClwOutput* WaveletDenoiser::FindOutput(InputSet const& input_set, Renderer::OutputType type)
    {
        auto iter = input_set.find(type);

        if (iter == input_set.cend())
        {
            return nullptr;
        }

        return static_cast<ClwOutput*>(iter->second);
    }

    inline void WaveletDenoiser::Apply(InputSet const& input_set, Output& output)
    {
        auto sigma_color = GetParameter("color_sensitivity").x;
        auto sigma_position = GetParameter("position_sensitivity").x;
        auto sigma_normal = GetParameter("normal_sensitivity").x;

        auto color = FindOutput(input_set, Renderer::OutputType::kColor);
        auto normal = FindOutput(input_set, Renderer::OutputType::kWorldShadingNormal);
        auto position = FindOutput(input_set, Renderer::OutputType::kWorldPosition);
        auto out_color = static_cast<ClwOutput*>(&output);

        auto color_width = color->width();
        auto color_height = color->height();
        
        // Create ping pong buffers if they still need to be initialized
        if (m_tmp_buffers[0] == nullptr || m_tmp_buffers[1] == nullptr)
        {
            m_tmp_buffers[0] = new ClwOutput(GetContext(), color_width, color_height);
            m_tmp_buffers[1] = new ClwOutput(GetContext(), color_width, color_height);
        }
        
        // Resize if size of AOV has changed
        if (color_width != m_tmp_buffers[0]->width() || color_height != m_tmp_buffers[0]->height())
        {
            for (uint32_t buffer_index = 0; buffer_index < m_num_tmp_buffers; buffer_index++)
            {
                delete m_tmp_buffers[buffer_index];
                m_tmp_buffers[buffer_index] = new ClwOutput(GetContext(), color_width, color_height);
            }
        }

        auto filter_kernel = GetKernel("WaveletFilter_main");

        for (uint32_t pass_index = 0; pass_index < m_max_wavelet_passes; pass_index++)
        {          
            Baikal::ClwOutput* current_input = pass_index == 0 ? color : m_tmp_buffers[pass_index % m_num_tmp_buffers];
            Baikal::ClwOutput* current_output = pass_index < m_max_wavelet_passes - 1 ? m_tmp_buffers[(pass_index + 1) % m_num_tmp_buffers] : out_color;

            const int step_width = 1 << pass_index;

            int argc = 0;

            // Set kernel parameters
            filter_kernel.SetArg(argc++, current_input->data());
            filter_kernel.SetArg(argc++, normal->data());
            filter_kernel.SetArg(argc++, position->data());
            filter_kernel.SetArg(argc++, pass_index);
            filter_kernel.SetArg(argc++, m_max_wavelet_passes);
            filter_kernel.SetArg(argc++, color->width());
            filter_kernel.SetArg(argc++, color->height());
            filter_kernel.SetArg(argc++, step_width);
            filter_kernel.SetArg(argc++, sigma_color);
            filter_kernel.SetArg(argc++, sigma_normal);
            filter_kernel.SetArg(argc++, sigma_position);
            filter_kernel.SetArg(argc++, current_output->data());

            // Run shading kernel
            {
                size_t gs[] = { static_cast<size_t>((output.width() + 7) / 8 * 8), static_cast<size_t>((output.height() + 7) / 8 * 8) };
                size_t ls[] = { 8, 8 };

                GetContext().Launch2D(0, gs, ls, filter_kernel);
            }
        }
    }

}
