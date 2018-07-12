#pragma once

#include "buffer.h"

#include <optional>


namespace Baikal::ML
{
    class Inference
    {
    public:
        virtual Buffer GetInputBuffer() = 0;
        virtual void PushInput(Buffer&& buffer) = 0;
        virtual std::optional<Buffer> PopOutput() const = 0;
        virtual ~Inference() = default;
    };

} // namespace Baikal::ML
