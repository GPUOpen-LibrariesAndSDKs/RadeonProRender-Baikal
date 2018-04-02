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
#include "SceneGraph/light.h"
#include "SceneGraph/shape.h"
#include "SceneGraph/material.h"
#include "image_io.h"

#define _USE_MATH_DEFINES
#include <math.h>

using namespace RadeonRays;

class MaterialTest : public BasicTest
{

protected:

    void LoadTestScene() override
    {
        m_scene = Baikal::SceneIo::LoadScene("sphere+plane+ibl.test", "");
    }

    void MaterialTestHelperFunction(
        const std::string& test_name,
        const std::vector<float> &iors,
        std::function<std::shared_ptr<Baikal::MultiBxdf>(float ior)> produce_material)
    {
        using namespace Baikal;

        m_camera->LookAt(
            RadeonRays::float3(0.f, 2.f, -10.f),
            RadeonRays::float3(0.f, 2.f, 0.f),
            RadeonRays::float3(0.f, 1.f, 0.f));

        for (auto ior : iors)
        {
            auto material = produce_material(ior);

            ApplyMaterialToObject("sphere", material);

            ClearOutput();
            
            ASSERT_NO_THROW(m_controller->CompileScene(m_scene));

            auto& scene = m_controller->GetCachedScene(m_scene);

            for (auto i = 0u; i < kNumIterations; ++i)
            {
                ASSERT_NO_THROW(m_renderer->Render(scene));
            }

            {
                std::ostringstream oss;

                if (iors.size() == 1 && iors[0] == 0)
                    oss << test_name << ".png";
                else
                    oss << test_name << "_" << ior << ".png";

                SaveOutput(oss.str());
                ASSERT_TRUE(CompareToReference(oss.str()));
            }
        }
    }
};

TEST_F(MaterialTest, Material_Diffuse)
{
    m_camera->LookAt(
        RadeonRays::float3(0.f, 2.f, -10.f),
        RadeonRays::float3(0.f, 2.f, 0.f),
        RadeonRays::float3(0.f, 1.f, 0.f));

    std::vector<RadeonRays::float3> colors =
    {
        RadeonRays::float3(0.9f, 0.2f, 0.1f),
        RadeonRays::float3(0.1f, 0.9f, 0.1f),
        RadeonRays::float3(0.3f, 0.2f, 0.8f)
    };

    std::string resource_dir = "../Resources/Textures/";
    std::string ext = ".jpg";
    std::vector<std::string> texture_names =
    {
        "test_albedo1",
        "test_albedo2",
        "test_albedo3"
    };

    for (auto& c : colors)
    {
        ClearOutput();

        auto material = Baikal::SingleBxdf::Create(Baikal::SingleBxdf::BxdfType::kLambert);
        material->SetInputValue("albedo", RadeonRays::float4(c));

        ApplyMaterialToObject("sphere", material);

        ASSERT_NO_THROW(m_controller->CompileScene(m_scene));

        auto& scene = m_controller->GetCachedScene(m_scene);

        for (auto i = 0u; i < kNumIterations; ++i)
        {
            ASSERT_NO_THROW(m_renderer->Render(scene));
        }

        {
            std::ostringstream oss;
            oss << test_name() << "_" << c.x << "_" << c.y << "_" << c.z << ".png";
            SaveOutput(oss.str());
            ASSERT_TRUE(CompareToReference(oss.str()));
        }
    }

    auto image_io = Baikal::ImageIo::CreateImageIo();

    for (auto& t : texture_names)
    {
        ClearOutput();

        auto texture = image_io->LoadImage(resource_dir + t + ext);

        auto material = Baikal::SingleBxdf::Create(Baikal::SingleBxdf::BxdfType::kLambert);
        material->SetInputValue("albedo", texture);

        ApplyMaterialToObject("sphere", material);

        ASSERT_NO_THROW(m_controller->CompileScene(m_scene));

        auto& scene = m_controller->GetCachedScene(m_scene);

        for (auto i = 0u; i < kNumIterations; ++i)
        {
            ASSERT_NO_THROW(m_renderer->Render(scene));
        }

        {
            std::ostringstream oss;
            oss << test_name() << "_" << t << ".png";
            SaveOutput(oss.str());
            ASSERT_TRUE(CompareToReference(oss.str()));
        }
    }
}

