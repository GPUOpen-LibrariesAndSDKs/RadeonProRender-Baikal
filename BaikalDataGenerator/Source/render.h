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

#include "data_generator.h"
#include "filesystem.h"

#include "Rpr/WrapObject/CameraObject.h"
#include "Rpr/WrapObject/LightObject.h"
#include "Rpr/WrapObject/SceneObject.h"

#include <algorithm>
#include <memory>
#include <vector>


namespace Baikal
{
    struct ClwScene;
    class ClwRenderFactory;
    class Light;
    class Output;
    class Scene1;
    class PerspectiveCamera;
    class MonteCarloRenderer;
    template <class T>
    class SceneController;
}

class CLWContext;
class CLWDevice;

struct OutputInfo;


class Render
{
public:
    // 'scene_file' - full path till .obj/.objm or some kind of this files with scene
    // 'output_width' - width of outputs which will be saved on disk
    // 'output_height' - height of outputs which will be saved on disk
    // 'num_bounces' - number of bounces for each ray
    Render(SceneObject* scene,
           size_t output_width,
           size_t output_height,
           std::uint32_t num_bounces,
           unsigned device_idx);

    void AttachLight(LightObject* light);

    /// This function generates dataset for network training
    ///
    /// @param output_dir Output directory to save dataset
    /// @param scene_name Scene name
    /// @param cameras_start_idx The save index of the 1st camera
    /// @param cameras_end_idx The save index of the last camera
    /// @param cam_states Camera states range
    /// @param lights - lights range
    /// 'spp' - spp vector
    /// 'gamma_correction_enabled' - flag to enable/disable gamma correction
    /// 'start_cam_id' - the number starting from will be named generated samples

    void SaveMetadata(const std::filesystem::path& output_dir,
                      const std::string& scene_name,
                      unsigned cameras_start_idx,
                      unsigned cameras_end_idx,
                      int cameras_index_offset,
                      bool gamma_correction_enabled) const;

    void GenerateSample(CameraObject* camera,
                        int camera_idx,
                        const std::vector<size_t>& spp,
                        const std::filesystem::path& output_dir,
                        bool gamma_correction_enabled);

    ~Render();

private:
    void SaveOutput(const OutputInfo& info,
                    const std::string& name,
                    bool gamma_correction_enabled,
                    const std::filesystem::path& output_dir);

    SceneObject* m_scene = nullptr;
    std::uint32_t m_width, m_height;
    std::uint32_t m_num_bounces;
    unsigned m_device_idx;
    std::unique_ptr<Baikal::MonteCarloRenderer> m_renderer;
    std::unique_ptr<Baikal::ClwRenderFactory> m_factory;
    std::unique_ptr<Baikal::SceneController<Baikal::ClwScene>> m_controller;
    std::vector<std::unique_ptr<Baikal::Output>> m_outputs;
    std::unique_ptr<CLWContext> m_context;
};
