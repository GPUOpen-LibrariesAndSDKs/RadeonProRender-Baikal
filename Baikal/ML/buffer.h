#pragma once

#include <cinttypes>
#include <initializer_list>
#include <utility>
#include <vector>
#include <cstddef>
#include <memory>


namespace Baikal::ML
{
    class Buffer
    {
    public:
        using ValueType = float;
        using Data = std::unique_ptr<ValueType[]>;
        using Shape = std::vector<size_t>;

        Buffer(Data data, size_t size, Shape shape) :
            m_data(std::move(data)),
            m_shape(std::move(shape)),
            m_size(size)
        {
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

} // namespace Baikal::ML