TEST_F(MaterialTest, Material_Reflect)
{
    m_camera->LookAt(
        RadeonRays::float3(0.f, 2.f, -10.f),
        RadeonRays::float3(0.f, 2.f, 0.f),
        RadeonRays::float3(0.f, 1.f, 0.f));

    std::vector<RadeonRays::float3> colors =
    {
        RadeonRays::float3(0.9f, 0.2f, 0.1f),
        RadeonRays::float3(0.1f, 0.9f, 0.1f),
        RadeonRays::float3(0.3f, 0.2f, 0.8f)
    };

    std::vector<float> iors =
    {
        0.f, 1.1f, 1.3f, 1.6f, 2.2f, 5.f
    };

    std::string resource_dir = "../Resources/Textures/";
    std::string ext = ".jpg";
    std::vector<std::string> texture_names =
    {
        "test_albedo1",
        "test_albedo2",
        "test_albedo3"
    };

    for (auto& c : colors)
    {
        for (auto& ior : iors)
        {
            ClearOutput();

            auto material = Baikal::SingleBxdf::Create(Baikal::SingleBxdf::BxdfType::kIdealReflect);
            material->SetInputValue("albedo", RadeonRays::float4(c));

            if (ior > 0.f)
            {
                material->SetInputValue("fresnel", RadeonRays::float3(1.f, 1.f, 1.f));
                material->SetInputValue("ior", RadeonRays::float3(ior, ior, ior));
            }


            ApplyMaterialToObject("sphere", material);

            ASSERT_NO_THROW(m_controller->CompileScene(m_scene));

            auto& scene = m_controller->GetCachedScene(m_scene);

            for (auto i = 0u; i < kNumIterations; ++i)
            {
                ASSERT_NO_THROW(m_renderer->Render(scene));
            }

            {
                std::ostringstream oss;
                oss << test_name() << ior <<
                    "_" << c.x << "_" << c.y << "_" << c.z << ".png";
                SaveOutput(oss.str());
                ASSERT_TRUE(CompareToReference(oss.str()));
            }
        }
    }


    auto image_io = Baikal::ImageIo::CreateImageIo();

    for (auto& t : texture_names)
    {
        for (auto& ior : iors)
        {
            ClearOutput();

            auto texture = image_io->LoadImage(resource_dir + t + ext);

            auto material = Baikal::SingleBxdf::Create(Baikal::SingleBxdf::BxdfType::kIdealReflect);
            material->SetInputValue("albedo", texture);

            if (ior > 0.f)
            {
                material->SetInputValue("fresnel", RadeonRays::float3(1.f, 1.f, 1.f));
                material->SetInputValue("ior", RadeonRays::float3(ior, ior, ior));
            }


            ApplyMaterialToObject("sphere", material);

            ASSERT_NO_THROW(m_controller->CompileScene(m_scene));
            auto& scene = m_controller->GetCachedScene(m_scene);

            for (auto i = 0u; i < kNumIterations; ++i)
            {
                ASSERT_NO_THROW(m_renderer->Render(scene));
            }

            {
                std::ostringstream oss;
                oss << test_name() << "_" << ior << "_" << t << ".png";
                SaveOutput(oss.str());
                ASSERT_TRUE(CompareToReference(oss.str()));
            }
        }
    }
}

TEST_F(MaterialTest, Material_MicrofacetGGX)
{
    m_camera->LookAt(
        RadeonRays::float3(0.f, 2.f, -10.f),
        RadeonRays::float3(0.f, 2.f, 0.f),
        RadeonRays::float3(0.f, 1.f, 0.f));

    std::vector<RadeonRays::float3> colors =
    {
        RadeonRays::float3(0.9f, 0.2f, 0.1f),
        RadeonRays::float3(0.1f, 0.9f, 0.1f),
        RadeonRays::float3(0.3f, 0.2f, 0.8f)
    };

    std::vector<float> iors =
    {
        0.f, 1.1f, 1.3f, 1.6f, 2.2f, 5.f
    };

    std::vector<float> roughnesses =
    {
        0.0001f, 0.01f, 0.1f, 0.4f
    };

    std::string resource_dir = "../Resources/Textures/";
    std::string ext = ".jpg";
    std::vector<std::string> texture_names =
    {
        "test_albedo1",
        "test_albedo2",
        "test_albedo3"
    };

    for (auto& r : roughnesses)
    {
        for (auto& c : colors)
        {
            for (auto& ior : iors)
            {
                ClearOutput();

                auto material = Baikal::SingleBxdf::Create(Baikal::SingleBxdf::BxdfType::kMicrofacetGGX);
                material->SetInputValue("albedo", RadeonRays::float4(c));

                if (ior > 0.f)
                {
                    material->SetInputValue("fresnel", RadeonRays::float3(1.f, 1.f, 1.f));
                    material->SetInputValue("ior", RadeonRays::float3(ior, ior, ior));
                }

                material->SetInputValue("roughness", RadeonRays::float4(r));

                ApplyMaterialToObject("sphere", material);

                ASSERT_NO_THROW(m_controller->CompileScene(m_scene));

                auto& scene = m_controller->GetCachedScene(m_scene);

                for (auto i = 0u; i < kNumIterations; ++i)
                {
                    ASSERT_NO_THROW(m_renderer->Render(scene));
                }

                {
                    std::ostringstream oss;
                    oss << test_name() << "_" << r << "_" << ior <<
                        "_" << c.x << "_" << c.y << "_" << c.z << ".png";
                    SaveOutput(oss.str());
                    ASSERT_TRUE(CompareToReference(oss.str()));
                }
            }
        }
    }


    auto image_io = Baikal::ImageIo::CreateImageIo();

    for (auto& r : roughnesses)
    {
        for (auto& t : texture_names)
        {
            for (auto& ior : iors)
            {
                ClearOutput();

                auto texture = image_io->LoadImage(resource_dir + t + ext);

                auto material = Baikal::SingleBxdf::Create(Baikal::SingleBxdf::BxdfType::kMicrofacetGGX);
                material->SetInputValue("albedo", texture);

                if (ior > 0.f)
                {
                    material->SetInputValue("fresnel", RadeonRays::float3(1.f, 1.f, 1.f));
                    material->SetInputValue("ior", RadeonRays::float3(ior, ior, ior));
                }

                material->SetInputValue("roughness", RadeonRays::float4(r));


                ApplyMaterialToObject("sphere", material);

                ASSERT_NO_THROW(m_controller->CompileScene(m_scene));

                auto& scene = m_controller->GetCachedScene(m_scene);

                for (auto i = 0u; i < kNumIterations; ++i)
                {
                    ASSERT_NO_THROW(m_renderer->Render(scene));
                }

                {
                    std::ostringstream oss;
                    oss << test_name() << "_" << r <<  "_" << ior << "_" << t << ".png";
                    SaveOutput(oss.str());
                    ASSERT_TRUE(CompareToReference(oss.str()));
                }
            }
        }
    }
}

