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

using namespace Baikal;

class MipmapTest : public BasicTest
{
    void LoadTestScene() override
    {
        m_scene = Baikal::SceneIo::LoadScene("sphere+plane+ibl.test", "");
    }
};

void DrawMipmap(
    char * dst_buf,
    std::uint32_t dst_width, std::uint32_t dst_height, std::uint32_t dst_pitch,
    const char *src_buf,
    const ClwScene::MipmapPyramid &mipmaps)
{
    const auto& level_info = mipmaps.level_info;

    auto src_row = src_buf + mipmaps.level_info[0].offset;
    auto dst_row = dst_buf;

    // draw first level
    for (int y = 0; y < level_info[0].height; y++)
    {
        memcpy(dst_row, src_row, level_info[0].pitch);
        src_row += level_info[0].pitch;
        dst_row += dst_pitch;
    }

    int dst_offset = level_info[0].pitch;
    // draw other levels
    for (int i = 1; i < mipmaps.level_num; i++)
    {
        int width = level_info[i].width;
        int height = level_info[i].height;
        int pitch = level_info[i].pitch;
        int offset = level_info[i].offset;

        auto src_row = src_buf + offset;
        auto dst_row = dst_buf + dst_offset;

        for (int y = 0; y < height; y++)
        {
            memcpy(dst_row, src_row, pitch);
            src_row += pitch;
            dst_row += dst_pitch;
        }
        dst_offset += height * dst_pitch;
    }
}

Texture::Ptr CreateTexture(
    ClwScene::Texture texture,
    char *texturedata,
    ClwScene::MipmapPyramid *mipmaps)
{
    auto mipmap = mipmaps[texture.mipmap_index];
    auto level_info = mipmap.level_info;
    int mipmap_width = level_info[0].width + level_info[1].width;
    int mipmap_pitch = level_info[0].pitch + level_info[1].pitch;

    int levels_height = 0;
    for (int i = 1; i < mipmap.level_num; i++)
    {
        levels_height += level_info[i].height;
    }
    int mipmap_height = std::max(level_info[0].height, levels_height);
    int size = mipmap_height * mipmap_pitch;

    // add size of the laset level
    std::unique_ptr<char> mipmap_image(new char[size]);
    memset(mipmap_image.get(), 0, size);

    DrawMipmap(
        mipmap_image.get(),
        mipmap_width,
        mipmap_height,
        mipmap_pitch,
        texturedata,
        mipmap);

    Texture::Format format;

    if (texture.fmt == ClwScene::RGBA8)
        format = Texture::Format::kRgba8;
    else if (texture.fmt == ClwScene::RGBA16)
        format = Texture::Format::kRgba16;
    else
        format = Texture::Format::kRgba32;


    return Texture::Create(
        mipmap_image.release(),
        RadeonRays::int3(mipmap_width, mipmap_height, 1),
        format);
}

