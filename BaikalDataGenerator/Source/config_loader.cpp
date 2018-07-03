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

// validation checks helper macroses to reduce copy paste
#define ASSERT_PATH(file_name, message) \
     if (file_name.empty()) { \
         THROW_EX(message) } \

#define ASSERT_EXT(file_name, message) \
     if (camera_file.extension() != ".xml") { \
         THROW_EX(message) } \

#define ASSERT_FILE_EXISTING(file_name, message) \
     if (!std::filesystem::exists(file_name)) { \
         THROW_EX(message) } \

void ConfigLoader::ValidateConfig(const DGenConfig& config) const
{
    auto camera_file = config.camera_file;
    auto light_file = config.light_file;
    auto spp_file = config.spp_file;
    auto scene_file = config.scene_file;
    auto output_dir = config.output_dir;

    // validate input config
    ASSERT_PATH(camera_file, "missed camera file");
    ASSERT_PATH(light_file, "missed light file");
    ASSERT_PATH(spp_file, "missed spp file");
    ASSERT_PATH(scene_file, "missed scene file");
    ASSERT_PATH(output_dir, "missed output_dir");

    // validate extansions
    ASSERT_EXT(camera_file, "camera config should has extenshion '.xml'")
    ASSERT_EXT(light_file, "light config should has extenshion '.xml'")
    ASSERT_EXT(spp_file, "spp config should has extenshion '.xml'")

    // validate that files really exists
    ASSERT_FILE_EXISTING(camera_file, "camera file doesn't exist")
    ASSERT_FILE_EXISTING(light_file, "light file doesn't exist")
    ASSERT_FILE_EXISTING(spp_file, "spp file doesn't exist")
    ASSERT_FILE_EXISTING(scene_file, "scene file doesn't exist")

    if (!std::filesystem::is_directory(output_dir))
    {
        THROW_EX("'output_dir' should be directory")
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
        if (light_info.type == "point") { }
        else if (light_info.type == "direct") { }
        else if (light_info.type == "spot")
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
        else
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
        THROW_EX("Failed to open lights set file.")
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

std::vector<CameraInfo> ConfigLoader::GetCameraStates() const
{
    return m_camera_states;
}

std::vector<LightInfo> ConfigLoader::GetLightSettings() const
{
    return m_light_settings;

}
std::vector<int> ConfigLoader::GetSpp() const
{
    return m_spp;
}