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

#include "render.h"


Render::Render(const std::string &file_name,
               const std::string &path,
               std::uint32_t output_width,
               std::uint32_t output_height)
{
    std::vector<CLWPlatform> platforms;
    CLWPlatform::CreateAllPlatforms(platforms);

    int platform_index = 0;

    for (auto j = 0u; j < platforms.size(); ++j)
    {
        for (auto i = 0u; i < platforms[j].GetDeviceCount(); ++i)
        {
            if (platforms[j].GetDevice(i).GetType() == CL_DEVICE_TYPE_GPU)
            {
                platform_index = j;
                break;
            }
        }
    }

    int device_index = 0;

    for (auto i = 0u; i < platforms[platform_index].GetDeviceCount(); ++i)
    {
        if (platforms[platform_index].GetDevice(i).GetType() == CL_DEVICE_TYPE_GPU)
        {
            device_index = i;
            break;
        }
    }

    m_scene = Baikal::SceneIo::LoadScene(file_name, path);

    auto platform = platforms[platform_index];
    auto device = platform.GetDevice(device_index);
    auto context = CLWContext::Create(device);

    m_factory = std::make_unique<Baikal::ClwRenderFactory>(context, "cache");
    m_renderer = m_factory->CreateRenderer(Baikal::ClwRenderFactory::RendererType::kUnidirectionalPathTracer);
    m_controller = m_factory->CreateSceneController();
    m_output = m_factory->CreateOutput(output_width, output_height);
    m_output->Clear(RadeonRays::float3(0.0f));
    m_renderer->SetOutput(Baikal::Renderer::OutputType::kColor, m_output.get());
}


namespace {

    struct RenderConcrete : public Render
    {
        RenderConcrete(const std::string &file_name,
                       const std::string &path,
                       std::uint32_t output_width,
                       std::uint32_t output_height)
            : Render(file_name, path, output_width, output_height) {}
    };
}

Render::Ptr Render::Create(const std::string& file_name,
                           const std::string& path,
                           std::uint32_t output_width,
                           std::uint32_t output_height)
{
    return std::make_shared<RenderConcrete>(file_name,
                                            path,
                                            output_width,
                                            output_height);
}