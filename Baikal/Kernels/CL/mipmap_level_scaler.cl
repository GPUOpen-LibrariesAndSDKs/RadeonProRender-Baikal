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
#include <../Baikal/Kernels/CL/texture.cl>

#pragma OPENCL EXTENSION cl_khr_fp16 : enable

#define DST_BUFFER_ARG_LIST GLOBAL uchar* restrict dst_buf, \
    /* in bytes */ const int dst_offset, /* in pixels */const int dst_width, \
    /* in pixels */const int dst_height, /* in bytes */ const int dst_pitch
    
#define SRC_BUFFER_ARG_LIST GLOBAL uchar* restrict src_buf, \
    /* in bytes */ const int src_offset, /* in pixels */const int src_width, \
    /* in pixels */const int src_height, /* in bytes */ const int src_pitch

#define GET_VALUE_PRODUCER(type)\
    inline type GetValue(type* buf, int index)\
    {\
        return buf[index];\
    }\

#define SET_VALUE_PRODUCER(type)\
    inline void SetValue(type* buf, int index, type value)\
    {\
        buf[index] = value;\
    }\

#define SCALE_X_1C_PRODUCER(type)\
    KERNEL\
    void ScaleX_1C_##type(\
        GLOBAL float3 const* restrict weights,\
        DST_BUFFER_ARG_LIST,\
        SRC_BUFFER_ARG_LIST)\
    {\
        int id = get_global_id(0);\
        int dst_x = id % dst_width;\
        int dst_y = (id - dst_x) / dst_width;\
        int src_x = 2 * dst_x;\
        int src_y = dst_y;\
        \
        type * dst_row = (type*) (dst_buf + dst_offset + dst_y * dst_pitch);\
        type * src_row = (type*) (src_buf + src_offset + src_y * src_pitch);\
        \
        if (dst_x == 0)\
        {\
            SetValue(dst_row, dst_x, (type)(\
                        weights[dst_x].y * ((float)GetValue(src_row, src_x)) +    \
                        weights[dst_x].z * ((float)GetValue(src_row, src_x + 1)));\
            return;\
        }\
        \
        if (dst_x == dst_width - 1)\
        {\
            SetValue(dst_row, dst_x, (type)(\
                        weights[dst_x].x * ((float)GetValue(src_row, src_x - 1)) +\
                        weights[dst_x].y * ((float)GetValue(src_row, src_x)));\
            return;\
        }\
        \
        if (id < dst_width * dst_height)\
        {\
            SetValue(dst_row, dst_x, (type)(\
                        weights[dst_x].x * ((float)GetValue(src_row, src_x - 1)) +\
                        weights[dst_x].y * ((float)GetValue(src_row, src_x)) +\
                        weights[dst_x].z * ((float)GetValue(src_row, src_x + 1)));\
        }\
    }\



#define SCALE_Y_1C_PRODUCER(type)\
    KERNEL \
    void ScaleX_1C_##type(\
        GLOBAL float3 const* restrict weights,\
        DST_BUFFER_ARG_LIST,\
        SRC_BUFFER_ARG_LIST)\
    {\
        int id = get_global_id(0);\
        int dst_x = id % dst_width;\
        int dst_y = (int)((id - dst_x) / dst_width);\
        int src_x = dst_x;\
        int src_y = 2 * dst_y;\
        \
        type * dst_row = (type*) (dst_buf + dst_offset + dst_y * dst_pitch);\
        type * top_src_row = (type*) (src_buf + src_offset + (src_y - 1) * src_pitch);\
        type * src_row = (type*) (src_buf + src_offset + src_y * src_pitch);\
        type * bottom_src_row = (type*) (src_buf + src_offset + (src_y + 1) * src_pitch);\
        \
        if (dst_y == 0)\
        {\
        \
            SetValue(dst_row, dst_x, (type)(\
                        weights[dst_y].y * (float)GetValue(src_row, src_x) +\
                        weights[dst_y].z * (float)GetValue(bottom_src_row, src_x));\
            return;\
        }\
        \
        if (dst_y == dst_height - 1)\
        {\
            SetValue(dst_row, dst_x, (type)(\
                        weights[dst_y].x * (float)GetValue(top_src_row, src_x) +\
                        weights[dst_y].y * (float)GetValue(src_row, src_x));\
            return;\
        }\
        \
        if (id < dst_width * dst_height)\
        {\
            SetValue(dst_row, dst_x, (type)(\
                        weights[dst_y].x * (float)GetValue(top_src_row, src_x) +\
                        weights[dst_y].y * (float)GetValue(src_row, src_x) +\
                        weights[dst_y].z * (float)GetValue(bottom_src_row, src_x));\
        }\
    }\

#define SCALE_X_4C_PRODUCER(type)\
    KERNEL\
    void ScaleX_4C_##type(\
        GLOBAL float3 const* restrict weights,\
        DST_BUFFER_ARG_LIST,\
        SRC_BUFFER_ARG_LIST)\
    {\
        int id = get_global_id(0);\
        int dst_x = id % dst_width;\
        int dst_y = (id - dst_x) / dst_width;\
        int src_x = 2 * dst_x;\
        int src_y = dst_y;\
    \
        type * dst_row = (type*) (dst_buf + dst_offset + dst_y * dst_pitch);\
        type * src_row = (type*) (src_buf + src_offset + src_y * src_pitch);\
    \
        if (dst_x == 0)\
        {\
            SetValue(dst_row, dst_x, (type)(\
                        (((float4)GetValue(src_row, src_x)).xyzw) * weights[dst_x].y +\
                        (((float4)GetValue(src_row, src_x + 1)).xyzw) * weights[dst_x].z);\
            return;\
        }\
    \
        if (dst_x == dst_width - 1)\
        {\
            SetValue(dst_row, dst_x, (type)(\
                        (((float4)GetValue(src_row, src_x - 1)).xyzw) * weights[dst_x].x +\
                        (((float4)GetValue(src_row, src_x)).xyzw) * weights[dst_x].y);\
        return;\
        }\
    \
        if (id < dst_width * dst_height)\
        {\
            SetValue(dst_row, dst_x, (type)(\
                        (((float4)GetValue(src_row, src_x - 1)).xyzw) * weights[dst_x].x +\
                        (((float4)GetValue(src_row, src_x)).xyzw) * weights[dst_x].y + \
                        (((float4)GetValue(src_row, src_x + 1)).xyzw) * weights[dst_x].z);\
        }\
    }\

#define SCALE_Y_4C_PRODUCER(type)\
    KERNEL\
    void ScaleY_4C_##type(\
        GLOBAL float3 const* restrict weights,\
        DST_BUFFER_ARG_LIST,\
        SRC_BUFFER_ARG_LIST)\
    {\
        int id = get_global_id(0);\
        int dst_x = id % dst_width;\
        int dst_y = (id - dst_x) / dst_width;\
        int src_x = dst_x;\
        int src_y = 2 * dst_row;\
    \
        type * dst_row = (type*) (dst_buf + dst_offset + dst_y * dst_pitch);\
        type * top_src_row = (type*) (src_buf + src_offset + (src_y - 1) * src_pitch);\
        type * src_row = (type*) (src_buf + src_offset + src_y * src_pitch);\
        type * bottom_src_row = (type*) (src_buf + src_offset + (src_y + 1) * src_pitch);\
    \
        if (dst_row == 0)\
        {\
            SetValue(dst_row, dst_x, (type)(\
                    (((float4)GetValue(src_row, src_x)).xyzw) * weights[dst_x].y +\
                    (((float4)GetValue(bottom_src_row, src_x)).xyzw) * weights[dst_x].z);\
            return;\
        }\
    \
        if (dst_row == dst_height - 1)\
        {\
            SetValue(dst_row, dst_x, (type)(\
                    (((float4)GetValue(top_src_row, src_x)).xyzw) * weights[dst_x].x +\
                    (((float4)GetValue(src_row, src_x)).xyzw) * weights[dst_x].y);\
            return;\
        }\
    \
        if (id < dst_width * dst_height)\
        {\
            SetValue(dst_row, dst_x, (type)(\
                    (((float4)GetValue(top_src_row, src_x)).xyzw) * weights[dst_x].x +\
                    (((float4)GetValue(src_row, src_x)).xyzw) * weights[dst_x].y +\
                    (((float4)GetValue(bottom_src_row, src_x)).xyzw) * weights[dst_x].z)\
        }\
    }\

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


// produce functions part
// order is important (!!!)

// produce GetValue helper functions
GET_VALUE_PRODUCER(uchar)
GET_VALUE_PRODUCER(float)
GET_VALUE_PRODUCER(float4)

inline float GetValue(half* buf, int index)
{
    return vload_half(index, buf);
}

inline float4 GetValue(half4* buf, int index)
{
    return vload_half4(index, buf);
}

// produce SetValue helper functions

SET_VALUE_PRODUCER(uchar)
SET_VALUE_PRODUCER(float)
SET_VALUE_PRODUCER(float4)

inline void SetValue(half* buf, int index, float value) 
{
    vstore_half(value, index, buf);
}

inline void SetValue(half4* buf, int index, float4 value) 
{
    vstore_half4(value, index, buf);
}

// produce ScaleX_1C functions
SCALE_X_1C_PRODUCER(uchar)
SCALE_X_1C_PRODUCER(half)
SCALE_X_1C_PRODUCER(float)

// produce ScaleY_1C functions
SCALE_Y_1C_PRODUCER(uchar)
SCALE_Y_1C_PRODUCER(half)
SCALE_Y_1C_PRODUCER(float)

// produce ScaleX_4C functions
SCALE_X_4C_PRODUCER(uchar4)
SCALE_X_4C_PRODUCER(half4)
SCALE_X_4C_PRODUCER(float4)

// produce ScaleY_4C functions
SCALE_Y_4C_PRODUCER(uchar4)
SCALE_Y_4C_PRODUCER(half4)
SCALE_Y_4C_PRODUCER(float4)

#endif // MIPMAP_LEVEL_SCALER_CL
