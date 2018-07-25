#include "texture.h"

#include "Utils/half.h"

namespace Baikal
{
    RadeonRays::float3 Texture::ComputeAverageValue() const
    {
        auto avg = RadeonRays::float3();
        auto num_elements = m_mip_sizes[0].x * m_mip_sizes[0].y * m_mip_sizes[0].z;

        switch (m_format)
        {
        case Format::kRgba8:
        {
            auto data = reinterpret_cast<std::uint8_t*>(m_data.get());

            for (auto i = 0; i < num_elements; ++i)
            {
                float r = data[4 * i] / 255.f;
                float g = data[4 * i + 1] / 255.f;
                float b = data[4 * i + 2] / 255.f;
                avg += RadeonRays::float3(r, g, b);
            }

            break;
        }
        case Format::kRgba16:
        {
            auto data = reinterpret_cast<std::uint16_t*>(m_data.get());

            for (auto i = 0; i < num_elements; ++i)
            {
                auto r = data[4 * i];
                auto g = data[4 * i + 1];
                auto b = data[4 * i + 2];

                half hr, hg, hb;
                hr.setBits(r);
                hg.setBits(g);
                hb.setBits(b);

                avg += RadeonRays::float3(hr, hg, hb);
            }

            break;
        }
        case Format::kRgba32:
        {
            auto data = reinterpret_cast<float*>(m_data.get());

            for (auto i = 0; i < num_elements; ++i)
            {
                auto r = data[4 * i];
                auto g = data[4 * i + 1];
                auto b = data[4 * i + 2];

                avg += RadeonRays::float3(r, g, b);
            }

            break;
        }
        default:
            throw std::runtime_error("Texture::ComputeAverageValue(): unsupported format");
        }

        avg *= (1.f / num_elements);

        return avg;
    }

    namespace {
        struct TextureConcrete : public Texture {
            TextureConcrete() = default;
            TextureConcrete(
                char* data,
                const std::vector<RadeonRays::int3> &levels,
                Format format) :
                Texture(data, levels, format) {}

            TextureConcrete(char* data, RadeonRays::int3 size, Format format, bool generate_mipmap) :
                Texture(data, size, format, generate_mipmap) {}
        };
    }

    Texture::Ptr Texture::Create() {
        return std::make_shared<TextureConcrete>();
    }

    Texture::Ptr Texture::Create(char* data, const std::vector<RadeonRays::int3> &levels, Format format)
    {
        return std::make_shared<TextureConcrete>(data, levels, format);
    }

    Texture::Ptr Texture::Create(char* data, RadeonRays::int3 size, Format format, bool generate_mipmap)
    {
        return std::make_shared<TextureConcrete>(data, size, format, generate_mipmap);
    }

    Texture::Texture(
        char* data,
        std::vector<RadeonRays::int3> const& mip_sizes,
        Format format)
        : m_data(data)
        , m_format(format)
        , m_needs_mip_generation(false)
    {
        if (mip_sizes.empty())
        {
            throw std::logic_error("Texture::Texture(...): mip_sizes is empty");
        }

        m_mip_sizes = mip_sizes;

        std::transform(m_mip_sizes.begin(), m_mip_sizes.end(), m_mip_sizes.begin(),
            [](RadeonRays::int3 size)
            {
                size.z = std::max(1, size.z);
                return size;
            });
    }

    Texture::Texture(
        char* data,
        RadeonRays::int3 size,
        Format format,
        bool generate_mipmap)
        : m_data(data)
        , m_format(format)
        , m_needs_mip_generation(generate_mipmap)
    {
        m_mip_sizes.push_back(
        {
            size.x,
            size.y,
            std::max(1, size.z)
        });

        if (m_needs_mip_generation)
        {
            int width  = (int)std::max(1.f, (float)std::ceil(m_mip_sizes[0].x / 2.0));
            int height = (int)std::max(1.f, (float)std::ceil(m_mip_sizes[0].y / 2.0));
            int depth  = (int)std::max(1.f, (float)std::ceil(m_mip_sizes[0].z / 2.0));

            while ((width != 1) || (height != 1))
            {
                m_mip_sizes.push_back(
                {
                    width,
                    height,
                    depth,
                });

                width  = (int)std::max(1.f, (float)std::ceil(width  / 2.0));
                height = (int)std::max(1.f, (float)std::ceil(height / 2.0));
                depth  = (int)std::max(1.f, (float)std::ceil(depth  / 2.0));
            }

            m_mip_sizes.push_back({1, 1, 1});
        }
    }

    int Texture::GetPixelSize(Texture::Format format)
    {
        std::uint32_t component_size = 1;

        switch (format)
        {
        case Format::kRgba8:
            component_size = 1;
            break;
        case Format::kRgba16:
            component_size = 2;
            break;
        case Format::kRgba32:
            component_size = 4;
            break;
        default:
            throw std::runtime_error("Texture::GetPixelSize(...): unsupported format");
        }

        return 4 * component_size;
    }
}
