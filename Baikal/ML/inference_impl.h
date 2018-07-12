#include "inference.h"

#include <memory>
#include <mutex>
#include <optional>


namespace Baikal::ML
{
    class InferenceImpl : public Inference
    {
    public:
        explicit InferenceImpl(Buffer::Shape input_shape);

        Buffer GetInputBuffer() override;
        void PushInput(Buffer&& buffer) override;
        std::optional<Buffer> PopOutput() const override;

    private:
        Buffer::Shape m_input_shape;
        Buffer::Shape m_output_shape;
    };

} // namespace Baikal::ML
