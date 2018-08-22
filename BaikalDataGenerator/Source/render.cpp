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

#include "CLW.h"
#include "Renderers/renderer.h"
#include "RenderFactory/clw_render_factory.h"
#include "SceneGraph/camera.h"
#include "scene_io.h"
#include "input_info.h"

#include "utils.h"
#include "render.h"
#include "scene_io.h"
#include "material_io.h"
#include "SceneGraph/light.h"
#include "Output/clwoutput.h"
#include "BaikalIO/image_io.h"
#include "Renderers/monte_carlo_renderer.h"
#include "OpenImageIO/imageio.h"

#include <filesystem>
#include <fstream>
#include "XML/tinyxml2.h"

using namespace Baikal;

struct OutputInfo
{
    Renderer::OutputType type;
    std::string name;
    // channels number can be only 1 or 3
    int channels_num;
};

// if you need to add new output for saving to disk
// for iterations number counted in spp.xml file
// just put its description in this collection
const std::vector<OutputInfo> kMultipleIteratedOutputs =
{
    {
        { Renderer::OutputType::kColor, "color", 3 },
        { Renderer::OutputType::kAlbedo, "albedo", 3 },
        { Renderer::OutputType::kGloss, "gloss", 1 }
    }
};

// if you need to add new output for saving to disk
// only for the one time
// just put its description in this collection
const std::vector<OutputInfo> kSingleIteratedOutputs =
{
    {
        { Renderer::OutputType::kViewShadingNormal, "view_shading_normal", 3 },
        { Renderer::OutputType::kDepth, "view_shading_depth", 1 }
    }
};

Render::Render(const std::filesystem::path& scene_file,
               size_t output_width,
               size_t output_height,
               std::uint32_t num_bounces)
    : m_width(static_cast<std::uint32_t>(output_width)),
      m_height(static_cast<std::uint32_t>(output_height)),
      m_num_bounces(num_bounces)
{
    using namespace Baikal;

    assert(m_width);
    assert(m_height);
    assert(num_bounces);

    std::vector<CLWPlatform> platforms;
    CLWPlatform::CreateAllPlatforms(platforms);

    bool device_found = false;

    for (const auto& platform : platforms)
    {
        for (auto i = 0u; i < platform.GetDeviceCount(); i++)
        {
            if (platform.GetDevice(i).GetType() == CL_DEVICE_TYPE_GPU)
            {
                m_context = std::make_unique<CLWContext>(CLWContext::Create(platform.GetDevice(i)));
                device_found = true;
                break;
            }
        }

        if (device_found)
            break;
    }

    if (!device_found)
    {
        THROW_EX("can't find device");
    }

    assert(m_context);

    m_factory = std::make_unique<ClwRenderFactory>(*m_context, "cache");

    m_renderer.reset(dynamic_cast<MonteCarloRenderer*>(
        m_factory->CreateRenderer(ClwRenderFactory::RendererType::kUnidirectionalPathTracer).release()));

    m_controller = m_factory->CreateSceneController();

    for (auto& output_info : kMultipleIteratedOutputs)
    {
        m_outputs.push_back(m_factory->CreateOutput(m_width, m_height));
        m_renderer->SetOutput(output_info.type, m_outputs.back().get());
    }
    for (auto& output_info : kSingleIteratedOutputs)
    {
        m_outputs.push_back(m_factory->CreateOutput(m_width, m_height));
        m_renderer->SetOutput(output_info.type, m_outputs.back().get());
    }

    m_renderer->SetMaxBounces(m_num_bounces);

    if (!std::filesystem::exists(scene_file))
    {
        THROW_EX("There is no any scene file to load");
    }

    // workaround to avoid issues with tiny_object_loader
    auto scene_dir = scene_file.parent_path().string();

    if (scene_dir.back() != '/' || scene_dir.back() != '\\')
    {
#ifdef WIN32
        scene_dir.append("\\");
#else
        scene_dir.append("/");
#endif
    }

    m_scene = SceneIo::LoadScene(scene_file.string(), scene_dir);

    // load materials.xml if it exists
    auto materials_file = scene_file.parent_path() / "materials.xml";
    auto mapping_file = scene_file.parent_path() / "mapping.xml";

    if (std::filesystem::exists(materials_file) &&
        std::filesystem::exists(mapping_file))
    {
        auto material_io = MaterialIo::CreateMaterialIoXML();
        auto materials = material_io->LoadMaterials(materials_file.string());
        auto mapping = material_io->LoadMaterialMapping(mapping_file.string());

        material_io->ReplaceSceneMaterials(*m_scene, *materials, mapping);
    }
    else
    {
        std::cout << "WARNING: materials.xml or mapping.xml is missed" << std::endl;
    }
}


