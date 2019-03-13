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

#include "data_preprocessor.h"

namespace Baikal
{
    namespace PostEffects
    {
        DataPreprocessor::DataPreprocessor(CLWContext context,
                                       CLProgramManager const* program_manager,
                                       std::uint32_t start_spp)
#ifdef BAIKAL_EMBED_KERNELS
        : ClwClass(context, program_manager, "denoise", g_denoise_opencl, g_denoise_opencl_headers)
#else
        : ClwClass(context, program_manager, "../Baikal/Kernels/CL/denoise.cl")
#endif
        , m_start_spp(start_spp)
        {  }


        CLWEvent DataPreprocessor::WriteToInputs(CLWBuffer<float> const& dst_buffer,
                                                 CLWBuffer<float> const& src_buffer,
                                                 int width,
                                                 int height,
                                                 int dst_channels_offset,
                                                 int dst_channels_num,
                                                 int src_channels_offset,
                                                 int src_channels_num,
                                                 int channels_to_copy)
        {
            auto copy_kernel = GetKernel("CopyInterleaved");

            unsigned argc = 0;
            copy_kernel.SetArg(argc++, dst_buffer);
            copy_kernel.SetArg(argc++, src_buffer);
            copy_kernel.SetArg(argc++, width);
            copy_kernel.SetArg(argc++, height);
            copy_kernel.SetArg(argc++, dst_channels_offset);
            copy_kernel.SetArg(argc++, dst_channels_num);
            // input and output buffers have the same width in pixels
            copy_kernel.SetArg(argc++, width);
            // input and output buffers have the same height in pixels
            copy_kernel.SetArg(argc++, height);
            copy_kernel.SetArg(argc++, src_channels_offset);
            copy_kernel.SetArg(argc++, src_channels_num);
            copy_kernel.SetArg(argc++, channels_to_copy);

            // run copy_kernel
            auto thread_num = ((width * height + 63) / 64) * 64;
            return GetContext().Launch1D(0,
                                         thread_num,
                                         64,
                                         copy_kernel);
        }

        unsigned DataPreprocessor::ReadSpp(CLWBuffer<RadeonRays::float3> const &buffer)
        {
            RadeonRays::float3 pixel;
            GetContext().ReadBuffer(0, buffer, &pixel, 1).Wait();
            return static_cast<unsigned>(pixel.w);
        }
    }
}
