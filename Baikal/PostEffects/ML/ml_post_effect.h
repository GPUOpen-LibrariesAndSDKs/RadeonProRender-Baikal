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
#include "PostEffects/ML/inference.h"
#include "PostEffects/clw_post_effect.h"

namespace Baikal
{
    namespace PostEffects
    {
        enum class ModelType
        {
            kDenoiser,
            kUpsampler,
        };

        class MLPostEffect : public ClwPostEffect
        {
        public:
            MLPostEffect(CLWContext context, const CLProgramManager* program_manager, ModelType type);

            void Apply(InputSet const& input_set, Output& output) override;

            void SetParameter(std::string const& name, Param value) override;

            InputTypes GetInputTypes() const override;

            void Resize_2x(CLWBuffer<RadeonRays::float3> dst, CLWBuffer<RadeonRays::float3> src);
        private:
            Inference::Ptr CreateInference(std::uint32_t width, std::uint32_t height);
            void Init(InputSet const& input_set, Output& output);

            Inference::Ptr m_inference;
            ModelType m_type;
            bool m_is_dirty;
            bool m_has_denoised_img;
            std::vector<RadeonRays::float3> m_host;
            CLWBuffer<RadeonRays::float3> m_last_image;
            CLWBuffer<RadeonRays::float3> m_resizer_cache;
            std::unique_ptr<DataPreprocessor> m_preproc;
            std::uint32_t m_width = 0;
            std::uint32_t m_height = 0;
            std::uint32_t m_start_seq = 0;
            std::uint32_t m_last_seq = 0;
            const CLProgramManager *m_program;
        };
    }
}
