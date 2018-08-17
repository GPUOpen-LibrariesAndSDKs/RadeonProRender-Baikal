/**********************************************************************
 Copyright (c) 2018 Advanced Micro Devices, Inc. All rights reserved.

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 ********************************************************************/

#ifndef MIPMAP_LEVEL_SCALER_CL
#define MIPMAP_LEVEL_SCALER_CL

#include <../Baikal/Kernels/CL/common.cl>
#include <../Baikal/Kernels/CL/payload.cl>
#include <../Baikal/Kernels/CL/utils.cl>
#include <../Baikal/Kernels/CL/texture.cl>

#pragma OPENCL EXTENSION cl_khr_fp16 : enable

/// Compute weights for mip filter when the texture size is even
KERNEL
void ComputeWeights_NoRounding(
    // Output weights
    GLOBAL float3* restrict weights,
    // Size of weight buffer
    const int size
)
{
    // Size of the next level will be twice less than the current
    // In this case the convolution formula is:
    // next_level[x] = 0.5 * (curr_level[2 * x] + curr_level[2 * x + 1])

    int id = get_global_id(0);

    if (id < size)
    {
        weights[id] = make_float3(.0f, .5f, .5f);
    }
}

/// Compute weights for mip filter when the texture is odd
KERNEL
void ComputeWeights_RoundingUp(
    // Output weights
    GLOBAL float3* restrict weights,
    // Size of weight buffer
    const int size
)
{
    // Size of the next level will be twice less than the current
    // and rounded up
    // In this case the convolution formula is:
    // next_level[x] = (size - x - 1) / (2 * n - 1) * curr_level[2 * x - 1]
    //               +  size          / (2 * n - 1) * curr_level[2 * x]
    //               +         x      / (2 * n - 1) * curr_level[2 * x + 1]

    int id = get_global_id(0);

    float denominator = 2.f * size - 1.f;

    if (id < size)
    {
        weights[id] = make_float3(
            ((float)size - (float)id - 1.f) / denominator,
            ((float)size) / denominator,
            ((float)id) / denominator);
    }
}

/// Convert uchar buffer element to float and multiply by weight
inline float ComputeMult_uchar(
    // Value buffer
    GLOBAL uchar* buf,
    // Index in the buffer
    int index,
    // Element weight
    float weight
)
{
    return weight * ((float)buf[index]);
}

/// Convert half buffer element to float and multiply by weight
inline float ComputeMult_half(
    // Value buffer
    GLOBAL half* buf,
    // Index in the buffer
    int index,
    // Element weight
    float weight
)
{
    return weight * vload_half(index, buf);
}

/// Multiply float buffer element by weight
inline float ComputeMult_float(
    // Value buffer
    GLOBAL float* buf,
    // Index in the buffer
    int index,
    // Element weight
    float weight
)
{
    return weight * buf[index];
}

/// Convert uchar4 buffer element to float4 and multiply by weight
inline float4 ComputeMult_uchar4(
    // Value buffer
    GLOBAL uchar4* buf,
    // Index in the buffer
    int index,
    // Element weight
    float weight
)
{
    uchar4 val = buf[index];
    return weight * make_float4(
        (float)val.x,
        (float)val.y,
        (float)val.z,
        (float)val.w);
}

/// Convert half4 buffer element to float4 and multiply by weight
inline float4 ComputeMult_half4(
    // Value buffer
    GLOBAL half4* buf,
    // Index in the buffer
    int index,
    // Element weight
    float weight
)
{
    return weight * vload_half4(index, (GLOBAL half*)buf);
}

/// Multiply float4 buffer element by weight
inline float4 ComputeMult_float4(
    // Value buffer
    GLOBAL float4* buf,
    // Index in the buffer
    int index,
    // Element weight
    float weight
)
{
    return weight * buf[index];
}

/// Convert float value to uchar and store it in buffer by index
inline void SetValue_uchar(
    // Value buffer
    GLOBAL uchar* buf,
    // Index in the buffer
    int index,
    // Value to store
    float value
)
{
    buf[index] = (uchar)value;
}

/// Convert float value to half and store it in buffer by index
inline void SetValue_half(
    // Value buffer
    GLOBAL float* buf,
    // Index in the buffer
    int index,
    // Value to store
    float value
)
{
    buf[index] = value;
}

/// Convert float value to float and store it in buffer by index
inline void SetValue_float(
    // Value buffer
    GLOBAL half* buf,
    // Index in the buffer
    int index,
    // Value to store
    float value
)
{
    vstore_half(value, index, buf);
}

/// Convert float4 value to uchar4 and store it in buffer by index
inline void SetValue_uchar4(
    // Value buffer
    GLOBAL uchar4* buf,
    // Index in the buffer
    int index,
    // Value to store
    float4 value
)
{
    uchar4 uchar_val;
    uchar_val.x = (uchar)value.x;
    uchar_val.y = (uchar)value.y;
    uchar_val.z = (uchar)value.z;
    uchar_val.w = (uchar)value.w;
    buf[index] = uchar_val;
}

/// Convert float4 value to half4 and store it in buffer by index
inline void SetValue_half4(
    // Value buffer
    GLOBAL half4* buf,
    // Index in the buffer
    int index,
    // Value to store
    float4 value
)
{
    vstore_half4(value, index, (GLOBAL half*)buf);
}

