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

#define _USE_MATH_DEFINES
#include <math.h>

using namespace RadeonRays;

class LightTest: public BasicTest
{
    void LoadSphereTestScene() override
    {
        auto io = Baikal::SceneIo::CreateSceneIoTest();
        m_scene = io->LoadScene("sphere+plane", "");
    }
};

TEST_F(LightTest, PointLight)
{
    m_camera->LookAt(
        RadeonRays::float3(0.f, 2.f, -10.f), 
        RadeonRays::float3(0.f, 2.f, 0.f), 
        RadeonRays::float3(0.f, 1.f, 0.f));


    std::vector<float3> positions{
        float3(3.f, 6.f, 0.f),
        float3(-2.f, 6.f, -1.f),
        float3(0.f, 6.f, -3.f)
    };

    std::vector<float3> colors{
        float3(3.f, 0.1f, 0.1f),
        float3(0.1f, 3.f, 0.1f),
        float3(0.1f, 0.1f, 3.f)
    };

    for (auto i = 0u; i < 3; ++i)
    {
        auto light = new Baikal::PointLight();
        light->SetPosition(positions[i]);
        light->SetEmittedRadiance(colors[i]);
        m_scene->AttachLight(light);
        m_scene->AttachAutoreleaseObject(light);
    }

    ClearOutput();

    ASSERT_NO_THROW(m_renderer->CompileScene(*m_scene));

    for (auto i = 0u; i < kNumIterations; ++i)
    {
        ASSERT_NO_THROW(m_renderer->Render(*m_scene));
    }

    {
        std::ostringstream oss;
        oss << test_name() << "_1.png";
        SaveOutput(oss.str());
        ASSERT_TRUE(CompareToReference(oss.str()));
    }

    auto iter = m_scene->CreateLightIterator();
    m_scene->DetachLight(iter->ItemAs <Baikal::Light const>());
    m_scene->DetachAutoreleaseObject(iter->ItemAs<Baikal::Light const> ());

    ClearOutput();

    ASSERT_NO_THROW(m_renderer->CompileScene(*m_scene));

    for (auto i = 0u; i < kNumIterations; ++i)
    {
        ASSERT_NO_THROW(m_renderer->Render(*m_scene));
    }

    {
        std::ostringstream oss;
        oss << test_name() << "_2.png";
        SaveOutput(oss.str());
        ASSERT_TRUE(CompareToReference(oss.str()));
    }
}

TEST_F(LightTest, PointLightMany)
{
    m_camera->LookAt(
        RadeonRays::float3(0.f, 2.f, -10.f),
        RadeonRays::float3(0.f, 2.f, 0.f),
        RadeonRays::float3(0.f, 1.f, 0.f));


    auto num_lights = 16;
    std::vector<float3> positions;
    std::vector<float3> colors;

    float step = 2 * M_PI / num_lights;
    for (auto i = 0u; i < num_lights; ++i)
    {
        auto x = 5.f * std::cos(i * step);
        auto y = 5.f;
        auto z = 5.f * std::sin(i * step);
        positions.push_back(float3(x, y, z));

        auto f = (float)i / num_lights;
        auto color = f * float3(1.f, 0.f, 0.f) + (1.f - f) * float3(0.f, 1.f, 0.f);
        colors.push_back(6.f * color);
    }

    for (auto i = 0u; i < num_lights; ++i)
    {
        auto light = new Baikal::PointLight();
        light->SetPosition(positions[i]);
        light->SetEmittedRadiance(colors[i]);
        m_scene->AttachLight(light);
        m_scene->AttachAutoreleaseObject(light);
    }

    ClearOutput();

    ASSERT_NO_THROW(m_renderer->CompileScene(*m_scene));

    for (auto i = 0u; i < num_lights * kNumIterations; ++i)
    {
        ASSERT_NO_THROW(m_renderer->Render(*m_scene));
    }

    {
        std::ostringstream oss;
        oss << test_name() << ".png";
        SaveOutput(oss.str());
        ASSERT_TRUE(CompareToReference(oss.str()));
    }
}

