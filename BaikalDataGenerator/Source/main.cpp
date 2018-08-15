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

    if (config.split_num != 0)
    {
        if (static_cast<size_t>(config.split_num) > config_loader.CamStates().size())
        {
            THROW_EX("'split_num' option value is bigger than camera states number");
        }

        if (config.split_idx >= config.split_num)
        {
            THROW_EX("'split_idx' must be less than split_num");
        }

        auto camera_states = config_loader.CamStates();
        auto dataset_size = camera_states.size() / config.split_num;
        auto begin_cam = camera_states.begin() + config.split_idx * dataset_size;

        // remaining cameras will be added into the last dataset
        auto end_cam = (config.split_idx < config.split_num - 1) ?
            (begin_cam  + dataset_size) : camera_states.end();

        render.GenerateDataset({ begin_cam, end_cam },
                               config_loader.Lights(),
                               config_loader.Spp(),
                               config.output_dir,
                               config.gamma_correction,
                               config.offset_idx);
    }
    else
    {
        render.GenerateDataset(config_loader.CamStates(),
                               config_loader.Lights(),
                               config_loader.Spp(),
                               config.output_dir,
                               config.gamma_correction);
    }
}

int main(int argc, char *argv[])
{
    try
    {
        CmdLineParser cmd_parser(argc, argv);

        if (cmd_parser.HasHelpOption())
        {
            cmd_parser.ShowHelp();
            return 0;
        }

        auto config = cmd_parser.Parse();

        Run(config);
    }
    catch (std::exception& ex)
    {
        std::cout << ex.what() << std::endl;
        return -1;
    }
}