void Render::SaveMetadata(const std::filesystem::path& output_dir) const
{
    using namespace tinyxml2;

    XMLDocument doc;

    auto file_name = output_dir;
    file_name.append("metadata.xml");

    XMLNode* root= doc.NewElement("metadata");
    doc.InsertFirstChild(root);

    // log outputs layout
    XMLElement* size_attribute = doc.NewElement("layout");
    size_attribute->SetAttribute("width", m_width);
    size_attribute->SetAttribute("height", m_height);
    root->InsertEndChild(size_attribute);

    // log outputs data
    std::vector<OutputInfo> outputs = kSingleIteratedOutputs;
    outputs.insert(outputs.end(), kMultipleIteratedOutputs.begin(), kMultipleIteratedOutputs.end());

    for (const auto& output : outputs)
    {
        XMLElement* output_attribute = doc.NewElement("input");
        output_attribute->SetAttribute("name", output.name.c_str());
        output_attribute->SetAttribute("type", "float32");
        output_attribute->SetAttribute("channels", output.channels_num);
        root->InsertEndChild(output_attribute);
    }

    // log render settings
    XMLElement* render_attribute = doc.NewElement("render");
    render_attribute->SetAttribute("num_bounce", m_num_bounces);
    root->InsertEndChild(render_attribute);

    doc.SaveFile(file_name.string().c_str());
}

void Render::SetLight(const LightInfo& light)
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

        // check that texture file is exists
        auto texture_path = std::filesystem::absolute(std::filesystem::relative(light.texture));
        if (!std::filesystem::exists(texture_path))
        {
            THROW_EX("texture image doesn't exist on specified path")
        }

        Texture::Ptr tex = image_io->LoadImage(texture_path.string());
        ibl->SetTexture(tex);
        ibl->SetMultiplier(light.mul);
    }
    else
    {
        THROW_EX("unsupported light type")
    }

    light_instance->SetPosition(light.pos);
    light_instance->SetDirection(light.dir);
    light_instance->SetEmittedRadiance(light.rad);
    m_scene->AttachLight(light_instance);
}

void Render::UpdateCameraSettings(const CameraInfo& cam_state)
{
    m_camera->SetAperture(cam_state.aperture);
    m_camera->SetFocalLength(cam_state.focal_length);
    m_camera->SetFocusDistance(cam_state.focus_distance);
    m_camera->LookAt(cam_state.pos, cam_state.at, cam_state.up);
}