TEST_F(LightTest, DirectionalLight)
{
    m_camera->LookAt(
        RadeonRays::float3(0.f, 2.f, -10.f),
        RadeonRays::float3(0.f, 2.f, 0.f),
        RadeonRays::float3(0.f, 1.f, 0.f));


    std::vector<float3> direction{
        float3(-1.f, -1.f, -1.f),
        float3(1.f, -1.f, -1.f),
        float3(1.f, -1.f, 1.f)
    };

    std::vector<float3> colors{
        float3(3.f, 0.1f, 0.1f),
        float3(0.1f, 3.f, 0.1f),
        float3(0.1f, 0.1f, 3.f)
    };

    for (auto i = 0u; i < 3; ++i)
    {
        auto light = new Baikal::DirectionalLight();
        light->SetDirection(direction[i]);
        light->SetEmittedRadiance(colors[i]);
        m_scene->AttachLight(light);
        m_scene->AttachAutoreleaseObject(light);
    }

    ClearOutput();

    ASSERT_NO_THROW(m_renderer->CompileScene(*m_scene));

    for (auto i = 0u; i < kNumIterations; ++i)
    {
        ASSERT_NO_THROW(m_renderer->Render(*m_scene));
    }

    {
        std::ostringstream oss;
        oss << test_name() << "_1.png";
        SaveOutput(oss.str());
        ASSERT_TRUE(CompareToReference(oss.str()));
    }

    auto iter = m_scene->CreateLightIterator();
    m_scene->DetachLight(iter->ItemAs <Baikal::Light const>());
    m_scene->DetachAutoreleaseObject(iter->ItemAs<Baikal::Light const>());

    ClearOutput();

    ASSERT_NO_THROW(m_renderer->CompileScene(*m_scene));

    for (auto i = 0u; i < kNumIterations; ++i)
    {
        ASSERT_NO_THROW(m_renderer->Render(*m_scene));
    }

    {
        std::ostringstream oss;
        oss << test_name() << "_2.png";
        SaveOutput(oss.str());
        ASSERT_TRUE(CompareToReference(oss.str()));
    }
}

TEST_F(LightTest, SpotLight)
{
    m_camera->LookAt(
        RadeonRays::float3(0.f, 2.f, -10.f),
        RadeonRays::float3(0.f, 2.f, 0.f),
        RadeonRays::float3(0.f, 1.f, 0.f));


    std::vector<float3> direction{
        float3(-1.f, -1.f, -1.f),
        float3(1.f, -1.f, -1.f),
        float3(1.f, -1.f, 1.f)
    };

    std::vector<float3> colors{
        float3(3.f, 0.1f, 0.1f),
        float3(0.1f, 3.f, 0.1f),
        float3(0.1f, 0.1f, 3.f)
    };

    std::vector<float3> positions{
        float3(3.f, 6.f, 0.f),
        float3(-2.f, 6.f, -1.f),
        float3(0.f, 6.f, -3.f)
    };

    for (auto i = 0u; i < 3; ++i)
    {
        auto light = new Baikal::SpotLight();
        light->SetPosition(positions[i]);
        light->SetDirection(direction[i]);
        light->SetEmittedRadiance(colors[i]);
        m_scene->AttachLight(light);
        m_scene->AttachAutoreleaseObject(light);
    }

    ClearOutput();

    ASSERT_NO_THROW(m_renderer->CompileScene(*m_scene));

    for (auto i = 0u; i < kNumIterations; ++i)
    {
        ASSERT_NO_THROW(m_renderer->Render(*m_scene));
    }

    {
        std::ostringstream oss;
        oss << test_name() << "_1.png";
        SaveOutput(oss.str());
        ASSERT_TRUE(CompareToReference(oss.str()));
    }

    auto iter = m_scene->CreateLightIterator();
    m_scene->DetachLight(iter->ItemAs <Baikal::Light const>());
    m_scene->DetachAutoreleaseObject(iter->ItemAs<Baikal::Light const>());

    ClearOutput();

    ASSERT_NO_THROW(m_renderer->CompileScene(*m_scene));

    for (auto i = 0u; i < kNumIterations; ++i)
    {
        ASSERT_NO_THROW(m_renderer->Render(*m_scene));
    }

    {
        std::ostringstream oss;
        oss << test_name() << "_2.png";
        SaveOutput(oss.str());
        ASSERT_TRUE(CompareToReference(oss.str()));
    }
}

TEST_F(LightTest, AreaLight)
{
    m_camera->LookAt(
        RadeonRays::float3(0.f, 2.f, -10.f),
        RadeonRays::float3(0.f, 2.f, 0.f),
        RadeonRays::float3(0.f, 1.f, 0.f));

    auto io = Baikal::SceneIo::CreateSceneIoTest();
    m_scene = io->LoadScene("sphere+plane+area", "");
    m_scene->SetCamera(m_camera.get());

    ClearOutput();

    ASSERT_NO_THROW(m_renderer->CompileScene(*m_scene));

    for (auto i = 0u; i < kNumIterations; ++i)
    {
        ASSERT_NO_THROW(m_renderer->Render(*m_scene));
    }

    {
        std::ostringstream oss;
        oss << test_name() << ".png";
        SaveOutput(oss.str());
        ASSERT_TRUE(CompareToReference(oss.str()));
    }
}


