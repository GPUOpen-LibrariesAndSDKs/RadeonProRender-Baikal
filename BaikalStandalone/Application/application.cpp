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

#ifdef __APPLE__
#include <OpenCL/OpenCL.h>
#define GLFW_INCLUDE_GLCOREARB
#define GLFW_NO_GLU
#include "GLFW/glfw3.h"
#elif WIN32
#define NOMINMAX
#include <Windows.h>
#include "GL/glew.h"
#include "GLFW/glfw3.h"
#else
#include <CL/cl.h>
#include <GL/glew.h>
#include <GL/glx.h>
#include "GLFW/glfw3.h"
#endif

#include "ImGUI/imgui.h"
#include "ImGUI/imgui_impl_glfw_gl3.h"

#include <memory>
#include <chrono>
#include <cassert>
#include <vector>
#include <stdexcept>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <algorithm>
#include <numeric>
#include <atomic>
#include <mutex>
#include <fstream>
#include <functional>

#include <OpenImageIO/imageio.h>

#define _USE_MATH_DEFINES
#include <math.h>

#ifdef RR_EMBED_KERNELS
#include "./CL/cache/kernels.h"
#endif

#include "CLW.h"

#include "math/mathutils.h"
#include "Application/application.h"

using namespace RadeonRays;

namespace Baikal
{
    static bool     g_is_left_pressed = false;
    static bool     g_is_right_pressed = false;
    static bool     g_is_fwd_pressed = false;
    static bool     g_is_back_pressed = false;
    static bool     g_is_climb_pressed = false;
    static bool     g_is_descent_pressed = false;
    static bool     g_is_mouse_tracking = false;
    static bool     g_is_double_click = false;
    static bool     g_is_f10_pressed = false;
    static float2   g_mouse_pos = float2(0, 0);
    static float2   g_mouse_delta = float2(0, 0);

    auto start = std::chrono::high_resolution_clock::now();

    void Application::OnMouseMove(GLFWwindow* window, double x, double y)
    {
        if (g_is_mouse_tracking)
        {
            g_mouse_delta = float2((float)x, (float)y) - g_mouse_pos;
            g_mouse_pos = float2((float)x, (float)y);
        }
    }

    void Application::OnMouseButton(GLFWwindow* window, int button, int action, int mods)
    {
        if (button == GLFW_MOUSE_BUTTON_RIGHT)
        {
            if (action == GLFW_PRESS)
            {
                g_is_mouse_tracking = true;

                double x, y;
                glfwGetCursorPos(window, &x, &y);
                g_mouse_pos = float2((float)x, (float)y);
                g_mouse_delta = float2(0, 0);
            }
            else if (action == GLFW_RELEASE && g_is_mouse_tracking)
            {
                g_is_mouse_tracking = false;
                g_mouse_delta = float2(0, 0);
            }
        }

        if (button == GLFW_MOUSE_BUTTON_LEFT)
        {
            if (action == GLFW_PRESS)
            {
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>
                    (std::chrono::high_resolution_clock::now() - start);

                if (duration.count() < 200)
                {
                    double x, y;
                    glfwGetCursorPos(window, &x, &y);
                    g_mouse_pos = float2((float)x, (float)y);
                    g_mouse_delta = float2(0, 0);
                    g_is_double_click = true;
                }
                start = std::chrono::high_resolution_clock::now();
            }
            else if (action == GLFW_RELEASE)
                g_is_double_click = false;
        }
    }

