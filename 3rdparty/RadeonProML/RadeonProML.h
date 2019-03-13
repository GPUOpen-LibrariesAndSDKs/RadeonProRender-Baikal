#pragma once

/**
 * @file Machine learning model runner API.
 *
 * Typical usage is as follows:
 * -# Create a context with mlCreateContext().
 * -# Set model parameters #ml_model_params.
 * -# Create a model with mlCreateModel() using the parameters.
 * -# Get input image information with mlGetModelInfo().
 * -# Specify required input image dimensions.
 * -# Set up model input image dimensions with mlSetModelInputInfo().
 * -# Get output image information with mlGetModelInfo().
 * -# Create input image.
 * -# Create output image.
 * -# Fill input image with data using mlMapImage() and mlUnmapImage().
 * -# Run inference with mlInfer().
 * -# Get output image data using mlMapImage() and mlUnmapImage().
 * -# If image size is changed, repeat from the step 5.
 * -# If image size is unchanged, repeat from the step 10.
 * -# In a case of a failure call  or mlGetModelError() to get details.
 * -# Release the images using mlReleaseImage().
 * -# Release the model using mlReleaseModel().
 * -# Release the context using mlReleaseContext().
 */

#include <stddef.h>


#if defined(_WIN32)
    #ifdef RADEONPROML_BUILD
        #define ML_API_ENTRY __declspec(dllexport)
    #else
        #define ML_API_ENTRY __declspec(dllimport)
    #endif
#elif defined(__GNUC__)
    #ifdef RADEONPROML_BUILD
        #define ML_API_ENTRY __attribute__((visibility ("default")))
    #else
        #define ML_API_ENTRY
    #endif
#endif


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Model parameters. All unused values must be initialized to 0.
 */
struct ml_model_params
{
    char const* model_path; /**< Path to a model in protobuf format. */

    char const* input_node; /**< Input graph node name, autodetect if null. */

