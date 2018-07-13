#pragma once

#include "Baikal/PostEffects/ML/tensor.h"


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
            virtual ~Inference() = default;
        };
    }
}
