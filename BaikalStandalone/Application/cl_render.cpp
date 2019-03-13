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
#include "OpenImageIO/imageio.h"
#include "image_io.h"

#include "Application/cl_render.h"
#include "Application/scene_load_utils.h"
#include "Application/gl_render.h"
#include "Utils/output_accessor.h"

#include "SceneGraph/scene1.h"
#include "SceneGraph/camera.h"
#include "SceneGraph/material.h"
#include "BaikalIO/scene_io.h"
#include "BaikalIO/image_io.h"
#include "BaikalIO/material_io.h"
#include "Renderers/monte_carlo_renderer.h"
#include "Renderers/adaptive_renderer.h"
#include "Utils/clw_class.h"

#include "OpenImageIO/imageio.h"

#include <fstream>
#include <sstream>
#include <thread>
#include <chrono>
#include <cmath>

#include "PostEffects/ML/ml_post_effect.h"

namespace Baikal
{

    namespace
    {
        constexpr float kGamma = 2.2f;

        // Define the IMAGE_DUMP_PATH value to dump render data
#ifdef IMAGE_DUMP_PATH
        std::unique_ptr<RendererOutputAccessor> s_output_accessor;
#endif
    }

    AppClRender::AppClRender(AppSettings& settings, GLuint tex)
    : m_tex(tex)
    , m_post_processing_type(settings.post_processing_type)
    {
        InitCl(settings, m_tex);
        InitPostEffect(settings);
        InitScene(settings);

#ifdef IMAGE_DUMP_PATH
        s_output_accessor = std::make_unique<RendererOutputAccessor>(
                IMAGE_DUMP_PATH, m_width, m_height);
#endif
    }

    void AppClRender::InitCl(AppSettings& settings, GLuint tex)
    {
        // Create cl context
        CreateConfigs(
            settings.mode,
            settings.interop,
            m_cfgs,
            settings.num_bounces,
            settings.platform_index,
            settings.device_index);

        m_width = (m_post_processing_type == PostProcessingType::kMLUpsample) ?
            settings.width / 2 : settings.width;

        m_height = (m_post_processing_type == PostProcessingType::kMLUpsample) ?
                   settings.height / 2 : settings.height;

        std::cout << "Running on devices: \n";

        for (std::size_t i = 0; i < m_cfgs.size(); ++i)
        {
            std::cout << i << ": " << m_cfgs[i].context.GetDevice(0).GetName() << "\n";
        }

        settings.interop = false;

        m_ctrl = std::make_unique<ControlData[]>(m_cfgs.size());

        for (std::size_t i = 0; i < m_cfgs.size(); ++i)
        {
            if (m_cfgs[i].type == DeviceType::kPrimary)
            {
                m_primary = i;

                if (m_cfgs[i].caninterop)
                {
                    m_cl_interop_image = m_cfgs[i].context.CreateImage2DFromGLTexture(tex);
                    settings.interop = true;
                }
            }

            m_ctrl[i].clear.store(1);
            m_ctrl[i].stop.store(0);
            m_ctrl[i].newdata.store(0);
            m_ctrl[i].idx = static_cast<int>(i);
            m_ctrl[i].scene_state = 0;
        }

        if (settings.interop)
        {
            std::cout << "OpenGL interop mode enabled\n";
        }
        else
        {
            std::cout << "OpenGL interop mode disabled\n";
        }

        m_outputs.resize(m_cfgs.size());

        //create renderer
        for (std::size_t i = 0; i < m_cfgs.size(); ++i)
        {
            AddRendererOutput(i, Renderer::OutputType::kColor);
            m_outputs[i].dummy_output = m_cfgs[i].factory->CreateOutput(m_width, m_height); // TODO: mldenoiser, clear?

            m_outputs[i].fdata.resize(m_width * m_height);
            m_outputs[i].udata.resize(4 * m_width * m_height);
        }

        m_shape_id_data.output = m_cfgs[m_primary].factory->CreateOutput(m_width, m_height);
        m_cfgs[m_primary].renderer->Clear(RadeonRays::float3(0, 0, 0), *m_shape_id_data.output);
        m_copybuffer = m_cfgs[m_primary].context.CreateBuffer<RadeonRays::float3>(m_width * m_height, CL_MEM_READ_WRITE);
    }

