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
#include "Application/app_utils.h"

#include <iostream>

namespace
{
    char const* kHelpMessage =
        "Baikal [-p path_to_models][-f model_name][-b][-r][-ns number_of_shadow_rays][-ao ao_radius][-w window_width][-h window_height][-nb number_of_indirect_bounces]";
}

namespace Baikal
{
    AppCliParser::AppCliParser()
    {

    }

    AppSettings AppCliParser::Parse(int argc, char * argv[])
    {
        AppSettings s;
        char* path = GetCmdOption(argv, argv + argc, "-p");
        s.path = path ? path : s.path;

        char* modelname = GetCmdOption(argv, argv + argc, "-f");
        s.modelname = modelname ? modelname : s.modelname;

        char* envmapname = GetCmdOption(argv, argv + argc, "-e");
        s.envmapname = envmapname ? envmapname : s.envmapname;

        char* width = GetCmdOption(argv, argv + argc, "-w");
        s.width = width ? atoi(width) : s.width;

        char* height = GetCmdOption(argv, argv + argc, "-h");
        s.height = width ? atoi(height) : s.height;

        char* aorays = GetCmdOption(argv, argv + argc, "-ao");
        s.ao_radius = aorays ? (float)atof(aorays) : s.ao_radius;

        char* bounces = GetCmdOption(argv, argv + argc, "-nb");
        s.num_bounces = bounces ? atoi(bounces) : s.num_bounces;

        char* camposx = GetCmdOption(argv, argv + argc, "-cpx");
        s.camera_pos.x = camposx ? (float)atof(camposx) : s.camera_pos.x;

        char* camposy = GetCmdOption(argv, argv + argc, "-cpy");
        s.camera_pos.y = camposy ? (float)atof(camposy) : s.camera_pos.y;

        char* camposz = GetCmdOption(argv, argv + argc, "-cpz");
        s.camera_pos.z = camposz ? (float)atof(camposz) : s.camera_pos.z;

        char* camatx = GetCmdOption(argv, argv + argc, "-tpx");
        s.camera_at.x = camatx ? (float)atof(camatx) : s.camera_at.x;

        char* camaty = GetCmdOption(argv, argv + argc, "-tpy");
        s.camera_at.y = camaty ? (float)atof(camaty) : s.camera_at.y;

        char* camatz = GetCmdOption(argv, argv + argc, "-tpz");
        s.camera_at.z = camatz ? (float)atof(camatz) : s.camera_at.z;

        char* envmapmul = GetCmdOption(argv, argv + argc, "-em");
        s.envmapmul = envmapmul ? (float)atof(envmapmul) : s.envmapmul;

        char* numsamples = GetCmdOption(argv, argv + argc, "-ns");
        s.num_samples = numsamples ? atoi(numsamples) : s.num_samples;

        char* camera_aperture = GetCmdOption(argv, argv + argc, "-a");
        s.camera_aperture = camera_aperture ? (float)atof(camera_aperture) : s.camera_aperture;

        char* camera_dist = GetCmdOption(argv, argv + argc, "-fd");
        s.camera_focus_distance = camera_dist ? (float)atof(camera_dist) : s.camera_focus_distance;

        char* camera_focal_length = GetCmdOption(argv, argv + argc, "-fl");
        s.camera_focal_length = camera_focal_length ? (float)atof(camera_focal_length) : s.camera_focal_length;

        char* camera_senor_size_x = GetCmdOption(argv, argv + argc, "-ssx");
        s.camera_sensor_size.x = camera_senor_size_x ? (float)atof(camera_senor_size_x) : s.camera_sensor_size.x;

        char* image_file_name = GetCmdOption(argv, argv + argc, "-ifn");
        s.base_image_file_name = image_file_name ? image_file_name : s.base_image_file_name;

        char* image_file_format = GetCmdOption(argv, argv + argc, "-iff");
        s.image_file_format = image_file_format ? image_file_format : s.image_file_format;

        char* camera_type = GetCmdOption(argv, argv + argc, "-ct");

        if (camera_type)
        {
            if (strcmp(camera_type, "perspective") == 0)
                s.camera_type = CameraType::kPerspective;
            else if (strcmp(camera_type, "orthographic") == 0)
                s.camera_type = CameraType::kOrthographic;
            else
                throw std::runtime_error("Unsupported camera type");
        }

        char* interop = GetCmdOption(argv, argv + argc, "-interop");
        s.interop = interop ? (atoi(interop) > 0) : s.interop;

        char* cspeed = GetCmdOption(argv, argv + argc, "-cs");
        s.cspeed = cspeed ? (float)atof(cspeed) : s.cspeed;


        char* cfg = GetCmdOption(argv, argv + argc, "-config");

        if (cfg)
        {
            if (strcmp(cfg, "cpu") == 0)
                s.mode = ConfigManager::Mode::kUseSingleCpu;
            else if (strcmp(cfg, "gpu") == 0)
                s.mode = ConfigManager::Mode::kUseSingleGpu;
            else if (strcmp(cfg, "mcpu") == 0)
                s.mode = ConfigManager::Mode::kUseCpus;
            else if (strcmp(cfg, "mgpu") == 0)
                s.mode = ConfigManager::Mode::kUseGpus;
            else if (strcmp(cfg, "all") == 0)
                s.mode = ConfigManager::Mode::kUseAll;
        }


        char* platform_index = GetCmdOption(argv, argv + argc, "-platform");
        s.platform_index = platform_index ? (atoi(platform_index)) : s.platform_index;

        char* device_index = GetCmdOption(argv, argv + argc, "-device");
        s.device_index = device_index ? (atoi(device_index)) : s.device_index;

        if ((s.device_index >= 0) && (s.platform_index < 0))
        {
            std::cout <<
                "Can not set device index, because platform index was not specified" << std::endl;
        }

        if (aorays)
        {
            s.num_ao_rays = atoi(aorays);
            s.ao_enabled = true;
        }

        if (CmdOptionExists(argv, argv + argc, "-r"))
        {
            s.progressive = true;
        }

        if (CmdOptionExists(argv, argv + argc, "-nowindow"))
        {
            s.cmd_line_mode = true;
        }

        return s;
    }