    void Application::OnKey(GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        ImGuiIO& io = ImGui::GetIO();
        Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
        auto map = io.KeyMap;
        const bool press_or_repeat = action == GLFW_PRESS || action == GLFW_REPEAT;

        if (action == GLFW_PRESS)
            io.KeysDown[key] = true;
        if (action == GLFW_RELEASE)
            io.KeysDown[key] = false;

        (void)mods; // Modifiers are not reliable across systems
        io.KeyCtrl = io.KeysDown[GLFW_KEY_LEFT_CONTROL] || io.KeysDown[GLFW_KEY_RIGHT_CONTROL];
        io.KeyShift = io.KeysDown[GLFW_KEY_LEFT_SHIFT] || io.KeysDown[GLFW_KEY_RIGHT_SHIFT];
        io.KeyAlt = io.KeysDown[GLFW_KEY_LEFT_ALT] || io.KeysDown[GLFW_KEY_RIGHT_ALT];
        io.KeySuper = io.KeysDown[GLFW_KEY_LEFT_SUPER] || io.KeysDown[GLFW_KEY_RIGHT_SUPER];

        switch (key)
        {
        case GLFW_KEY_W:
            g_is_fwd_pressed = press_or_repeat;
            break;
        case GLFW_KEY_S:
            g_is_back_pressed = press_or_repeat;
            break;
        case GLFW_KEY_A:
            g_is_left_pressed = press_or_repeat;
            break;
        case GLFW_KEY_D:
            g_is_right_pressed = press_or_repeat;
            break;
        case GLFW_KEY_Q:
            g_is_climb_pressed = press_or_repeat;
            break;
        case GLFW_KEY_E:
            g_is_descent_pressed = press_or_repeat;
            break;
        case GLFW_KEY_F1:
            app->m_settings.gui_visible = action == GLFW_PRESS ? !app->m_settings.gui_visible : app->m_settings.gui_visible;
            break;
        case GLFW_KEY_F3:
            app->m_settings.benchmark = action == GLFW_PRESS ? true : app->m_settings.benchmark;
            break;
        case GLFW_KEY_F10:
            g_is_f10_pressed = action == GLFW_PRESS;
            break;
        default:
            break;
        }
    }

    void Application::Update(bool update_required)
    {
        static auto prevtime = std::chrono::high_resolution_clock::now();
        auto time = std::chrono::high_resolution_clock::now();
        auto dt = std::chrono::duration_cast<std::chrono::duration<double>>(time - prevtime);
        prevtime = time;

        bool update = update_required;
        float camrotx = 0.f;
        float camroty = 0.f;

        const float kMouseSensitivity = 0.001125f;
        auto camera = m_cl->GetCamera();
        if (!m_settings.benchmark && !m_settings.time_benchmark)
        {
            float2 delta = g_mouse_delta * float2(kMouseSensitivity, kMouseSensitivity);
            camrotx = -delta.x;
            camroty = -delta.y;


            if (std::abs(camroty) > 0.001f)
            {
                camera->Tilt(camroty);
                //g_camera->ArcballRotateVertically(float3(0, 0, 0), camroty);
                update = true;
            }

            if (std::abs(camrotx) > 0.001f)
            {

                camera->Rotate(camrotx);
                //g_camera->ArcballRotateHorizontally(float3(0, 0, 0), camrotx);
                update = true;
            }

            const float kMovementSpeed = m_settings.cspeed;
            if (g_is_fwd_pressed)
            {
                camera->MoveForward((float)dt.count() * kMovementSpeed);
                update = true;
            }

            if (g_is_back_pressed)
            {
                camera->MoveForward(-(float)dt.count() * kMovementSpeed);
                update = true;
            }

            if (g_is_right_pressed)
            {
                camera->MoveRight((float)dt.count() * kMovementSpeed);
                update = true;
            }

            if (g_is_left_pressed)
            {
                camera->MoveRight(-(float)dt.count() * kMovementSpeed);
                update = true;
            }

            if (g_is_climb_pressed)
            {
                camera->MoveUp((float)dt.count() * kMovementSpeed);
                update = true;
            }

            if (g_is_descent_pressed)
            {
                camera->MoveUp(-(float)dt.count() * kMovementSpeed);
                update = true;
            }

            if (g_is_f10_pressed)
            {
                g_is_f10_pressed = false; //one time execution
                SaveToFile(time);
            }
        }

        if (update)
        {
            //if (g_num_samples > -1)
            {
                m_settings.samplecount = 0;
            }

            m_cl->UpdateScene();
        }

        if (m_settings.num_samples == -1 || m_settings.samplecount <  m_settings.num_samples)
        {
            m_cl->Render(m_settings.samplecount);
            ++m_settings.samplecount;
        }
        else if (m_settings.samplecount == m_settings.num_samples)
        {
            m_cl->SaveFrameBuffer(m_settings);
            std::cout << "Target sample count reached\n";
            ++m_settings.samplecount;
            //exit(0);
        }

        m_cl->Update(m_settings);
    }

