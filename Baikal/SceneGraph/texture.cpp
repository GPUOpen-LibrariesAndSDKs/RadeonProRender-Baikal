#include "texture.h"

#include <vector>
#include <algorithm>
#include <cassert>

namespace Baikal
{
    static std::uint32_t GetComponentSize(Texture::Format format)
    {
            std::uint32_t component_size = 1;

            switch (format) {
            case Texture::Format::kRgba8:
                component_size = 1;
                break;
            case Texture::Format::kRgba16:
                component_size = 2;
                break;
            case Texture::Format::kRgba32:
                component_size = 4;
                break;
            default:
                break;
            }

            return component_size;
    }

    std::uint32_t Texture::GetPixelSizeInBytes() const
    {
        return GetComponentSize(m_format) * 4;
    }

    void Texture::GenerateMipLevels()
    {
        if (m_size.x & (m_size.x - 1) != 0 ||
            m_size.y & (m_size.y - 1) != 0)
        {
            throw std::runtime_error("Only power of two texture sizes are supported for mip generation");
        }

        // Current mip size
        auto size = m_size;
        // Total number of pixels in all mips
        auto num_pixels = 0;

        // Mip level offsets in bytes
        m_mip_offsets.clear();
        // Mip level sizes in pixels
        m_mip_sizes.clear();

        // Calculate mip sizes and offsets
        while (size.x >= 1 && size.y >= 1)
        {
            m_mip_offsets.push_back(num_pixels * GetPixelSizeInBytes());
            m_mip_sizes.push_back(size);
            num_pixels += size.x * size.y;
            size.x >>= 1;
            size.y >>= 1;
        }

        m_num_mip_levels = m_mip_offsets.size();

        // Allocate space for mip pyramid
        std::unique_ptr<char[]> data(new char[num_pixels * GetPixelSizeInBytes()]);
        // Copy zero level
        std::copy(m_data.get(), m_data.get() + m_size.x * m_size.y * GetPixelSizeInBytes(), data.get());

        for (auto mip = 1U; mip < m_num_mip_levels; ++mip)
        {
            auto prev_mip_size = m_mip_sizes[mip - 1];
            auto mip_size = m_mip_sizes[mip];
            for (auto x = 0U; x < mip_size.x; ++x)
                for (auto y = 0U; y < mip_size.y; ++y)
                {
                    switch (m_format)
                    {
                    case Format::kRgba32:
                    {
                        auto my_mip = reinterpret_cast<RadeonRays::float4*>(data.get() + m_mip_offsets[mip]);
                        auto prev_mip = reinterpret_cast<RadeonRays::float4*>(data.get() + m_mip_offsets[mip - 1]);

                        my_mip[y * mip_size.x + x] = 0.25f * (prev_mip[2 * y * prev_mip_size.x + 2 * x] +
                            prev_mip[2 * y * prev_mip_size.x + 2 * x + 1] + prev_mip[(2 * y + 1) * prev_mip_size.x + 2 * x]
                            + prev_mip[(2 * y + 1) * prev_mip_size.x + 2 * x + 1]);
                    }
                    }
                }
        }

        // TODO: calculate all mips here
        std::swap(m_data, data);

        //SetDirty(true);
    }

    std::uint32_t Texture::GetMipOffsetInBytes(std::uint32_t idx) const
    {
        assert(idx < GetNumMipLevels());

        return m_mip_offsets[idx];
    }

    RadeonRays::int2 Texture::GetMipSize(std::uint32_t idx) const
    {
        assert(idx < GetNumMipLevels());

        return m_mip_sizes[idx];
    }
}