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

#include <memory>

#include "CLW.h"

namespace Baikal
{
    class Renderer;
    class Output;
    class PostEffect;
    
    /**
        \brief RenderFactory class is in charge of render entities creation.
     
        \details RenderFactory makes sure renderer objects are compatible between 
        each other since many of them might use either CPU or GPU implementation. 
        Entities create via the same factory are known to be compatible.
     */
    class RenderFactory
    {
    public:
        enum class RendererType
        {
            kUnidirectionalPathTracer
        };
        
        enum class PostEffectType
        {
            kBilateralDenoiser
        };
        
        static std::unique_ptr<RenderFactory> CreateClwRenderFactory(CLWContext context,
                                                              int device_index);
        
        RenderFactory() = default;
        virtual ~RenderFactory() = default;
        
        // Create a renderer of specified type
        virtual std::unique_ptr<Renderer> CreateRenderer(RendererType type) const = 0;
        // Create an output of specified type
        virtual std::unique_ptr<Output> CreateOutput(std::uint32_t w, std::uint32_t h) const = 0;
        // Create post effect of specified type
        virtual std::unique_ptr<PostEffect> CreatePostEffect(PostEffectType type) const = 0;
        
        RenderFactory(RenderFactory const&) = delete;
        RenderFactory const& operator = (RenderFactory const&) = delete;
    };
    
    
}
