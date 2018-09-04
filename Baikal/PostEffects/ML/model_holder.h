#pragma once

#include "PostEffects/ML/model.h"

#include <memory>
#include <string>


namespace Baikal
{
    namespace PostEffects
    {
        class SharedObject;

        class ModelHolder
        {
        public:
            ModelHolder();

            ModelHolder(std::string const& model_path,
                        float gpu_memory_fraction,
                        std::string const& visible_devices);

            void Reset(std::string const& model_path,
                       float gpu_memory_fraction,
                       std::string const& visible_devices);

            ML::Model* operator ->() const
            {
                return m_model.get();
            }

            ML::Model& operator *() const
            {
                return *m_model;
            }

        private:
            std::shared_ptr<SharedObject> m_shared_object;
            std::unique_ptr<ML::Model> m_model;
        };
    }
}