TEST_F(LightTest, DirectionalAndAreaLight)
{
    m_camera->LookAt(
        RadeonRays::float3(0.f, 2.f, -10.f),
        RadeonRays::float3(0.f, 2.f, 0.f),
        RadeonRays::float3(0.f, 1.f, 0.f));

    auto io = Baikal::SceneIo::CreateSceneIoTest();
    m_scene = io->LoadScene("sphere+plane+area", "");
    m_scene->SetCamera(m_camera.get());

    std::vector<float3> direction{
        float3(-1.f, -1.f, -1.f),
        float3(1.f, -1.f, -1.f),
        float3(1.f, -1.f, 1.f)
    };

    std::vector<float3> colors{
        float3(1.f, 0.1f, 0.1f),
        float3(0.1f, 1.f, 0.1f),
        float3(0.1f, 0.1f, 1.f)
    };

    for (auto i = 0u; i < 3; ++i)
    {
        auto light = new Baikal::DirectionalLight();
        light->SetDirection(direction[i]);
        light->SetEmittedRadiance(colors[i]);
        m_scene->AttachLight(light);
        m_scene->AttachAutoreleaseObject(light);
    }

    ClearOutput();

    ASSERT_NO_THROW(m_renderer->CompileScene(*m_scene));

    for (auto i = 0u; i < kNumIterations; ++i)
    {
        ASSERT_NO_THROW(m_renderer->Render(*m_scene));
    }

    {
        std::ostringstream oss;
        oss << test_name() << ".png";
        SaveOutput(oss.str());
        ASSERT_TRUE(CompareToReference(oss.str()));
    }
}

TEST_F(LightTest, PointAndAreaLight)
{
    m_camera->LookAt(
        RadeonRays::float3(0.f, 2.f, -10.f),
        RadeonRays::float3(0.f, 2.f, 0.f),
        RadeonRays::float3(0.f, 1.f, 0.f));

    auto io = Baikal::SceneIo::CreateSceneIoTest();
    m_scene = io->LoadScene("sphere+plane+area", "");
    m_scene->SetCamera(m_camera.get());

    std::vector<float3> positions{
        float3(3.f, 6.f, 0.f),
        float3(-2.f, 6.f, -1.f),
        float3(0.f, 6.f, -3.f)
    };

    std::vector<float3> colors{
        float3(3.f, 0.1f, 0.1f),
        float3(0.1f, 3.f, 0.1f),
        float3(0.1f, 0.1f, 3.f)
    };

    for (auto i = 0u; i < 3; ++i)
    {
        auto light = new Baikal::PointLight();
        light->SetPosition(positions[i]);
        light->SetEmittedRadiance(colors[i]);
        m_scene->AttachLight(light);
        m_scene->AttachAutoreleaseObject(light);
    }

    ClearOutput();

    ASSERT_NO_THROW(m_renderer->CompileScene(*m_scene));

    for (auto i = 0u; i < kNumIterations; ++i)
    {
        ASSERT_NO_THROW(m_renderer->Render(*m_scene));
    }

    {
        std::ostringstream oss;
        oss << test_name() << ".png";
        SaveOutput(oss.str());
        ASSERT_TRUE(CompareToReference(oss.str()));
    }
}

TEST_F(LightTest, SpotAndAreaLight)
{
    m_camera->LookAt(
        RadeonRays::float3(0.f, 2.f, -10.f),
        RadeonRays::float3(0.f, 2.f, 0.f),
        RadeonRays::float3(0.f, 1.f, 0.f));

    auto io = Baikal::SceneIo::CreateSceneIoTest();
    m_scene = io->LoadScene("sphere+plane+area", "");
    m_scene->SetCamera(m_camera.get());

    std::vector<float3> direction{
        float3(-1.f, -1.f, -1.f),
        float3(1.f, -1.f, -1.f),
        float3(1.f, -1.f, 1.f)
    };

    std::vector<float3> colors{
        float3(3.f, 0.1f, 0.1f),
        float3(0.1f, 3.f, 0.1f),
        float3(0.1f, 0.1f, 3.f)
    };

    std::vector<float3> positions{
        float3(3.f, 6.f, 0.f),
        float3(-2.f, 6.f, -1.f),
        float3(0.f, 6.f, -3.f)
    };

    for (auto i = 0u; i < 3; ++i)
    {
        auto light = new Baikal::SpotLight();
        light->SetPosition(positions[i]);
        light->SetDirection(direction[i]);
        light->SetEmittedRadiance(colors[i]);
        m_scene->AttachLight(light);
        m_scene->AttachAutoreleaseObject(light);
    }

    ClearOutput();

    ASSERT_NO_THROW(m_renderer->CompileScene(*m_scene));

    for (auto i = 0u; i < kNumIterations; ++i)
    {
        ASSERT_NO_THROW(m_renderer->Render(*m_scene));
    }

    {
        std::ostringstream oss;
        oss << test_name() << ".png";
        SaveOutput(oss.str());
        ASSERT_TRUE(CompareToReference(oss.str()));
    }
}