    char* AppCliParser::GetCmdOption(char ** begin, char ** end, const std::string & option)
    {
        char ** itr = std::find(begin, end, option);
        if (itr != end && ++itr != end)
        {
            return *itr;
        }
        return 0;
    }

    bool AppCliParser::CmdOptionExists(char** begin, char** end, const std::string& option)
    {
        return std::find(begin, end, option) != end;
    }

    void AppCliParser::ShowHelp()
    {
        std::cout << kHelpMessage << "\n";
    }

    AppSettings::AppSettings()
        : path("../Resources/CornellBox")
        , modelname("orig.objm")
        , envmapname("../Resources/Textures/studio015.hdr")
        //render
        , width(512)
        , height(512)
        , num_bounces(5)
        , num_samples(-1)
        , interop(true)
        , cspeed(10.25f)
        , mode(ConfigManager::Mode::kUseSingleGpu)
        //ao
        , ao_radius(1.f)
        , num_ao_rays(1)
        , ao_enabled(false)

        //camera
        , camera_pos(0.f, 1.f, 3.f)
        , camera_at(0.f, 1.f, 0.f)
        , camera_up(0.f, 1.f, 0.f)
        , camera_sensor_size(0.036f, 0.024f)  // default full frame sensor 36x24 mm
        , camera_zcap(0.0f, 100000.f)
        , camera_aperture(0.f)
        , camera_focus_distance(1.f)
        , camera_focal_length(0.035f) // 35mm lens
        , camera_type (CameraType::kPerspective)

        //app
        , progressive(false)
        , cmd_line_mode(false)
        , recording_enabled(false)
        , benchmark(false)
        , gui_visible(true)
        , time_benchmarked(false)
        , rt_benchmarked(false)
        , time_benchmark(false)
        , time_benchmark_time(0.f)

		//imagefile
	    , base_image_file_name("out")
	    , image_file_format("png")

        //unused
        , num_shadow_rays(1)
        , samplecount(0)
        , envmapmul(1.f)
        , platform_index(-1)
        , device_index(-1)
    {
    }
}