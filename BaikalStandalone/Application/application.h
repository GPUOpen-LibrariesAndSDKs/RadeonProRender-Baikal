
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

#include "Application/app_utils.h"
#include "Application/cl_render.h"
#include "Application/gl_render.h"

#include <future>
#include <memory>
#include <chrono>

namespace Baikal
{

    class Application
    {
    public:
        Application(int argc, char * argv[]);
        void Run();
    private:
        void Update(bool update_required);
        
        //update app state according to gui
        // return: true if scene update required
        bool UpdateGui();
        void CollectSceneStats();

        void SaveToFile(std::chrono::high_resolution_clock::time_point time) const;

        //input callbacks
        //Note: use glfwGetWindowUserPointer(window) to get app instance
        static void OnKey(GLFWwindow* window, int key, int scancode, int action, int mods);
        static void OnMouseMove(GLFWwindow* window, double x, double y);
        static void OnMouseButton(GLFWwindow* window, int button, int action, int mods);

        AppSettings m_settings;
        std::unique_ptr<AppClRender> m_cl;
        std::unique_ptr<AppGlRender> m_gl;

        GLFWwindow* m_window;

        //scene stats stuff
        int m_num_triangles;
        int m_num_instances;

        int m_shape_id_val;
        std::future<int> m_future;
        Material::Ptr m_material = nullptr;
    };
}
