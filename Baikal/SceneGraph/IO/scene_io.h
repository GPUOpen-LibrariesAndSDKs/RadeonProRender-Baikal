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

/**
 \file scene_io.h
 \author Dmitry Kozlov
 \version 1.0
 \brief Contains an interface for scene loading
 */
#pragma once

#include <string>
#include <memory>

namespace Baikal
{
    class Scene1;
    
    /**
     \brief Interface for scene loading
     
     SceneIO implementation is responsible for translation of various scene formats into Baikal.
     */
    class SceneIo
    {
    public:
        // Create OBJ scene loader
        static std::unique_ptr<SceneIo> CreateSceneIoObj();
        // Create test scene loader
        static std::unique_ptr<SceneIo> CreateSceneIoTest();

        // Constructor
        SceneIo() = default;
        // Destructor
        virtual ~SceneIo() = default;
        
        // Load the scene from file using resourse base path
        virtual std::unique_ptr<Scene1> LoadScene(std::string const& filename, std::string const& basepath) const = 0;
        
        // Disallow copying
        SceneIo(SceneIo const&) = delete;
        SceneIo& operator = (SceneIo const&) = delete;
    };
}