/// Store float4 value in buffer by index
inline void SetValue_float4(
    // Value buffer
    GLOBAL float4* buf,
    // Index in the buffer
    int index,
    // Value to store
    float4 value
)
{
    buf[index] = value;
}

/// Polymorphic macros that is being used for producing texture downsampling kernel in x direction
#define SCALE_X_PRODUCER(type)\
    KERNEL\
    void ScaleX_##type(\
        int texture_index,\
        int mip_level_index,\
        GLOBAL float3 const* restrict weights,\
        TEXTURE_ARG_LIST,\
        GLOBAL uchar const* restrict tmp_buffer\
    )\
    {\
        int id = get_global_id(0);\
        \
        GLOBAL Texture const* texture = textures + texture_index;\
        /* Get two mip levels */ \
        MipLevel src_mip_level = mip_levels[texture->mip_offset + mip_level_index];\
        MipLevel dst_mip_level = mip_levels[texture->mip_offset + mip_level_index + 1];\
        \
        /* Get texel coordinates of the next and the current level */ \
        int dst_x = id % dst_mip_level.w;\
        int dst_y = id / dst_mip_level.w;\
        int src_x = 2 * dst_x;\
        int src_y = dst_y;\
        \
        GLOBAL type * dst_row = (GLOBAL type*) (tmp_buffer) + dst_y * dst_mip_level.w;\
        GLOBAL type * src_row = (GLOBAL type*) (texturedata + src_mip_level.dataoffset) + src_y * src_mip_level.w;\
        if (id < dst_mip_level.w * src_mip_level.h)\
        {\
            /* Use convolution formula:
            next_level[x] = w0 * curr_level[2 * x - 1]
                          + w1 * curr_level[2 * x]
                          + w2 * curr_level[2 * x + 1]
            Where w0, w1 and w2 are components of weights buffer element */ \
            SetValue_##type(dst_row, dst_x, (\
                        ComputeMult_##type(src_row, WrapTexel(src_x - 1, src_mip_level.w), weights[dst_x].x) +\
                        ComputeMult_##type(src_row, src_x, weights[dst_x].y) +\
                        ComputeMult_##type(src_row, WrapTexel(src_x + 1, src_mip_level.w), weights[dst_x].z)));\
        }\
    }

/// Polymorphic macros that is being used for producing texture downsampling kernel in y direction
#define SCALE_Y_PRODUCER(type)\
    KERNEL\
    void ScaleY_##type(\
        int texture_index,\
        int mip_level_index,\
        GLOBAL float3 const* restrict weights,\
        TEXTURE_ARG_LIST,\
        GLOBAL uchar const* restrict tmp_buffer\
    )\
    {\
        int id = get_global_id(0);\
        \
        GLOBAL Texture const* texture = textures + texture_index;\
        /* Get two mip levels */ \
        MipLevel src_mip_level = mip_levels[texture->mip_offset + mip_level_index];\
        MipLevel dst_mip_level = mip_levels[texture->mip_offset + mip_level_index + 1];\
        \
        /* Get texel coordinates of the next and the current level */ \
        int dst_x = id % dst_mip_level.w;\
        int dst_y = id / dst_mip_level.w;\
        int src_x = dst_x;\
        int src_y = 2 * dst_y;\
        \
        GLOBAL type * dst_row        = (GLOBAL type*) (texturedata + dst_mip_level.dataoffset) + dst_y * dst_mip_level.w;\
        GLOBAL type * top_src_row    = (GLOBAL type*) (tmp_buffer) + WrapTexel(src_y - 1, src_mip_level.h) * dst_mip_level.w;\
        GLOBAL type * src_row        = (GLOBAL type*) (tmp_buffer) + src_y                                 * dst_mip_level.w;\
        GLOBAL type * bottom_src_row = (GLOBAL type*) (tmp_buffer) + WrapTexel(src_y + 1, src_mip_level.h) * dst_mip_level.w;\
        \
        if (id < dst_mip_level.w * dst_mip_level.h)\
        {\
            /* Use convolution formula:
            next_level[x] = w0 * curr_level[2 * x - 1]
                          + w1 * curr_level[2 * x]
                          + w2 * curr_level[2 * x + 1]
            Where w0, w1 and w2 are components of weights buffer element */ \
            SetValue_##type(dst_row, dst_x, (\
                        ComputeMult_##type(top_src_row,    src_x, weights[dst_y].x) +\
                        ComputeMult_##type(src_row,        src_x, weights[dst_y].y) +\
                        ComputeMult_##type(bottom_src_row, src_x, weights[dst_y].z)));\
        }\
    }

// Produce ScaleX 1-channel functions
SCALE_X_PRODUCER(uchar)
SCALE_X_PRODUCER(half)
SCALE_X_PRODUCER(float)

// Produce ScaleY 1-channel functions
SCALE_Y_PRODUCER(uchar)
SCALE_Y_PRODUCER(half)
SCALE_Y_PRODUCER(float)

// Produce ScaleX 4-channel functions
SCALE_X_PRODUCER(uchar4)
SCALE_X_PRODUCER(half4)
SCALE_X_PRODUCER(float4)

// Produce ScaleY 4-channel functions
SCALE_Y_PRODUCER(uchar4)
SCALE_Y_PRODUCER(half4)
SCALE_Y_PRODUCER(float4)

#endif // MIPMAP_LEVEL_SCALER_CL
