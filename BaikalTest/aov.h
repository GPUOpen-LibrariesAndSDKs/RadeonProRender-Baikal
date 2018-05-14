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

class AovTest : public BasicTest
{   };

TEST_F(AovTest, Aov_WorldPosition)
{
    auto output_ws = m_factory->CreateOutput(
        m_output->width(), m_output->height()
    );
    
    m_renderer->SetOutput(Baikal::Renderer::OutputType::kWorldPosition,
                         output_ws.get());
    
    ClearOutput(output_ws.get());
    ASSERT_NO_THROW(m_controller->CompileScene(m_scene));
        
    auto& scene = m_controller->GetCachedScene(m_scene);
        
    for (auto i = 0u; i < kNumIterations; ++i)
    {
        ASSERT_NO_THROW(m_renderer->Render(scene));
    }
        
    std::ostringstream oss;
    oss << test_name() << ".png";
    SaveOutput(oss.str(), output_ws.get());
    ASSERT_TRUE(CompareToReference(oss.str()));
}

TEST_F(AovTest, Aov_WorldNormal)
{
    auto output_ws = m_factory->CreateOutput(
         m_output->width(), m_output->height()
    );
    
    m_renderer->SetOutput(Baikal::Renderer::OutputType::kWorldGeometricNormal,
                          output_ws.get());
    
    ClearOutput(output_ws.get());
    ASSERT_NO_THROW(m_controller->CompileScene(m_scene));
    
    auto& scene = m_controller->GetCachedScene(m_scene);
    
    for (auto i = 0u; i < kNumIterations; ++i)
    {
        ASSERT_NO_THROW(m_renderer->Render(scene));
    }
    
    std::ostringstream oss;
    oss << test_name() << ".png";
    SaveOutput(oss.str(), output_ws.get());
    ASSERT_TRUE(CompareToReference(oss.str()));
}

TEST_F(AovTest, Aov_ShadingNormal)
{
    auto output_ws = m_factory->CreateOutput(
         m_output->width(), m_output->height()
    );
    
    m_renderer->SetOutput(Baikal::Renderer::OutputType::kWorldShadingNormal,
                          output_ws.get());
    
    ClearOutput(output_ws.get());
    ASSERT_NO_THROW(m_controller->CompileScene(m_scene));

    auto& scene = m_controller->GetCachedScene(m_scene);
    
    for (auto i = 0u; i < kNumIterations; ++i)
    {
        ASSERT_NO_THROW(m_renderer->Render(scene));
    }
    
    std::ostringstream oss;
    oss << test_name() << ".png";
    SaveOutput(oss.str(), output_ws.get());
    ASSERT_TRUE(CompareToReference(oss.str()));
}

TEST_F(AovTest, Aov_Tangent)
{
    auto output_ws = m_factory->CreateOutput(
         m_output->width(), m_output->height()
    );
    
    m_renderer->SetOutput(Baikal::Renderer::OutputType::kWorldTangent,
                          output_ws.get());
    
    ClearOutput(output_ws.get());
    ASSERT_NO_THROW(m_controller->CompileScene(m_scene));
    
    auto& scene = m_controller->GetCachedScene(m_scene);
    
    for (auto i = 0u; i < kNumIterations; ++i)
    {
        ASSERT_NO_THROW(m_renderer->Render(scene));
    }
    
    std::ostringstream oss;
    oss << test_name() << ".png";
    SaveOutput(oss.str(), output_ws.get());
    ASSERT_TRUE(CompareToReference(oss.str()));
}

TEST_F(AovTest, Aov_Bitangent)
{
    auto output_ws = m_factory->CreateOutput(
         m_output->width(), m_output->height()
    );
    
    m_renderer->SetOutput(Baikal::Renderer::OutputType::kWorldBitangent,
                          output_ws.get());
    
    ClearOutput(output_ws.get());
    ASSERT_NO_THROW(m_controller->CompileScene(m_scene));
    
    auto& scene = m_controller->GetCachedScene(m_scene);
    
    for (auto i = 0u; i < kNumIterations; ++i)
    {
        ASSERT_NO_THROW(m_renderer->Render(scene));
    }
    
    std::ostringstream oss;
    oss << test_name() << ".png";
    SaveOutput(oss.str(), output_ws.get());
    ASSERT_TRUE(CompareToReference(oss.str()));
}

TEST_F(AovTest, Aov_Albedo)
{
    auto output_ws = m_factory->CreateOutput(
        m_output->width(), m_output->height()
    );
    
    m_renderer->SetOutput(Baikal::Renderer::OutputType::kAlbedo,
                          output_ws.get());
    
    ClearOutput(output_ws.get());
    ASSERT_NO_THROW(m_controller->CompileScene(m_scene));
    
    auto& scene = m_controller->GetCachedScene(m_scene);
    
    for (auto i = 0u; i < kNumIterations; ++i)
    {
        ASSERT_NO_THROW(m_renderer->Render(scene));
    }
    
    std::ostringstream oss;
    oss << test_name() << ".png";
    SaveOutput(oss.str(), output_ws.get());
    ASSERT_TRUE(CompareToReference(oss.str()));
}

