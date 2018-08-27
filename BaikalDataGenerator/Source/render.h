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
    class ClwRenderFactory;
    class Output;
    class Scene1;
    class PerspectiveCamera;
    class MonteCarloRenderer;
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
    // 'num_bounces' - number of bounces for each ray
    Render(const std::filesystem::path& scene_file,
           size_t output_width,
           size_t output_height,
           std::uint32_t num_bounces = 5);

    // This function generates dataset for network training
    // 'cam_states' - camera states range
    // 'lights' - lights range
    // 'spp' - spp vector
    // 'output_dir' - output directory to save dataset
    // 'gamma_correction_enabled' - flag to enable/disable gamma correction
    // 'start_cam_id' - the number starting from will be named generated samples
    template<template <class, class...> class TCamStatesRange,
             template <class, class...> class TLightsRange,
             class... Args1,
             class... Args2>
    void GenerateDataset(const TCamStatesRange<CameraInfo, Args1 ...>& cam_states,
                            const TLightsRange<LightInfo, Args2 ...>& lights,
                            const std::vector<size_t>& spp,
                            const std::filesystem::path& output_dir,
                            bool gamma_correction_enabled = true,
                            size_t start_cam_id = 0);

    ~Render();

private:
    void UpdateCameraSettings(const CameraInfo& cam_state);

    void SetLight(const LightInfo& light);

    template<template<class, class...> class TCamStatesRange, class... Args>
    void SetLightConfig(const TCamStatesRange<LightInfo, Args...>& lights)
    {
        for (const auto& light : lights)
        {
            SetLight(light);
        }
    }

    void SaveOutput(const OutputInfo& info,
                    const std::string& name,
                    bool gamma_correction_enabled,
                    const std::filesystem::path& output_dir);

    void GenerateSample(const CameraInfo& cam_state,
                        const std::vector<size_t>& spp,
                        const std::filesystem::path& output_dir,
                        bool gamma_correction_enabled,
                        size_t start_cam_id);

    void SaveMetadata(const std::filesystem::path& output_dir) const;

    std::uint32_t m_num_bounces;
    std::uint32_t m_width, m_height;
    std::unique_ptr<Baikal::MonteCarloRenderer> m_renderer;
    std::unique_ptr<Baikal::ClwRenderFactory> m_factory;
    std::unique_ptr<Baikal::SceneController<Baikal::ClwScene>> m_controller;
    std::vector<std::unique_ptr<Baikal::Output>> m_outputs;
    std::shared_ptr<Baikal::Scene1> m_scene;
    std::shared_ptr<Baikal::PerspectiveCamera> m_camera;
    std::unique_ptr<CLWContext> m_context;
};



////////////////////////////////////////////////
//// GenerateDataset impl
////////////////////////////////////////////////

template<template<class, class...> class TCamStatesRange,
         template<class, class...> class TLightsRange,
         class... Args1, class... Args2>
void Render::GenerateDataset(const TCamStatesRange<CameraInfo, Args1 ...>& cam_states,
                             const TLightsRange<LightInfo, Args2 ...>& lights,
                             const std::vector<size_t>& spp,
                             const std::filesystem::path& output_dir,
                             bool gamma_correction_enabled,
                             size_t start_cam_id)
{
    using namespace RadeonRays;

    if (!std::filesystem::is_directory(output_dir))
    {
        THROW_EX("incorrect output directory signature");
    }

    // check if number of samples to render wasn't specified
    if (spp.empty())
    {
        THROW_EX("spp collection is empty");
    }

    SetLightConfig(lights);

    auto sorted_spp = spp;
    std::sort(sorted_spp.begin(), sorted_spp.end());

    sorted_spp.erase(std::unique(sorted_spp.begin(), sorted_spp.end()), sorted_spp.end());

    if (sorted_spp.front() <= 0)
    {
        THROW_EX("spp should be positive");
    }

    SaveMetadata(output_dir);

    for (const auto& cam_state : cam_states)
    {
        GenerateSample(cam_state, sorted_spp, output_dir, gamma_correction_enabled, start_cam_id);
        start_cam_id++;
    }
}