#include "PostEffects/ML/denoiser.h"
#include "PostEffects/ML/inference_impl.h"


namespace Baikal
{
    namespace PostEffects
    {
        std::unique_ptr<Inference> Baikal::PostEffects::CreateMLDenoiser(MLDenoiserInputs inputs,
                                                                         float gpu_memory_fraction,
                                                                         std::string const& visible_devices,
                                                                         std::size_t width,
                                                                         std::size_t height)
        {
            std::string model_path = "model/path.pb";
            // TODO: select model_path based on MLDenoiserInputs
            return std::make_unique<InferenceImpl>(
                model_path, 
                gpu_memory_fraction, 
                visible_devices,
                width, 
                height);
        }
    }
}
