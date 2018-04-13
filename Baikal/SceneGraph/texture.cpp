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
            auto num_elements = m_size.x * m_size.y * m_size.z;


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
            auto num_elements = m_size.x * m_size.y * m_size.z;

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
            auto num_elements = m_size.x * m_size.y * m_size.z;

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
            TextureConcrete(char* data, RadeonRays::int3 size, Format format,
                const std::vector<MipLevel> &mip_levels) :
                Texture(data, size, format, mip_levels) {}

            TextureConcrete(char* data, RadeonRays::int3 size, Format format, bool generate_mipmap) :
                Texture(data, size, format, generate_mipmap) {}
        };
    }

    Texture::Ptr Texture::Create() {
        return std::make_shared<TextureConcrete>();
    }

    Texture::Ptr Texture::Create(char* data, RadeonRays::int3 size, Format format, const std::vector<MipLevel> &mip_levels)
    {
        return std::make_shared<TextureConcrete>(data, size, format, mip_levels);
    }

    Texture::Ptr Texture::Create(char* data, RadeonRays::int3 size, Format format, bool generate_mipmap)
    {
        return std::make_shared<TextureConcrete>(data, size, format, generate_mipmap);
    }

    Texture::Texture(
        char* data,
        RadeonRays::int3 size,
        Format format,
        const std::vector<MipLevel>& mip_levels)
        : m_data(data)
        , m_size(size)
        , m_format(format)
        , m_generate_mipmap(false)
    {
        if (size.z == 0)
        {
            m_size.z = 1;
        }

        if (!mip_levels.empty())
        {
            m_mip_level = mip_levels;
        }
    }

    Texture::Texture(
        char* data,
        RadeonRays::int3 size,
        Format format,
        bool generate_mipmap)
        : m_data(data)
        , m_size(size)
        , m_format(format)
        , m_generate_mipmap(generate_mipmap)
    {
        if (size.z == 0)
        {
            m_size.z = 1;
        }

        if (generate_mipmap)
        {
            int width = size.x;
            int height = size.y;
            int pitch = GetPixelSize(format) * width;

            while ((width != 1) && (height != 1))
            {
                m_mip_level.push_back(MipLevel
                {
                    width,
                    height,
                    pitch
                });

                width = (int)std::max(1.f, std::ceilf(width / 2.f));
                height = (int)std::max(1.f, std::ceilf(height / 2.f));
                pitch = GetPixelSize(format) * width;
            }

            m_mip_level.push_back(MipLevel{ 1, 1, GetPixelSize(format) });
        }
    }

    int Texture::GetPixelSize(Texture::Format format)
    {
        int pixel_size = 0;
        switch (format)
        {
        case Texture::Format::kRgba8:
            pixel_size = 4;
            break;
        case Texture::Format::kRgba16:
            pixel_size = 8;
            break;
        case Texture::Format::kRgba32:
            pixel_size = 16;
            break;
        default:
            throw std::runtime_error("Texture::Texture(...): unsupported format");
        }
        return pixel_size;
    }
}
