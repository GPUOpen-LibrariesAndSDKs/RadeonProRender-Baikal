#include "model.h"


namespace ML
{
    Model* LoadModel(char const* model_path, 
                     float gpu_memory_fraction, 
                     char const* visible_devices)
    {
        return new Model(model_path, gpu_memory_fraction, visible_devices);
    }
}