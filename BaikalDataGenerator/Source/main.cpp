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
#include "config_loader.h"
#include "render.h"

void Run(const DGenConfig& config)
{
    ConfigLoader config_loader(config);

    Render render(config.scene_file, config.width, config.height);

    render.GenerateDataset(config_loader.GetCameraStates(),
                           config_loader.GetLightSettings(),
                           config_loader.GetSpp(),
                           config.output_dir);
}

int main(int argc, char *argv[])
{
    try
    {
        CmdLineParser cline_parser;

        if (cline_parser.CmdOptionExists(argv, argv + argc, "-help"))
        {
            cline_parser.ShowHelp();
            return 0;
        }

        auto config = cline_parser.Parse(argc, argv);

        Run(config);
    }
    catch (std::exception& ex)
    {
        std::cout << ex.what();
        return -1;
    }
}