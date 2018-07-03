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

#include "CLW.h"
#include "Renderers/renderer.h"
#include "RenderFactory/clw_render_factory.h"
#include "Output/output.h"
#include "SceneGraph/camera.h"
#include "scene_io.h"
#include "input_info.h"

#include "OpenImageIO/imageio.h"

#include <vector>
#include <set>
#include <memory>
#include <algorithm>
#include <iostream>

struct OutputDesc;

class Render
{
public:
    Render(std::filesystem::path scene_file,
           std::uint32_t output_width,
           std::uint32_t output_height);

    void GenerateDataset(const std::vector<CameraInfo>& camera_states,
                         const std::vector<LightInfo>& light_settings,
                         const std::vector<int>& spp,
                         const std::filesystem::path& output_dir,
                         bool gamma_correction_enabled = false);

private:
    void UpdateCameraSettings(const CameraInfo& cam_state);

    void SetLight(const std::vector<LightInfo>& light_settings);

    void SaveOutput(OutputDesc desc,
                    const std::filesystem::path& output_dir,
                    int cam_index,
                    int spp,
                    bool gamma_correction_enabled);

    std::unique_ptr<Baikal::Renderer> m_renderer;
    std::unique_ptr<Baikal::ClwRenderFactory> m_factory;
    std::unique_ptr<Baikal::SceneController<Baikal::ClwScene>> m_controller;
    std::vector<std::unique_ptr<Baikal::Output>> m_outputs;
    Baikal::Scene1::Ptr m_scene;
    Baikal::PerspectiveCamera::Ptr m_camera;
    CLWContext m_context;
};