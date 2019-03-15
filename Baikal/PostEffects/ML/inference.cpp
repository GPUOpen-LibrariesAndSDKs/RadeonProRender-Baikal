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

#include "PostEffects/ML/error_handler.h"
#include "inference.h"
#include "RadeonProML.h"

#include <cassert>

namespace Baikal
{
    namespace PostEffects
    {
        Inference::Inference(std::string const& model_path,
                             size_t input_height,
                             size_t input_width,
                             float gpu_memory_fraction,
                             std::string const& visible_devices)
                : m_model(model_path,
                          gpu_memory_fraction,
                          visible_devices)
        {
            CheckStatus(mlGetModelInfo(m_model.GetModel(), &m_input_info, nullptr));
            // Set unspecified input tensor dimensions
            m_input_info.width = input_width;
            m_input_info.height = input_height;
            CheckStatus(mlSetModelInputInfo(m_model.GetModel(), &m_input_info));

            // Get output tensor shape for model
            CheckStatus(mlGetModelInfo(m_model.GetModel(), nullptr, &m_output_info));

            m_worker = std::thread(&Inference::DoInference, this);
        }

        Inference::~Inference()
        {
            Shutdown();
        }

        ml_image_info Inference::GetInputInfo() const
        {
            return m_input_info;
        }

        ml_image_info Inference::GetOutputInfo() const
        {
            return m_output_info;
        }

        Image Inference::GetInputData()
        {
            return {0, AllocImage(m_input_info, ML_READ_WRITE)};
        }

        void Inference::PushInput(Image&& image)
        {
            m_input_queue.push(std::move(image));
        }

        Image Inference::PopOutput()
        {
            Image output_tensor = {0, nullptr};
            m_output_queue.try_pop(output_tensor);
            return output_tensor;
        }

        ml_image Inference::AllocImage(ml_image_info info, ml_access_mode access_mode)
        {
            auto image = m_model.CreateImage(info, access_mode);

            if (image == nullptr)
            {
                throw std::runtime_error("Couldn't not create image");
            }

            return image;
        }

        void Inference::DoInference()
        {
            for (;;)
            {
                Image input;
                m_input_queue.wait_and_pop(input);

                if (input.image == nullptr)
                {
                    break;
                }

                if (m_input_queue.size() > 0)
                {
                    continue;
                }

                Image output = { input.tag, AllocImage(m_output_info, ML_READ_WRITE) };

                CheckStatus(mlInfer(m_model.GetModel(), input.image, output.image));

                m_output_queue.push(std::move(output));
            }
        }

        void Inference::Shutdown()
        {
            m_input_queue.push({0, nullptr});
            m_worker.join();
        }
    }
}
