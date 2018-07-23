#pragma once

#include "PostEffects/ML/tensor.h"

#include <memory>
#include <string>


namespace Baikal
{
    namespace PostEffects
    {
        class Inference
        {
        public:
            virtual Tensor::Shape GetInputShape() const = 0;
            virtual Tensor::Shape GetOutputShape() const = 0;
            virtual Tensor GetInputTensor() = 0;
            virtual void PushInput(Tensor&& tensor) = 0;
            virtual Tensor PopOutput() = 0;
            virtual ~Inference() {};
        };
    }
}
