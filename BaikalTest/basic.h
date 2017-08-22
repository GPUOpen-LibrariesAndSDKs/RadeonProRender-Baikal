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

#include "gtest/gtest.h"

#include "CLW.h"
#include "Renderers/ptrenderer.h"
#include "RenderFactory/render_factory.h"
#include "Output/output.h"
#include "SceneGraph/camera.h"
#include "SceneGraph/IO/scene_io.h"

#include "OpenImageIO/imageio.h"

#include <vector>
#include <memory>
#include <algorithm>
#include <cstdlib>

extern int g_argc;
extern char** g_argv;

class BasicTest : public ::testing::Test
{
public:
    static std::uint32_t constexpr kMaxPlatforms = 5;
    static std::uint32_t constexpr kOutputWidth = 256;
    static std::uint32_t constexpr kOutputHeight = 256;
    static std::uint32_t constexpr kNumIterations = 32;

    virtual void SetUp()
    {
        std::vector<CLWPlatform> platforms;

        ASSERT_NO_THROW(CLWPlatform::CreateAllPlatforms(platforms));
        ASSERT_GT(platforms.size(), 0);

        char* device_index_option = GetCmdOption(g_argv, g_argv + g_argc, "-device");
        char* platform_index_option = GetCmdOption(g_argv, g_argv + g_argc, "-platform");

        auto platform_index = platform_index_option ? (int)atoi(platform_index_option) : 0;
        auto device_index = device_index_option ? (int)atoi(device_index_option) : 0;

        ASSERT_LT(platform_index, platforms.size());
        ASSERT_LT(device_index, platforms[platform_index].GetDeviceCount());

        auto platform = platforms[platform_index];
        auto device = platform.GetDevice(device_index);
        auto context = CLWContext::Create(device);

        ASSERT_NO_THROW(m_factory = Baikal::RenderFactory::CreateClwRenderFactory(context, 0));
        ASSERT_NO_THROW(m_renderer = m_factory->CreateRenderer(Baikal::RenderFactory::RendererType::kUnidirectionalPathTracer));
        ASSERT_NO_THROW(m_output = m_factory->CreateOutput(kOutputWidth, kOutputHeight));
        ASSERT_NO_THROW(m_renderer->SetOutput(Baikal::Renderer::OutputType::kColor, m_output.get()));

        ASSERT_NO_THROW(LoadSphereTestScene());
        ASSERT_NO_THROW(SetupCamera());
    }

    virtual void TearDown()
    {
    }

    virtual void ClearOutput() const
    {
        ASSERT_NO_THROW(m_renderer->Clear(RadeonRays::float3(), *m_output));
    }

    virtual void LoadSphereTestScene()
    {
        auto io = Baikal::SceneIo::CreateSceneIoTest();
        m_scene = io->LoadScene("sphere+ibl", "");
    }

    virtual  void SetupCamera()
    {
        m_camera = std::make_unique<Baikal::PerspectiveCamera>(
            RadeonRays::float3(0.f, 0.f, -6.f),
            RadeonRays::float3(0.f, 0.f, 0.f),
            RadeonRays::float3(0.f, 1.f, 0.f));

        m_camera->SetSensorSize(RadeonRays::float2(0.036f, 0.036f));
        m_camera->SetDepthRange(RadeonRays::float2(0.0f, 100000.f));
        m_camera->SetFocalLength(0.035f);
        m_camera->SetFocusDistance(1.f);
        m_camera->SetAperture(0.f);

        m_scene->SetCamera(m_camera.get());
    }


    void SaveOutput(std::string const& file_name) const
    {
        std::string path = "OutputImages/";
        path.append(file_name);

        OIIO_NAMESPACE_USING;
        using namespace RadeonRays;

        auto width = m_output->width();
        auto height = m_output->height();
        std::vector<float3> data(width * height);
        std::vector<float3> data1(width * height);
        m_output->GetData(&data[0]);

        for (auto y = 0; y < height; ++y)
            for (auto x = 0; x < width; ++x)
            {

                float3 val = data[(height - 1 - y) * width + x];
                val *= (1.f / val.w);
                data1[y * width + x].x = std::pow(val.x, 1.f / 2.2f);
                data1[y * width + x].y = std::pow(val.y, 1.f / 2.2f);
                data1[y * width + x].z = std::pow(val.z, 1.f / 2.2f);
            }

        ImageOutput* out = ImageOutput::create(path);

        if (!out)
        {
            throw std::runtime_error("Can't create image file on disk");
        }

        ImageSpec spec(width, height, 3, TypeDesc::FLOAT);

        out->open(path, spec);
        out->write_image(TypeDesc::FLOAT, &data1[0], sizeof(float3));
        out->close();

        delete out;
    }

    std::string test_name() const
    {
        return ::testing::UnitTest::GetInstance()->current_test_info()->name();
    }

    static char* GetCmdOption(char ** begin, char ** end, const std::string & option)
    {
        char ** itr = std::find(begin, end, option);
        if (itr != end && ++itr != end)
        {
            return *itr;
        }
        return 0;
    }

    static bool CmdOptionExists(char** begin, char** end, const std::string& option)
    {
        return std::find(begin, end, option) != end;
    }

    std::unique_ptr<Baikal::Renderer> m_renderer;
    std::unique_ptr<Baikal::RenderFactory> m_factory;
    std::unique_ptr<Baikal::Output> m_output;
    std::unique_ptr<Baikal::Scene1> m_scene;
    std::unique_ptr<Baikal::PerspectiveCamera> m_camera;
};


// Test renderer instance creation
TEST_F(BasicTest, Init)
{
    ASSERT_TRUE(true);
}

TEST_F(BasicTest, RenderTestScene)
{
    ClearOutput();

    ASSERT_NO_THROW(m_renderer->CompileScene(*m_scene));

    for (auto i = 0u; i < kNumIterations; ++i)
    {
        ASSERT_NO_THROW(m_renderer->Render(*m_scene));
    }

    SaveOutput(test_name() + ".png");
}


