#include <cstddef>

#if defined(_WIN32)
    #ifdef MODEL_RUNNER_BUILD
        #define MODEL_RUNNER_API __declspec(dllexport)
    #else
        #define MODEL_RUNNER_API __declspec(dllimport)
    #endif
#elif defined(__GNUC__)
    #ifdef MODEL_RUNNER_BUILD
        #define MODEL_RUNNER_API __attribute__((visibility ("default")))
    #else
        #define MODEL_RUNNER_API
    #endif
#endif

namespace ML
{
    /**
     * Model interface.
     */
    class MODEL_RUNNER_API Model
    {
    public:
        using ValueType = float;

        /**
         * Takes input 3D array with dimensions (height, width, channels)
         * and infers output 3D array with dimensions (height, width, 3).
         *
         * @param[in] input Input array.
         * @param[in] height First array dimension.
         * @param[in] width Second array dimension.
         * @param[in] channels Last array dimension.
         * @param[out] output Output array.
         */
        virtual void infer(ValueType const* input,
                           std::size_t width,
                           std::size_t height,
                           std::size_t channels,
                           ValueType* output) const = 0;

        virtual ~Model() {}
    };
}

extern "C"
{
    /**
     * Creates and initializes a model.
     *
     * @param model_path Path to a model.
     * @param gpu_memory_fraction Fraction of GPU memory allowed to use
     *                            by versions with GPU support (0..1].
     *                            Use 0 for default configuration.
     * @param visible_devices Comma-delimited list of GPU devices
     *                        accessible for calculations.
     * @return A new model object.
     */
    MODEL_RUNNER_API ML::Model* LoadModel(char const* model_path,
                                          float gpu_memory_fraction,
                                          char const* visible_devices);
}
