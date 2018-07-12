#include "inference_impl.h"

#include <numeric>
#include <valarray>


namespace Baikal::ML
{
    namespace
    {
        Buffer AllocBuffer(const Buffer::Shape& shape)
        {
            size_t size = std::accumulate(shape.begin(),
                                          shape.end(),
                                          1u,
                                          std::multiplies<>());
            return Buffer(Buffer::Data(new Buffer::ValueType[size]), size, shape);
        }
    }

    InferenceImpl::InferenceImpl(Buffer::Shape input_shape) :
        m_input_shape(std::move(input_shape)),
        m_output_shape(m_input_shape),
    {
        m_output_shape[m_output_shape.size() - 1] = 3;
    }

    Buffer InferenceImpl::GetInputBuffer()
    {
        return AllocBuffer(m_input_shape);
    }

    void InferenceImpl::PushInput(Buffer&& buffer)
    {
    }

    std::optional<Buffer> InferenceImpl::PopOutput() const
    {
        return AllocBuffer(m_output_shape);
    }

} // namespace Baikal::ML
