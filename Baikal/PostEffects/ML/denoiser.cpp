#include "Baikal/PostEffects/ML/denoiser.h"

#include "Baikal/PostEffects/ML/inference_impl.h"


namespace Baikal
{
    namespace PostEffects
    {
        std::unique_ptr<Inference> Baikal::PostEffects::CreateMLDenoiser(MLDenoiserInputs inputs,
                                                                         size_t width,
                                                                         size_t height)
        {
            return std::make_unique<InferenceImpl>("model/path.pb", width, height);
        }
    }
}
