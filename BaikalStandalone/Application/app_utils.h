
/**********************************************************************
 Copyright (c) 2017 Advanced Micro Devices, Inc. All rights reserved.

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

#include <string>

#include "radeon_rays.h"
#include "Utils/config_manager.h"
#include "Utils/cmd_parser.h"
#include "Baikal/Renderers/renderer.h"
#include "Baikal/Estimators/estimator.h"

namespace Baikal
{
    struct AppSettings
    {
        AppSettings();

        bool help;

        //model load settings
        std::string path;
        std::string modelname;
        std::string envmapname;

        //render
        std::uint32_t width;
        std::uint32_t height;
        int num_bounces;
        int num_samples;
        bool interop;
        float cspeed;
        Mode mode;
        bool split_output = false;

        //ao
        float ao_radius;
        int num_ao_rays;
        bool ao_enabled;

        //camera
        RadeonRays::float3 camera_pos;
        RadeonRays::float3 camera_at;
        RadeonRays::float3 camera_up;
        RadeonRays::float2 camera_sensor_size;
        RadeonRays::float2 camera_zcap;
        float camera_aperture;
        float camera_focus_distance;
        float camera_focal_length;
        CameraType camera_type;

        //folder to store camera position output
        std::string camera_out_folder;

        //app
        bool progressive;
        bool cmd_line_mode;
        bool recording_enabled;
        bool benchmark;
        bool gui_visible;

        //bencmark
        Estimator::RayTracingStats stats;
        bool time_benchmarked;
        bool rt_benchmarked;
        bool time_benchmark;
        float time_benchmark_time;

        //image file
        std::string base_image_file_name;
        std::string image_file_format;

        std::string light_file;

        //unused
        int num_shadow_rays;
        int samplecount;
        float envmapmul;
        // OpenCL platform & device settings
        int platform_index;
        int device_index;

        // device settings
        float gpu_mem_fraction = 0; // float number from 0 to 1, percentage of max used device memory, 0 for default behavior
        std::string visible_devices;

        // denoiser settings
        PostProcessingType post_processing_type = PostProcessingType::kNone;
        std::uint32_t denoiser_start_spp = 8;
    };

    class AppCliParser
    {
    public:

        AppCliParser(int argc, char * argv[]);
        AppSettings Parse();
        static void ShowHelp();

    private:

        CmdParser m_cmd_parser;
    };
}
