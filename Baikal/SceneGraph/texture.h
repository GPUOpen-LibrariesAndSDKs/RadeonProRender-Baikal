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
  \version 1.0
  \brief Contains declaration of a texture class.
  */
#pragma once

#include "math/float3.h"
#include "math/float2.h"
#include "math/int3.h"
#include <memory>
#include <string>

#include "scene_object.h"

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
        void SetData(char* data, const std::vector<RadeonRays::int3> &level_sizes, Format format);

        // Get texture dimensions
        RadeonRays::int3 GetSize() const;
        // Get texture raw data
        char const* GetData() const;
        // Get texture format
        Format GetFormat() const;
        // Get data size in bytes
        std::size_t GetSizeInBytes() const;

        // Average normalized value
        RadeonRays::float3 ComputeAverageValue() const;

        // Disallow copying
        Texture(Texture const&) = delete;
        Texture& operator = (Texture const&) = delete;

        // returns specs of each mipmap level for this texture
        std::vector<RadeonRays::int3> GetLevelsInfo() const
        { return m_levels; }

        // marks that generation of the mipmap is required
        bool MipmapGenerationReq() const
        { return m_generate_mipmap; }

        // marks that texture has mipmap support
        bool MipmapEnabled() const;

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
        // stores texture level dims, if mipmap enabled holds all pyramid level sizes
        // in other ways stores only one image dim for original texture
        std::vector<RadeonRays::int3> m_levels;
        // flag to determine that mipmap generation is required
        bool m_generate_mipmap;
    };

    inline Texture::Texture()
        : m_data(new char[16])
        , m_format(Format::kRgba8)
        , m_generate_mipmap (false)
    {
        m_levels.push_back({ 2, 2, 1 });

        // Create checkerboard by default
        m_data[0] = m_data[1] = m_data[2] = m_data[3] = (char)0xFF;
        m_data[4] = m_data[5] = m_data[6] = m_data[7] = (char)0x00;
        m_data[8] = m_data[9] = m_data[10] = m_data[11] = (char)0xFF;
        m_data[12] = m_data[13] = m_data[14] = m_data[15] = (char)0x00;
    }

    inline bool Texture::MipmapEnabled() const
    { return m_generate_mipmap || (!m_levels.empty()); }

    inline void Texture::SetData(char* data, const std::vector<RadeonRays::int3> &level_sizes, Format format)
    {
        if (level_sizes.empty())
        {
            throw std::logic_error(
                "Texture::SetData(...): 'level_sizes' isn't containg any level info");
        }

        m_data.reset(data);
        m_levels = level_sizes;

        if (m_levels[0].z == 0)
        {
            m_levels[0].z = 1;
        }

        m_format = format;
        SetDirty(true);
    }

    inline RadeonRays::int3 Texture::GetSize() const
    {
        return m_levels[0];
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
        auto pixel_size = GetPixelSize(m_format);
        int texture_size = 0;
        for (const auto & level : m_levels)
        {
            texture_size += pixel_size * level.x * level.y * level.z;
        }

        return texture_size;
    }
}