    void Application::SaveToFile(std::chrono::high_resolution_clock::time_point time) const
    {
        using namespace OIIO;
        int w, h;
        glfwGetFramebufferSize(m_window, &w, &h);
        assert(glGetError() == 0);
        const auto channels = 3;
        auto *data = new GLubyte[channels * w * h];
        glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, data);

        //opengl coordinates to oiio coordinates
        for (auto i = 0; i <= h / 2; ++i)
        {
            std::swap_ranges(data + channels * w * i, data + channels * w * (i + 1) - 1, data + channels * w * (h - (i + 1)));
        }
        
        const auto filename = m_settings.path + "/" + m_settings.base_image_file_name + "-" + std::to_string(time.time_since_epoch().count()) + "." + m_settings.image_file_format;
        auto out = ImageOutput::create(filename);
        if (out)
        {
            ImageSpec spec{ w, h, channels, TypeDesc::UINT8 };
            out->open(filename, spec);
            out->write_image(TypeDesc::UINT8, data);
            out->close();
            delete out; // ImageOutput::destroy not found
        }
        else
        {
            std::cout << "Wrong file format\n";
        }
        
        delete[] data;
    }


    void OnError(int error, const char* description)
    {
        std::cout << description << "\n";
    }

    bool GradeTimeBenchmarkResults(std::string const& scene, int time_in_sec, std::string& rating, ImVec4& color)
    {
        if (scene == "classroom.obj")
        {
            if (time_in_sec < 70)
            {
                rating = "Excellent";
                color = ImVec4(0.1f, 0.7f, 0.1f, 1.f);
            }
            else if (time_in_sec < 100)
            {
                rating = "Good";
                color = ImVec4(0.1f, 0.7f, 0.1f, 1.f);
            }
            else if (time_in_sec < 120)
            {
                rating = "Average";
                color = ImVec4(0.7f, 0.7f, 0.1f, 1.f);
            }
            else
            {
                rating = "Poor";
                color = ImVec4(0.7f, 0.1f, 0.1f, 1.f);
            }

            return true;
        }

        return false;
    }

    Application::Application(int argc, char * argv[])
        : m_window(nullptr)
        , m_num_triangles(0)
        , m_num_instances(0)
        , m_image_io(ImageIo::CreateImageIo())
    {
        // Command line parsing
        AppCliParser cli;
        m_settings = cli.Parse(argc, argv);
        if (!m_settings.cmd_line_mode)
        {
            // Initialize GLFW
            {
                auto err = glfwInit();
                if (err != GLFW_TRUE)
                {
                    std::cout << "GLFW initialization failed\n";
                    exit(-1);
                }
            }
            // Setup window
            glfwSetErrorCallback(OnError);
            glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    #if __APPLE__
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif

            // GLUT Window Initialization:
            m_window = glfwCreateWindow(m_settings.width, m_settings.height, "Baikal standalone demo", nullptr, nullptr);
            glfwMakeContextCurrent(m_window);

    #ifndef __APPLE__
            {
                glewExperimental = GL_TRUE;
                GLenum err = glewInit();
                if (err != GLEW_OK)
                {
                    std::cout << "GLEW initialization failed\n";
                    exit(-1);
                }
            }
    #endif

            ImGui_ImplGlfwGL3_Init(m_window, true);

            try
            {
                m_gl.reset(new AppGlRender(m_settings));
                m_cl.reset(new AppClRender(m_settings, m_gl->GetTexture()));

                //set callbacks
                using namespace std::placeholders;
                glfwSetWindowUserPointer(m_window, this);
                glfwSetMouseButtonCallback(m_window, Application::OnMouseButton);
                glfwSetCursorPosCallback(m_window, Application::OnMouseMove);
                glfwSetKeyCallback(m_window, Application::OnKey);
            }
            catch (std::runtime_error& err)
            {
                glfwDestroyWindow(m_window);
                std::cout << err.what();
                exit(-1);
            }
        }
        else
        {
            m_settings.interop = false;
            m_cl.reset(new AppClRender(m_settings, -1));
        }
    }

    void Application::Run()
    {
        CollectSceneStats();

        if (!m_settings.cmd_line_mode)
        {
            try
            {
                m_cl->StartRenderThreads();
                static bool update = true;
                while (!glfwWindowShouldClose(m_window))
                {

                    ImGui_ImplGlfwGL3_NewFrame();
                    Update(update);
                    m_gl->Render(m_window);
                    update = UpdateGui();

                    glfwSwapBuffers(m_window);
                    glfwPollEvents();
                }

                m_cl->StopRenderThreads();

                glfwDestroyWindow(m_window);
            }
            catch (std::runtime_error& err)
            {
                glfwDestroyWindow(m_window);
                std::cout << err.what();
                exit(-1);
            }
        }
        else
        {
            m_cl.reset(new AppClRender(m_settings, -1));
                        
            std::cout << "Number of triangles: " << m_num_triangles << "\n";
            std::cout << "Number of instances: " << m_num_instances << "\n";

            //compile scene
            m_cl->UpdateScene();
            m_cl->RunBenchmark(m_settings);

            auto minutes = (int)(m_settings.time_benchmark_time / 60.f);
            auto seconds = (int)(m_settings.time_benchmark_time - minutes * 60);

            std::cout << "General benchmark results:\n";
            std::cout << "\tRendering time: " << minutes << "min:" << seconds << "s\n";
            std::string rating;
            ImVec4 color;
            if (GradeTimeBenchmarkResults(m_settings.modelname, minutes * 60 + seconds, rating, color))
            {
                std::cout << "\tRating: " << rating.c_str() << "\n";
            }
            else
            {
                std::cout << "\tRating: N/A\n";
            }

            std::cout << "RT benchmark results:\n";
            std::cout << "\tPrimary: " << m_settings.stats.primary_throughput * 1e-6f << " Mrays/s\n";
            std::cout << "\tSecondary: " << m_settings.stats.secondary_throughput * 1e-6f << " Mrays/s\n";
            std::cout << "\tShadow: " << m_settings.stats.shadow_throughput * 1e-6f << " Mrays/s\n";
        }
    }

    bool Application::UpdateGui()
    {
        static bool load_texture = false;
        static float aperture = 0.0f;
        static float focal_length = 35.f;
        static float focus_distance = 1.f;
        static int num_bounces = 5;
        static char const* outputs =
            "Color\0"
            "World position\0"
            "Shading normal\0"
            "Geometric normal\0"
            "Texture coords\0"
            "Wire\0"
            "Albedo\0"
            "Tangent\0"
            "Bitangent\0"
            "Gloss\0"
            "Depth\0\0"
            ;

        static int output = 0;
        bool update = false;
        if (m_settings.gui_visible)
        {
            ImGui::SetNextWindowSizeConstraints(ImVec2(380, 580), ImVec2(380, 580));
            ImGui::Begin("Baikal settings");
            ImGui::Text("Use wsad keys to move");
            ImGui::Text("Q/E to climb/descent");
            ImGui::Text("Mouse+RMB to look around");
            ImGui::Text("F1 to hide/show GUI");
            ImGui::Separator();
            ImGui::Text("Device vendor: %s", m_cl->GetDevice(0).GetVendor().c_str());
            ImGui::Text("Device name: %s", m_cl->GetDevice(0).GetName().c_str());
            ImGui::Text("OpenCL: %s", m_cl->GetDevice(0).GetVersion().c_str());
            ImGui::Separator();
            ImGui::Text("Resolution: %dx%d ", m_settings.width, m_settings.height);
            ImGui::Text("Scene: %s", m_settings.modelname.c_str());
            ImGui::Text("Unique triangles: %d", m_num_triangles);
            ImGui::Text("Number of instances: %d", m_num_instances);
            ImGui::Separator();
            ImGui::SliderInt("GI bounces", &num_bounces, 1, 10);

            auto camera = m_cl->GetCamera();

            if (m_settings.camera_type == CameraType::kPerspective)
            {
                auto perspective_camera = std::dynamic_pointer_cast<PerspectiveCamera>(camera);

                if (!perspective_camera)
                {
                    throw std::runtime_error("Application::UpdateGui(...): can not cast to perspective camera");
                }

                if (aperture != m_settings.camera_aperture * 1000.f)
                {
                    m_settings.camera_aperture = aperture / 1000.f;
                    perspective_camera->SetAperture(m_settings.camera_aperture);
                    update = true;
                }

                if (focus_distance != m_settings.camera_focus_distance)
                {
                    m_settings.camera_focus_distance = focus_distance;
                    perspective_camera->SetFocusDistance(m_settings.camera_focus_distance);
                    update = true;
                }

                if (focal_length != m_settings.camera_focal_length * 1000.f)
                {
                    m_settings.camera_focal_length = focal_length / 1000.f;
                    perspective_camera->SetFocalLength(m_settings.camera_focal_length);
                    update = true;
                }

                ImGui::SliderFloat("Aperture(mm)", &aperture, 0.0f, 100.0f);
                ImGui::SliderFloat("Focal length(mm)", &focal_length, 5.f, 200.0f);
                ImGui::SliderFloat("Focus distance(m)", &focus_distance, 0.05f, 20.f);
            }

            if (num_bounces != m_settings.num_bounces)
            {
                m_settings.num_bounces = num_bounces;
                m_cl->SetNumBounces(num_bounces);
                update = true;
            }

            auto gui_out_type = static_cast<Baikal::Renderer::OutputType>(output);

            if (gui_out_type != m_cl->GetOutputType())
            {
                m_cl->SetOutputType(gui_out_type);
                update = true;
            }

            RadeonRays::float3 eye, at;
            eye = camera->GetPosition();
            at = eye + camera->GetForwardVector();

            ImGui::Combo("Output", &output, outputs);
            ImGui::Text(" ");
            ImGui::Text("Number of samples: %d", m_settings.samplecount);
            ImGui::Text("Frame time %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::Text("Renderer performance %.3f Msamples/s", (ImGui::GetIO().Framerate *m_settings.width * m_settings.height) / 1000000.f);
            ImGui::Text("Eye: x = %.3f y = %.3f z = %.3f", eye.x, eye.y, eye.z);
            ImGui::Text("At: x = %.3f y = %.3f z = %.3f", at.x, at.y, at.z);
            ImGui::Separator();

            if (m_settings.time_benchmark)
            {
                ImGui::ProgressBar(m_settings.samplecount / 512.f);
            }

            static decltype(std::chrono::high_resolution_clock::now()) time_bench_start_time;
            if (!m_settings.time_benchmark && !m_settings.benchmark)
            {
                if (ImGui::Button("Start benchmark") && m_settings.num_samples == -1)
                {
                    time_bench_start_time = std::chrono::high_resolution_clock::now();
                    m_settings.time_benchmark = true;
                    update = true;
                }

                if (!m_settings.time_benchmark && ImGui::Button("Start RT benchmark"))
                {
                    m_settings.benchmark = true;
                }
            }

            if (m_settings.time_benchmark && m_settings.samplecount > 511)
            {
                m_settings.time_benchmark = false;
                auto delta = std::chrono::duration_cast<std::chrono::milliseconds>
                    (std::chrono::high_resolution_clock::now() - time_bench_start_time).count();
                m_settings.time_benchmark_time = delta / 1000.f;
                m_settings.time_benchmarked = true;
            }

            if (m_settings.time_benchmarked)
            {
                auto minutes = (int)(m_settings.time_benchmark_time / 60.f);
                auto seconds = (int)(m_settings.time_benchmark_time - minutes * 60);
                ImGui::Separator();

                ImVec4 color;
                std::string rating;
                ImGui::Text("Rendering time: %2dmin:%ds", minutes, seconds);
                if (GradeTimeBenchmarkResults(m_settings.modelname, minutes * 60 + seconds, rating, color))
                {
                    ImGui::TextColored(color, "Rating: %s", rating.c_str());
                }
                else
                {
                    ImGui::Text("Rating: N/A");
                }
            }

            if (m_settings.rt_benchmarked)
            {
                auto& stats = m_settings.stats;

                ImGui::Separator();
                ImGui::Text("Primary rays: %f Mrays/s", stats.primary_throughput * 1e-6f);
                ImGui::Text("Secondary rays: %f Mrays/s", stats.secondary_throughput * 1e-6f);
                ImGui::Text("Shadow rays: %f Mrays/s", stats.shadow_throughput * 1e-6f);
            }

#ifdef ENABLE_DENOISER
            ImGui::Separator();

            static float sigmaPosition = m_cl->GetDenoiserFloatParam("position_sensitivity").x;
            static float sigmaNormal = m_cl->GetDenoiserFloatParam("normal_sensitivity").x;
            static float sigmaColor = m_cl->GetDenoiserFloatParam("color_sensitivity").x;

            ImGui::Text("Denoiser settings");
            ImGui::SliderFloat("Position sigma", &sigmaPosition, 0.f, 0.3f);
            ImGui::SliderFloat("Normal sigma", &sigmaNormal, 0.f, 5.f);
            ImGui::SliderFloat("Color sigma", &sigmaColor, 0.f, 5.f);       

            if (m_cl->GetDenoiserFloatParam("position_sensitivity").x != sigmaPosition ||
                m_cl->GetDenoiserFloatParam("normal_sensitivity").x != sigmaNormal ||
                m_cl->GetDenoiserFloatParam("color_sensitivity").x != sigmaColor)
            {
                m_cl->SetDenoiserFloatParam("position_sensitivity", sigmaPosition);
                m_cl->SetDenoiserFloatParam("normal_sensitivity", sigmaNormal);
                m_cl->SetDenoiserFloatParam("color_sensitivity", sigmaColor);
            }
#endif
            ImGui::End();

            // Get shape/material info from renderer
            if (m_shape_id_future.valid())
            {
                auto shape = m_cl->GetShapeById(m_shape_id_future.get());
                m_material = (shape) ? (shape->GetMaterial()) : (nullptr);
            }

            // Process double click event if it occured
            if (g_is_double_click)
            {
                m_shape_id_future = m_cl->GetShapeId((std::uint32_t)g_mouse_pos.x, (std::uint32_t)g_mouse_pos.y);
            }

            // draw material props
            if (m_material)
            {
                ImGui::Begin("Material info");

                if (g_is_double_click)
                {
                    ImGui::SetWindowPos(ImVec2(g_mouse_pos.x, g_mouse_pos.y));
                    g_is_double_click = false;
                }

                bool is_scene_changed = false;
                MaterialAccessor material_accessor(m_material);

                // process material inputs
                std::vector<Material::Input> float_inputs;
                std::vector<Material::Input> uint_inputs;
                std::vector<Material::Input> texture_inputs;

                for (int i = 0; i < m_material->GetNumInputs(); i++)
                {
                    auto input = m_material->GetInput(i);
                    switch (input.value.type)
                    {
                    case Material::InputType::kFloat4:
                        float_inputs.push_back(input);
                        break;
                    case Material::InputType::kUint:
                        uint_inputs.push_back(input);
                        break;
                    case Material::InputType::kTexture:
                        texture_inputs.push_back(input);
                        break;
                    default:
                        break;
                    }
                }
                // draw uint inputs
                for (const auto& input : uint_inputs)
                {
                    auto name = input.info.name.c_str();
                    std::uint32_t input_value = input.value.uint_value;
                    ImGui::InputInt(name, (int*)(&input_value));
                    if (input.value.uint_value != input_value)
                    {
                        m_material->SetInputValue(input.info.name, input_value);
                        is_scene_changed = true;
                    }
                }

                // draw float4 inputs
                for (const auto& input : float_inputs)
                {
                    auto name = input.info.name.c_str();

                    float color[3] = { 0 };
                    color[0] = input.value.float_value.x;
                    color[1] = input.value.float_value.y;
                    color[2] = input.value.float_value.z;
                    ImGui::ColorEdit3(name, color);

                    if ((input.value.float_value.x != color[0]) ||
                        (input.value.float_value.y != color[1]) ||
                        (input.value.float_value.z != color[2]))
                    {
                        RadeonRays::float4 value(
                            color[0],
                            color[1],
                            color[2],
                            0);
                        m_material->SetInputValue(input.info.name, value);
                        is_scene_changed = true;
                    }
                }

                // draw texture inputs
                for (const auto& input : texture_inputs)
                {
                    const size_t buffer_size = 2048;
                    char text_buffer[buffer_size] = { 0 };
                    auto name = input.info.name;

                    if (ImGui::InputText(name.c_str(), text_buffer, buffer_size, ImGuiInputTextFlags_EnterReturnsTrue))
                    {
                        Texture::Ptr texture = nullptr;
                        if (strlen(text_buffer) != 0)
                        {
                            m_material->SetInputValue(name, texture);
                        }
                        else
                        {
                            try
                            {
                                texture = m_image_io->LoadImage(text_buffer);
                                m_material->SetInputValue(input.info.name, texture);
                            }
                            catch (std::exception&)
                            {
                                printf("WARNING: Can not load texture by specified path\n");
                            }
                        }
                        is_scene_changed = true;
                    }
                    
                }

                // Get material type settings
                std::string material_info;
                for (const auto& iter : material_accessor.GetTypeInfo())
                {
                    material_info += iter;
                    material_info.push_back('\0');
                }

                int material_type_output = material_accessor.GetType();
                ImGui::Combo("Material type", &material_type_output, material_info.c_str());

                if (material_accessor.GetType() != material_type_output)
                {
                    material_accessor.SetType(material_type_output);
                    is_scene_changed = true;
                }

                if (is_scene_changed)
                {
                    m_cl->UpdateScene();
                }

                ImGui::End();
            }


            ImGui::Render();
        }

        return update;
    }

    void Application::CollectSceneStats()
    {
        // Collect some scene statistics
        m_num_triangles = 0U;
        m_num_instances = 0U;
        {
            auto scene = m_cl->GetScene();
            auto shape_iter = scene->CreateShapeIterator();

            for (; shape_iter->IsValid(); shape_iter->Next())
            {
                auto shape = shape_iter->ItemAs<Baikal::Shape>();
                auto mesh = std::dynamic_pointer_cast<Baikal::Mesh>(shape);

                if (mesh)
                {
                    m_num_triangles += (int)(mesh->GetNumIndices() / 3);
                }
                else
                {
                    ++m_num_instances;
                }
            }
        }
    }

}
