#pragma once

#include "Baikal/PostEffects/ML/inference.h"


namespace Baikal
{
    namespace PostEffects
    {
        enum class MLDenoiserInputs
        {
            kColorDepthNormalGloss7,
        };

        std::unique_ptr<Inference> CreateMLDenoiser(MLDenoiserInputs inputs,
                                                    size_t width,
                                                    size_t height);
    }
}