    void AppClRender::AddPostEffect(size_t device_idx, PostEffectType type)
    {
        m_post_effect = m_cfgs[device_idx].factory->CreatePostEffect(type);

        // create or get inputs for post-effect
        for (auto required_input : m_post_effect->GetInputTypes())
        {
            AddRendererOutput(device_idx, required_input);
            m_post_effect_inputs[required_input] = GetRendererOutput(device_idx, required_input);
        }

        // create buffer for post-effect output
        if (m_post_processing_type != PostProcessingType::kMLUpsample)
        {
            m_post_effect_output = m_cfgs[device_idx].factory->CreateOutput(m_width, m_height);
        }
        else
        {
            m_post_effect_output = m_cfgs[device_idx].factory->CreateOutput(2 * m_width, 2 * m_height);
            m_upscaled_img = m_cfgs[device_idx].factory->CreateOutput(2 * m_width, 2 * m_height);
        }

        m_shape_id_data.output = m_cfgs[m_primary].factory->CreateOutput(m_width, m_height);
        m_cfgs[m_primary].renderer->Clear(RadeonRays::float3(0, 0, 0), *m_shape_id_data.output);
    }

    PostProcessingType AppClRender::GetDenoiserType() const
    {
        return m_post_processing_type;
    }

    void AppClRender::SetDenoiserFloatParam(std::string const& name, float value)
    {
        m_post_effect->SetParameter(name, value);
    }

    float AppClRender::GetDenoiserFloatParam(std::string const& name) const
    {
        return m_post_effect->GetParameter(name).GetFloat();
    }

    void AppClRender::InitScene(AppSettings& settings)
    {
        rand_init();

        m_scene = LoadScene(settings);

        switch (settings.camera_type)
        {
        case CameraType::kPerspective:
            m_camera = Baikal::PerspectiveCamera::Create(
                    settings.camera_pos
                    , settings.camera_at
                    , settings.camera_up);
            break;
        case CameraType::kOrthographic:
            m_camera = Baikal::OrthographicCamera::Create(
                    settings.camera_pos
                    , settings.camera_at
                    , settings.camera_up);
            break;
        default:
            throw std::runtime_error("AppClRender::InitCl(...): unsupported camera type");
        }

        m_scene->SetCamera(m_camera);

        // Adjust sensor size based on current aspect ratio
        float aspect = (float)settings.width / settings.height;
        settings.camera_sensor_size.y = settings.camera_sensor_size.x / aspect;

        m_camera->SetSensorSize(settings.camera_sensor_size);
        m_camera->SetDepthRange(settings.camera_zcap);

        auto perspective_camera = std::dynamic_pointer_cast<Baikal::PerspectiveCamera>(m_camera);

        // if camera mode is kPerspective
        if (perspective_camera)
        {
            perspective_camera->SetFocalLength(settings.camera_focal_length);
            perspective_camera->SetFocusDistance(settings.camera_focus_distance);
            perspective_camera->SetAperture(settings.camera_aperture);
            std::cout << "Camera type: " << (perspective_camera->GetAperture() > 0.f ? "Physical" : "Pinhole") << "\n";
            std::cout << "Lens focal length: " << perspective_camera->GetFocalLength() * 1000.f << "mm\n";
            std::cout << "Lens focus distance: " << perspective_camera->GetFocusDistance() << "m\n";
            std::cout << "F-Stop: " << 1.f / (perspective_camera->GetAperture() * 10.f) << "\n";
        }

        std::cout << "Sensor size: " << settings.camera_sensor_size.x * 1000.f << "x" << settings.camera_sensor_size.y * 1000.f << "mm\n";
    }

    void AppClRender::UpdateScene()
    {
        for (std::size_t i = 0; i < m_cfgs.size(); ++i)
        {
            if (i == m_primary)
            {
                m_cfgs[i].renderer->Clear(float3(), *GetRendererOutput(i, Renderer::OutputType::kColor));
                m_cfgs[i].controller->CompileScene(m_scene);
                ++m_ctrl[i].scene_state;

                if (m_post_effect_output)
                {
                    m_post_effect_output->Clear(float3());
                }
            }
            else
                m_ctrl[i].clear.store(true);
        }

        for (auto& output : m_outputs[m_primary].render_outputs)
        {
            output.second->Clear(float3());
        }
    }

