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
#include "render.h"

void Run(DGenConfig config)
{
    config.scene_file = (!config.scene_file.empty()) ? (config.scene_file) :
                                                       ("sphere + ibl.test");

    auto render = Render::Create(config.scene_file, config.scene_dir);

    render->LoadLightXml(config.light_dir, config.light_file);
    render->LoadCameraXml(config.camera_dir, config.camera_file);
    render->GenerateDataset(config.outpute_dir, config.outpute_file);
}

int main(int argc, char *argv[])
{
    try
    {
        ClineParser cline_parser;

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
    }
}