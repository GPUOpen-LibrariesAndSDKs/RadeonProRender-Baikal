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

#include "cline_parser.h"
#include <iostream>

namespace
{
    char const* kHelpMessage =
        "Baikal [-light_dir path_to_light_config]"
        "[-light_file name_of_the_light_config]"
        "[-camera_dir path_to_camera_config]"
        "[-camera_file name_of_the_camera_config]"
        "[-outpute_dir path_to_generate_data]";
}

ClineParser::ClineParser() {}

char* ClineParser::GetCmdOption(char ** begin, char ** end, const std::string & option)
{
    char ** itr = std::find(begin, end, option);

    if (itr != end && ++itr != end)
    {
        return *itr;
    }

    return 0;
}

bool ClineParser::CmdOptionExists(char** begin, char** end, const std::string& option)
{
    return std::find(begin, end, option) != end;
}

DGenConfig ClineParser::Parse(int argc, char * argv[])
{
    DGenConfig config;

    char* light_dir = GetCmdOption(argv, argv + argc, "-light_dir");
    config.light_dir = light_dir ? light_dir : "";

    char* light_file = GetCmdOption(argv, argv + argc, "-light_file");
    config.light_file = light_file ? light_file : "";

    char* camera_dir = GetCmdOption(argv, argv + argc, "-camera_dir");
    config.camera_dir = camera_dir ? camera_dir : "";

    char* camera_file = GetCmdOption(argv, argv + argc, "-camera_file");
    config.camera_file = camera_file ? camera_file : "";

    char* output_file = GetCmdOption(argv, argv + argc, "-output_file");
    config.output_file = output_file ? output_file : "";

    char* output_dir = GetCmdOption(argv, argv + argc, "-output_dir");
    config.output_dir = output_dir ? output_dir : "";

    char* scene_dir = GetCmdOption(argv, argv + argc, "-scene_dir");
    config.scene_dir = scene_dir ? scene_dir : "";

    char* scene_file = GetCmdOption(argv, argv + argc, "-scene_file");
    config.scene_file = scene_file ? scene_file : "";

    return config;
}

void ClineParser::ShowHelp()
{
    std::cout << kHelpMessage << "\n";
}