void Render::GenerateSample(const CameraInfo& cam_state,
                            const std::vector<size_t>& sorted_spp,
                            const std::filesystem::path& output_dir,
                            bool gamma_correction_enabled,
                            size_t start_cam_id)
{
    // create camera if it wasn't  done earlier
    if (!m_camera)
    {
        m_camera = Baikal::PerspectiveCamera::Create(cam_state.at,
                                                     cam_state.pos,
                                                     cam_state.up);

        // default sensor width
        float sensor_width = 0.036f;
        float inverserd_aspect_ration = static_cast<float>(m_height) /
            static_cast<float>(m_width);
        float sensor_height = sensor_width * inverserd_aspect_ration;

        m_camera->SetSensorSize(float2(0.036f, sensor_height));
        m_camera->SetDepthRange(float2(0.0f, 100000.f));

        m_scene->SetCamera(m_camera);
    }

    UpdateCameraSettings(cam_state);
    for (const auto& output : m_outputs)
    {
        output->Clear(float3());
    }

    // recompile scene cause of changing camera pos and settings
    m_controller->CompileScene(m_scene);
    auto& scene = m_controller->GetCachedScene(m_scene);

    auto spp_iter = sorted_spp.begin();

    for (auto i = 1u; i <= sorted_spp.back(); i++)
    {
        m_renderer->Render(scene);

        if (i == 1)
        {
            for (const auto& output : kSingleIteratedOutputs)
            {
                std::stringstream ss;

                ss << "cam_" << start_cam_id << "_"
                    << output.name << ".bin";

                SaveOutput(output,
                    ss.str(),
                    gamma_correction_enabled,
                    output_dir);
            }
        }

        if (*spp_iter == i)
        {
            for (const auto& output : kMultipleIteratedOutputs)
            {
                std::stringstream ss;

                ss << "cam_" << start_cam_id << "_"
                    << output.name << "_spp_" << i << ".bin";

                SaveOutput(output,
                    ss.str(),
                    gamma_correction_enabled,
                    output_dir);
            }
            ++spp_iter;
            std::cout << "cam_" << start_cam_id << "_spp_" << i << "_generated" << std::endl;
        }
    }
}

void Render::SaveOutput(const OutputInfo& info,
                        const std::string& name,
                        bool gamma_correction_enabled,
                        const std::filesystem::path& output_dir)
{
    OIIO_NAMESPACE_USING;

    auto output = m_renderer->GetOutput(info.type);

    assert(output);

    auto buffer = static_cast<Baikal::ClwOutput*>(output)->data();

    std::vector<RadeonRays::float3> output_data(buffer.GetElementCount());

    output->GetData(output_data.data());

    std::vector<float> image_data(info.channels_num * m_width * m_height);

    float* dst_row = image_data.data();

    if (gamma_correction_enabled &&
       (info.type == Renderer::OutputType::kColor) &&
       (info.channels_num == 3))
    {
        for (auto y = 0u; y < m_height; ++y)
        {
            for (auto x = 0u; x < m_width; ++x)
            {
                float3 val = output_data[(m_height - 1 - y) * m_width + x];
                // "The 4-th pixel component is a count of accumulated samples.
                // It can be different for every pixel in case of adaptive sampling.
                // So, we need to normalize pixel values here".
                val *= (1.f / val.w);
                // gamma corection
                dst_row[info.channels_num * x] = std::pow(val.x, 1.f / 2.2f);
                dst_row[info.channels_num * x + 1] = std::pow(val.y, 1.f / 2.2f);
                dst_row[info.channels_num * x + 2] = std::pow(val.z, 1.f / 2.2f);
            }
            dst_row += info.channels_num * m_width;
        }
    }
    else
    {
        for (auto y = 0u; y < m_height; ++y)
        {
            for (auto x = 0u; x < m_width; ++x)
            {
                float3 val = output_data[(m_height - 1 - y) * m_width + x];
                // "The 4-th pixel component is a count of accumulated samples.
                // It can be different for every pixel in case of adaptive sampling.
                // So, we need to normalize pixel values here".
                val *= (1.f / val.w);

                if (info.channels_num == 3)
                {
                    int dst_pixel = y * m_width + x;
                    // invert the image 
                    dst_row[info.channels_num * x] = val.x;
                    dst_row[info.channels_num * x + 1] = val.y;
                    dst_row[info.channels_num * x + 2] = val.z;
                }
                else // info.channels_num = 1
                {
                    // invert the image 
                    dst_row[x] = val.x;
                }
            }
            dst_row += info.channels_num * m_width;
        }
    }

    std::filesystem::path file_name = output_dir;
    file_name.append(name);

    std::ofstream f (file_name.string(), std::ofstream::binary);

    f.write(reinterpret_cast<const char*>(image_data.data()),
            sizeof(float) * image_data.size());
}
Render::~Render() = default;
