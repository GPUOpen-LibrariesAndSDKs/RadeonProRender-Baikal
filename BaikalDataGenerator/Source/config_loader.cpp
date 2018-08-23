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

#include "config_loader.h"
#include "XML/tinyxml2.h"
#include "material_io.h"
#include <filesystem>
#include <sstream>

// validation checks helper macroses to reduce copy paste
#define ASSERT_PATH(file_name) \
     if (file_name.empty()) { \
         std::stringstream ss; \
         ss << "missed " << file_name.string(); \
         THROW_EX(ss.str()) } \

#define ASSERT_XML(file_name) \
     if (file_name.extension() != ".xml") { \
         std::stringstream ss; \
         ss << "not xml file: " << file_name.string(); \
         THROW_EX(ss.str()) } \

#define ASSERT_FILE_EXISTS(file_name) \
     if (!std::filesystem::exists(file_name)) { \
         std::stringstream ss; \
         ss << "there is no file on specified path: " << file_name.string(); \
         THROW_EX(ss.str()) } \

void ConfigLoader::ValidateConfig(const DGenConfig& config) const
{
    // validate input config
    ASSERT_PATH(config.camera_file);
    ASSERT_PATH(config.light_file);
    ASSERT_PATH(config.spp_file);
    ASSERT_PATH(config.scene_file);
    ASSERT_PATH(config.output_dir);

    // validate extansions
    ASSERT_XML(config.camera_file)
    ASSERT_XML(config.light_file)
    ASSERT_XML(config.spp_file)

    // validate that files really exists
    ASSERT_FILE_EXISTS(config.camera_file)
    ASSERT_FILE_EXISTS(config.light_file)
    ASSERT_FILE_EXISTS(config.spp_file)
    ASSERT_FILE_EXISTS(config.scene_file)

    if (!std::filesystem::is_directory(config.output_dir))
    {
        THROW_EX(config.output_dir.string() + " should be directory")
    }
}

ConfigLoader::ConfigLoader(const DGenConfig& config)
{
    ValidateConfig(config);

    LoadCameraConfig(config.camera_file);
    LoadLightConfig(config.light_file);
    LoadSppConfig(config.spp_file);
}

void ConfigLoader::LoadCameraConfig(const std::filesystem::path& file_name)
{
    tinyxml2::XMLDocument doc;
    doc.LoadFile(file_name.string().c_str());

    auto root = doc.FirstChildElement("cam_list");

    if (!root)
    {
        THROW_EX("Failed to open lights set file.")
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

        if (cam_info.up.sqnorm() == 0.f)
        {
            cam_info.up = RadeonRays::float3(0.f, 1.f, 0.f);
        }

        //other values
        cam_info.focal_length = elem->FloatAttribute("focal_length");
        cam_info.focus_distance = elem->FloatAttribute("focus_dist");
        cam_info.aperture = elem->FloatAttribute("aperture");

        m_camera_states.push_back(cam_info);
        elem = elem->NextSiblingElement("camera");
    }
}

void ConfigLoader::LoadLightConfig(const std::filesystem::path& file_name)
{
    m_ligths_dir = file_name.parent_path();

    tinyxml2::XMLDocument doc;
    doc.LoadFile(file_name.string().c_str());
    auto root = doc.FirstChildElement("light_list");

    if (!root)
    {
        THROW_EX("Failed to open lights set file.")
    }

    tinyxml2::XMLElement* elem = root->FirstChildElement("light");

    while (elem)
    {
        LightInfo light_info;
        //type
        light_info.type = elem->Attribute("type");

        // validate light type
        if (light_info.type == "spot")
        {
            //this option available only for spot light
            light_info.cs.x = elem->FloatAttribute("csx");
            light_info.cs.y = elem->FloatAttribute("csy");
        }
        else if (light_info.type == "ibl")
        {
            //this options available only for ibl
            light_info.texture = elem->Attribute("tex");
            light_info.mul = elem->FloatAttribute("mul");
        }
        else if ((light_info.type != "point") && (light_info.type != "direct"))
        {
            THROW_EX(light_info.type + "Is invalid light type");
        }

        RadeonRays::float3 pos;
        RadeonRays::float3 dir;
        RadeonRays::float3 rad;

        pos.x = elem->FloatAttribute("posx");
        pos.y = elem->FloatAttribute("posy");
        pos.z = elem->FloatAttribute("posz");

        dir.x = elem->FloatAttribute("dirx");
        dir.y = elem->FloatAttribute("diry");
        dir.z = elem->FloatAttribute("dirz");

        rad.x = elem->FloatAttribute("radx");
        rad.y = elem->FloatAttribute("rady");
        rad.z = elem->FloatAttribute("radz");

        light_info.pos = pos;
        light_info.dir = dir;
        light_info.rad = rad;

        m_light_settings.push_back(light_info);

        elem = elem->NextSiblingElement("light");
    }
}

void ConfigLoader::LoadSppConfig(const std::filesystem::path& file_name)
{
    tinyxml2::XMLDocument doc;
    doc.LoadFile(file_name.string().c_str());
    auto root = doc.FirstChildElement("spp_list");

    if (!root)
    {
        THROW_EX("Failed to open SPP file: " + file_name.string())
    }

    tinyxml2::XMLElement* elem = root->FirstChildElement("spp");

    m_spp.clear();

    while (elem)
    {
        int spp = (int)elem->Int64Attribute("iter_num");
        m_spp.push_back(spp);
        elem = elem->NextSiblingElement("spp");
    }
}

CameraIterator ConfigLoader::CamStatesBegin() const
{
    return m_camera_states.begin();
}


CameraIterator ConfigLoader::CamStatesEnd() const
{
    return m_camera_states.end();
}

LightsIterator ConfigLoader::LightsBegin() const
{
    return m_light_settings.begin();
}

LightsIterator ConfigLoader::LightsEnd() const
{
    return m_light_settings.end();
}

const std::filesystem::path& ConfigLoader::LightsDir() const
{
    return m_ligths_dir;
}

SppIterator ConfigLoader::SppBegin() const
{
    return m_spp.begin();
}

SppIterator ConfigLoader::SppEnd() const
{
    return m_spp.end();
}