TEST_F(MaterialTest, Material_MicrofacetBeckmann)
{
    m_camera->LookAt(
        RadeonRays::float3(0.f, 2.f, -10.f),
        RadeonRays::float3(0.f, 2.f, 0.f),
        RadeonRays::float3(0.f, 1.f, 0.f));

    std::vector<RadeonRays::float3> colors =
    {
        RadeonRays::float3(0.9f, 0.2f, 0.1f),
        RadeonRays::float3(0.1f, 0.9f, 0.1f),
        RadeonRays::float3(0.3f, 0.2f, 0.8f)
    };

    std::vector<float> iors =
    {
        0.f, 1.1f, 1.3f, 1.6f, 2.2f, 5.f
    };

    std::vector<float> roughnesses =
    {
        0.0001f, 0.01f, 0.1f, 0.4f
    };

    std::string resource_dir = "../Resources/Textures/";
    std::string ext = ".jpg";
    std::vector<std::string> texture_names =
    {
        "test_albedo1",
        "test_albedo2",
        "test_albedo3"
    };

    for (auto& r : roughnesses)
    {
        for (auto& c : colors)
        {
            for (auto& ior : iors)
            {
                ClearOutput();

                auto material = Baikal::SingleBxdf::Create(Baikal::SingleBxdf::BxdfType::kMicrofacetBeckmann);
                material->SetInputValue("albedo", RadeonRays::float4(c));

                if (ior > 0.f)
                {
                    material->SetInputValue("fresnel", RadeonRays::float3(1.f, 1.f, 1.f));
                    material->SetInputValue("ior", RadeonRays::float3(ior, ior, ior));
                }

                material->SetInputValue("roughness", RadeonRays::float4(r));

                ApplyMaterialToObject("sphere", material);

                ASSERT_NO_THROW(m_controller->CompileScene(m_scene));

                auto& scene = m_controller->GetCachedScene(m_scene);

                for (auto i = 0u; i < kNumIterations; ++i)
                {
                    ASSERT_NO_THROW(m_renderer->Render(scene));
                }

                {
                    std::ostringstream oss;
                    oss << test_name() << "_" << r << "_" << ior <<
                        "_" << c.x << "_" << c.y << "_" << c.z << ".png";
                    SaveOutput(oss.str());
                    ASSERT_TRUE(CompareToReference(oss.str()));
                }
            }
        }
    }


    auto image_io = Baikal::ImageIo::CreateImageIo();

    for (auto& r : roughnesses)
    {
        for (auto& t : texture_names)
        {
            for (auto& ior : iors)
            {
                ClearOutput();

                auto texture = image_io->LoadImage(resource_dir + t + ext);

                auto material = Baikal::SingleBxdf::Create(Baikal::SingleBxdf::BxdfType::kMicrofacetBeckmann);
                material->SetInputValue("albedo", texture);

                if (ior > 0.f)
                {
                    material->SetInputValue("fresnel", RadeonRays::float3(1.f, 1.f, 1.f));
                    material->SetInputValue("ior", RadeonRays::float3(ior, ior, ior));
                }

                material->SetInputValue("roughness", RadeonRays::float4(r));


                ApplyMaterialToObject("sphere", material);

                ASSERT_NO_THROW(m_controller->CompileScene(m_scene));

                auto& scene = m_controller->GetCachedScene(m_scene);

                for (auto i = 0u; i < kNumIterations; ++i)
                {
                    ASSERT_NO_THROW(m_renderer->Render(scene));
                }

                {
                    std::ostringstream oss;
                    oss << test_name() << "_" << r <<  "_" << ior << "_" << t << ".png";
                    SaveOutput(oss.str());
                    ASSERT_TRUE(CompareToReference(oss.str()));
                }
            }
        }
    }
}

TEST_F(MaterialTest, Material_Refract)
{
    m_camera->LookAt(
        RadeonRays::float3(0.f, 2.f, -10.f),
        RadeonRays::float3(0.f, 2.f, 0.f),
        RadeonRays::float3(0.f, 1.f, 0.f));

    std::vector<RadeonRays::float3> colors =
    {
        RadeonRays::float3(0.9f, 0.2f, 0.1f),
        RadeonRays::float3(0.1f, 0.9f, 0.1f),
        RadeonRays::float3(0.3f, 0.2f, 0.8f)
    };

    std::vector<float> iors =
    {
        1.1f, 1.3f, 1.6f, 2.2f, 5.f
    };

    std::string resource_dir = "../Resources/Textures/";
    std::string ext = ".jpg";
    std::vector<std::string> texture_names =
    {
        "test_albedo1",
        "test_albedo2",
        "test_albedo3"
    };

    for (auto& c : colors)
    {
        for (auto& ior : iors)
        {
            ClearOutput();

            auto material = Baikal::SingleBxdf::Create(Baikal::SingleBxdf::BxdfType::kIdealRefract);
            material->SetInputValue("albedo", RadeonRays::float4(c));
            material->SetInputValue("ior", RadeonRays::float3(ior, ior, ior));


            ApplyMaterialToObject("sphere", material);

            ASSERT_NO_THROW(m_controller->CompileScene(m_scene));

            auto& scene = m_controller->GetCachedScene(m_scene);

            for (auto i = 0u; i < kNumIterations; ++i)
            {
                ASSERT_NO_THROW(m_renderer->Render(scene));
            }

            {
                std::ostringstream oss;
                oss << test_name() << ior <<
                    "_" << c.x << "_" << c.y << "_" << c.z << ".png";
                SaveOutput(oss.str());
                ASSERT_TRUE(CompareToReference(oss.str()));
            }
        }
    }


    auto image_io = Baikal::ImageIo::CreateImageIo();

    for (auto& t : texture_names)
    {
        for (auto& ior : iors)
        {
            ClearOutput();

            auto texture = image_io->LoadImage(resource_dir + t + ext);

            auto material = Baikal::SingleBxdf::Create(Baikal::SingleBxdf::BxdfType::kIdealRefract);
            material->SetInputValue("albedo", texture);
            material->SetInputValue("ior", RadeonRays::float3(ior, ior, ior));

            ApplyMaterialToObject("sphere", material);

            ASSERT_NO_THROW(m_controller->CompileScene(m_scene));
            auto& scene = m_controller->GetCachedScene(m_scene);

            for (auto i = 0u; i < kNumIterations; ++i)
            {
                ASSERT_NO_THROW(m_renderer->Render(scene));
            }

            {
                std::ostringstream oss;
                oss << test_name()  <<  "_" << ior << "_" << t << ".png";
                SaveOutput(oss.str());
                ASSERT_TRUE(CompareToReference(oss.str()));
            }
        }
    }
}

