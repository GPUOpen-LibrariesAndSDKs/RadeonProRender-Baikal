#pragma once

#include <cstddef>
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
            using Shape = std::tuple<std::size_t, std::size_t, std::size_t>;

            Tensor(Tensor const&) = delete;
            Tensor& operator=(Tensor const&) = delete;

            Tensor()
                : Tensor(std::make_tuple(0, 0, 0))
            {
            }

            Tensor(Shape shape)
                : Tensor(nullptr, std::move(shape))
            {
            }

            Tensor(Data data, Shape shape)
                : m_data(std::move(data))
                , m_shape(std::move(shape))
                , m_size(std::get<0>(m_shape) *
                    std::get<1>(m_shape) *
                    std::get<2>(m_shape))
            {
            }

            Tensor(Tensor&&) = default;
            Tensor& operator=(Tensor&&) = default;

            bool empty() const
            {
                return m_data == nullptr;
            }

            ValueType* data() const
            {
                return m_data.get();
            }

            std::size_t size() const
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
