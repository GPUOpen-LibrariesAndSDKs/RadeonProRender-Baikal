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
#pragma once

#include "basic.h"
#include "../BaikalIO/image_io.h"
#include <algorithm>

using namespace Baikal;

class MipmapTest : public BasicTest
{
public:
    void LoadTestScene() override
    {
        m_scene = Baikal::SceneIo::LoadScene("sphere+plane+ibl.test", "");
    }

    int TexelSize(int format)
    {
        if (format == ClwScene::RGBA32)
        {
            return 16;
        }
        else if (format == ClwScene::RGBA16)
        {
            return 8;
        }
        else if (format == ClwScene::RGBA8)
        {
            return 4;
        }
        else
        {
            throw std::runtime_error("TexelSize(...): unknown texture format!");
        }
    }

    Texture::Ptr CreateMipmapAtlas(
        ClwScene::Texture texture,
        ClwScene::MipLevel* mip_levels,
        char* texturedata)
    {
        ClwScene::MipLevel* texture_levels = mip_levels + texture.mip_offset;

        int total_width = texture_levels[0].w + texture_levels[1].w;

        int levels_height = 0;
        for (int i = 1; i < texture.mip_count; i++)
        {
            levels_height += texture_levels[i].h;
        }
        int total_height = std::max(texture_levels[0].h, levels_height);

        int texel_size = TexelSize(texture.fmt);

        int size_in_bytes = total_width * total_height * texel_size;
        std::unique_ptr<char> mipmap_image(new char[size_in_bytes]);

        // Fill background with green color (for 8 bit)
        {
            int* image_int = reinterpret_cast<int*>(mipmap_image.get());
            std::fill(image_int, image_int + size_in_bytes / sizeof(int), 0x0000FF00);
        }

        // Draw first level
        {
            auto src_row = texturedata + texture_levels[0].dataoffset;
            auto dst_row = mipmap_image.get();

            for (int y = 0; y < texture_levels[0].h; y++)
            {
                memcpy(dst_row, src_row, texture_levels[0].w * texel_size);
                src_row += texture_levels[0].w * texel_size;
                dst_row += total_width * texel_size;
            }
        }

        // Draw other levels
        int dst_offset = texture_levels[0].w * texel_size;

        for (int i = 1; i < texture.mip_count; i++)
        {
            int width = texture_levels[i].w;
            int height = texture_levels[i].h;

            auto src_row = texturedata + texture_levels[i].dataoffset;
            auto dst_row = mipmap_image.get() + dst_offset;

            for (int y = 0; y < height; y++)
            {
                memcpy(dst_row, src_row, width * texel_size);
                src_row += width * texel_size;
                dst_row += total_width * texel_size;
            }
            dst_offset += height * total_width * texel_size;;
        }

        Texture::Format format;

        if (texture.fmt == ClwScene::RGBA8)
            format = Texture::Format::kRgba8;
        else if (texture.fmt == ClwScene::RGBA16)
            format = Texture::Format::kRgba16;
        else
            format = Texture::Format::kRgba32;


        return Texture::Create(
            mipmap_image.release(),
            RadeonRays::int3(total_width, total_height, 1),
            format);
    }

    void MipGenerationTestHelper()
    {
        auto& clw_scene = m_controller->GetCachedScene(m_scene);

        auto textures_num = clw_scene.textures.GetElementCount();
        auto mipmap_num = clw_scene.mip_levels.GetElementCount();
        auto texturedata_size = clw_scene.texturedata.GetElementCount();

        std::unique_ptr<ClwScene::Texture[]> textures(new ClwScene::Texture[textures_num]);
        std::unique_ptr<char[]> texturedata(new char[texturedata_size]);
        std::unique_ptr<ClwScene::MipLevel[]> mip_levels(new ClwScene::MipLevel[mipmap_num]);

        m_context->ReadBuffer<ClwScene::Texture>(0, clw_scene.textures, textures.get(), textures_num).Wait();
        m_context->ReadBuffer<ClwScene::MipLevel>(0, clw_scene.mip_levels, mip_levels.get(), mipmap_num).Wait();
        m_context->ReadBuffer<char>(0, clw_scene.texturedata, texturedata.get(), texturedata_size).Wait();

        auto image_io = ImageIo::CreateImageIo();

        for (std::size_t i = 0, mip_index = 0; i < textures_num; i++)
        {
            auto texture = textures[i];
            if (texture.mip_count <= 1)
            {
                continue;
            }

            auto sg_texture = CreateMipmapAtlas(
                texture,
                mip_levels.get(),
                texturedata.get());

            // Save image
            {
                std::ostringstream oss;
                oss << test_name() << "_" << mip_index << ".bmp";

                std::string path = m_generate ? m_reference_path : m_output_path;

                image_io->SaveImage(path + oss.str(), sg_texture);
                ASSERT_TRUE(CompareToReference(oss.str()));
            }

            ++mip_index;
        }
    }

};


TEST_F(MipmapTest, Mipmap_Generation8bit)
{
    auto image_io = ImageIo::CreateImageIo();
    auto texture = image_io->LoadImage("../Resources/Textures/test_albedo1.jpg", true);

    auto material = Baikal::UberV2Material::Create();
    material->SetLayers(Baikal::UberV2Material::Layers::kDiffuseLayer);
    auto material_texture = Baikal::InputMap_Sampler::Create(texture);
    material->SetInputValue("uberv2.diffuse.color", material_texture);

    ApplyMaterialToObject("sphere", material);

    ASSERT_NO_THROW(m_controller->CompileScene(m_scene));

    MipGenerationTestHelper();

}