TEST_F(MaterialTest, Material_MicrofacetRefractGGX)
{
    m_camera->LookAt(
        RadeonRays::float3(0.f, 2.f, -10.f),
        RadeonRays::float3(0.f, 2.f, 0.f),
        RadeonRays::float3(0.f, 1.f, 0.f));

    std::vector<RadeonRays::float3> colors =
    {
        RadeonRays::float3(0.9f, 0.2f, 0.1f),
        RadeonRays::float3(0.1f, 0.9f, 0.1f),
        RadeonRays::float3(0.3f, 0.2f, 0.8f)
    };

    std::vector<float> iors =
    {
        1.1f, 1.3f, 1.6f, 2.2f, 5.f
    };

    std::vector<float> roughnesses =
    {
        0.0001f, 0.01f, 0.1f, 0.4f
    };

    std::string resource_dir = "../Resources/Textures/";
    std::string ext = ".jpg";
    std::vector<std::string> texture_names =
    {
        "test_albedo1",
        "test_albedo2",
        "test_albedo3"
    };

    for (auto& r : roughnesses)
    {
        for (auto& c : colors)
        {
            for (auto& ior : iors)
            {
                ClearOutput();

                auto material = Baikal::SingleBxdf::Create(Baikal::SingleBxdf::BxdfType::kMicrofacetRefractionGGX);
                material->SetInputValue("albedo", RadeonRays::float4(c));

                material->SetInputValue("ior", RadeonRays::float3(ior, ior, ior));

                material->SetInputValue("roughness", RadeonRays::float4(r));

                ApplyMaterialToObject("sphere", material);

                ASSERT_NO_THROW(m_controller->CompileScene(m_scene));

                auto& scene = m_controller->GetCachedScene(m_scene);

                for (auto i = 0u; i < kNumIterations; ++i)
                {
                    ASSERT_NO_THROW(m_renderer->Render(scene));
                }

                {
                    std::ostringstream oss;
                    oss << test_name() << "_" << r << "_" << ior <<
                        "_" << c.x << "_" << c.y << "_" << c.z << ".png";
                    SaveOutput(oss.str());
                    ASSERT_TRUE(CompareToReference(oss.str()));
                }
            }
        }
    }


    auto image_io = Baikal::ImageIo::CreateImageIo();

    for (auto& r : roughnesses)
    {
        for (auto& t : texture_names)
        {
            for (auto& ior : iors)
            {
                ClearOutput();

                auto texture = image_io->LoadImage(resource_dir + t + ext);

                auto material = Baikal::SingleBxdf::Create(Baikal::SingleBxdf::BxdfType::kMicrofacetRefractionGGX);
                material->SetInputValue("albedo", texture);
                material->SetInputValue("ior", RadeonRays::float3(ior, ior, ior));


                material->SetInputValue("roughness", RadeonRays::float4(r));


                ApplyMaterialToObject("sphere", material);

                ASSERT_NO_THROW(m_controller->CompileScene(m_scene));

                auto& scene = m_controller->GetCachedScene(m_scene);

                for (auto i = 0u; i < kNumIterations; ++i)
                {
                    ASSERT_NO_THROW(m_renderer->Render(scene));
                }

                {
                    std::ostringstream oss;
                    oss << test_name() << "_" << r <<  "_" << ior << "_" << t << ".png";
                    SaveOutput(oss.str());
                    ASSERT_TRUE(CompareToReference(oss.str()));
                }
            }
        }
    }
}

