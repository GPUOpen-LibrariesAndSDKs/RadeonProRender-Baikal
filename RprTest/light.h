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

class LightTest : public BasicTest
{
public:
    virtual void SetUp() override
    {
        BasicTest::SetUp();
        CreateScene(SceneType::kSphereAndPlane);
    }
};

TEST_F(LightTest, Light_PointLight)
{
    std::vector<float3> positions
    {
        float3( 3.0f, 6.0f,  0.0f),
        float3(-2.0f, 6.0f, -1.0f),
        float3( 0.0f, 6.0f, -3.0f)
    };

    std::vector<float3> colors
    {
        float3(3.0f, 0.1f, 0.1f),
        float3(0.1f, 3.0f, 0.1f),
        float3(0.1f, 0.1f, 3.0f)
    };

    for (size_t i = 0; i < 3; ++i)
    {
        AddPointLight(positions[i], colors[i] * 10.0f);
    }

    ClearFramebuffer();
    Render();
    SaveAndCompare("1");

    RemoveLight(0);
    ClearFramebuffer();
    Render();
    SaveAndCompare("2");

}

TEST_F(LightTest, Light_PointLightMany)
{
    const size_t num_lights = 16;
    std::vector<float3> positions;
    std::vector<float3> colors;

    float step = (float)(2.f * M_PI / num_lights);
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

    for (size_t i = 0; i < num_lights; ++i)
    {
        AddPointLight(positions[i], colors[i] * 10.0f);
    }

    ClearFramebuffer();
    Render();
    SaveAndCompare();
}
