/**********************************************************************
Copyright (c) 2018 Advanced Micro Devices, Inc. All rights reserved.

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

#include "utils.h"
#include "render.h"
#include "scene_io.h"
#include "material_io.h"
#include "SceneGraph/light.h"
#include "Output/clwoutput.h"
#include "BaikalIO/image_io.h"

#include <filesystem>
#include <fstream>
#include "XML/tinyxml2.h"

using namespace Baikal;

namespace
{
    std::uint32_t constexpr kNumIterations = 4096;

    bool operator != (RadeonRays::float3 left, RadeonRays::float3 right)
    {
        return (left.x != right.x) ||
               (left.y != right.y) ||
               (left.z != right.z);
    }
}

struct OutputDesc
{
    Renderer::OutputType type;
    std::string name;
    // extension of the file to save
    std::string file_ext;
    // bites per pixel
    int width, height;
};

// if you need to add new output for saving to disk
// just put its description in thic collection
std::vector<OutputDesc> outputs_collection = {
    { Renderer::OutputType::kColor, "color", "png" },
    { Renderer::OutputType::kViewShadingNormal, "view_shading_depth", "png" },
    { Renderer::OutputType::kDepth, "depth", "png" },
    { Renderer::OutputType::kAlbedo, "albedo", "png" },
    { Renderer::OutputType::kGloss, "gloss", "png" }
};

Render::Render(std::filesystem::path scene_file,
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

    auto platform = platforms[platform_index];
    auto device = platform.GetDevice(device_index);
    m_context = CLWContext::Create(device);

    m_factory = std::make_unique<Baikal::ClwRenderFactory>(m_context, "cache");
    m_renderer = m_factory->CreateRenderer(Baikal::ClwRenderFactory::RendererType::kUnidirectionalPathTracer);
    m_controller = m_factory->CreateSceneController();

    for (auto& output_info: outputs_collection)
    {
        m_outputs.push_back(m_factory->CreateOutput(output_width, output_height));
        m_renderer->SetOutput(output_info.type, m_outputs.back().get());
        output_info.width = output_width;
        output_info.height = output_height;
    }

    if (!std::filesystem::exists(scene_file))
    {
        THROW_EX("There is no any scene file to load");
    }

    m_scene = Baikal::SceneIo::LoadScene(scene_file.string(),
                                         scene_file.parent_path().string());
}

void Render::UpdateCameraSettings(const CameraInfo& cam_state)
{
    if (cam_state.aperture != m_camera->GetAperture())
    {
        m_camera->SetAperture(cam_state.aperture);
    }

    if (cam_state.focal_length != m_camera->GetFocalLength())
    {
        m_camera->SetFocalLength(cam_state.focal_length);
    }

    if (cam_state.focus_distance != m_camera->GetFocusDistance())
    {
        m_camera->SetFocusDistance(cam_state.focus_distance);
    }

    auto cur_pos = m_camera->GetPosition();
    auto at = m_camera->GetForwardVector();
    auto up = m_camera->GetUpVector();

    if (cur_pos != cam_state.pos ||
        at != cam_state.at ||
        up != cam_state.up)
    {
        m_camera->LookAt(cam_state.pos, cam_state.at, cam_state.up);
    }
}

void Render::SaveOutput(OutputDesc desc,
                        const std::filesystem::path& output_dir,
                        int cam_index,
                        int spp)
{
    OIIO_NAMESPACE_USING;

    std::vector<RadeonRays::float3> output_data;
    std::vector<RadeonRays::float3> image_data;


    auto output = m_renderer->GetOutput(desc.type);
    auto width = output->width();
    auto height = output->height();

    assert(output);

    auto buffer = static_cast<Baikal::ClwOutput*>(output)->data();
    output_data.resize(buffer.GetElementCount());
    image_data.resize(buffer.GetElementCount());

    output->GetData(output_data.data());

    for (auto y = 0u; y < height; ++y)
    {
        for (auto x = 0u; x < width; ++x)
        {
            float3 val = output_data[(height - 1 - y) * width + x];
            val *= (1.f / val.w);
            image_data[y * width + x].x = std::pow(val.x, 1.f / 2.2f);
            image_data[y * width + x].y = std::pow(val.y, 1.f / 2.2f);
            image_data[y * width + x].z = std::pow(val.z, 1.f / 2.2f);
        }
    }

    std::stringstream ss;

    ss << "cam_" << cam_index << "_"
        << desc.name << "_spp_" << spp << ".png";

    std::filesystem::path file_name = output_dir;
    file_name.append(ss.str());

    std::unique_ptr<ImageOutput> out(ImageOutput::create(ss.str()));

    if (!out)
    {
        THROW_EX("Can't create image file on disk");
    }

    ImageSpec spec(width, height, 3, TypeDesc::FLOAT);
    out->open(file_name.string(), spec);
    out->write_image(TypeDesc::FLOAT, image_data.data(), sizeof(float3));
    out->close();
}

void Render::SetLight(const std::vector<LightInfo>& light_settings)
{
    // clear light
    auto iter = m_scene->CreateLightIterator();
    while (iter->IsValid())
    {
        m_scene->DetachLight(iter->ItemAs<Baikal::Light>());
        iter = m_scene->CreateLightIterator();
    }

    // set light
    for (const auto& light: light_settings)
    {
        Light::Ptr light_instance;

        if (light.type == "point")
        {
            light_instance = PointLight::Create();
        }
        else if (light.type == "direct")
        {
            light_instance = DirectionalLight::Create();
        }
        else if (light.type == "spot")
        {
            light_instance = SpotLight::Create();
            SpotLight::Ptr spot = std::dynamic_pointer_cast<SpotLight>(light_instance);
            spot->SetConeShape(light.cs);
        }
        else if (light.type == "ibl")
        {
            light_instance = ImageBasedLight::Create();

            ImageBasedLight::Ptr ibl = std::dynamic_pointer_cast<
                ImageBasedLight>(light_instance);

            auto image_io(ImageIo::CreateImageIo());
            // check that texture file is exist
            auto texure_path = std::filesystem::path(light.texture);

            if (!std::filesystem::exists(texure_path))
            {
                THROW_EX("textrue image doesn't exist on specidied path")
            }

            Texture::Ptr tex = image_io->LoadImage(light.texture.c_str());
            ibl->SetTexture(tex);
            ibl->SetMultiplier(light.mul);
        }
        else
        {
            THROW_EX("unsupported light type")
        }

        light_instance->SetPosition(light.pos);
        light_instance->SetPosition(light.dir);
        light_instance->SetPosition(light.rad);
        m_scene->AttachLight(light_instance);
    }
}

void Render::GenerateDataset(const std::vector<CameraInfo>& camera_states,
                             const std::vector<LightInfo>& light_settings,
                             const std::vector<int>& spp,
                             const std::filesystem::path& output_dir)
{
    using namespace RadeonRays;

    if (!std::filesystem::is_directory(output_dir))
    {
        THROW_EX("incorrect output directory signature");
    }

    // check if number of samples to render wasn't specified
    if (spp.empty())
    {
        return;
    }

    const int iter_number = *(std::max_element(spp.begin(), spp.end()));

    SetLight(light_settings);

    int counter = 1;
    for (const auto &cam_state: camera_states)
    {
        // create camera if it wasn't  done earlier
        if (!m_camera)
        {
            m_camera = Baikal::PerspectiveCamera::Create(cam_state.at,
                                                         cam_state.pos,
                                                         cam_state.up);

            m_camera->SetSensorSize(RadeonRays::float2(0.036f, 0.036f));
            m_camera->SetDepthRange(RadeonRays::float2(0.0f, 100000.f));

            m_scene->SetCamera(m_camera);
        }

        UpdateCameraSettings(cam_state);

        for (const auto& output: m_outputs)
        {
            output->Clear(RadeonRays::float3());
        }

        // recompile scene cause of changing camera pos and settings
        m_controller->CompileScene(m_scene);
        auto& scene = m_controller->GetCachedScene(m_scene);

        for (auto i = 0; i < iter_number; i++)
        {
            m_renderer->Render(scene);

            if (std::find(spp.begin(), spp.end(), i + 1) != spp.end())
            {
                for (const auto& output : outputs_collection)
                {
                    SaveOutput(output,
                               output_dir,
                               counter,
                               i + 1);
                }
            }
        }

        counter++;
    }
}