TEST_F(MipmapTest, MipPyramid_8bit)
{
    auto image_io = ImageIo::CreateImageIo();
    std::string resource_dir = "../Resources/Textures/";

    ClearOutput();

    auto material_texture = image_io->LoadImage(
        resource_dir + std::string("test_albedo1.jpg"), true);

    auto material = SingleBxdf::Create(SingleBxdf::BxdfType::kLambert);
    material->SetInputValue("albedo", material_texture);

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
    auto mipmap_num = clw_scene.mipmap.GetElementCount();
    auto texturedata_size = clw_scene.texturedata.GetElementCount();

    std::unique_ptr<ClwScene::Texture[]> textures(new ClwScene::Texture[textures_num]);
    std::unique_ptr<char> texturedata (new char[texturedata_size]);
    std::unique_ptr<ClwScene::MipmapPyramid[]> mipmaps(new ClwScene::MipmapPyramid[mipmap_num]);

    m_context->ReadBuffer<ClwScene::Texture>(0, clw_scene.textures, textures.get(), textures_num).Wait();
    m_context->ReadBuffer<char>(0, clw_scene.texturedata, texturedata.get(), texturedata_size).Wait();
    m_context->ReadBuffer<ClwScene::MipmapPyramid>(0, clw_scene.mipmap, mipmaps.get(), mipmap_num).Wait();

    for (int i = 0; i < textures_num; i++)
    {
        auto texture = textures[i];
        if (texture.mipmap_enabled)
        {
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
}

//TEST_F(MipmapTest, MipPyramid_16bit)
//{
//    auto image_io = ImageIo::CreateImageIo();
//    std::string resource_dir = "../Resources/Textures/";
//
//    ClearOutput();
//
//    auto material_texture = image_io->LoadImage(
//        resource_dir + std::string("Soma"), true);
//
//    auto material = SingleBxdf::Create(SingleBxdf::BxdfType::kLambert);
//    material->SetInputValue("albedo", material_texture);
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
//    auto& clw_scene = m_controller->GetCachedScene(m_scene);
//
//    auto textures_num = clw_scene.textures.GetElementCount();
//    auto mipmap_num = clw_scene.mipmap.GetElementCount();
//    auto texturedata_size = clw_scene.texturedata.GetElementCount();
//
//    std::unique_ptr<ClwScene::Texture[]> textures(new ClwScene::Texture[textures_num]);
//    std::unique_ptr<char> texturedata(new char[texturedata_size]);
//    std::unique_ptr<ClwScene::MipmapPyramid[]> mipmaps(new ClwScene::MipmapPyramid[mipmap_num]);
//
//    m_context->ReadBuffer<ClwScene::Texture>(0, clw_scene.textures, textures.get(), textures_num).Wait();
//    m_context->ReadBuffer<char>(0, clw_scene.texturedata, texturedata.get(), texturedata_size).Wait();
//    m_context->ReadBuffer<ClwScene::MipmapPyramid>(0, clw_scene.mipmap, mipmaps.get(), mipmap_num).Wait();
//
//    for (int i = 0; i < textures_num; i++)
//    {
//        auto texture = textures[i];
//        if (texture.mipmap_enabled)
//        {
//            auto sg_texture = CreateTexture(
//                texture,
//                texturedata.get(),
//                mipmaps.get());
//
//            // save image
//            std::string path = m_generate ? m_reference_path : m_output_path;
//            std::string file_name = test_name();
//            file_name.append("_");
//            file_name.append(std::to_string(i));
//            file_name.append(".png");
//            path.append(file_name);
//
//            image_io->SaveImage(path, sg_texture);
//            ASSERT_TRUE(CompareToReference(file_name));
//        }
//    }
//}

TEST_F(MipmapTest, MipPyramid_32bit)
{
    auto image_io = ImageIo::CreateImageIo();
    std::string resource_dir = "../Resources/Textures/";

    ClearOutput();

    auto material_texture = image_io->LoadImage(
        resource_dir + std::string("checkerboard.png"), true);

    auto material = SingleBxdf::Create(SingleBxdf::BxdfType::kLambert);
    material->SetInputValue("albedo", material_texture);

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
    auto mipmap_num = clw_scene.mipmap.GetElementCount();
    auto texturedata_size = clw_scene.texturedata.GetElementCount();

    std::unique_ptr<ClwScene::Texture[]> textures(new ClwScene::Texture[textures_num]);
    std::unique_ptr<char> texturedata(new char[texturedata_size]);
    std::unique_ptr<ClwScene::MipmapPyramid[]> mipmaps(new ClwScene::MipmapPyramid[mipmap_num]);

    m_context->ReadBuffer<ClwScene::Texture>(0, clw_scene.textures, textures.get(), textures_num).Wait();
    m_context->ReadBuffer<char>(0, clw_scene.texturedata, texturedata.get(), texturedata_size).Wait();
    m_context->ReadBuffer<ClwScene::MipmapPyramid>(0, clw_scene.mipmap, mipmaps.get(), mipmap_num).Wait();

    for (int i = 0; i < textures_num; i++)
    {
        auto texture = textures[i];
        if (texture.mipmap_enabled)
        {
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
}