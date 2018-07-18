#include "PostEffects/ML/inference_impl.h"

#include <functional>
#include <numeric>
#include <cassert>

namespace Baikal
{
    namespace PostEffects
    {
        InferenceImpl::InferenceImpl(std::string const& model_path,
                                     size_t width,
                                     size_t height)
            : m_width(width), m_height(height)
        {
        }

        Tensor::Shape InferenceImpl::GetInputShape() const
        {
            return {m_height, m_width, 7};
        }

        Tensor::Shape InferenceImpl::GetOutputShape() const
        {
            return {m_height, m_width, 3};
        }

        Tensor InferenceImpl::GetInputTensor()
        {
            return AllocTensor(std::get<2>(GetInputShape()));
        }

        void InferenceImpl::PushInput(Tensor&& tensor)
        {
            assert(tensor.shape() == GetInputShape());
        }

        Tensor InferenceImpl::PopOutput()
        {
            return AllocTensor(std::get<2>(GetOutputShape()));
        }

        Tensor InferenceImpl::AllocTensor(size_t channels)
        {
            auto deleter = [](Tensor::ValueType* data)
            {
                delete[] data;
            };
            size_t size = m_width * m_height * channels;
            return Tensor(Tensor::Data(new Tensor::ValueType[size], deleter),
                          {m_height, m_width, channels});
        }
    }
}
