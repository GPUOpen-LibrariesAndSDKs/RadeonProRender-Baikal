/**********************************************************************
 Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved.

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 ********************************************************************/

 /**
  \file texture.h
  \author Dmitry Kozlov
  \version 1.1
  \brief Contains declaration of a texture class.
  */
#pragma once

#include "math/float3.h"
#include "math/float2.h"
#include "math/int3.h"
#include "scene_object.h"
#include <memory>
#include <string>
#include <cassert>


namespace Baikal
{
    class Material;

    /**
     \brief Texture class.

     Texture is used to host CPU memory for image data.
     */
    class Texture : public SceneObject
    {
    public:
        enum class Format
        {
            kRgba8 = 0,
            kRgba16,
            kRgba32
        };

        using Ptr = std::shared_ptr<Texture>;

        static Ptr Create(char* data, RadeonRays::int3 size, Format format, bool generate_mipmap = false);
        static Ptr Create(char* data, const std::vector<RadeonRays::int3> &levels, Format format);

        static Ptr Create();

        // Destructor (the data is destroyed as well)
        virtual ~Texture() = default;

        // Set data
        void SetData(char* data, const RadeonRays::int3 &size, Format format);
        void SetData(char* data, const std::vector<RadeonRays::int3> &level_sizes, Format format);

        // Get texture dimensions
        RadeonRays::int3 GetSize(std::size_t mip_level = 0) const;
        // Number of mip levels
        std::size_t GetMipLevelCount() const;
        // Checks that generation of the mipmap is required
        bool NeedsMipGeneration() const;

        // Get texture raw data
        char const* GetData() const;

        // Get texture format
        Format GetFormat() const;

        // Get data size in bytes
        std::size_t GetSizeInBytes() const;
        // Get size in bytes of specified level
        std::size_t GetLevelSizeInBytes(std::size_t mip_level) const;

        // Average normalized value
        RadeonRays::float3 ComputeAverageValue() const;

        // Disallow copying
        Texture(Texture const&) = delete;
        Texture& operator = (Texture const&) = delete;

        // helper function, returns pixel (texel) size depends on format
        static int GetPixelSize(Texture::Format format);

    protected:
        // Constructor
        Texture();
        // Note, that texture takes ownership of its data array
        Texture(
            char* data,
            const std::vector<RadeonRays::int3> &levels,
            Format format); // collection of mip levels infos

        Texture(
            char* data,
            RadeonRays::int3 size,
            Format format,
            bool generate_mipmap = false); // collection of mip levels infos

    private:
        // Image data
        std::unique_ptr<char[]> m_data;
        // Format
        Format m_format;
        // flag to determine that mipmap generation is required
        bool m_needs_mip_generation;
        // stores texture level dims, if mipmap enabled holds all pyramid level sizes
        // in other ways stores only one image dim for original texture
        std::vector<RadeonRays::int3> m_mip_sizes;
    };

    inline Texture::Texture()
        : m_data(new char[16])
        , m_format(Format::kRgba8)
        , m_needs_mip_generation(false)
    {
        m_mip_sizes.push_back({ 2, 2, 1 });

        // Create checkerboard by default
        m_data[0] = m_data[1] = m_data[2] = m_data[3] = (char)0xFF;
        m_data[4] = m_data[5] = m_data[6] = m_data[7] = (char)0x00;
        m_data[8] = m_data[9] = m_data[10] = m_data[11] = (char)0xFF;
        m_data[12] = m_data[13] = m_data[14] = m_data[15] = (char)0x00;
    }

    inline void Texture::SetData(char* data, const RadeonRays::int3 &size, Format format)
    {
        SetData(data, std::vector<RadeonRays::int3>{ size }, format);
    }

    inline void Texture::SetData(char* data, std::vector<RadeonRays::int3> const& mip_sizes, Format format)
    {
        if (mip_sizes.empty())
        {
            throw std::logic_error(
                "Texture::SetData(...): 'level_sizes' isn't containg any level info");
        }

        m_data.reset(data);
        m_mip_sizes = mip_sizes;

        std::transform(m_mip_sizes.begin(), m_mip_sizes.end(), m_mip_sizes.begin(),
            [](RadeonRays::int3 size)
        {
            size.z = std::max(1, size.z);
            return size;
        });

        m_format = format;
        SetDirty(true);
    }

    inline RadeonRays::int3 Texture::GetSize(std::size_t mip_level) const
    {
        return m_mip_sizes[mip_level];
    }

    inline std::size_t Texture::GetMipLevelCount() const
    {
        return m_mip_sizes.size();
    }

    inline bool Texture::NeedsMipGeneration() const
    {
        return m_needs_mip_generation;
    }

    inline char const* Texture::GetData() const
    {
        return m_data.get();
    }

    inline Texture::Format Texture::GetFormat() const
    {
        return m_format;
    }

    inline std::size_t Texture::GetSizeInBytes() const
    {
        assert(!m_mip_sizes.empty());

        std::size_t texture_size = 0;

        for (std::size_t i = 0; i < GetMipLevelCount(); ++i)
        {
            texture_size += GetLevelSizeInBytes(i);
        }

        return texture_size;
    }

    inline std::size_t Texture::GetLevelSizeInBytes(std::size_t mip_level) const
    {
        auto pixel_size = GetPixelSize(m_format);

        auto const& level_size = m_mip_sizes[mip_level];

        return pixel_size * level_size.x * level_size.y * level_size.z;
    }
}
