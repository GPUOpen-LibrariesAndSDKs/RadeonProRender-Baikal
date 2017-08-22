
/**********************************************************************
 Copyright (c) 2017 Advanced Micro Devices, Inc. All rights reserved.
 
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

#include <thread>
#include <atomic>
#include <mutex>

#include "RenderFactory/render_factory.h"
#include "Renderers/ptrenderer.h"
#include "Renderers/bdptrenderer.h"
#include "Output/clwoutput.h"
#include "Application/app_utils.h"
#include "Utils/config_manager.h"
#include "Application/gl_render.h"

#ifdef ENABLE_DENOISER
#include "PostEffects/bilateral_denoiser.h"
#endif


namespace Baikal
{
    class AppClRender
    {
        struct OutputData
        {
            std::unique_ptr<Baikal::Output> output;

#ifdef ENABLE_DENOISER
            std::unique_ptr<Baikal::Output> output_position;
            std::unique_ptr<Baikal::Output> output_normal;
            std::unique_ptr<Baikal::Output> output_albedo;
            std::unique_ptr<Baikal::Output> output_denoised;
            std::unique_ptr<Baikal::PostEffect> denoiser;
#endif

            std::vector<float3> fdata;
            std::vector<unsigned char> udata;
            CLWBuffer<float3> copybuffer;
        };

        struct ControlData
        {
            std::atomic<int> clear;
            std::atomic<int> stop;
            std::atomic<int> newdata;
            std::mutex datamutex;
            int idx;
        };

    public:
        AppClRender(AppSettings& settings, GLuint tex);
        //copy data from to GL
        void Update(AppSettings& settings);

        //compile scene
        void UpdateScene();
        //render
        void Render(int sample_cnt);
        void StartRenderThreads();
        void StopRenderThreads();
        void RunBenchmark(AppSettings& settings);

        //save cl frame buffer to file
        void SaveFrameBuffer(AppSettings& settings);
        void SaveImage(const std::string& name, int width, int height, const RadeonRays::float3* data);

        CameraPtr GetCamera() { return m_camera; };
        Baikal::Scene1* GetScene() { return m_scene.get(); };
        CLWDevice GetDevice(int i) { return m_cfgs[m_primary].context.GetDevice(i); };
        Renderer::OutputType GetOutputType() { return m_output_type; };

        void SetNumBounces(int num_bounces);
        void SetOutputType(Renderer::OutputType type);
    private:
        void InitCl(AppSettings& settings, GLuint tex);
        void LoadScene(AppSettings& settings);
        void RenderThread(ControlData& cd);

        std::unique_ptr<Baikal::Scene1> m_scene;
        CameraPtr m_camera;

        std::vector<ConfigManager::Config> m_cfgs;
        std::vector<OutputData> m_outputs;
        std::unique_ptr<ControlData[]> m_ctrl;
        std::vector<std::thread> m_renderthreads;
        int m_primary = -1;

        //if interop
        CLWImage2D m_cl_interop_image;
        //save GL tex for no interop case
        GLuint m_tex;
        Renderer::OutputType m_output_type;
    };
}
