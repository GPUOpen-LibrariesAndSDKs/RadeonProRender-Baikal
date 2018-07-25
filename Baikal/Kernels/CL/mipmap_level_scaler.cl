/**********************************************************************
Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved.

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

KERNEL
void ComputeWeights_NoRounding(
    GLOBAL float3* restrict weights,
    // size of weight vector
    const int size)
{
    int id = get_global_id(0);

    if (id < size)
    {
        weights[id].x = .0f;
        weights[id].y = .5f;
        weights[id].z = .5f;
    }
}

KERNEL
void ComputeWeights_RoundingUp(
    GLOBAL float3* restrict weights,
    // size of weight vector
    const int size)
{
    int id = get_global_id(0);

    float denominator = 2.f * size - 1.f;

    // first weight
    if (id == 0)
    {
        weights[id].x = .0f;
        weights[id].y = ((float)size) / denominator;
        weights[id].z = ((float)size - 1.f) / denominator;
        return;
    }

    // last weight
    if (id == size - 1)
    {
        weights[id].x = ((float)size - 1.f) / denominator;
        weights[id].y = ((float)size) / denominator;
        weights[id].z = .0f;
        return;
    }

    if (id < size - 1)
    {
        weights[id].x = ((float)size - (float)id - 1.f) / denominator;
        weights[id].y = ((float)size) / denominator;
        weights[id].z = ((float)id) / denominator;
    }
}

// computes type conversion to float/float4 and multiplication
inline float ComputeMult_uchar(
    GLOBAL uchar* buf, int index, float weight)
{
    GLOBAL uchar *val = buf + index;
    return weight * ((float)(*val));
}

inline float ComputeMult_float(
    GLOBAL float* buf, int index, float weight)
{
    GLOBAL float *val = buf + index;
    return weight * (*val);
}

inline float ComputeMult_half(
    GLOBAL half* buf, int index, float weight)
{
    return weight * vload_half(index, buf);
}

inline float4 ComputeMult_uchar4(
    GLOBAL uchar4* buf, int index, float weight)
{
    GLOBAL uchar4 *val = buf + index;
    return weight * make_float4(
        (float)(*val).x,
        (float)(*val).y,
        (float)(*val).z,
        (float)(*val).w);
}

inline float4 ComputeMult_float4(
    GLOBAL float4* buf, int index, float weight)
{
    return weight * (*(buf + index));
}

inline float4 ComputeMult_half4(
    GLOBAL half4* buf, int index, float weight)
{
    return weight * vload_half4(index, (GLOBAL half*)buf);
}

// convert float/float4 type to user and store it in buffer by index
inline void SetValue_uchar(
    GLOBAL uchar* buf, int index, float value)
{
    buf[index] = (uchar)value;
}

inline void SetValue_float(
    GLOBAL half* buf, int index, float value)
{
    vstore_half(value, index, buf);
}

inline void SetValue_half(
    GLOBAL float* buf, int index, float value)
{
    buf[index] = value;
}

inline void SetValue_uchar4(
    GLOBAL uchar4* buf, int index, float4 value)
{
    uchar4 uchar_val;
    uchar_val.x = (uchar)value.x;
    uchar_val.y = (uchar)value.y;
    uchar_val.z = (uchar)value.z;
    uchar_val.w = (uchar)value.w;
    buf[index] = uchar_val;
}

inline void SetValue_float4(
    GLOBAL float4* buf, int index, float4 value)
{
    buf[index] = value;
}

inline void SetValue_half4(
    GLOBAL half4* buf, int index, float4 value)
{
    vstore_half4(value, index, (GLOBAL half*)buf);
}

// level scaler kernels scheme
#define TEXEL_SIZE_uchar 1
#define TEXEL_SIZE_half  2
#define TEXEL_SIZE_float 4

#define TEXEL_SIZE_uchar4 4
#define TEXEL_SIZE_half4  8
#define TEXEL_SIZE_float4 16

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
        MipLevel src_mip_level = mip_levels[texture->mip_offset + mip_level_index];\
        MipLevel dst_mip_level = mip_levels[texture->mip_offset + mip_level_index + 1];\
        \
        int dst_x = id % dst_mip_level.w;\
        int dst_y = id / dst_mip_level.w;\
        int src_x = 2 * dst_x;\
        int src_y = dst_y;\
        \
        GLOBAL type * dst_row = (GLOBAL type*) (tmp_buffer + dst_y * dst_mip_level.w * TEXEL_SIZE_##type);\
        GLOBAL type * src_row = (GLOBAL type*) (texturedata + src_mip_level.dataoffset + src_y * src_mip_level.w * TEXEL_SIZE_##type);\
        \
        if (dst_x == 0)\
        {\
            SetValue_##type(dst_row, dst_x, (\
                        ComputeMult_##type(src_row, src_x, weights[dst_x].y) +\
                        ComputeMult_##type(src_row, src_x + 1, weights[dst_x].z)));\
            return;\
        }\
        \
        if (dst_x == dst_mip_level.w - 1)\
        {\
            SetValue_##type(dst_row, dst_x, (\
                        ComputeMult_##type(src_row, src_x - 1, weights[dst_x].x) +\
                        ComputeMult_##type(src_row, src_x, weights[dst_x].y)));\
            return;\
        }\
        \
        if (id < dst_mip_level.w * src_mip_level.h)\
        {\
            SetValue_##type(dst_row, dst_x, (\
                        ComputeMult_##type(src_row, src_x - 1, weights[dst_x].x) +\
                        ComputeMult_##type(src_row, src_x, weights[dst_x].y) +\
                        ComputeMult_##type(src_row, src_x + 1, weights[dst_x].z)));\
        }\
    }

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
        MipLevel src_mip_level = mip_levels[texture->mip_offset + mip_level_index];\
        MipLevel dst_mip_level = mip_levels[texture->mip_offset + mip_level_index + 1];\
        \
        int dst_x = id % dst_mip_level.w;\
        int dst_y = id / dst_mip_level.w;\
        int src_x = dst_x;\
        int src_y = 2 * dst_y;\
        \
        GLOBAL type * dst_row        = (GLOBAL type*) (texturedata + dst_mip_level.dataoffset + dst_y * dst_mip_level.w * TEXEL_SIZE_##type);\
        GLOBAL type * top_src_row    = (GLOBAL type*) (tmp_buffer + (src_y - 1) * dst_mip_level.w * TEXEL_SIZE_##type);\
        GLOBAL type * src_row        = (GLOBAL type*) (tmp_buffer + src_y       * dst_mip_level.w * TEXEL_SIZE_##type);\
        GLOBAL type * bottom_src_row = (GLOBAL type*) (tmp_buffer + (src_y + 1) * dst_mip_level.w * TEXEL_SIZE_##type);\
        \
        if (dst_y == 0)\
        {\
            SetValue_##type(dst_row, dst_x, (\
                        ComputeMult_##type(src_row, src_x, weights[dst_y].y) +\
                        ComputeMult_##type(bottom_src_row, src_x, weights[dst_y].z)));\
            return;\
        }\
        \
        if (dst_y == dst_mip_level.h - 1)\
        {\
            SetValue_##type(dst_row, dst_x, (\
                        ComputeMult_##type(top_src_row, src_x, weights[dst_y].x) +\
                        ComputeMult_##type(src_row, src_x, weights[dst_y].y)));\
            return;\
        }\
        \
        if (id < dst_mip_level.w * dst_mip_level.h)\
        {\
            SetValue_##type(dst_row, dst_x, (\
                        ComputeMult_##type(top_src_row, src_x, weights[dst_y].x) +\
                        ComputeMult_##type(src_row, src_x, weights[dst_y].y) +\
                        ComputeMult_##type(bottom_src_row, src_x, weights[dst_y].z)));\
        }\
    }

// produce functions part

// produce ScaleX 1 chanel functions
SCALE_X_PRODUCER(uchar)
SCALE_X_PRODUCER(half)
SCALE_X_PRODUCER(float)

// produce ScaleY 1 chanel functions
SCALE_Y_PRODUCER(uchar)
SCALE_Y_PRODUCER(half)
SCALE_Y_PRODUCER(float)

// produce ScaleX 4 chanel functions
SCALE_X_PRODUCER(uchar4)
SCALE_X_PRODUCER(half4)
SCALE_X_PRODUCER(float4)

// produce ScaleY 4 chanel functions
SCALE_Y_PRODUCER(uchar4)
SCALE_Y_PRODUCER(half4)
SCALE_Y_PRODUCER(float4)

#endif // MIPMAP_LEVEL_SCALER_CL
