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
            using Data = std::shared_ptr<ValueType>;

            struct Shape
            {
                std::size_t width = 0;
                std::size_t height = 0;
                std::size_t channels = 0;

                Shape() = default;

                Shape(std::size_t width_, std::size_t height_, std::size_t channels_)
                    : width(width_)
                    , height(height_)
                    , channels(channels_)
                {
                }

                bool operator==(const Shape& rhs) const
                {
                    return width == rhs.width &&
                        height == rhs.height &&
                        channels == rhs.channels;
                }
            };

            Tensor(Tensor const&) = delete;
            Tensor& operator=(Tensor const&) = delete;

            Tensor()
                : Tensor(Shape())
            {
            }

            explicit Tensor(Shape shape)
                : Tensor(nullptr, shape)
            {
            }

            Tensor(Data data, Shape shape)
                : m_data(std::move(data))
                , m_shape(shape)
                , m_size(m_shape.width * m_shape.height * m_shape.channels)
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

            std::uint32_t tag = 0;

        private:
            Data m_data;
            Shape m_shape;
            std::size_t m_size;
        };

    }
}
