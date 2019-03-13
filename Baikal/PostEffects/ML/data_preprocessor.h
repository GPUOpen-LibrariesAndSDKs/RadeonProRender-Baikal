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

#ifdef BAIKAL_EMBED_KERNELS
#include "embed_kernels.h"
#endif

#include "CLW.h"
#include "Utils/clw_class.h"
#include "PostEffects/post_effect.h"
#include "image.h"


namespace Baikal
{
    namespace PostEffects
    {
        class DataPreprocessor : public ClwClass
        {
        public:
            DataPreprocessor(CLWContext context,
                             CLProgramManager const* program_manager,
                             std::uint32_t start_spp = 1);

            virtual Image MakeInput(PostEffect::InputSet const& inputs) = 0;

            virtual std::set<Renderer::OutputType> GetInputTypes() const = 0;

            // returns channels num at input and output
            virtual std::tuple<std::uint32_t, std::uint32_t> ChannelsNum() const = 0;

            void SetStartSpp(std::uint32_t start_spp)
            { m_start_spp = start_spp; }

        protected:
            template<class T>
            using Handle = std::unique_ptr<typename std::remove_pointer<T>::type, void (*)(T)>;

            unsigned ReadSpp(CLWBuffer<RadeonRays::float3> const& buffer);

            CLWEvent WriteToInputs(CLWBuffer<float> const& dst_buffer,
                                   CLWBuffer<float> const& src_buffer,
                                   int width,
                                   int height,
                                   int dst_channels_offset,
                                   int dst_channels_num,
                                   int src_channels_offset,
                                   int src_channels_num,
                                   int channels_to_copy);

            std::uint32_t  m_start_spp;
        };
    }
}
