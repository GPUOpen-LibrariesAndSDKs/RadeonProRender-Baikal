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
    constexpr char const* kHelpMessage =
        "Baikal [-light_file name_of_the_light_config]"
        "[-camera_file name_of_the_camera_config]"
        "[-scene_file name_of_the_scene_config]"
        "[-spp_file name_of_the_spp_config]"
        "[-outpute_dir path_to_generate_data]"
        "[-width output_width]"
        "[-height output_height]"
        "[-gamma enables_gamma_correction]";
}

CmdLineParser::CmdLineParser(int argc, char* argv[])
    : m_cmd_parser(argc, argv)
{   }

DGenConfig CmdLineParser::Parse() const
{
    DGenConfig config;

    config.light_file = m_cmd_parser.GetOption("-light_file");

    config.camera_file = m_cmd_parser.GetOption("-camera_file");

    config.output_dir = m_cmd_parser.GetOption("-output_dir");

    config.scene_file = m_cmd_parser.GetOption("-scene_file");

    config.spp_file = m_cmd_parser.GetOption("-spp_file");

    config.width = m_cmd_parser.GetOption<size_t>("-width");

    config.height = m_cmd_parser.GetOption<size_t>("-height");

    config.split_num = m_cmd_parser.GetOption<size_t>("-split_num", config.split_num);

    config.split_idx = m_cmd_parser.GetOption<size_t>("-split_idx", config.split_idx);

    config.offset_idx = m_cmd_parser.GetOption<size_t>("-offset_idx", config.offset_idx);

    config.gamma_correction = (m_cmd_parser.GetOption<int>("-gamma", 0) == 1);

    config.num_bounces = m_cmd_parser.GetOption<std::uint32_t>("-nb", config.num_bounces);

    return config;
}

void CmdLineParser::ShowHelp() const
{
    std::cout << kHelpMessage << "\n";
}

bool CmdLineParser::HasHelpOption() const
{
    return m_cmd_parser.OptionExists("-help");
}