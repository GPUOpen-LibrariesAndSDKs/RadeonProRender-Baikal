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

#include "PostEffects/ML/model_holder.h"

#include "../RadeonRays/RadeonRays/src/async/thread_pool.h"
#include "image.h"

#include <memory>
#include <string>
#include <thread>


namespace Baikal
{
    namespace PostEffects
    {
        class Inference
        {
        public:
            using Ptr = std::unique_ptr<Inference>;

            Inference(std::string const& model_path,
                      // input shapes
                      ml_image_info const& input_desc,
                      ml_image_info const& output_desc,
                      // model params
                      float gpu_memory_fraction,
                      std::string const& visible_devices);

            ml_image_info GetInputShape() const;
            ml_image_info GetOutputShape() const;

            Image GetInputData();
            void PushInput(Image&& image);

            Image PopOutput();
            virtual ~Inference();

        protected:
            void DoInference();
            ml_image AllocImage(ml_image_info info);

            RadeonRays::thread_safe_queue<Image> m_input_queue;
            RadeonRays::thread_safe_queue<Image> m_output_queue;

            ModelHolder m_model;
            ml_image_info m_input_desc;
            ml_image_info m_output_desc;

        private:
            void Shutdown();

            std::thread m_worker;
        };
    }
}
