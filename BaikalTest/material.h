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
#include "SceneGraph/IO/image_io.h"

#define _USE_MATH_DEFINES
#include <math.h>

using namespace RadeonRays;

class MaterialTest : public BasicTest
{
    void LoadSphereTestScene() override
    {
        auto io = Baikal::SceneIo::CreateSceneIoTest();
        m_scene = io->LoadScene("sphere+plane+ibl", "");
    }

protected:
    void ApplyMaterialToObject(
        std::string const& name,
        Baikal::Material* material
    )
    {
        for (auto iter = m_scene->CreateShapeIterator();
            iter->IsValid();
            iter->Next())
        {
            // TODO: fix this nasty cast
            auto mesh = const_cast<Baikal::Mesh*>(iter->ItemAs<Baikal::Mesh const>());
            if (mesh->GetName() == name)
            {
                mesh->SetMaterial(material);
            }
        }

        m_scene->AttachAutoreleaseObject(material);
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

        auto material = new Baikal::SingleBxdf(Baikal::SingleBxdf::BxdfType::kLambert);
        material->SetInputValue("albedo", c);

        ApplyMaterialToObject("sphere", material);

        ASSERT_NO_THROW(m_controller->CompileScene(*m_scene));

        auto& scene = m_controller->GetCachedScene(*m_scene);

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

        auto material = new Baikal::SingleBxdf(Baikal::SingleBxdf::BxdfType::kLambert);
        material->SetInputValue("albedo", texture);
        m_scene->AttachAutoreleaseObject(texture);

        ApplyMaterialToObject("sphere", material);

        ASSERT_NO_THROW(m_controller->CompileScene(*m_scene));

        auto& scene = m_controller->GetCachedScene(*m_scene);

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

            auto material = new Baikal::SingleBxdf(Baikal::SingleBxdf::BxdfType::kIdealReflect);
            material->SetInputValue("albedo", c);

            if (ior > 0.f)
            {
                material->SetInputValue("fresnel", RadeonRays::float3(1.f, 1.f, 1.f));
                material->SetInputValue("ior", RadeonRays::float3(ior, ior, ior));
            }


            ApplyMaterialToObject("sphere", material);

            ASSERT_NO_THROW(m_controller->CompileScene(*m_scene));

            auto& scene = m_controller->GetCachedScene(*m_scene);

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

            auto material = new Baikal::SingleBxdf(Baikal::SingleBxdf::BxdfType::kIdealReflect);
            material->SetInputValue("albedo", texture);
            m_scene->AttachAutoreleaseObject(texture);

            if (ior > 0.f)
            {
                material->SetInputValue("fresnel", RadeonRays::float3(1.f, 1.f, 1.f));
                material->SetInputValue("ior", RadeonRays::float3(ior, ior, ior));
            }


            ApplyMaterialToObject("sphere", material);

            ASSERT_NO_THROW(m_controller->CompileScene(*m_scene));

            auto& scene = m_controller->GetCachedScene(*m_scene);

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

                auto material = new Baikal::SingleBxdf(Baikal::SingleBxdf::BxdfType::kMicrofacetGGX);
                material->SetInputValue("albedo", c);

                if (ior > 0.f)
                {
                    material->SetInputValue("fresnel", RadeonRays::float3(1.f, 1.f, 1.f));
                    material->SetInputValue("ior", RadeonRays::float3(ior, ior, ior));
                }

                material->SetInputValue("roughness", r);

                ApplyMaterialToObject("sphere", material);

                ASSERT_NO_THROW(m_controller->CompileScene(*m_scene));

                auto& scene = m_controller->GetCachedScene(*m_scene);

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

                auto material = new Baikal::SingleBxdf(Baikal::SingleBxdf::BxdfType::kMicrofacetGGX);
                material->SetInputValue("albedo", texture);
                m_scene->AttachAutoreleaseObject(texture);

                if (ior > 0.f)
                {
                    material->SetInputValue("fresnel", RadeonRays::float3(1.f, 1.f, 1.f));
                    material->SetInputValue("ior", RadeonRays::float3(ior, ior, ior));
                }

                material->SetInputValue("roughness", r);


                ApplyMaterialToObject("sphere", material);

                ASSERT_NO_THROW(m_controller->CompileScene(*m_scene));

                auto& scene = m_controller->GetCachedScene(*m_scene);

                for (auto i = 0u; i < kNumIterations; ++i)
                {
                    ASSERT_NO_THROW(m_renderer->Render(scene));
                }

                {
                    std::ostringstream oss;
                    oss << test_name() << "_" << r <<  "_" << t << ".png";
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

                auto material = new Baikal::SingleBxdf(Baikal::SingleBxdf::BxdfType::kMicrofacetBeckmann);
                material->SetInputValue("albedo", c);

                if (ior > 0.f)
                {
                    material->SetInputValue("fresnel", RadeonRays::float3(1.f, 1.f, 1.f));
                    material->SetInputValue("ior", RadeonRays::float3(ior, ior, ior));
                }

                material->SetInputValue("roughness", r);

                ApplyMaterialToObject("sphere", material);

                ASSERT_NO_THROW(m_controller->CompileScene(*m_scene));

                auto& scene = m_controller->GetCachedScene(*m_scene);

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

                auto material = new Baikal::SingleBxdf(Baikal::SingleBxdf::BxdfType::kMicrofacetBeckmann);
                material->SetInputValue("albedo", texture);
                m_scene->AttachAutoreleaseObject(texture);

                if (ior > 0.f)
                {
                    material->SetInputValue("fresnel", RadeonRays::float3(1.f, 1.f, 1.f));
                    material->SetInputValue("ior", RadeonRays::float3(ior, ior, ior));
                }

                material->SetInputValue("roughness", r);


                ApplyMaterialToObject("sphere", material);

                ASSERT_NO_THROW(m_controller->CompileScene(*m_scene));

                auto& scene = m_controller->GetCachedScene(*m_scene);

                for (auto i = 0u; i < kNumIterations; ++i)
                {
                    ASSERT_NO_THROW(m_renderer->Render(scene));
                }

                {
                    std::ostringstream oss;
                    oss << test_name() << "_" << r << "_" << t << ".png";
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

            auto material = new Baikal::SingleBxdf(Baikal::SingleBxdf::BxdfType::kIdealRefract);
            material->SetInputValue("albedo", c);
            material->SetInputValue("ior", RadeonRays::float3(ior, ior, ior));


            ApplyMaterialToObject("sphere", material);

            ASSERT_NO_THROW(m_controller->CompileScene(*m_scene));

            auto& scene = m_controller->GetCachedScene(*m_scene);

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

            auto material = new Baikal::SingleBxdf(Baikal::SingleBxdf::BxdfType::kIdealRefract);
            material->SetInputValue("albedo", texture);
            m_scene->AttachAutoreleaseObject(texture);
            material->SetInputValue("ior", RadeonRays::float3(ior, ior, ior));

            ApplyMaterialToObject("sphere", material);

            ASSERT_NO_THROW(m_controller->CompileScene(*m_scene));

            auto& scene = m_controller->GetCachedScene(*m_scene);

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

                auto material = new Baikal::SingleBxdf(Baikal::SingleBxdf::BxdfType::kMicrofacetRefractionGGX);
                material->SetInputValue("albedo", c);

                material->SetInputValue("ior", RadeonRays::float3(ior, ior, ior));

                material->SetInputValue("roughness", r);

                ApplyMaterialToObject("sphere", material);

                ASSERT_NO_THROW(m_controller->CompileScene(*m_scene));

                auto& scene = m_controller->GetCachedScene(*m_scene);

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

                auto material = new Baikal::SingleBxdf(Baikal::SingleBxdf::BxdfType::kMicrofacetRefractionGGX);
                material->SetInputValue("albedo", texture);
                m_scene->AttachAutoreleaseObject(texture);
                material->SetInputValue("ior", RadeonRays::float3(ior, ior, ior));


                material->SetInputValue("roughness", r);


                ApplyMaterialToObject("sphere", material);

                ASSERT_NO_THROW(m_controller->CompileScene(*m_scene));

                auto& scene = m_controller->GetCachedScene(*m_scene);

                for (auto i = 0u; i < kNumIterations; ++i)
                {
                    ASSERT_NO_THROW(m_renderer->Render(scene));
                }

                {
                    std::ostringstream oss;
                    oss << test_name() << "_" << r << "_" << t << ".png";
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

                auto material = new Baikal::SingleBxdf(Baikal::SingleBxdf::BxdfType::kMicrofacetRefractionBeckmann);
                material->SetInputValue("albedo", c);

                material->SetInputValue("ior", RadeonRays::float3(ior, ior, ior));

                material->SetInputValue("roughness", r);

                ApplyMaterialToObject("sphere", material);

                ASSERT_NO_THROW(m_controller->CompileScene(*m_scene));

                auto& scene = m_controller->GetCachedScene(*m_scene);

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

                auto material = new Baikal::SingleBxdf(Baikal::SingleBxdf::BxdfType::kMicrofacetRefractionBeckmann);
                material->SetInputValue("albedo", texture);
                m_scene->AttachAutoreleaseObject(texture);
                material->SetInputValue("ior", RadeonRays::float3(ior, ior, ior));


                material->SetInputValue("roughness", r);


                ApplyMaterialToObject("sphere", material);

                ASSERT_NO_THROW(m_controller->CompileScene(*m_scene));

                auto& scene = m_controller->GetCachedScene(*m_scene);

                for (auto i = 0u; i < kNumIterations; ++i)
                {
                    ASSERT_NO_THROW(m_renderer->Render(scene));
                }

                {
                    std::ostringstream oss;
                    oss << test_name() << "_" << r << "_" << t << ".png";
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

        auto material = new Baikal::SingleBxdf(Baikal::SingleBxdf::BxdfType::kTranslucent);
        material->SetInputValue("albedo", c);

        ApplyMaterialToObject("sphere", material);

        ASSERT_NO_THROW(m_controller->CompileScene(*m_scene));

        auto& scene = m_controller->GetCachedScene(*m_scene);

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

        auto material = new Baikal::SingleBxdf(Baikal::SingleBxdf::BxdfType::kTranslucent);
        material->SetInputValue("albedo", texture);
        m_scene->AttachAutoreleaseObject(texture);

        ApplyMaterialToObject("sphere", material);

        ASSERT_NO_THROW(m_controller->CompileScene(*m_scene));

        auto& scene = m_controller->GetCachedScene(*m_scene);

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