/**********************************************************************
 C opyright (c*) 2016 Advanced Micro Devices, Inc. All rights reserved.

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
#include "SceneGraph/uberv2material.h"
#include "SceneGraph/inputmaps.h"


#define _USE_MATH_DEFINES
#include <math.h>
#include <memory>

using namespace RadeonRays;

class InputMapsTest : public BasicTest
{

protected:

    void LoadTestScene() override
    {
        auto io = Baikal::SceneIo::CreateSceneIoTest();
        m_scene = io->LoadScene("sphere+plane+ibl", "");
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

TEST_F(InputMapsTest, InputMap_ConstFloat4)
{
    m_camera->LookAt(
        RadeonRays::float3(0.f, 2.f, -10.f),
        RadeonRays::float3(0.f, 2.f, 0.f),
        RadeonRays::float3(0.f, 1.f, 0.f));

    auto material = Baikal::UberV2Material::Create();
    auto diffuse_color = std::static_pointer_cast<Baikal::InputMap_ConstantFloat3>(Baikal::InputMap_ConstantFloat3::Create(float3(0.0f, 0.0f, 0.0f, 0.0f)));
    material->SetInputValue("uberv2.diffuse.color", diffuse_color);

    std::vector<float3> colors =
    {
        float3(1.0f, 0.0f, 0.0f),
        float3(0.0f, 1.0f, 0.0f),
        float3(0.0f, 0.0f, 1.0f)
    };

    for (auto& c : colors)
    {
        diffuse_color->m_value = c;
        ClearOutput();

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
}