TEST_F(AovTest, Aov_Uv)
{
    auto output_ws = m_factory->CreateOutput(
      m_output->width(), m_output->height()
    );

    m_renderer->SetOutput(Baikal::Renderer::OutputType::kUv,
                          output_ws.get());

    ClearOutput(output_ws.get());
    ASSERT_NO_THROW(m_controller->CompileScene(m_scene));
    
    auto& scene = m_controller->GetCachedScene(m_scene);
    
    for (auto i = 0u; i < kNumIterations; ++i)
    {
        ASSERT_NO_THROW(m_renderer->Render(scene));
    }
    
    std::ostringstream oss;
    oss << test_name() << ".png";
    SaveOutput(oss.str(), output_ws.get());
    ASSERT_TRUE(CompareToReference(oss.str()));
}

TEST_F(AovTest, Aov_Background)
{
    auto output_ws = m_factory->CreateOutput(
        m_output->width(), m_output->height()
    );

    m_renderer->SetOutput(Baikal::Renderer::OutputType::kBackground,
        output_ws.get());

    std::vector<float> intensities = 
    {
        0.1f, 0.5f, 1.0f, 2.0f, 5.0f
    };

    // Get image based light
    auto light_it = m_scene->CreateLightIterator();
    auto light = std::dynamic_pointer_cast<Baikal::ImageBasedLight>(light_it->Item());
    
    for (float intensity : intensities)
    {
        light->SetMultiplier(intensity);
        ClearOutput(output_ws.get());
        ASSERT_NO_THROW(m_controller->CompileScene(m_scene));
        auto& scene = m_controller->GetCachedScene(m_scene);
                
        for (auto i = 0u; i < kNumIterations; ++i)
        {
            ASSERT_NO_THROW(m_renderer->Render(scene));
        }

        {
            std::ostringstream oss;
            oss << test_name() << "_"<< intensity <<".png";
            SaveOutput(oss.str(), output_ws.get());
            ASSERT_TRUE(CompareToReference(oss.str()));
        }
    }

    // Test background override
    auto image_io(Baikal::ImageIo::CreateImageIo());
    auto background_texture = image_io->LoadImage("../Resources/Textures/test_albedo1.jpg");
    light->SetBackgroundTexture(background_texture);
    light->SetMultiplier(1.0f);

    ClearOutput(output_ws.get());
    ASSERT_NO_THROW(m_controller->CompileScene(m_scene));
    {
        auto& scene = m_controller->GetCachedScene(m_scene);

        for (auto i = 0u; i < kNumIterations; ++i)
        {
            ASSERT_NO_THROW(m_renderer->Render(scene));
        }
    }

    {
        std::ostringstream oss;
        oss << test_name() << "_bgOverride.png";
        SaveOutput(oss.str(), output_ws.get());
        ASSERT_TRUE(CompareToReference(oss.str()));
    }

    // Test no image based light => black image
    m_scene->DetachLight(light);
    auto point_light = Baikal::PointLight::Create();
    point_light->SetPosition(float3(5.0f, 5.0f, 5.0f));
    m_scene->AttachLight(point_light);

    ClearOutput(output_ws.get());
    ASSERT_NO_THROW(m_controller->CompileScene(m_scene));
    {
        auto& scene = m_controller->GetCachedScene(m_scene);

        for (auto i = 0u; i < kNumIterations; ++i)
        {
            ASSERT_NO_THROW(m_renderer->Render(scene));
        }
    }

    {
        std::ostringstream oss;
        oss << test_name() << "_noIBL.png";
        SaveOutput(oss.str(), output_ws.get());
        ASSERT_TRUE(CompareToReference(oss.str()));
    }

    // Test background image
    m_scene->SetBackgroundImage(background_texture);

    ClearOutput(output_ws.get());
    ASSERT_NO_THROW(m_controller->CompileScene(m_scene));
    {
        auto& scene = m_controller->GetCachedScene(m_scene);

        for (auto i = 0u; i < kNumIterations; ++i)
        {
            ASSERT_NO_THROW(m_renderer->Render(scene));
        }
    }

    {
        std::ostringstream oss;
        oss << test_name() << "_BackgroundImage.png";
        SaveOutput(oss.str(), output_ws.get());
        ASSERT_TRUE(CompareToReference(oss.str()));
    }
}

TEST_F(AovTest, Aov_Visibility)
{
    auto output_ws = m_factory->CreateOutput(
        m_output->width(), m_output->height()
    );

    m_renderer->SetOutput(Baikal::Renderer::OutputType::kVisibility,
        output_ws.get());

    ClearOutput(output_ws.get());
    ASSERT_NO_THROW(m_controller->CompileScene(m_scene));

    auto& scene = m_controller->GetCachedScene(m_scene);

    for (auto i = 0u; i < kNumIterations; ++i)
    {
        ASSERT_NO_THROW(m_renderer->Render(scene));
    }

    std::ostringstream oss;
    oss << test_name() << ".png";
    SaveOutput(oss.str(), output_ws.get());
    ASSERT_TRUE(CompareToReference(oss.str()));
}
