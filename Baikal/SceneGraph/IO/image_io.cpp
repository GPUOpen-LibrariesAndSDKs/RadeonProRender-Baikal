#include "image_io.h"
#include "../texture.h"

#include "OpenImageIO/imageio.h"

namespace Baikal
{
    class Oiio : public ImageIo
    {
    public:
        Texture::Ptr LoadImage(std::string const& filename) const override;
        void SaveImage(std::string const& filename, Texture::Ptr texture) const override;
    };
    
    static Texture::Format GetTextureFormat(OIIO_NAMESPACE::ImageSpec const& spec)
    {
        OIIO_NAMESPACE_USING
        
        if (spec.format.basetype == TypeDesc::UINT8)
            return Texture::Format::kRgba8;
        else if (spec.format.basetype == TypeDesc::HALF)
            return Texture::Format::kRgba16;
        else
            return Texture::Format::kRgba32;
    }
    
    static OIIO_NAMESPACE::TypeDesc GetTextureFormat(Texture::Format fmt)
    {
        OIIO_NAMESPACE_USING
        
        if (fmt == Texture::Format::kRgba8)
            return  TypeDesc::UINT8;
        else if (fmt == Texture::Format::kRgba16)
            return TypeDesc::HALF;
        else
            return TypeDesc::FLOAT;
    }
    
    Texture::Ptr Oiio::LoadImage(const std::string &filename) const
    {
        OIIO_NAMESPACE_USING
        
        std::unique_ptr<ImageInput> input{ImageInput::open(filename)};
        
        if (!input)
        {
            throw std::runtime_error("Can't load " + filename + " image");
        }

        std::vector<RadeonRays::int3> levels_spec;
        ImageSpec spec = input->spec();
        auto fmt = GetTextureFormat(spec);

        int texture_data_size = 0;

        std::unique_ptr<char[]> texture_data (new char[texture_data_size]);
        auto data_inplace = texture_data.get();

        int level_counter = 0; // reset counter to zero
        while (input->seek_subimage(0, level_counter, spec))
        {
            int size = 0;
            if (fmt == Texture::Format::kRgba8)
            {
                size = spec.width * spec.height * spec.depth * 4;

                // Read data to storage
                input->read_image(TypeDesc::UINT8, data_inplace, sizeof(char) * 4);

                if (spec.nchannels == 1)
                {
                    // set B, G and A components to R component value
                    for (auto i = 0; i < size; i += 4)
                    {
                        data_inplace[i + 1] = data_inplace[i];
                        data_inplace[i + 2] = data_inplace[i];
                        data_inplace[i + 3] = data_inplace[i];
                    }
                }
            }
            else if (fmt == Texture::Format::kRgba16)
            {
                size = spec.width * spec.height * spec.depth * sizeof(float) * 2;

                // Read data to storage
                input->read_image(TypeDesc::HALF, data_inplace, sizeof(float) * 2);
            }
            else
            {
                size = spec.width * spec.height * spec.depth * sizeof(RadeonRays::float3);

                // Read data to storage
                input->read_image(TypeDesc::FLOAT, data_inplace, sizeof(RadeonRays::float3));
            }

            levels_spec.push_back(
            {
                spec.width,
                spec.height,
                spec.depth
            });

            data_inplace += size;
            level_counter++;
        }

        // Close handle
        input->close();

        spec = input->spec(); // size of original level

        Texture::Ptr texture;
        if (levels_spec.size() == 1)
        {
            texture = Texture::Create(
                texture_data.release(),
                levels_spec[0],
                fmt);
        }
        else
        {
            texture = Texture::Create(
                texture_data.release(),
                levels_spec,
                fmt);
        }

        return texture;
    }

    void Oiio::SaveImage(std::string const& filename, Texture::Ptr texture) const
    {
        OIIO_NAMESPACE_USING;

        std::unique_ptr<ImageOutput> out{ImageOutput::create(filename)};
        
        if (!out)
        {
            throw std::runtime_error("Can't create image file on disk");
        }

        auto dim = texture->GetSize();
        auto fmt = GetTextureFormat(texture->GetFormat());

        ImageSpec spec(dim.x, dim.y, 4, fmt);

        out->open(filename, spec);

        out->write_image(fmt, texture->GetData());

        out->close();
    }

    std::unique_ptr<ImageIo> ImageIo::CreateImageIo()
    {
        return std::make_unique<Oiio>();
    }
}