TEST_F(MipmapTest, Mipmap_Generation32bit)
{
    auto image_io = ImageIo::CreateImageIo();
    auto texture = image_io->LoadImage("../Resources/Textures/checkerboard.png", true);

    auto material = Baikal::UberV2Material::Create();
    material->SetLayers(Baikal::UberV2Material::Layers::kDiffuseLayer);
    auto material_texture = Baikal::InputMap_Sampler::Create(texture);
    material->SetInputValue("uberv2.diffuse.color", material_texture);

    ApplyMaterialToObject("sphere", material);

    ASSERT_NO_THROW(m_controller->CompileScene(m_scene));

    MipGenerationTestHelper();

}

TEST_F(MipmapTest, Mipmap_PrimaryRayLookup)
{
    m_camera->LookAt(
        RadeonRays::float3(0.f, 2.f, -10.f),
        RadeonRays::float3(0.f, 2.f, 0.f),
        RadeonRays::float3(0.f, 1.f, 0.f));

    ClearOutput();

    auto image_io = ImageIo::CreateImageIo();
    auto texture = image_io->LoadImage("../Resources/Textures/debug_miplevel.dds", true);

    auto material = Baikal::UberV2Material::Create();
    material->SetLayers(Baikal::UberV2Material::Layers::kDiffuseLayer);
    auto material_texture = Baikal::InputMap_Pow::Create(
        Baikal::InputMap_Sampler::Create(texture),
        Baikal::InputMap_ConstantFloat::Create(2.2f));
    material->SetInputValue("uberv2.diffuse.color", material_texture);

    ApplyMaterialToObject("sphere", material);
    ApplyMaterialToObject("quad", material);

    {
        ASSERT_NO_THROW(m_controller->CompileScene(m_scene));

        auto& scene = m_controller->GetCachedScene(m_scene);

        for (auto i = 0u; i < kNumIterations; ++i)
        {
            ASSERT_NO_THROW(m_renderer->Render(scene));
        }

        {
            std::ostringstream oss;
            oss << test_name() << ".png";
            SaveOutput(oss.str());
            ASSERT_TRUE(CompareToReference(oss.str()));
        }
    }

}

TEST_F(MipmapTest, Mipmap_ReflectionLookup)
{
    m_camera->LookAt(
        RadeonRays::float3(0.f, 2.f, -10.f),
        RadeonRays::float3(0.f, 2.f, 0.f),
        RadeonRays::float3(0.f, 1.f, 0.f));

    ClearOutput();

    auto image_io = ImageIo::CreateImageIo();
    auto texture = image_io->LoadImage("../Resources/Textures/debug_miplevel.dds", true);

    auto diffuse_material = Baikal::UberV2Material::Create();
    diffuse_material->SetLayers(Baikal::UberV2Material::Layers::kDiffuseLayer);
    auto material_texture = Baikal::InputMap_Pow::Create(
        Baikal::InputMap_Sampler::Create(texture),
        Baikal::InputMap_ConstantFloat::Create(2.2f));
    diffuse_material->SetInputValue("uberv2.diffuse.color", material_texture);

    auto reflection_material = Baikal::UberV2Material::Create();
    reflection_material->SetLayers(Baikal::UberV2Material::Layers::kReflectionLayer);
    reflection_material->SetInputValue("uberv2.reflection.roughness", Baikal::InputMap_ConstantFloat::Create(0.0f));

    ApplyMaterialToObject("sphere", reflection_material);
    ApplyMaterialToObject("quad", diffuse_material);

    {
        ASSERT_NO_THROW(m_controller->CompileScene(m_scene));

        auto& scene = m_controller->GetCachedScene(m_scene);

        for (auto i = 0u; i < kNumIterations; ++i)
        {
            ASSERT_NO_THROW(m_renderer->Render(scene));
        }

        {
            std::ostringstream oss;
            oss << test_name() << ".png";
            SaveOutput(oss.str());
            ASSERT_TRUE(CompareToReference(oss.str()));
        }
    }

}

TEST_F(MipmapTest, Mipmap_RefractionLookup)
{
    m_camera->LookAt(
        RadeonRays::float3(0.f, 2.f, -10.f),
        RadeonRays::float3(0.f, 2.f, 0.f),
        RadeonRays::float3(0.f, 1.f, 0.f));

    ClearOutput();

    auto image_io = ImageIo::CreateImageIo();
    auto texture = image_io->LoadImage("../Resources/Textures/debug_miplevel.dds", true);

    auto diffuse_material = Baikal::UberV2Material::Create();
    diffuse_material->SetLayers(Baikal::UberV2Material::Layers::kDiffuseLayer);
    auto material_texture = Baikal::InputMap_Pow::Create(
        Baikal::InputMap_Sampler::Create(texture),
        Baikal::InputMap_ConstantFloat::Create(2.2f));
    diffuse_material->SetInputValue("uberv2.diffuse.color", material_texture);

    auto reflection_material = Baikal::UberV2Material::Create();
    reflection_material->SetLayers(Baikal::UberV2Material::Layers::kRefractionLayer);
    reflection_material->SetInputValue("uberv2.refraction.roughness", Baikal::InputMap_ConstantFloat::Create(0.0f));
    reflection_material->SetInputValue("uberv2.refraction.ior", Baikal::InputMap_ConstantFloat::Create(5.f));

    ApplyMaterialToObject("sphere", reflection_material);
    ApplyMaterialToObject("quad", diffuse_material);

    {
        ASSERT_NO_THROW(m_controller->CompileScene(m_scene));

        auto& scene = m_controller->GetCachedScene(m_scene);

        for (auto i = 0u; i < kNumIterations; ++i)
        {
            ASSERT_NO_THROW(m_renderer->Render(scene));
        }

        {
            std::ostringstream oss;
            oss << test_name() << ".png";
            SaveOutput(oss.str());
            ASSERT_TRUE(CompareToReference(oss.str()));
        }
    }

}
