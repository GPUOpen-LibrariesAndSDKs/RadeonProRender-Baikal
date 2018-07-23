#pragma once

#include <cstddef>


namespace ML
{
    class Model
    {
    public:
        using ValueType = float;

        Model() = default;
        Model(char const* model_path, float gpu_memory_fraction, char const* visible_devices) 
        {
        }

        void infer(ValueType const* input,
            std::size_t width,
            std::size_t height,
            std::size_t channels,
            ValueType* output)
        {
        }
    };

    Model* LoadModel(char const* model_path, 
                     float gpu_memory_fraction,
                     char const* visible_devices);
}


