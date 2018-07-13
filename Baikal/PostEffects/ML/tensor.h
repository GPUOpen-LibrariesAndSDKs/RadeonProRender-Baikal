#pragma once

#include <memory>
#include <tuple>


namespace Baikal
{
    namespace PostEffects
    {
        class Tensor
        {
        public:
            using ValueType = float;
            using Data = std::shared_ptr<ValueType[]>;
            using Shape = std::tuple<size_t, size_t, size_t>;

            Tensor(Data data, Shape shape)
                : m_data(std::move(data))
                , m_shape(std::move(shape))
                , m_size(std::get<0>(m_shape) *
                         std::get<1>(m_shape) *
                         std::get<2>(m_shape))
            {
            }

            bool empty() const
            {
                return m_data == nullptr;
            }

            ValueType* data() const
            {
                return m_data.get();
            }

            size_t size() const
            {
                return m_size;
            }

            const Shape& shape() const
            {
                return m_shape;
            }

        private:
            Data m_data;
            Shape m_shape;
            std::size_t m_size;
        };
    }
}
