#include "PostEffects/ML/model.h"
#include "PostEffects/ML/inference.h"
#include "PostEffects/ML/thread_safe_queue.h"

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
            std::unique_ptr<ML::Model> m_model;
            void Shutdown();
            Tensor AllocTensor(std::size_t channels);
            void DoInference();

            ML::thread_safe_queue<Tensor> m_input_queue;
            ML::thread_safe_queue<Tensor> m_output_queue;
            std::thread m_worker;

            std::size_t m_width;
            std::size_t m_height;

            std::size_t m_input_channels;
            static const std::size_t m_output_channels = 3;

            bool m_shutdown;
        };
    }
}