    void AppClRender::Update(AppSettings& settings)
    {
        ++settings.samplecount;

#ifdef IMAGE_DUMP_PATH
        s_output_accessor->SaveAllOutputs(m_outputs);
#endif

        for (std::size_t i = 0; i < m_cfgs.size(); ++i)
        {
            if (m_cfgs[i].type == DeviceType::kPrimary)
            {
                continue;
            }

            int desired = 1;
            if (std::atomic_compare_exchange_strong(&m_ctrl[i].newdata, &desired, 0))
            {
                if (m_ctrl[i].scene_state != m_ctrl[m_primary].scene_state)
                {
                    // Skip update if worker has sent us non-actual data
                    continue;
                }

                m_cfgs[m_primary].context.WriteBuffer(
                        0, m_copybuffer,
                        &m_outputs[i].fdata[0],
                        m_width * m_height);

                auto acckernel = static_cast<MonteCarloRenderer*>(m_cfgs[m_primary].renderer.get())->GetAccumulateKernel();

                int argc = 0;
                acckernel.SetArg(argc++, m_copybuffer);
                acckernel.SetArg(argc++, m_width * m_height);
                acckernel.SetArg(argc++,
                        static_cast<Baikal::ClwOutput*>(GetRendererOutput(
                                m_primary, Renderer::OutputType::kColor))->data());

                int globalsize = m_width * m_height;
                m_cfgs[m_primary].context.Launch1D(0, ((globalsize + 63) / 64) * 64, 64, acckernel);
                settings.samplecount += m_ctrl[i].new_samples_count;
            }
        }

        if (!settings.interop)
        {
            if (m_post_effect_output)
            {
                m_post_effect_output->GetData(&m_outputs[m_primary].fdata[0]);
                ApplyGammaCorrection(m_primary, kGamma, false);
            }
            else
            {
                GetOutputData(m_primary, Renderer::OutputType::kColor, &m_outputs[m_primary].fdata[0]);
                ApplyGammaCorrection(m_primary, kGamma, true);
            }

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, m_tex);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_width, m_height, GL_RGBA, GL_UNSIGNED_BYTE, &m_outputs[m_primary].udata[0]);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        else
        {
            if (!m_post_effect_output)
            {
                CopyToGL(GetRendererOutput(m_primary, m_output_type));
            }
            else
            {
                if (m_output_type == Renderer::OutputType::kColor)
                {
                    if (settings.split_output)
                    {
                        if (m_post_processing_type != PostProcessingType::kMLUpsample)
                        {
                            CopyToGL(GetRendererOutput(m_primary, Renderer::OutputType::kColor),
                                     m_post_effect_output.get());
                        }
                        else
                        {
                            auto ml_post_effect = dynamic_cast<PostEffects::MLPostEffect*>(m_post_effect.get());

                            ml_post_effect->Resize_2x(dynamic_cast<ClwOutput*>(m_upscaled_img.get())->data(),
                                                      dynamic_cast<ClwOutput*>(GetRendererOutput(
                                                      m_primary,
                                                      Renderer::OutputType::kColor))->data());

                            CopyToGL(m_upscaled_img.get(), m_post_effect_output.get());
                        }
                    }
                    else
                    {
                        CopyToGL(m_post_effect_output.get());
                    }
                }
                else
                {
                    CopyToGL(GetRendererOutput(m_primary, m_output_type));
                }
            }
        }

