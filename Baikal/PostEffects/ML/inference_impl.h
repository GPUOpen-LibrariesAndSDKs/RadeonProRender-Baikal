#include "Baikal/PostEffects/ML/inference.h"


namespace Baikal
{
    namespace PostEffects
    {
        class InferenceImpl : public Inference
        {
        public:
            InferenceImpl(const std::string& model_path,
                          size_t width,
                          size_t height);

            Tensor::Shape GetInputShape() const override;
            Tensor::Shape GetOutputShape() const override;
            Tensor GetInputTensor() override;
            void PushInput(Tensor&& tensor) override;
            Tensor PopOutput() override;

        private:
            Tensor AllocTensor(size_t channels);

            size_t m_width;
            size_t m_height;
        };
    }
}
