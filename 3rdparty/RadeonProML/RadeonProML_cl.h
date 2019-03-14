#pragma once

/**
 * @file Interop with OpenCL.
 */

#include <RadeonProML.h>

#if defined(__APPLE__) || defined(__MACOSX)
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif


#ifdef __cplusplus
extern "C" {
#endif

/**
 * Creates a context from an OpenCL context.
 *
 * @param[in] queue An OpenCL command queue handle.
 *
 * @return A valid context handle in case of success, NULL otherwise.
 *         The context should be released with mlReleaseContext().
 */

ML_API_ENTRY ml_context mlCreateContextFromClQueue(cl_command_queue queue);

/**
 * Creates an image from an OpenCL buffer.
 *
 * @param[in] context A valid context handle.
 * @param[in] buffer  A valid OpenCL memory object handle.
 * @param[in] info    Image description with all dimensions specified.
 * @param[in] mode    Image data access mode.
 *
 * @return A valid image handle in case of success, NULL otherwise.
 *         The image should be released with mlReleaseImage().
 *         To get more details in case of failure, call mlGetContextError().
 */

ML_API_ENTRY ml_image mlCreateImageFromClBuffer(ml_context context,
                                                cl_mem buffer,
                                                const ml_image_info* info,
                                                ml_access_mode mode);

#ifdef __cplusplus
} // extern "C"
#endif