TEST_F(MaterialTest, Material_MicrofacetRefractBeckmann)
{
    m_camera->LookAt(
        RadeonRays::float3(0.f, 2.f, -10.f),
        RadeonRays::float3(0.f, 2.f, 0.f),
        RadeonRays::float3(0.f, 1.f, 0.f));

    std::vector<RadeonRays::float3> colors =
    {
        RadeonRays::float3(0.9f, 0.2f, 0.1f),
        RadeonRays::float3(0.1f, 0.9f, 0.1f),
        RadeonRays::float3(0.3f, 0.2f, 0.8f)
    };

    std::vector<float> iors =
    {
        1.1f, 1.3f, 1.6f, 2.2f, 5.f
    };

    std::vector<float> roughnesses =
    {
        0.0001f, 0.01f, 0.1f, 0.4f
    };

    std::string resource_dir = "../Resources/Textures/";
    std::string ext = ".jpg";
    std::vector<std::string> texture_names =
    {
        "test_albedo1",
        "test_albedo2",
        "test_albedo3"
    };

    for (auto& r : roughnesses)
    {
        for (auto& c : colors)
        {
            for (auto& ior : iors)
            {
                ClearOutput();

                auto material = Baikal::SingleBxdf::Create(Baikal::SingleBxdf::BxdfType::kMicrofacetRefractionBeckmann);
                material->SetInputValue("albedo", RadeonRays::float4(c));

                material->SetInputValue("ior", RadeonRays::float3(ior, ior, ior));

                material->SetInputValue("roughness", RadeonRays::float4(r));

                ApplyMaterialToObject("sphere", material);

                ASSERT_NO_THROW(m_controller->CompileScene(m_scene));

                auto& scene = m_controller->GetCachedScene(m_scene);

                for (auto i = 0u; i < kNumIterations; ++i)
                {
                    ASSERT_NO_THROW(m_renderer->Render(scene));
                }

                {
                    std::ostringstream oss;
                    oss << test_name() << "_" << r << "_" << ior <<
                        "_" << c.x << "_" << c.y << "_" << c.z << ".png";
                    SaveOutput(oss.str());
                    ASSERT_TRUE(CompareToReference(oss.str()));
                }
            }
        }
    }


    auto image_io = Baikal::ImageIo::CreateImageIo();

    for (auto& r : roughnesses)
    {
        for (auto& t : texture_names)
        {
            for (auto& ior : iors)
            {
                ClearOutput();

                auto texture = image_io->LoadImage(resource_dir + t + ext);

                auto material = Baikal::SingleBxdf::Create(Baikal::SingleBxdf::BxdfType::kMicrofacetRefractionBeckmann);
                material->SetInputValue("albedo", texture);
                material->SetInputValue("ior", RadeonRays::float3(ior, ior, ior));


                material->SetInputValue("roughness", RadeonRays::float4(r));


                ApplyMaterialToObject("sphere", material);

                ASSERT_NO_THROW(m_controller->CompileScene(m_scene));

                auto& scene = m_controller->GetCachedScene(m_scene);

                for (auto i = 0u; i < kNumIterations; ++i)
                {
                    ASSERT_NO_THROW(m_renderer->Render(scene));
                }

                {
                    std::ostringstream oss;
                    oss << test_name() << "_" << r <<  "_" << ior << "_" << t << ".png";
                    SaveOutput(oss.str());
                    ASSERT_TRUE(CompareToReference(oss.str()));
                }
            }
        }
    }
}

TEST_F(MaterialTest, Material_Translucent)
{
    m_camera->LookAt(
        RadeonRays::float3(0.f, 2.f, -10.f),
        RadeonRays::float3(0.f, 2.f, 0.f),
        RadeonRays::float3(0.f, 1.f, 0.f));

    std::vector<RadeonRays::float3> colors =
    {
        RadeonRays::float3(0.9f, 0.2f, 0.1f),
        RadeonRays::float3(0.1f, 0.9f, 0.1f),
        RadeonRays::float3(0.3f, 0.2f, 0.8f)
    };

    std::string resource_dir = "../Resources/Textures/";
    std::string ext = ".jpg";
    std::vector<std::string> texture_names =
    {
        "test_albedo1",
        "test_albedo2",
        "test_albedo3"
    };

    for (auto& c : colors)
    {
        ClearOutput();

        auto material = Baikal::SingleBxdf::Create(Baikal::SingleBxdf::BxdfType::kTranslucent);
        material->SetInputValue("albedo", RadeonRays::float4(c));

        ApplyMaterialToObject("sphere", material);

        ASSERT_NO_THROW(m_controller->CompileScene(m_scene));

        auto& scene = m_controller->GetCachedScene(m_scene);

        for (auto i = 0u; i < kNumIterations; ++i)
        {
            ASSERT_NO_THROW(m_renderer->Render(scene));
        }

        {
            std::ostringstream oss;
            oss << test_name() << "_" << c.x << "_" << c.y << "_" << c.z << ".png";
            SaveOutput(oss.str());
            ASSERT_TRUE(CompareToReference(oss.str()));
        }
    }

    auto image_io = Baikal::ImageIo::CreateImageIo();

    for (auto& t : texture_names)
    {
        ClearOutput();

        auto texture = image_io->LoadImage(resource_dir + t + ext);

        auto material = Baikal::SingleBxdf::Create(Baikal::SingleBxdf::BxdfType::kTranslucent);
        material->SetInputValue("albedo", texture);

        ApplyMaterialToObject("sphere", material);

        ASSERT_NO_THROW(m_controller->CompileScene(m_scene));

        auto& scene = m_controller->GetCachedScene(m_scene);

        for (auto i = 0u; i < kNumIterations; ++i)
        {
            ASSERT_NO_THROW(m_renderer->Render(scene));
        }

        {
            std::ostringstream oss;
            oss << test_name() << "_" << t << ".png";
            SaveOutput(oss.str());
            ASSERT_TRUE(CompareToReference(oss.str()));
        }
    }
}

TEST_F(MaterialTest, Material_DiffuseAndMicrofacet)
{
    using namespace Baikal;

    std::vector<float> iors =
    {
        1.1f, 1.3f, 1.6f, 2.2f, 3.f
    };

    MaterialTestHelperFunction(
        test_name(),
        iors,
        [&](float ior)
        {
            using namespace Baikal;

            auto base_material = SingleBxdf::Create(SingleBxdf::BxdfType::kLambert);
            base_material->SetInputValue("albedo", RadeonRays::float3(0.9f, 0.2f, 0.1f));

            auto top_material = SingleBxdf::Create(SingleBxdf::BxdfType::kMicrofacetBeckmann);
            top_material->SetInputValue("albedo", RadeonRays::float3(0.1f, 0.9f, 0.1f));
            top_material->SetInputValue("roughness", float4(0.002f, 0.002f, 0.002f, 1.f));

            auto blended_material = MultiBxdf::Create(MultiBxdf::Type::kFresnelBlend);

            blended_material->SetInputValue("base_material", base_material);
            blended_material->SetInputValue("top_material", top_material);
            blended_material->SetInputValue("ior", float3(ior, ior, ior));
            blended_material->SetThin(false);

            return blended_material;
        });
}

