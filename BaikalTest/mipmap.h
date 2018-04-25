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
#include "SceneGraph/IO/image_io.h"

using namespace Baikal;

class MipmapTest : public BasicTest
{
    void LoadTestScene() override
    {
        auto io = SceneIo::CreateSceneIoTest();
        m_scene = io->LoadScene("sphere+plane+ibl", "");
    }
};

TEST_F(MipmapTest, SingleMipPyramid)
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
    //std::unique_ptr<char[]> texturedata(new char[texturedata_size]);
    std::unique_ptr<char> texturedata (new char[texturedata_size]);
    std::unique_ptr<ClwScene::MipmapPyramid[]> mipmaps(new ClwScene::MipmapPyramid[mipmap_num]);

    m_context->ReadBuffer<ClwScene::Texture>(0, clw_scene.textures, textures.get(), textures_num).Wait();
    m_context->ReadBuffer<char>(0, clw_scene.texturedata, texturedata.get(), texturedata_size).Wait();
    m_context->ReadBuffer<ClwScene::MipmapPyramid>(0, clw_scene.mipmap, mipmaps.get(), mipmap_num).Wait();

    auto mipmap = mipmaps[0];

    for (int i = 0; i < textures_num; i++)
    {
        auto texture = textures[i];
        if (texture.mipmap_enabled)
        {
            int size = 0;
            auto mipmap_info = mipmaps[texture.mipmap_index];
            for (int j = 0; j < mipmap_info.level_num; j++)
            {
                size += mipmap_info.level_info[j].offset;
            }
            // add size of the laset level
            size += mipmap_info.level_info[mipmap_info.level_num - 1].pitch;
            std::unique_ptr<char> mipmap_image(new char[size]);
            memcpy(
                mipmap_image.get(),
                texturedata.get() + textures[i].dataoffset,
                size);
        }
    }

    //image_io->SaveImage("E://Test//texture1111.jpg", texture);
}