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

#include "cmd_line_parser.h"
#include <iostream>

namespace
{
    char const* kHelpMessage =
        "Baikal [-light_file name_of_the_light_config]"
        "[-camera_file name_of_the_camera_config]"
        "[-scene_file name_of_the_scene_config]"
        "[-spp_file name_of_the_spp_config]"
        "[-outpute_dir path_to_generate_data]"
        "[-width output_width]"
        "[-height output_height]"
        "[-gamma enables_gamma_correction]";
}

CmdLineParser::CmdLineParser() {}

bool CmdLineParser::CmdOptionExists(char** begin, char** end, const std::string& option)
{
    return m_cmd_parser.CmdOptionExists(begin, end, option);
}

DGenConfig CmdLineParser::Parse(int argc, char* argv[])
{
    DGenConfig config;

    char* light_file = m_cmd_parser.GetCmdOption(argv, argv + argc, "-light_file");
    config.light_file = light_file ? light_file : "";

    char* camera_file = m_cmd_parser.GetCmdOption(argv, argv + argc, "-camera_file");
    config.camera_file = camera_file ? camera_file : "";

    char* output_dir = m_cmd_parser.GetCmdOption(argv, argv + argc, "-output_dir");
    config.output_dir = output_dir ? output_dir : "";

    char* scene_file = m_cmd_parser.GetCmdOption(argv, argv + argc, "-scene_file");
    config.scene_file = scene_file ? scene_file : "";

    char* spp_file = m_cmd_parser.GetCmdOption(argv, argv + argc, "-spp_file");
    config.spp_file = spp_file ? spp_file : "";

    char* gamma_correction_opt = m_cmd_parser.GetCmdOption(argv, argv + argc, "-gamma");
    config.gamma_correction = spp_file ? true : false;

    char* width_str = m_cmd_parser.GetCmdOption(argv, argv + argc, "-width");
    config.width = (width_str) ? atoi(width_str) : 256;

    char* height_str = m_cmd_parser.GetCmdOption(argv, argv + argc, "-heiht");
    config.height = (height_str) ? atoi(height_str) : 256;

    return config;
}

void CmdLineParser::ShowHelp()
{
    std::cout << kHelpMessage << "\n";
}