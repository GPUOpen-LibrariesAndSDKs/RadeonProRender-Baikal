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

class TestScenesTest : public BasicTest
{
};

TEST_F(TestScenesTest, TestScenes_CornellBox)
{
    auto io = Baikal::SceneIo::CreateSceneIoObj();
    m_scene = io->LoadScene("../Resources/CornellBox/orig.objm",
                            "../Resources/CornellBox/");
    
    m_camera = Baikal::PerspectiveCamera::Create(
         RadeonRays::float3(0.f, 1.f, 3.f),
         RadeonRays::float3(0.f, 1.f, 0.f),
         RadeonRays::float3(0.f, 1.f, 0.f));
    
    m_camera->SetSensorSize(RadeonRays::float2(0.036f, 0.036f));
    m_camera->SetDepthRange(RadeonRays::float2(0.0f, 100000.f));
    m_camera->SetFocalLength(0.035f);
    m_camera->SetFocusDistance(1.f);
    m_camera->SetAperture(0.f);
    
    m_scene->SetCamera(m_camera);
    
    ClearOutput();
    ASSERT_NO_THROW(m_controller->CompileScene(m_scene));
    
    auto& scene = m_controller->GetCachedScene(m_scene);
    
    for (auto i = 0u; i < kNumIterations; ++i)
    {
        ASSERT_NO_THROW(m_renderer->Render(scene));
    }
    
    std::ostringstream oss;
    oss << test_name() << ".png";
    SaveOutput(oss.str());
    ASSERT_TRUE(CompareToReference(oss.str()));
}