TEST_F(MaterialTest, Material_DiffuseAndTransparency)
{
    using namespace Baikal;

    std::vector<float> iors =
    {
        1.1f, 1.3f, 1.6f, 2.2f, 3.f
    };

    MaterialTestHelperFunction(
        test_name(),
        iors,
        [&](float ior)
        {
            using namespace Baikal;

            auto base_material = SingleBxdf::Create(SingleBxdf::BxdfType::kLambert);
            base_material->SetInputValue("albedo", RadeonRays::float3(0.9f, 0.2f, 0.1f));

            auto top_material = SingleBxdf::Create(SingleBxdf::BxdfType::kPassthrough);
            top_material->SetInputValue("albedo", RadeonRays::float3(0.1f, 0.9f, 0.1f));
            top_material->SetInputValue("roughness", float4(0.002f, 0.002f, 0.002f, 1.f));

            auto blended_material = MultiBxdf::Create(MultiBxdf::Type::kFresnelBlend);

            blended_material->SetInputValue("base_material", base_material);
            blended_material->SetInputValue("top_material", top_material);
            blended_material->SetInputValue("ior", float3(ior, ior, ior));
            blended_material->SetThin(false);

            return blended_material;
        });
}


TEST_F(MaterialTest, Material_RefractionAndMicrofacet)
{
    using namespace Baikal;

    std::vector<float> iors =
    {
        1.1f, 1.3f, 1.6f, 2.2f, 3.f
    };

    MaterialTestHelperFunction(
        test_name(),
        iors,
        [&](float ior)
        {
            using namespace Baikal;

            auto base_material = SingleBxdf::Create(SingleBxdf::BxdfType::kIdealRefract);
            base_material->SetInputValue("albedo", RadeonRays::float3(1.f, 1.f, 1.f));
            base_material->SetInputValue("ior", float3(ior, ior, ior));

            auto top_material = SingleBxdf::Create(SingleBxdf::BxdfType::kMicrofacetBeckmann);
            top_material->SetInputValue("albedo", RadeonRays::float3(1.f, 1.f, 1.f));
            top_material->SetInputValue("roughness", float4(0.002f, 0.002f, 0.002f, 1.f));

            auto blended_material = MultiBxdf::Create(MultiBxdf::Type::kFresnelBlend);

            blended_material->SetInputValue("base_material", base_material);
            blended_material->SetInputValue("top_material", top_material);
            blended_material->SetInputValue("ior", float3(ior, ior, ior));
            blended_material->SetThin(false);

            return blended_material;
        });
}

TEST_F(MaterialTest, Material_RefractionAndDoubleMicrofacet)
{
    using namespace Baikal;

    std::vector<float> iors =
    {
        1.1f, 1.3f, 1.6f, 2.2f, 3.f
    };

    MaterialTestHelperFunction(
        test_name(),
        iors,
        [&](float ior)
        {
            auto base_material = SingleBxdf::Create(SingleBxdf::BxdfType::kIdealRefract);
            base_material->SetInputValue("albedo", RadeonRays::float3(1.f, 1.f, 1.f));
            base_material->SetInputValue("ior", float3(ior, ior, ior));

            auto top_material = SingleBxdf::Create(SingleBxdf::BxdfType::kMicrofacetBeckmann);
            top_material->SetInputValue("albedo", RadeonRays::float3(1.f, 1.f, 1.f));
            top_material->SetInputValue("roughness", float4(0.002f, 0.002f, 0.002f, 1.f));

            auto base_blend_material = MultiBxdf::Create(MultiBxdf::Type::kFresnelBlend);

            base_blend_material->SetInputValue("base_material", base_material);
            base_blend_material->SetInputValue("top_material", top_material);
            base_blend_material->SetThin(false);

            base_blend_material->SetInputValue("ior", float3(ior, ior, ior));

            top_material = SingleBxdf::Create(SingleBxdf::BxdfType::kMicrofacetGGX);
            top_material->SetInputValue("albedo", float4(.1f, 1.f, .1f, 1.f));
            top_material->SetInputValue("roughness", float4(0.002f, 0.002f, 0.002f, 1.f));

            auto blended_material = MultiBxdf::Create(Baikal::MultiBxdf::Type::kFresnelBlend);
            blended_material->SetInputValue("base_material", base_blend_material);
            blended_material->SetInputValue("top_material", top_material);
            blended_material->SetInputValue("ior", float3(ior, ior, ior));
            return blended_material;
        });
}


TEST_F(MaterialTest, Material_MixRefractAndMicrofacet)
{
    using namespace Baikal;

    std::vector<float> iors =
    {
        1.1f, 1.3f, 1.6f, 2.2f, 3.f
    };

    auto compute_mix_material = 
        [&](float ior, float mix_weight)
        {
            auto base_material = SingleBxdf::Create(SingleBxdf::BxdfType::kIdealRefract);
            base_material->SetInputValue("albedo", RadeonRays::float3(1.f, 1.f, 1.f));
            base_material->SetInputValue("ior", float3(ior, ior, ior));

            auto top_material = SingleBxdf::Create(SingleBxdf::BxdfType::kMicrofacetBeckmann);
            top_material->SetInputValue("albedo", RadeonRays::float3(1.f, .2f, .1f));
            top_material->SetInputValue("roughness", float4(0.002f, 0.002f, 0.002f, 1.f));

            auto mix_material = MultiBxdf::Create(MultiBxdf::Type::kMix);

            mix_material->SetInputValue("weight", RadeonRays::float4(mix_weight));
            mix_material->SetInputValue("base_material", base_material);
            mix_material->SetInputValue("top_material", top_material);

            return mix_material;
        };

    MaterialTestHelperFunction(
        test_name() + "_0.2",
        iors,
        [&](float ior)
        {
            return compute_mix_material(ior, .2f);
        });

    MaterialTestHelperFunction(
        test_name() + "_0.5",
        iors,
        [&](float ior)
        {
            return compute_mix_material(ior, .5f);
        });

    MaterialTestHelperFunction(
        test_name() + "_0.8",
        iors,
        [&](float ior)
        {
            return compute_mix_material(ior, .8f);
        });
}

