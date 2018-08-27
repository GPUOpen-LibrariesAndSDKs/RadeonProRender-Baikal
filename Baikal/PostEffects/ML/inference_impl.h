#include "PostEffects/ML/inference.h"
#include "PostEffects/ML/model_holder.h"
#include "../RadeonRays/RadeonRays/src/async/thread_pool.h"

#include <cstddef>
#include <string>
#include <thread>

namespace Baikal
{
    namespace PostEffects
    {
        class InferenceImpl : public Inference
        {
        public:
            InferenceImpl(std::string const& model_path,
                          float gpu_memory_fraction,
                          std::string const& visible_devices,
                          std::size_t width,
                          std::size_t height,
                          std::size_t input_channels);
            ~InferenceImpl() override;

            Tensor::Shape GetInputShape() const override;
            Tensor::Shape GetOutputShape() const override;
            Tensor GetInputTensor() override;
            void PushInput(Tensor&& tensor) override;
            Tensor PopOutput() override;

        private:
            void Shutdown();
            Tensor AllocTensor(std::size_t channels);
            void DoInference();

            RadeonRays::thread_safe_queue<Tensor> m_input_queue;
            RadeonRays::thread_safe_queue<Tensor> m_output_queue;

            ModelHolder m_model;

            std::size_t m_width;
            std::size_t m_height;

            std::size_t m_input_channels;

            const std::size_t m_output_channels = 3;

            std::thread m_worker;
        };
    }
}
