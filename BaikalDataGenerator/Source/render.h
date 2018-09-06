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

#include "config_loader.h"

#include <algorithm>
#include <memory>
#include <vector>

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
class CLWDevice;

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
           std::uint32_t num_bounces,
           unsigned device_idx);

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
                         const std::filesystem::path& lights_dir,
                         const std::vector<size_t>& spp,
                         const std::filesystem::path& output_dir,
                         std::int32_t cameras_index_offset = 0,
                         bool gamma_correction_enabled = true);

    ~Render();

private:
    void UpdateCameraSettings(const CameraInfo& cam_state);

    void SetLight(const LightInfo& light, const std::filesystem::path& lights_dir);

    template<template<class, class...> class TCamStatesRange, class... Args>
    void SetLightConfig(const TCamStatesRange<LightInfo, Args...>& lights,
                        const std::filesystem::path& lights_dir)
    {
        for (const auto& light : lights)
        {
            SetLight(light, lights_dir);
        }
    }

    void SaveOutput(const OutputInfo& info,
                    const std::string& name,
                    bool gamma_correction_enabled,
                    const std::filesystem::path& output_dir);

    void GenerateSample(const CameraInfo& cam_state,
                        const std::vector<size_t>& spp,
                        const std::filesystem::path& output_dir,
                        std::int32_t cameras_index_offset,
                        bool gamma_correction_enabled);

    void SaveMetadata(const std::filesystem::path& output_dir,
                      size_t cameras_start_idx,
                      size_t cameras_end_idx,
                      std::int32_t cameras_index_offset,
                      bool gamma_correction_enabled) const;

    std::filesystem::path m_scene_file;
    std::uint32_t m_width, m_height;
    std::uint32_t m_num_bounces;
    unsigned m_device_idx;
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
                             const std::filesystem::path& lights_dir,
                             const std::vector<size_t>& spp,
                             const std::filesystem::path& output_dir,
                             std::int32_t cameras_index_offset,
                             bool gamma_correction_enabled)
{
    using namespace RadeonRays;

    if (!std::filesystem::is_directory(output_dir))
    {
        THROW_EX("Incorrect output directory signature");
    }

    // check if number of samples to render wasn't specified
    if (spp.empty())
    {
        THROW_EX("SPP collection is empty");
    }

    SetLightConfig(lights, lights_dir);

    auto sorted_spp = spp;
    std::sort(sorted_spp.begin(), sorted_spp.end());

    sorted_spp.erase(std::unique(sorted_spp.begin(), sorted_spp.end()), sorted_spp.end());

    if (sorted_spp.front() <= 0)
    {
        THROW_EX("Found negative SPP: " << sorted_spp.front());
    }

    SaveMetadata(output_dir,
                 cam_states.begin()->index,
                 (cam_states.end() - 1)->index,
                 cameras_index_offset,
                 gamma_correction_enabled);

    for (const auto& cam_state : cam_states)
    {
        GenerateSample(cam_state,
                       sorted_spp,
                       output_dir,
                       cameras_index_offset,
                       gamma_correction_enabled);
    }
}