    char const* output_node; /**< Output graph node name, autodetect if null. */

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
 * Context handle.
 */
typedef struct ml_context_t {}* ml_context;

/**
 * Model handle.
 */
typedef struct ml_model_t {}* ml_model;

/**
 * Image handle.
 */
typedef struct ml_image_t {}* ml_image;

#define ML_INVALID_HANDLE NULL

/**
 * Operation status.
 */
enum ml_status
{
    ML_OK,
    ML_FAIL
};

/**
 * Image underlying data type.
 */
enum ml_data_type
{
    ML_FLOAT32
};

/**
 * 3D image description.
 */
struct ml_image_info
{
    ml_data_type dtype; /**< Underlying data type. */
    size_t width;       /**< Image width. in pixels. 0 if unspecified. */
    size_t height;      /**< Image height, in pixels. 0 if unspecified. */
    size_t channels;    /**< Image channel count. 0 if unspecified. */
};


/**
 * Creates a context.
 *
 * @return A valid context handle in case of success, ML_INVALID_HANDLE
 *         otherwise. The context should be released with mlReleaseContext().
 */
ML_API_ENTRY ml_context mlCreateContext();

/**
 * Returns a formatted message with the last operation error.
 * May be called in case an operation returns ML_FAIL or ML_INVALID_HANDLE.
 *
 * @param[in]  context     A valid context handle.
 * @param[out] buffer      A buffer for the message.
 * @param[in]  buffer_size The buffer size, in bytes.
 *
 * @return The formatted message.
 */
ML_API_ENTRY char* mlGetContextError(ml_context context, char* buffer, size_t buffer_size);

/**
 * Releases a context created with mlCreateContext(), invalidates the handle.
 *
 * @param model A valid context handle.
 */
ML_API_ENTRY void mlReleaseContext(ml_context context);


/**
 * Creates a 3D image with a given description.
 * Image dimension order is (height, width, channels).
 *
 * @param[in] context A valid context handle.
 * @param[in] info    Image description with all dimensions specified.
 *
 * @return A valid image handle in case of success, ML_INVALID_HANDLE
 *         otherwise. The image should be released with mlReleaseImage().
 *         To get more details in case of failure, call mlGetContextError().
 */
ML_API_ENTRY ml_image mlCreateImage(ml_context context, ml_image_info const* info);

/**
 * Returns image description.
 *
 * @param[in]  image A valid image handle.
 * @param[out] info  A pointer to a info structure.
 *
 * @return ML_OK in case of success, ML_FAIL otherwise.
 */
ML_API_ENTRY ml_status mlGetImageInfo(ml_image image, ml_image_info* info);

/**
 * Map the image data into the host address and returns a pointer
 * to the mapped region.
 *
 * @param[in]  image A valid image handle.
 * @param[out] size  A pointer to a size variable. If not null, the referenced
 *                   value is set to image size, in bytes.
 *
 * @return A pointer to image data.
 */
ML_API_ENTRY void* mlMapImage(ml_image image, size_t* size);

/**
 * Unmaps a previously mapped image data.
 *
 * @param[in] image A valid image handle.
 * @param[in] size  A pointer to the previously mapped data.
 *
 * @return ML_OK in case of success, ML_FAIL otherwise.
 */
ML_API_ENTRY ml_status mlUnmapImage(ml_image image, void* data);

/**
 * Releases an image created with mlCreateImage(), invalidates the handle.
 *
 * @param[in] image A valid image handle.
 */
ML_API_ENTRY void mlReleaseImage(ml_image image);


/**
 * Loads model data from a file.
 *
 * @param[in] model  A valid context handle.
 * @param[in] params Model parameters. @see #ml_model_params.
 *
 * @return ML_OK in case of success, ML_FAIL otherwise.
 *         To get more details in case of failure, call mlGetContextError().
 */
ML_API_ENTRY ml_model mlCreateModel(ml_context context, ml_model_params const* params);

/**
 * Returns a formatted message with the last operation error.
 * May be called in case an operation returns ML_FAIL or ML_INVALID_HANDLE.
 *
 * @param[in]  model       A valid model handle.
 * @param[out] buffer      A buffer for the message.
 * @param[in]  buffer_size The buffer size, in bytes.
 *
 * @return The formatted message.
 */
ML_API_ENTRY char* mlGetModelError(ml_model model, char* buffer, size_t buffer_size);

/**
 * Returns input image information.
 *
 * @param[in]  model       A valid model handle.
 * @param[out] input_info  A pointer to the result input info structure, may be null.
 *                         If mlSetModelInputInfo() was not previously called,
 *                         some dimensions may not be specified.
 * @param[out] output_info A pointer to the result output info structure, may be null.
 *                         If mlSetModelInputInfo() was not previously called,
 *                         some dimensions may not be specified.
 *
 * @return ML_OK in case of success, ML_FAIL otherwise.
 *         To get more details in case of failure, call mlGetModelError().
 */
ML_API_ENTRY ml_status mlGetModelInfo(ml_model model,
                                      ml_image_info* input_info,
                                      ml_image_info* output_info);

/**
 * Updates input image information. All image dimensions must be specified.
 * @note This is a heavy operation, so avoid using it frequently.
 *
 * @param[in] model A valid model handle.
 * @param[in] info  Input image information. The specified dimensions must
 *                  match the dimensions already known by the model.
 */
ML_API_ENTRY ml_status mlSetModelInputInfo(ml_model model, ml_image_info const* info);

/**
 * Gets an input image and fills an output image.
 *
 * @param[in] model  A valid model handle.
 * @param[in] input  A valid input image descriptor.
 * @param[in] output A valid output image descriptor.
 *
 * @return ML_OK in case of success, ML_FAIL otherwise.
 *         To get more details in case of failure, call mlGetModelError().
 */
ML_API_ENTRY ml_status mlInfer(ml_model model, ml_image input, ml_image output);

/**
 * Releases a model loaded with mlCreateModel(), invalidates the handle.
 *
 * @param model A valid model handle.
 */
ML_API_ENTRY void mlReleaseModel(ml_model model);


#ifdef __cplusplus
} // extern "C"
#endif