TEST_F(MaterialTest, Material_MixDiffuseAndMicrofacet)
{
    using namespace Baikal;

    std::vector<float> iors = { .0f };

    auto compute_mix_material =
        [&](float mix_weight)
        {
            auto base_material = SingleBxdf::Create(SingleBxdf::BxdfType::kLambert);
            base_material->SetInputValue("albedo", RadeonRays::float3(1.f, 1.f, 1.f));

            auto top_material = SingleBxdf::Create(SingleBxdf::BxdfType::kMicrofacetBeckmann);
            top_material->SetInputValue("albedo", RadeonRays::float3(1.f, .2f, .1f));
            top_material->SetInputValue("roughness", float4(0.002f, 0.002f, 0.002f, 1.f));

            auto mix_material = MultiBxdf::Create(MultiBxdf::Type::kMix);

            mix_material->SetInputValue("weight", RadeonRays::float4(mix_weight));
            mix_material->SetInputValue("base_material", base_material);
            mix_material->SetInputValue("top_material", top_material);

            return mix_material;
        };

    MaterialTestHelperFunction(
        test_name() + "_0.2",
        iors,
        [&](float ior)
    {
        (void) ior; //unused
        return compute_mix_material(.2f);
    });

    MaterialTestHelperFunction(
        test_name() + "_0.5",
        iors,
        [&](float ior)
    {
        (void)ior; //unused
        return compute_mix_material(.5f);
    });

    MaterialTestHelperFunction(
        test_name() + "_0.8",
        iors,
        [&](float ior)
    {
        (void)ior; //unused
        return compute_mix_material(.8f);
    });
}

TEST_F(MaterialTest, Material_MixDiffuseAndTransparencyMask)
{
    using namespace Baikal;

    std::vector<float> iors = { .0f };

    MaterialTestHelperFunction(
        test_name(),
        iors,
        [&](float ior)
        {
            (void)ior; // unused;

            auto base_material = SingleBxdf::Create(SingleBxdf::BxdfType::kLambert);
            base_material->SetInputValue("albedo", float4(1.f, 1.f, 1.f, 1.f));

            auto top_material = SingleBxdf::Create(SingleBxdf::BxdfType::kPassthrough);
            top_material->SetInputValue("albedo", float4(.5f, 1.f, 8.f, 1.f));

            auto mixed_material = MultiBxdf::Create(Baikal::MultiBxdf::Type::kMix);

            auto image_io(Baikal::ImageIo::CreateImageIo());
            auto texture = image_io->LoadImage("../Resources/Textures/test_albedo3.jpg");

            mixed_material->SetInputValue("weight", texture);
            mixed_material->SetInputValue("base_material", base_material);
            mixed_material->SetInputValue("top_material", top_material);

            return mixed_material;
        });
}

TEST_F(MaterialTest, Material_VolumeScattering)
{
    using namespace Baikal;

    m_camera->LookAt(
        RadeonRays::float3(0.f, 2.f, -10.f),
        RadeonRays::float3(0.f, 2.f, 0.f),
        RadeonRays::float3(0.f, 1.f, 0.f));

    std::vector<RadeonRays::float3> scattering =
    {
        RadeonRays::float3(0.9f, 0.2f, 0.1f),
        RadeonRays::float3(0.1f, 0.9f, 0.1f),
        RadeonRays::float3(0.3f, 0.2f, 0.8f)
    };

    for (auto scatter : scattering)
    {
        ClearOutput();

        auto material = SingleBxdf::Create(SingleBxdf::BxdfType::kPassthrough);
        auto volume = VolumeMaterial::Create();

        volume->SetInputValue("absorption", RadeonRays::float4(.1f, .1f, .1f, .1f));
        volume->SetInputValue("scattering", scatter);
        volume->SetInputValue("emission", RadeonRays::float4(.0f, .0f, .0f, .0f));
        volume->SetInputValue("g", RadeonRays::float4(.0f, .0f, .0f, .0f));

        for (auto iter = m_scene->CreateShapeIterator();
            iter->IsValid();
            iter->Next())
        {
            auto mesh = iter->ItemAs<Mesh>();
            if (mesh->GetName() == "sphere")
            {
                mesh->SetMaterial(material);
                mesh->SetVolumeMaterial(volume);
            }
        }

        ASSERT_NO_THROW(m_controller->CompileScene(m_scene));

        auto& scene = m_controller->GetCachedScene(m_scene);

        for (auto i = 0u; i < kNumIterations; ++i)
        {
            ASSERT_NO_THROW(m_renderer->Render(scene));
        }

        {
            std::ostringstream oss;
            oss << test_name() << "_" << scatter.x << "_" << scatter.y << "_" << scatter.z << ".png";
            SaveOutput(oss.str());
            ASSERT_TRUE(CompareToReference(oss.str()));
        }
    }
}

