#include "PostEffects/ML/inference_impl.h"

#include <functional>
#include <numeric>
#include <cassert>

namespace Baikal
{
    namespace PostEffects
    {
        InferenceImpl::InferenceImpl(std::string const& model_path,
                                     float gpu_memory_fraction,
                                     std::string const& visible_devices,
                                     std::size_t width,
                                     std::size_t height,
                                     std::size_t input_channels)
        : m_model(model_path, gpu_memory_fraction, visible_devices)
        , m_width(width)
        , m_height(height)
        , m_input_channels(input_channels)
        , m_worker(&InferenceImpl::DoInference, this)
        {
        }

        InferenceImpl::~InferenceImpl()
        {
            Shutdown();
        }

        Tensor::Shape InferenceImpl::GetInputShape() const
        {
            return { m_width, m_height, m_input_channels };
        }

        Tensor::Shape InferenceImpl::GetOutputShape() const
        {
            return { m_width, m_height, m_output_channels };
        }

        Tensor InferenceImpl::GetInputTensor()
        {
            return AllocTensor(m_input_channels);
        }

        void InferenceImpl::PushInput(Tensor&& tensor)
        {
            assert(tensor.shape() == GetInputShape());
            m_input_queue.push(std::move(tensor));
        }

        Tensor InferenceImpl::PopOutput()
        {
            Tensor output_tensor;
            m_output_queue.try_pop(output_tensor);
            return output_tensor;
        }

        Tensor InferenceImpl::AllocTensor(std::size_t channels)
        {
            auto deleter = [](Tensor::ValueType* data)
            {
                delete[] data;
            };
            size_t size = m_width * m_height * channels;
            return Tensor(Tensor::Data(new Tensor::ValueType[size], deleter),
                          { m_width, m_height, channels });
        }

        void InferenceImpl::DoInference()
        {
            for (;;)
            {
                Tensor input_tensor;
                m_input_queue.wait_and_pop(input_tensor);
                if (input_tensor.empty())
                {
                    break;
                }

                if (m_input_queue.size() > 0)
                {
                    continue;
                }

                Tensor output_tensor = AllocTensor(m_output_channels);
                m_model->infer(
                    input_tensor.data(),
                    m_width,
                    m_height,
                    m_input_channels,
                    output_tensor.data());

                output_tensor.tag = input_tensor.tag;
                m_output_queue.push(std::move(output_tensor));
            }
        }

        void InferenceImpl::Shutdown()
        {
            m_input_queue.push(Tensor());
            m_worker.join();
        }
    }
}
