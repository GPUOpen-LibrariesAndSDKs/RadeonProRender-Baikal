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

#pragma once

#include <vector>
#include <set>
#include <memory>
#include <algorithm>
#include <iostream>

#include "config_loader.h"

struct OutputInfo;

namespace Baikal
{
    struct ClwScene;
    class Renderer;
    class ClwRenderFactory;
    class Output;
    class Scene1;
    class PerspectiveCamera;

    template <class T = ClwScene>
    class SceneController;
}

class CLWContext;

class Render
{
public:
    // 'scene_file' - full path till .obj/.objm or some kind of this files with scene
    // 'output_width' - width of outputs which will be saved on disk
    // 'output_height' - height of outputs which will be saved on disk
    Render(const std::filesystem::path& scene_file,
           std::uint32_t output_width,
           std::uint32_t output_height);

    // This function generates dataset for network training
    // 'cam_begin' - begin iterator on camera states collection
    // 'cam_end' - end iterator camera states collection
    // 'light_begin' - begin iterator on lights collection
    // 'light_end' - end iterator on lights collection
    // 'spp_begin' - begin iterator on spp collection
    // 'spp_end' - end iterator on spp collection
    // 'output_dir' - output directory to save dataset
    // 'gamma_correction_enabled' - flag to enable/disable gamma correction
    void GenerateDataset(CameraIterator cam_begin, CameraIterator cam_end,
                         LightsIterator light_begin, LightsIterator light_end,
                         SppIterator spp_begin, SppIterator spp_end,
                         const std::filesystem::path& output_dir,
                         bool gamma_correction_enabled = false);

    ~Render();

private:
    void UpdateCameraSettings(CameraIterator cam_state);

    void SetLightConfig(LightsIterator begin, LightsIterator end);

    void SaveOutput(const OutputInfo& info,
                    const std::string& name,
                    bool gamma_correction_enabled,
                    const std::filesystem::path& output_dir);

    std::uint32_t m_width, m_height;
    std::unique_ptr<Baikal::Renderer> m_renderer;
    std::unique_ptr<Baikal::ClwRenderFactory> m_factory;
    std::unique_ptr<Baikal::SceneController<Baikal::ClwScene>> m_controller;
    std::vector<std::unique_ptr<Baikal::Output>> m_outputs;
    std::shared_ptr<Baikal::Scene1> m_scene;
    std::shared_ptr<Baikal::PerspectiveCamera> m_camera;
    std::unique_ptr<CLWContext> m_context;
};