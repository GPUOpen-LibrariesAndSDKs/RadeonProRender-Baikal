#pragma once

#include "PostEffects/ML/inference.h"

#include <cstddef>
#include <memory>
#include <string>


namespace Baikal
{
    namespace PostEffects
    {
        enum class MLDenoiserInputs
        {
            kColorDepthNormalGloss7,
        };

        std::unique_ptr<Inference> CreateMLDenoiser(MLDenoiserInputs inputs,
                                                    float gpu_memory_fraction,
                                                    std::string const& visible_devices,
                                                    std::size_t width,
                                                    std::size_t height);
    }
}
