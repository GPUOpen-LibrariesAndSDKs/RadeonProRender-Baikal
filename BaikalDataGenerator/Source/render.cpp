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
#include "XML/tinyxml2.h"
#include "SceneGraph/light.h"
#include "Output/clwoutput.h"
#include "BaikalIO/image_io.h"

using namespace Baikal;

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
    m_context = CLWContext::Create(device);

    m_factory = std::make_unique<Baikal::ClwRenderFactory>(m_context, "cache");
    m_renderer = m_factory->CreateRenderer(Baikal::ClwRenderFactory::RendererType::kUnidirectionalPathTracer);
    m_controller = m_factory->CreateSceneController();
    m_output = m_factory->CreateOutput(output_width, output_height);
    m_output->Clear(RadeonRays::float3(0.0f));
    m_renderer->SetOutput(Baikal::Renderer::OutputType::kColor, m_output.get());
}

void Render::LoadCameraXml(const std::string &full_path)
{
    tinyxml2::XMLDocument doc;
    doc.LoadFile(full_path.c_str());
    auto root = doc.FirstChildElement("cam_list");

    if (!root)
    {
        throw std::runtime_error("Render::LoadCameraXml(...):"
                                 "Failed to open lights set file.");
    }

    tinyxml2::XMLElement* elem = root->FirstChildElement("camera");

    m_camera_states.clear();

    while (elem)
    {
        CameraInfo cam_info;

        // eye
        cam_info.pos.x = elem->FloatAttribute("cpx");
        cam_info.pos.y = elem->FloatAttribute("cpy");
        cam_info.pos.z = elem->FloatAttribute("cpz");

        // center
        cam_info.at.x = elem->FloatAttribute("tpx");
        cam_info.at.y = elem->FloatAttribute("tpy");
        cam_info.at.z = elem->FloatAttribute("tpz");

        // up
        cam_info.up.x = elem->FloatAttribute("upx");
        cam_info.up.y = elem->FloatAttribute("upy");
        cam_info.up.z = elem->FloatAttribute("upz");

        //other values
        cam_info.focal_length = elem->FloatAttribute("focal_length");
        cam_info.focus_distance = elem->FloatAttribute("focus_dist");
        cam_info.aperture = elem->FloatAttribute("aperture");

        m_camera_states.push_back(cam_info);
    }
}

void Render::LoadLightXml(const std::string &full_path)
{
    tinyxml2::XMLDocument doc;
    doc.LoadFile(full_path.c_str());
    auto root = doc.FirstChildElement("light_list");

    if (!root)
    {
        throw std::runtime_error("Render::LoadLightXml(...):"
                                 "Failed to open lights set file.");
    }

    Light::Ptr new_light;
    tinyxml2::XMLElement* elem = root->FirstChildElement("light");

    while (elem)
    {
        //type
        std::string type = elem->Attribute("type");
        if (type == "point")
        {
            new_light = PointLight::Create();
        }
        else if (type == "direct")
        {
            new_light = DirectionalLight::Create();
        }
        else if (type == "spot")
        {
            new_light = SpotLight::Create();
            RadeonRays::float2 cs;
            cs.x = elem->FloatAttribute("csx");
            cs.y = elem->FloatAttribute("csy");
            //this option available only for spot light
            SpotLight::Ptr spot = std::dynamic_pointer_cast<SpotLight>(new_light);
            spot->SetConeShape(cs);
        }
        else if (type == "ibl")
        {
            new_light = ImageBasedLight::Create();
            std::string tex_name = elem->Attribute("tex");
            float mul = elem->FloatAttribute("mul");
            
            //this option available only for ibl
            ImageBasedLight::Ptr ibl = std::dynamic_pointer_cast<ImageBasedLight>(new_light);
            auto image_io = ImageIo::CreateImageIo();
            Texture::Ptr tex = image_io->LoadImage(tex_name.c_str());
            ibl->SetTexture(tex);
            ibl->SetMultiplier(mul);
        }
        else
        {
            throw std::runtime_error("Render::LoadLightXml(...): Invalid light type " + type);
        }

        RadeonRays::float3 p;
        RadeonRays::float3 d;
        RadeonRays::float3 r;

        p.x = elem->FloatAttribute("posx");
        p.y = elem->FloatAttribute("posy");
        p.z = elem->FloatAttribute("posz");

        d.x = elem->FloatAttribute("dirx");
        d.y = elem->FloatAttribute("diry");
        d.z = elem->FloatAttribute("dirz");

        r.x = elem->FloatAttribute("radx");
        r.y = elem->FloatAttribute("rady");
        r.z = elem->FloatAttribute("radz");

        new_light->SetPosition(p);
        new_light->SetDirection(d);
        new_light->SetEmittedRadiance(r);
        m_scene->AttachLight(new_light);
        elem = elem->NextSiblingElement("light");
    }
}

static bool operator != (RadeonRays::float3 left, RadeonRays::float3 right)
{
    return (left.x != right.x) ||
           (left.y != right.y) ||
           (left.z != right.z);
}

void Render::UpdateCameraPos(const CameraInfo& cam_state)
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

void Render::SaveOutput(Renderer::OutputType type,
                        const std::string& path,
                        const std::string& file_name,
                        int bpp)
{
    OIIO_NAMESPACE_USING;

    std::vector<RadeonRays::float3> output_data;
    auto output = m_renderer->GetOutput(type);
    int width = output->width;
    int height = output->height;

    assert(output);

    auto buffer = static_cast<Baikal::ClwOutput*>(output)->data();
    output_data.resize(buffer.GetElementCount());

    m_context.ReadBuffer(0,
                         static_cast<Baikal::ClwOutput*>(output)->data(),
                         &output_data[0],
                         output_data.size()).Wait();

    TypeDesc fmt;
    switch (bpp)
    {
        case 8:
            fmt = TypeDesc::UINT8;
            break;
        case 16:
            fmt = TypeDesc::UINT16;
            break;
        case 32:
            fmt = TypeDesc::FLOAT;
            break;
        default:
            throw std::runtime_error("Render::SaveOutput(...):"
                                     "Unhandled bpp of image.");
    }

    for (auto y = 0u; y < height; ++y)
    {
        for (auto x = 0u; x < width; ++x)
        {
            float3 val = output_data[(height - 1 - y) * width + x];
            val *= (1.f / val.w);
            output_data[y * width + x].x = std::pow(val.x, 1.f / 2.2f);
            output_data[y * width + x].y = std::pow(val.y, 1.f / 2.2f);
            output_data[y * width + x].z = std::pow(val.z, 1.f / 2.2f);
        }
    }

    std::string full_path = path;
    full_path.append("/");
    full_path.append(file_name);

    auto out = std::make_unique<ImageOutput>(ImageOutput::create(full_path));

    if (!out)
    {
        throw std::runtime_error("Render::SaveOutput(...):"
                                 "Can't create image file on disk");
    }

    ImageSpec spec(width, height, 3, fmt);
    out->open(file_name, spec);
    out->write_image(TypeDesc::FLOAT, &output_data[0], sizeof(float3));
    out->close();
}

void Render::GenerateDataset(const std::string &full_path)
{
    using namespace RadeonRays;

    auto width = m_output->width();
    auto height = m_output->height();
    std::vector<float3> data(width * height);
    std::vector<float3>  out_data(width * height);

    for (const auto &cam_state: m_camera_states)
    {
        UpdateCameraPos(cam_state);
    }
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