TEST_F(MaterialTest, Material_VolumeAbsorption)
{
    using namespace Baikal;

    m_camera->LookAt(
        RadeonRays::float3(0.f, 2.f, -10.f),
        RadeonRays::float3(0.f, 2.f, 0.f),
        RadeonRays::float3(0.f, 1.f, 0.f));

    std::vector<RadeonRays::float3> absorptions =
    {
        RadeonRays::float3(0.9f, 0.2f, 0.1f),
        RadeonRays::float3(1.1f, 1.9f, 0.1f),
        RadeonRays::float3(0.3f, 1.2f, 1.8f)
    };

    for (auto absorption : absorptions)
    {
        ClearOutput();

        auto material = SingleBxdf::Create(SingleBxdf::BxdfType::kPassthrough);
        auto volume = VolumeMaterial::Create();

        volume->SetInputValue("absorption", absorption);
        volume->SetInputValue("scattering", RadeonRays::float4(.1f, .1f, .1f, .1f));
        volume->SetInputValue("emission", RadeonRays::float4(.0f, .0f, .0f, .0f));
        volume->SetInputValue("g", RadeonRays::float4(.0f, .0f, .0f, .0f));

        for (auto iter = m_scene->CreateShapeIterator();
            iter->IsValid();
            iter->Next())
        {
            auto mesh = iter->ItemAs<Mesh>();
            if (mesh->GetName() == "sphere")
            {
                mesh->SetMaterial(material);
                mesh->SetVolumeMaterial(volume);
            }
        }

        ASSERT_NO_THROW(m_controller->CompileScene(m_scene));

        auto& scene = m_controller->GetCachedScene(m_scene);

        for (auto i = 0u; i < kNumIterations; ++i)
        {
            ASSERT_NO_THROW(m_renderer->Render(scene));
        }

        {
            std::ostringstream oss;
            oss << test_name() << "_" << absorption.x << "_" << absorption.y << "_" << absorption.z << ".png";
            SaveOutput(oss.str());
            ASSERT_TRUE(CompareToReference(oss.str()));
        }
    }
}

TEST_F(MaterialTest, Material_VolumeEmission)
{
    using namespace Baikal;

    m_camera->LookAt(
        RadeonRays::float3(0.f, 2.f, -10.f),
        RadeonRays::float3(0.f, 2.f, 0.f),
        RadeonRays::float3(0.f, 1.f, 0.f));

    std::vector<RadeonRays::float3> emissions =
    {
        RadeonRays::float3(0.9f, 0.2f, 0.1f),
        RadeonRays::float3(1.1f, 1.9f, 0.1f),
        RadeonRays::float3(0.3f, 1.2f, 1.8f)
    };

    for (auto emission : emissions)
    {
        ClearOutput();

        auto material = SingleBxdf::Create(SingleBxdf::BxdfType::kPassthrough);
        auto volume = VolumeMaterial::Create();

        volume->SetInputValue("absorption", RadeonRays::float4(.1f, .1f, .1f, .1f));
        volume->SetInputValue("scattering", RadeonRays::float4(.1f, .1f, .1f, .1f));
        volume->SetInputValue("emission", emission);
        volume->SetInputValue("g", RadeonRays::float4(.0f, .0f, .0f, .0f));

        for (auto iter = m_scene->CreateShapeIterator();
            iter->IsValid();
            iter->Next())
        {
            auto mesh = iter->ItemAs<Mesh>();
            if (mesh->GetName() == "sphere")
            {
                mesh->SetMaterial(material);
                mesh->SetVolumeMaterial(volume);
            }
        }

        ASSERT_NO_THROW(m_controller->CompileScene(m_scene));

        auto& scene = m_controller->GetCachedScene(m_scene);

        for (auto i = 0u; i < kNumIterations; ++i)
        {
            ASSERT_NO_THROW(m_renderer->Render(scene));
        }

        {
            std::ostringstream oss;
            oss << test_name() << "_" << emission.x << "_" << emission.y << "_" << emission.z << ".png";
            SaveOutput(oss.str());
            ASSERT_TRUE(CompareToReference(oss.str()));
        }
    }
}

TEST_F(MaterialTest, Material_PhaseFunction)
{
    using namespace Baikal;

    m_camera->LookAt(
        RadeonRays::float3(0.f, 2.f, -10.f),
        RadeonRays::float3(0.f, 2.f, 0.f),
        RadeonRays::float3(0.f, 1.f, 0.f));

    std::vector<RadeonRays::float3> gs =
    {
        RadeonRays::float3(0.f),
        RadeonRays::float3(-0.5f),
        RadeonRays::float3(0.5f),
        RadeonRays::float3(-1.f),
        RadeonRays::float3(1.f),
    };

    for (auto g : gs)
    {
        ClearOutput();

        auto material = SingleBxdf::Create(SingleBxdf::BxdfType::kPassthrough);
        auto volume = VolumeMaterial::Create();

        volume->SetInputValue("absorption", RadeonRays::float4(.1f, .1f, .1f, .1f));
        volume->SetInputValue("scattering", RadeonRays::float4(.1f, .1f, .1f, .1f));
        volume->SetInputValue("emission", RadeonRays::float4(.0f, .0f, .0f, .0f));
        volume->SetInputValue("g", g);

        for (auto iter = m_scene->CreateShapeIterator();
            iter->IsValid();
            iter->Next())
        {
            auto mesh = iter->ItemAs<Mesh>();
            if (mesh->GetName() == "sphere")
            {
                mesh->SetMaterial(material);
                mesh->SetVolumeMaterial(volume);
            }
        }

        ASSERT_NO_THROW(m_controller->CompileScene(m_scene));

        auto& scene = m_controller->GetCachedScene(m_scene);

        for (auto i = 0u; i < kNumIterations; ++i)
        {
            ASSERT_NO_THROW(m_renderer->Render(scene));
        }

        {
            std::ostringstream oss;
            oss << test_name() << "_" << g.x << ".png";
            SaveOutput(oss.str());
            ASSERT_TRUE(CompareToReference(oss.str()));
        }
    }
}