TEST_F(LightTest, EmissiveSphere)
{
    m_camera->LookAt(
        RadeonRays::float3(0.f, 2.f, -10.f),
        RadeonRays::float3(0.f, 2.f, 0.f),
        RadeonRays::float3(0.f, 1.f, 0.f));

    ClearOutput();

    auto io = Baikal::SceneIo::CreateSceneIoTest();
    m_scene = io->LoadScene("sphere+plane", "");
    m_scene->SetCamera(m_camera.get());

    auto emission = new Baikal::SingleBxdf(Baikal::SingleBxdf::BxdfType::kEmissive);
    emission->SetInputValue("albedo", float3(2.f, 2.f, 2.f));
    m_scene->AttachAutoreleaseObject(emission);

    auto iter = m_scene->CreateShapeIterator();

    for (; iter->IsValid(); iter->Next())
    {
        auto mesh = iter->ItemAs<Baikal::Mesh const>();
        if (mesh->GetName() == "sphere")
        {
            const_cast<Baikal::Mesh*>(mesh)->SetMaterial(emission);

            for (auto i = 0u; i < mesh->GetNumIndices() / 3; ++i)
            {
                auto light = new Baikal::AreaLight(mesh, i);
                m_scene->AttachLight(light);
                m_scene->AttachAutoreleaseObject(light);
            }
        }
    }

    ASSERT_NO_THROW(m_renderer->CompileScene(*m_scene));

    for (auto i = 0u; i < kNumIterations; ++i)
    {
        ASSERT_NO_THROW(m_renderer->Render(*m_scene));
    }

    {
        std::ostringstream oss;
        oss << test_name() << ".png";
        SaveOutput(oss.str());
        ASSERT_TRUE(CompareToReference(oss.str()));
    }
}

TEST_F(LightTest, DirectionalAndEmissiveSphere)
{
    m_camera->LookAt(
        RadeonRays::float3(0.f, 2.f, -10.f),
        RadeonRays::float3(0.f, 2.f, 0.f),
        RadeonRays::float3(0.f, 1.f, 0.f));

    auto io = Baikal::SceneIo::CreateSceneIoTest();
    m_scene = io->LoadScene("sphere+plane", "");
    m_scene->SetCamera(m_camera.get());

    auto light = new Baikal::DirectionalLight();
    light->SetDirection(float3(-0.5, -1.f, -0.5f));
    light->SetEmittedRadiance(5.f * float3(1.f, 0.f, 0.f));
    m_scene->AttachLight(light);
    m_scene->AttachAutoreleaseObject(light);

    auto emission = new Baikal::SingleBxdf(Baikal::SingleBxdf::BxdfType::kEmissive);
    emission->SetInputValue("albedo", float3(2.f, 2.f, 2.f));
    m_scene->AttachAutoreleaseObject(emission);

    auto iter = m_scene->CreateShapeIterator();

    for (; iter->IsValid(); iter->Next())
    {
        auto mesh = iter->ItemAs<Baikal::Mesh const>();
        if (mesh->GetName() == "sphere")
        {
            const_cast<Baikal::Mesh*>(mesh)->SetMaterial(emission);

            for (auto i = 0u; i < mesh->GetNumIndices() / 3; ++i)
            {
                auto light = new Baikal::AreaLight(mesh, i);
                m_scene->AttachLight(light);
                m_scene->AttachAutoreleaseObject(light);
            }
        }
    }

    ClearOutput();

    ASSERT_NO_THROW(m_renderer->CompileScene(*m_scene));

    for (auto i = 0u; i < kNumIterations; ++i)
    {
        ASSERT_NO_THROW(m_renderer->Render(*m_scene));
    }

    {
        std::ostringstream oss;
        oss << test_name() << ".png";
        SaveOutput(oss.str());
        ASSERT_TRUE(CompareToReference(oss.str()));
    }
}