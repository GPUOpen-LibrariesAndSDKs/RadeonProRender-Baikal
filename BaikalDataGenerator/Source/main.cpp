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
#include "data_generator.h"
#include "devices.h"
#include "logging.h"
#include "object_loader.h"
#include "utils.h"

#include "Rpr/WrapObject/SceneObject.h"
#include "Rpr/WrapObject/CameraObject.h"
#include "Rpr/WrapObject/LightObject.h"

#include "BaikalIO/scene_io.h"

#include <ctime>
#include <csignal>


void Run(const AppConfig& config)
{
    ObjectLoader object_loader(config);
    auto params = object_loader.GetDataGeneratorParams();
    GenerateDataset(&params);
}

int main(int argc, char* argv[])
try
{
    auto OnCancel = [](int signal)
    {
        DG_LOG(KeyValue("status", "canceled")
            << KeyValue("end_ts", std::time(nullptr))
            << KeyValue("signal", signal));
        std::exit(-1);
    };
#ifdef WIN32
    std::signal(SIGBREAK, OnCancel);
#endif
    std::signal(SIGINT, OnCancel);
    std::signal(SIGTERM, OnCancel);

    auto OnFailure = [](int signal)
    {
        DG_LOG(KeyValue("status", "failed")
            << KeyValue("end_ts", std::time(nullptr))
            << KeyValue("signal", signal));
    };
    std::signal(SIGABRT, OnFailure);
    std::signal(SIGFPE, OnFailure);
    std::signal(SIGILL, OnFailure);
    std::signal(SIGSEGV, OnFailure);

    CmdLineParser cmd_parser(argc, argv);

    if (cmd_parser.HasHelpOption())
    {
        cmd_parser.ShowHelp();
        return 0;
    }

    if (cmd_parser.HasListDevicesOption())
    {
        std::cout << "Device list:\n";
        auto devices = GetDevices();
        for (std::size_t idx = 0; idx < devices.size(); ++idx)
        {
            DG_LOG(KeyValue("idx", idx)
                << KeyValue("name", devices[idx].GetName())
                << KeyValue("vendor", devices[idx].GetVendor())
                << KeyValue("version", devices[idx].GetVersion()));
        }
        return 0;
    }

    DG_LOG(KeyValue("status", "running") << KeyValue("start_ts", std::time(nullptr)));

    auto config = cmd_parser.Parse();
    ObjectLoader object_loader(config);
    auto params = object_loader.GetDataGeneratorParams();

    auto progress_callback = [](int camera_idx)
    {
        DG_LOG(KeyValue("event", "generated")
            << KeyValue("status", "generating")
            << KeyValue("camera_idx", camera_idx));
    };

    params.progress_callback = progress_callback;

    auto result = GenerateDataset(&params);
    if (result != kDataGeneratorSuccess)
    {
        THROW_EX("Generation failed: result=" << result);
    }

    DG_LOG(KeyValue("status", "finished") << KeyValue("end_ts", std::time(nullptr)));
}
catch (std::exception& ex)
{
    DG_LOG(KeyValue("status", "failed") << KeyValue("end_ts", std::time(nullptr)));
    std::cout << ex.what() << std::endl;
    return -1;
}
