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

#pragma once

#include "data_preprocessor.h"

#include "CLW.h"
#include "Utils/clw_class.h"

#ifdef BAIKAL_EMBED_KERNELS
#include "embed_kernels.h"
#endif

#include <cstddef>
#include <memory>
#include <vector>


namespace Baikal
{
    namespace PostEffects
    {
        enum class Model
        {
            kColorDepthNormalGloss7,
            kColorAlbedoNormal8,
            kColorAlbedoDepthNormal9
        };

        class DenoiserPreprocessor: public DataPreprocessor
        {
        public:
            DenoiserPreprocessor(CLWContext context,
                               CLProgramManager const* program_manager,
                               std::uint32_t start_spp = 8);

            Image MakeInput(PostEffect::InputSet const& inputs) override;

            std::set<Renderer::OutputType> GetInputTypes() const override;

            std::tuple<std::uint32_t, std::uint32_t> ChannelsNum() const override;
        private:
            void Init(std::uint32_t width, std::uint32_t height);

            // layout of the outputs in input tensor in terms of channels
            using MemoryLayout = std::vector<std::pair<Renderer::OutputType, int>>;

            void DivideBySampleCount(CLWBuffer<RadeonRays::float3> const& dst,
                                     CLWBuffer<RadeonRays::float3> const& src);

            bool m_is_initialized = false;
            CLWParallelPrimitives m_primitives;
            std::uint32_t m_width, m_height;
            std::uint32_t m_channels = 0;
            Model m_model;
            MemoryLayout m_layout;
            CLWBuffer<float> m_cache;
            CLWBuffer<float> m_input;
            Handle<ml_context> m_context;
            ml_image m_image;
        };
    }
}
