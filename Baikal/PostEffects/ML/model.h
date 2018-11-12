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
        virtual void infer(void const* input,
                           std::size_t width,
                           std::size_t height,
                           std::size_t channels,
                           void* output) const = 0;

        virtual ~Model() {}
    };
}

extern "C"
{
    /**
     * Model parameters. All unused values must be initialized to 0.
     */
    struct ModelParams
    {
        char const* model_path; /**< Path to a model in protobuf format. */

        char const* input_node; /**< Input graph node name, "inputs" by default. */

        char const* output_node; /**< Output graph node name, "outputs" by default. */

        float gpu_memory_fraction; /**<
                                    * Fraction of GPU memory allowed to use
                                    * by versions with GPU support, (0..1].
                                    * All memory is used by default.
                                    */

        char const* visible_devices; /**<
                                      * Comma-delimited list of GPU devices
                                      * accessible for calculations.
                                      * All devices are visible by default.
                                      */
    };

    /**
     * Creates and initializes a model.
     *
     * @param params Model parameters.
     *
     * @return A new model object in case of success, null otherwise.
     */
    MODEL_RUNNER_API ML::Model* LoadModel(ModelParams const* params);
}
