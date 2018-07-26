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
    void LoadTestScene() override
    {
        m_scene = Baikal::SceneIo::LoadScene("sphere+plane+ibl.test", "");
    }
};

void DrawMipmap(
    char* dst_buf,
    int total_width,
    int total_height,
    int texel_size,
    char const* texturedata,
    ClwScene::MipLevel const* mip_levels,
    std::size_t mip_count)
{
    // draw first level
    {
        auto src_row = texturedata + mip_levels[0].dataoffset;
        auto dst_row = dst_buf;

        for (int y = 0; y < mip_levels[0].h; y++)
        {
            memcpy(dst_row, src_row, mip_levels[0].w * texel_size);
            src_row += mip_levels[0].w * texel_size;
            dst_row += total_width * texel_size;
        }
    }

    // draw other levels
    int dst_offset = mip_levels[0].w * texel_size;

    for (int i = 1; i < mip_count; i++)
    {
        int width = mip_levels[i].w;
        int height = mip_levels[i].h;

        auto src_row = texturedata + mip_levels[i].dataoffset;
        auto dst_row = dst_buf + dst_offset;

        for (int y = 0; y < height; y++)
        {
            memcpy(dst_row, src_row, width * texel_size);
            src_row += width * texel_size;
            dst_row += total_width * texel_size;
        }
        dst_offset += height * total_width * texel_size;;
    }
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

Texture::Ptr CreateTexture(
    ClwScene::Texture texture,
    char* texturedata,
    ClwScene::MipLevel* mip_levels)
{
    ClwScene::MipLevel* texture_levels = mip_levels + texture.mip_offset;

    int total_width = texture_levels[0].w + texture_levels[1].w;

    int levels_height = 0;
    for (int i = 1; i < texture.mip_count; i++)
    {
        levels_height += texture_levels[i].h;
    }
    int total_height = std::max(texture_levels[0].h, levels_height);

    // add size of the laset level
    int size_in_bytes = total_width * total_height * TexelSize(texture.fmt);
    std::unique_ptr<char> mipmap_image(new char[size_in_bytes]);

    {
        int* image_int = reinterpret_cast<int*>(mipmap_image.get());
        std::fill(image_int, image_int + size_in_bytes / sizeof(int), 0x0000FF00);
    }

    DrawMipmap(
        mipmap_image.get(),
        total_width,
        total_height,
        TexelSize(texture.fmt),
        texturedata,
        texture_levels,
        texture.mip_count);

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

TEST_F(MipmapTest, Mipmap_8bit)
{
    auto image_io = ImageIo::CreateImageIo();
    std::string resource_dir = "../Resources/Textures/";

    ClearOutput();

    auto light_texture = image_io->LoadImage("../Resources/Textures/studio015.hdr");
    auto refraction_texture = image_io->LoadImage("../Resources/Textures/sky.hdr");
    auto texture = image_io->LoadImage(
        resource_dir + std::string("test_albedo1.jpg"), true);

    auto material = Baikal::UberV2Material::Create();
    material->SetLayers(Baikal::UberV2Material::Layers::kDiffuseLayer);
    auto material_texture = Baikal::InputMap_Sampler::Create(texture);
    material->SetInputValue("uberv2.diffuse.color", material_texture);

    ApplyMaterialToObject("sphere", material);

    auto light = ImageBasedLight::Create();

    light->SetTexture(light_texture);
    light->SetRefractionTexture(refraction_texture);
    m_scene->AttachLight(light);

    ASSERT_NO_THROW(m_controller->CompileScene(m_scene));

    auto& clw_scene = m_controller->GetCachedScene(m_scene);

    auto textures_num = clw_scene.textures.GetElementCount();
    auto mipmap_num = clw_scene.mip_levels.GetElementCount();
    auto texturedata_size = clw_scene.texturedata.GetElementCount();

    std::unique_ptr<ClwScene::Texture[]> textures(new ClwScene::Texture[textures_num]);
    std::unique_ptr<char[]> texturedata (new char[texturedata_size]);
    std::unique_ptr<ClwScene::MipLevel[]> mipmaps(new ClwScene::MipLevel[mipmap_num]);

    m_context->ReadBuffer<ClwScene::Texture>(0, clw_scene.textures, textures.get(), textures_num).Wait();
    m_context->ReadBuffer<char>(0, clw_scene.texturedata, texturedata.get(), texturedata_size).Wait();
    m_context->ReadBuffer<ClwScene::MipLevel>(0, clw_scene.mip_levels, mipmaps.get(), mipmap_num).Wait();

    for (int i = 0; i < textures_num; i++)
    {
        auto texture = textures[i];
        if (texture.mip_count <= 1)
        {
            continue;
        }

        auto sg_texture = CreateTexture(
            texture,
            texturedata.get(),
            mipmaps.get());

        // save image
        std::string path = m_generate ? m_reference_path : m_output_path;
        std::string file_name = test_name();
        file_name.append("_");
        file_name.append(std::to_string(i));
        file_name.append(".jpg");
        path.append(file_name);

        image_io->SaveImage(path, sg_texture);
        ASSERT_TRUE(CompareToReference(file_name));
    }
}

TEST_F(MipmapTest, Mipmap_32bit)
{
    auto image_io = ImageIo::CreateImageIo();
    std::string resource_dir = "../Resources/Textures/";

    ClearOutput();

    auto texture = image_io->LoadImage(
        resource_dir + std::string("checkerboard.png"), true);

    auto material = Baikal::UberV2Material::Create();
    material->SetLayers(Baikal::UberV2Material::Layers::kDiffuseLayer);
    auto material_texture = Baikal::InputMap_Sampler::Create(texture);
    material->SetInputValue("uberv2.diffuse.color", material_texture);

    ApplyMaterialToObject("sphere", material);

    auto light_texture = image_io->LoadImage("../Resources/Textures/studio015.hdr");
    auto refraction_texture = image_io->LoadImage("../Resources/Textures/sky.hdr");
    auto light = ImageBasedLight::Create();

    light->SetTexture(light_texture);
    light->SetRefractionTexture(refraction_texture);
    m_scene->AttachLight(light);

    ASSERT_NO_THROW(m_controller->CompileScene(m_scene));

    auto& clw_scene = m_controller->GetCachedScene(m_scene);

    auto textures_num = clw_scene.textures.GetElementCount();
    auto mipmap_num = clw_scene.mip_levels.GetElementCount();
    auto texturedata_size = clw_scene.texturedata.GetElementCount();

    std::unique_ptr<ClwScene::Texture[]> textures(new ClwScene::Texture[textures_num]);
    std::unique_ptr<ClwScene::MipLevel[]> mipmaps(new ClwScene::MipLevel[mipmap_num]);
    std::unique_ptr<char[]> texturedata(new char[texturedata_size]);

    m_context->ReadBuffer<ClwScene::Texture>(0, clw_scene.textures, textures.get(), textures_num).Wait();
    m_context->ReadBuffer<ClwScene::MipLevel>(0, clw_scene.mip_levels, mipmaps.get(), mipmap_num).Wait();
    m_context->ReadBuffer<char>(0, clw_scene.texturedata, texturedata.get(), texturedata_size).Wait();

    for (int i = 0; i < textures_num; i++)
    {
        auto texture = textures[i];
        if (texture.mip_count <= 1)
        {
            continue;
        }

        auto sg_texture = CreateTexture(
            texture,
            texturedata.get(),
            mipmaps.get());

        // save image
        std::string path = m_generate ? m_reference_path : m_output_path;
        std::string file_name = test_name();
        file_name.append("_");
        file_name.append(std::to_string(i));
        file_name.append(".png");
        path.append(file_name);

        image_io->SaveImage(path, sg_texture);
        ASSERT_TRUE(CompareToReference(file_name));
    }
}

//TEST_F(MipmapTest, MipPyramid_TestCameraRayMipmap)
//{
//    m_camera->LookAt(
//        RadeonRays::float3(0.f, 2.f, -10.f),
//        RadeonRays::float3(0.f, 2.f, 0.f),
//        RadeonRays::float3(0.f, 1.f, 0.f));
//
//    auto output_ws = m_factory->CreateOutput(
//        m_output->width(), m_output->height()
//    );
//
//    //m_renderer->SetOutput(Baikal::Renderer::OutputType::kAlbedo,
//    //    output_ws.get());
//
//    auto image_io = ImageIo::CreateImageIo();
//    std::string resource_dir = "../Resources/Textures/";
//
//    auto texture = image_io->LoadImage(
//        resource_dir + std::string("checkerboard.png"), true);
//
//    auto material = Baikal::UberV2Material::Create();
//    material->SetLayers(Baikal::UberV2Material::Layers::kDiffuseLayer);
//    auto material_texture = Baikal::InputMap_Sampler::Create(texture);
//    material->SetInputValue("uberv2.diffuse.color", material_texture);
//
//    ApplyMaterialToObject("sphere", material);
//
//    auto light_texture = image_io->LoadImage("../Resources/Textures/studio015.hdr");
//    auto refraction_texture = image_io->LoadImage("../Resources/Textures/sky.hdr");
//    auto light = ImageBasedLight::Create();
//
//    light->SetTexture(light_texture);
//    light->SetRefractionTexture(refraction_texture);
//    m_scene->AttachLight(light);
//
//    ASSERT_NO_THROW(m_controller->CompileScene(m_scene));
//
//    auto& scene = m_controller->GetCachedScene(m_scene);
//    for (auto i = 0u; i < kNumIterations; ++i)
//    {
//        ASSERT_NO_THROW(m_renderer->Render(scene));
//    }
//
//    {
//        std::ostringstream oss;
//        oss << test_name() << "_" << ".png";
//        SaveOutput(oss.str()
//            //, output_ws.get()
//        );
//        ASSERT_TRUE(CompareToReference(oss.str()));
//    }
//}
