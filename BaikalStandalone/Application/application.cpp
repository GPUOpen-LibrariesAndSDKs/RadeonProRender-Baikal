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
    static bool     g_is_home_pressed = false;
    static bool     g_is_end_pressed = false;
    static bool     g_is_mouse_tracking = false;
    static float2   g_mouse_pos = float2(0, 0);
    static float2   g_mouse_delta = float2(0, 0);

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
    }

    void Application::OnKey(GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
        switch (key)
        {
        case GLFW_KEY_UP:
            g_is_fwd_pressed = action == GLFW_PRESS ? true : false;
            break;
        case GLFW_KEY_DOWN:
            g_is_back_pressed = action == GLFW_PRESS ? true : false;
            break;
        case GLFW_KEY_LEFT:
            g_is_left_pressed = action == GLFW_PRESS ? true : false;
            break;
        case GLFW_KEY_RIGHT:
            g_is_right_pressed = action == GLFW_PRESS ? true : false;
            break;
        case GLFW_KEY_HOME:
            g_is_home_pressed = action == GLFW_PRESS ? true : false;
            break;
        case GLFW_KEY_END:
            g_is_end_pressed = action == GLFW_PRESS ? true : false;
            break;
        case GLFW_KEY_F1:
            app->m_settings.gui_visible = action == GLFW_PRESS ? !app->m_settings.gui_visible : app->m_settings.gui_visible;
            break;
        case GLFW_KEY_F3:
            app->m_settings.benchmark = action == GLFW_PRESS ? true : app->m_settings.benchmark;
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

            if (g_is_home_pressed)
            {
                camera->MoveUp((float)dt.count() * kMovementSpeed);
                update = true;
            }

            if (g_is_end_pressed)
            {
                camera->MoveUp(-(float)dt.count() * kMovementSpeed);
                update = true;
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
            "Gloss\0\0"
            ;

        static int output = 0;
        bool update = false;
        if (m_settings.gui_visible)
        {
            ImGui::SetNextWindowSizeConstraints(ImVec2(380, 580), ImVec2(380, 580));
            ImGui::Begin("Baikal settings");
            ImGui::Text("Use arrow keys to move");
            ImGui::Text("PgUp/Down to climb/descent");
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
            ImGui::SliderFloat("Aperture(mm)", &aperture, 0.0f, 100.0f);
            ImGui::SliderFloat("Focal length(mm)", &focal_length, 5.f, 200.0f);
            ImGui::SliderFloat("Focus distance(m)", &focus_distance, 0.05f, 20.f);

            auto camera = m_cl->GetCamera();
            if (aperture != m_settings.camera_aperture * 1000.f)
            {
                m_settings.camera_aperture = aperture / 1000.f;
                camera->SetAperture(m_settings.camera_aperture);
                update = true;
            }

            if (focus_distance != m_settings.camera_focus_distance)
            {
                m_settings.camera_focus_distance = focus_distance;
                camera->SetFocusDistance(m_settings.camera_focus_distance);
                update = true;
            }

            if (focal_length != m_settings.camera_focal_length * 1000.f)
            {
                m_settings.camera_focal_length = focal_length / 1000.f;
                camera->SetFocalLength(m_settings.camera_focal_length);
                update = true;
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

            ImGui::Combo("Output", &output, outputs);
            ImGui::Text(" ");
            ImGui::Text("Number of samples: %d", m_settings.samplecount);
            ImGui::Text("Frame time %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::Text("Renderer performance %.3f Msamples/s", (ImGui::GetIO().Framerate *m_settings.width * m_settings.height) / 1000000.f);
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
            std::unique_ptr<Baikal::Iterator> shape_iter(scene->CreateShapeIterator());

            for (; shape_iter->IsValid(); shape_iter->Next())
            {
                auto shape = shape_iter->ItemAs<Baikal::Shape const>();
                auto mesh = dynamic_cast<Baikal::Mesh const*>(shape);

                if (mesh)
                {
                    m_num_triangles += mesh->GetNumIndices() / 3;
                }
                else
                {
                    ++m_num_instances;
                }
            }
        }
    }

}
