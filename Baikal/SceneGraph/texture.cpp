#include "texture.h"

#include "Utils/half.h"

namespace Baikal
{
    RadeonRays::float3 Texture::ComputeAverageValue() const
    {
        auto avg = RadeonRays::float3();

        switch (m_format) {
        case Format::kRgba8:
        {
            auto data = reinterpret_cast<std::uint8_t*>(m_data.get());
            auto num_elements = m_levels[0].x * m_levels[0].y * m_levels[0].z;


            for (auto i = 0; i < num_elements; ++i)
            {
                float r = data[4 * i] / 255.f;
                float g = data[4 * i + 1] / 255.f;
                float b = data[4 * i + 2] / 255.f;
                avg += RadeonRays::float3(r, g, b);
            }

            avg *= (1.f / num_elements);
            break;
        }
        case Format::kRgba16:
        {
            auto data = reinterpret_cast<std::uint16_t*>(m_data.get());
            auto num_elements = m_levels[0].x * m_levels[0].y * m_levels[0].z;

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

            avg *= (1.f / num_elements);
            break;
        }
        case Format::kRgba32:
        {
            auto data = reinterpret_cast<float*>(m_data.get());
            auto num_elements = m_levels[0].x * m_levels[0].y * m_levels[0].z;

            for (auto i = 0; i < num_elements; ++i)
            {
                auto r = data[4 * i];
                auto g = data[4 * i + 1];
                auto b = data[4 * i + 2];

                avg += RadeonRays::float3(r, g, b);
            }

            avg *= (1.f / num_elements);
            break;
        }
        default:
            break;
        }

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
        const std::vector<RadeonRays::int3> &levels,
        Format format)
        : m_data(data)
        , m_format(format)
        , m_generate_mipmap(false)
    {
        if (levels.empty())
            throw std::logic_error("Texture::Texture(...): texture 'levels' is empty");

        m_levels = levels;

        std::transform(m_levels.begin(), m_levels.end(), m_levels.begin(),
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
        , m_generate_mipmap(generate_mipmap)
    {
        m_levels.push_back(
        {
            size.x,
            size.y,
            std::max(1, size.z)
        });

        if (generate_mipmap)
        {
            int width = (int)std::max(1.f, std::ceilf(m_levels[0].x / 2.f));
            int height = (int)std::max(1.f, std::ceilf(m_levels[0].y / 2.f));

            while ((width != 1) && (height != 1))
            {
                m_levels.push_back(
                {
                    width,
                    height,
                    1
                });

                width = (int)std::max(1.f, std::ceilf(width / 2.f));
                height = (int)std::max(1.f, std::ceilf(height / 2.f));
            }

            m_levels.push_back({ 1, 1, 1});
        }
    }

    int Texture::GetPixelSize(Texture::Format format)
    {
        std::uint32_t component_size = 1;

        switch (format) {
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
            throw std::runtime_error("Texture::Texture(...): unsupported format");
        }

        return 4 * component_size;
    }
}