        if (settings.benchmark)
        {
            auto& scene = m_cfgs[m_primary].controller->CompileScene(m_scene);
            dynamic_cast<MonteCarloRenderer*>(m_cfgs[m_primary].renderer.get())->Benchmark(scene, settings.stats);

            settings.benchmark = false;
            settings.rt_benchmarked = true;
        }

    }

    void AppClRender::Render(int sample_cnt)
    {
        SetPostEffectParams(sample_cnt);

        auto& scene = m_cfgs[m_primary].controller->GetCachedScene(m_scene);
        m_cfgs[m_primary].renderer->Render(scene);

        if (m_shape_id_requested)
        {
            // offset in OpenCl memory till necessary item
            auto offset = (std::uint32_t)(m_width * (m_height - m_shape_id_pos.y) + m_shape_id_pos.x);
            // copy shape id elem from OpenCl
            float4 shape_id;
            m_shape_id_data.output->GetData((float3*)&shape_id, offset, 1);
            m_promise.set_value(static_cast<int>(shape_id.x));
            // clear output to stop tracking shape id map in openCl
            m_cfgs[m_primary].renderer->SetOutput(Renderer::OutputType::kShapeId, nullptr);
            m_shape_id_requested = false;
        }

        if (m_post_effect && m_output_type == Renderer::OutputType::kColor)
        {
            m_post_effect->Apply(m_post_effect_inputs, *m_post_effect_output);
        }
    }

    void AppClRender::SaveFrameBuffer(AppSettings& settings)
    {
        std::vector<RadeonRays::float3> data;

        //read cl output in case of iterop
        std::vector<RadeonRays::float3> output_data;
        if (settings.interop)
        {
            auto output = GetRendererOutput(m_primary, Renderer::OutputType::kColor);
            auto buffer = dynamic_cast<Baikal::ClwOutput*>(output)->data();
            output_data.resize(buffer.GetElementCount());
            m_cfgs[m_primary].context.ReadBuffer(
                    0,
                    dynamic_cast<Baikal::ClwOutput*>(output)->data(),
                    &output_data[0],
                    output_data.size()).Wait();
        }

        //use already copied to CPU cl data in case of no interop
        auto& fdata = settings.interop ? output_data : m_outputs[m_primary].fdata;

        data.resize(fdata.size());
        std::transform(fdata.cbegin(), fdata.cend(), data.begin(),
            [](RadeonRays::float3 const& v)
        {
            float invw = 1.f / v.w;
            return v * invw;
        });

        std::stringstream oss;
        auto camera_position = m_camera->GetPosition();
        auto camera_direction = m_camera->GetForwardVector();
        oss << "../Output/" << settings.modelname << "_p" << camera_position.x << camera_position.y << camera_position.z <<
            "_d" << camera_direction.x << camera_direction.y << camera_direction.z <<
            "_s" << settings.num_samples << ".exr";

        SaveImage(oss.str(), settings.width, settings.height, data.data());
    }

    void AppClRender::SaveImage(const std::string& name, int width, int height, const RadeonRays::float3* data)
    {
        OIIO_NAMESPACE_USING;

        std::vector<float3> tempbuf(width * height);
        tempbuf.assign(data, data + width * height);

        for (auto y = 0; y < height; ++y)
            for (auto x = 0; x < width; ++x)
            {

                float3 val = data[(height - 1 - y) * width + x];
                tempbuf[y * width + x] = (1.f / val.w) * val;

                tempbuf[y * width + x].x = std::pow(tempbuf[y * width + x].x, 1.f / kGamma);
                tempbuf[y * width + x].y = std::pow(tempbuf[y * width + x].y, 1.f / kGamma);
                tempbuf[y * width + x].z = std::pow(tempbuf[y * width + x].z, 1.f / kGamma);
            }

        ImageOutput* out = ImageOutput::create(name);

        if (!out)
        {
            throw std::runtime_error("Can't create image file on disk");
        }

        ImageSpec spec(width, height, 3, TypeDesc::FLOAT);

        out->open(name, spec);
        out->write_image(TypeDesc::FLOAT, &tempbuf[0], sizeof(float3));
        out->close();
    }

    void AppClRender::RenderThread(ControlData& cd)
    {
        auto renderer = m_cfgs[cd.idx].renderer.get();
        auto controller = m_cfgs[cd.idx].controller.get();

        auto updatetime = std::chrono::high_resolution_clock::now();

        std::uint32_t scene_state = 0;
        std::uint32_t new_samples_count = 0;

        while (!cd.stop.load())
        {
            int result = 1;
            bool update = false;
            if (std::atomic_compare_exchange_strong(&cd.clear, &result, 0))
            {
                for (auto& output : m_outputs[cd.idx].render_outputs)
                {
                    output.second->Clear(float3());
                }
                controller->CompileScene(m_scene);
                scene_state = m_ctrl[m_primary].scene_state;
                update = true;
            }

            auto& scene = controller->GetCachedScene(m_scene);
            renderer->Render(scene);
            ++new_samples_count;

            auto now = std::chrono::high_resolution_clock::now();

            update = update || (std::chrono::duration_cast<std::chrono::seconds>(now - updatetime).count() > 1);

            if (update)
            {
                GetOutputData(cd.idx, Renderer::OutputType::kColor, &m_outputs[cd.idx].fdata[0]);
                updatetime = now;
                m_ctrl[cd.idx].scene_state = scene_state;
                m_ctrl[cd.idx].new_samples_count = new_samples_count;
                new_samples_count = 0;
                cd.newdata.store(1);
            }

            m_cfgs[cd.idx].context.Finish(0);
        }
    }

    void AppClRender::StartRenderThreads()
    {
        for (std::size_t i = 0; i < m_cfgs.size(); ++i)
        {
            if (i != m_primary)
            {
                m_renderthreads.emplace_back(&AppClRender::RenderThread, this, std::ref(m_ctrl[i]));
            }
        }

        std::cout << m_renderthreads.size() << " OpenCL submission threads started\n";
    }

    void AppClRender::StopRenderThreads()
    {
        for (std::size_t i = 0; i < m_cfgs.size(); ++i)
        {
            if (i != m_primary)
            {
                m_ctrl[i].stop.store(true);
            }
        }

        for (std::size_t i = 0; i < m_renderthreads.size(); ++i)
        {
            m_renderthreads[i].join();
        }
    }

    void AppClRender::RunBenchmark(AppSettings& settings)
    {
        std::cout << "Running general benchmark...\n";

        auto time_bench_start_time = std::chrono::high_resolution_clock::now();
        for (auto i = 0U; i < 512; ++i)
        {
            Render(0);
        }

        m_cfgs[m_primary].context.Finish(0);

        auto delta = std::chrono::duration_cast<std::chrono::milliseconds>
            (std::chrono::high_resolution_clock::now() - time_bench_start_time).count();

        settings.time_benchmark_time = delta / 1000.f;

        GetOutputData(m_primary, Renderer::OutputType::kColor, &m_outputs[m_primary].fdata[0]);
        ApplyGammaCorrection(m_primary, kGamma, true);

        auto& fdata = m_outputs[m_primary].fdata;
        std::vector<RadeonRays::float3> data(fdata.size());
        std::transform(fdata.cbegin(), fdata.cend(), data.begin(),
            [](RadeonRays::float3 const& v)
        {
            float invw = 1.f / v.w;
            return v * invw;
        });

        std::stringstream oss;
        oss << "../Output/" << settings.modelname << ".exr";

        SaveImage(oss.str(), settings.width, settings.height, &data[0]);

        std::cout << "Running RT benchmark...\n";

        auto& scene = m_cfgs[m_primary].controller->GetCachedScene(m_scene);
        dynamic_cast<MonteCarloRenderer*>(m_cfgs[m_primary].renderer.get())->Benchmark(scene, settings.stats);
    }

    void AppClRender::ApplyGammaCorrection(size_t device_idx, float gamma, bool divide_by_spp = true)
    {
        auto adjust_gamma = [gamma](float val)
        {
            return (unsigned char)clamp(std::pow(val , 1.f / gamma) * 255, 0, 255);
        };

        if (divide_by_spp)
        {
            for (int i = 0; i < (int)m_outputs[device_idx].fdata.size(); ++i)
            {
                auto &fdata = m_outputs[device_idx].fdata[i];
                auto &udata = m_outputs[device_idx].udata;

                udata[4 * i] = adjust_gamma(fdata.x / fdata.w);
                udata[4 * i + 1] = adjust_gamma(fdata.y / fdata.w);
                udata[4 * i + 2] = adjust_gamma(fdata.z / fdata.w);
                udata[4 * i + 3] = 1;
            }
        }
        else
        {
            for (int i = 0; i < (int)m_outputs[device_idx].fdata.size(); ++i)
            {
                auto &fdata = m_outputs[device_idx].fdata[i];
                auto &udata = m_outputs[device_idx].udata;

                udata[4 * i] = adjust_gamma(fdata.x);
                udata[4 * i + 1] = adjust_gamma(fdata.y);
                udata[4 * i + 2] = adjust_gamma(fdata.z);
                udata[4 * i + 3] = 1;
            }
        }
    }

    void AppClRender::SetNumBounces(int num_bounces)
    {
        for (std::size_t i = 0; i < m_cfgs.size(); ++i)
        {
            dynamic_cast<Baikal::MonteCarloRenderer*>(m_cfgs[i].renderer.get())->SetMaxBounces(num_bounces);
        }
    }

    void AppClRender::SetOutputType(Renderer::OutputType type)
    {
        m_output_type = type;
    }

    std::future<int> AppClRender::GetShapeId(std::uint32_t x, std::uint32_t y)
    {
        m_promise = std::promise<int>();
        if (x >= m_width || y >= m_height)
            throw std::logic_error(
                "AppClRender::GetShapeId(...): x or y cords beyond the size of image");

        if (m_cfgs.empty())
            throw std::runtime_error("AppClRender::GetShapeId(...): config vector is empty");

        // enable aov shape id output from OpenCl
        m_cfgs[m_primary].renderer->SetOutput(
            Renderer::OutputType::kShapeId, m_shape_id_data.output.get());
        m_shape_id_pos = RadeonRays::float2((float)x, (float)y);
        // request shape id from render
        m_shape_id_requested = true;
        return m_promise.get_future();
    }

    Baikal::Shape::Ptr AppClRender::GetShapeById(int shape_id)
    {
        if (shape_id < 0)
            return nullptr;

        // find shape in scene by its id
        for (auto iter = m_scene->CreateShapeIterator(); iter->IsValid(); iter->Next())
        {
            auto shape = iter->ItemAs<Shape>();
            if (shape->GetId() == static_cast<std::size_t>(shape_id))
                return shape;
        }
        return nullptr;
    }

    void AppClRender::AddRendererOutput(size_t device_idx, Renderer::OutputType type)
    {
        auto it = m_outputs[device_idx].render_outputs.find(type);
        if (it == m_outputs[device_idx].render_outputs.end())
        {
            const auto& config = m_cfgs.at(device_idx);
            auto output = config.factory->CreateOutput(m_width, m_height);
            config.renderer->SetOutput(type, output.get());
            config.renderer->Clear(RadeonRays::float3(0, 0, 0), *output);

            m_outputs[device_idx].render_outputs.emplace(type, std::move(output));
        }
    }

    Output* AppClRender::GetRendererOutput(size_t device_idx, Renderer::OutputType type)
    {
        return m_outputs.at(device_idx).render_outputs.at(type).get();
    }

    void AppClRender::GetOutputData(size_t device_idx, Renderer::OutputType type, RadeonRays::float3* data) const
    {
        m_outputs.at(device_idx).render_outputs.at(type)->GetData(data);
    }

    void AppClRender::CopyToGL(Output* output)
    {
        std::vector<cl_mem> objects = {m_cl_interop_image};
        m_cfgs[m_primary].context.AcquireGLObjects(0, objects);

        auto copykernel = dynamic_cast<MonteCarloRenderer*>(
                m_cfgs[m_primary].renderer.get())->GetCopyKernel();

        copykernel.SetArg(0, dynamic_cast<ClwOutput*>(output)->data());
        copykernel.SetArg(1, output->width());
        copykernel.SetArg(2, output->height());
        copykernel.SetArg(3, kGamma);
        copykernel.SetArg(4, m_cl_interop_image);

        int globalsize = output->width() * output->height();
        m_cfgs[m_primary].context.Launch1D(0, (globalsize + 63) / 64 * 64, 64, copykernel);

        m_cfgs[m_primary].context.ReleaseGLObjects(0, objects);
        m_cfgs[m_primary].context.Finish(0);
    }

    void AppClRender::CopyToGL(Output* left_output, Output* right_output)
    {
        assert(left_output->width() == right_output->width());
        assert(left_output->height() == right_output->height());

        std::vector<cl_mem> objects = {m_cl_interop_image};
        m_cfgs[m_primary].context.AcquireGLObjects(0, objects);

        auto copykernel = dynamic_cast<MonteCarloRenderer*>(
                m_cfgs[m_primary].renderer.get())->GetCopySplitKernel();

        copykernel.SetArg(0, dynamic_cast<ClwOutput*>(left_output)->data());
        copykernel.SetArg(1, dynamic_cast<ClwOutput*>(right_output)->data());
        copykernel.SetArg(2, left_output->width());
        copykernel.SetArg(3, left_output->height());
        copykernel.SetArg(4, kGamma);
        copykernel.SetArg(5, m_cl_interop_image);

        int globalsize = left_output->width() * left_output->height();
        m_cfgs[m_primary].context.Launch1D(0, (globalsize + 63) / 64 * 64, 64, copykernel);

        m_cfgs[m_primary].context.ReleaseGLObjects(0, objects);
        m_cfgs[m_primary].context.Finish(0);
    }

    void AppClRender::AddOutput(Renderer::OutputType type)
    {
        for (std::size_t idx = 0; idx < m_cfgs.size(); ++idx)
        {
            AddRendererOutput(idx, type);
        }
    }

    void AppClRender::InitPostEffect(AppSettings const& settings)
    {
        switch (m_post_processing_type)
        {
        case PostProcessingType::kNone:
            break;
        case PostProcessingType::kBilateralDenoiser:
            if (settings.camera_type != CameraType::kPerspective)
            {
                throw std::logic_error("Bilateral denoiser requires perspective camera");
            }
            AddPostEffect(m_primary, PostEffectType::kBilateralDenoiser);
            break;
        case PostProcessingType::kWaveletDenoser:
            AddPostEffect(m_primary, PostEffectType::kWaveletDenoiser);
            break;
        case PostProcessingType::kMLDenoiser:
            AddPostEffect(m_primary, PostEffectType::kMLDenoiser);
            m_post_effect->SetParameter("gpu_memory_fraction", settings.gpu_mem_fraction);
            m_post_effect->SetParameter("visible_devices", settings.visible_devices);
            m_post_effect->SetParameter("start_spp", settings.denoiser_start_spp);
            break;
        case PostProcessingType::kMLUpsample:
            AddPostEffect(m_primary, PostEffectType::kMLUpsampler);
            m_post_effect->SetParameter("gpu_memory_fraction", settings.gpu_mem_fraction);
            m_post_effect->SetParameter("visible_devices", settings.visible_devices);
            m_post_effect->SetParameter("start_spp", settings.denoiser_start_spp);
            break;
        default:
            throw std::runtime_error("AppClRender(...): Unsupported denoiser type");
        }
    }

    void AppClRender::SetPostEffectParams(int sample_cnt)
    {
        switch (m_post_processing_type)
        {
        case PostProcessingType::kBilateralDenoiser:
        {
            const auto radius = 10U - RadeonRays::clamp((sample_cnt / 16), 1U, 9U);
            m_post_effect->SetParameter("radius", static_cast<float>(radius));
            m_post_effect->SetParameter("color_sensitivity", (radius / 10.f) * 2.f);
            m_post_effect->SetParameter("normal_sensitivity", 0.1f + (radius / 10.f) * 0.15f);
            m_post_effect->SetParameter("position_sensitivity", 5.f + 10.f * (radius / 10.f));
            m_post_effect->SetParameter("albedo_sensitivity", 0.5f + (radius / 10.f) * 0.5f);
            break;
        }
        case PostProcessingType::kWaveletDenoser:
        {
            auto pCamera = dynamic_cast<PerspectiveCamera*>(m_camera.get());
            m_post_effect->SetParameter("camera_focal_length", pCamera->GetFocalLength());
            m_post_effect->SetParameter("camera_sensor_size", pCamera->GetSensorSize());
            m_post_effect->SetParameter("camera_depth_range", pCamera->GetDepthRange());
            m_post_effect->SetParameter("camera_up_vector", pCamera->GetUpVector());
            m_post_effect->SetParameter("camera_forward_vector", pCamera->GetForwardVector());
            m_post_effect->SetParameter("camera_right_vector", -pCamera->GetRightVector());
            m_post_effect->SetParameter("camera_position", pCamera->GetPosition());
            m_post_effect->SetParameter("camera_aspect_ratio", pCamera->GetAspectRatio());
            break;
        }
        case PostProcessingType::kMLDenoiser:
        case PostProcessingType::kMLUpsample:
        case PostProcessingType::kNone:
        default:
            break;
        }
    }

} // Baikal
