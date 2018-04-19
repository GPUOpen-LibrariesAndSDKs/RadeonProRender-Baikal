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

KERNEL
void ComputeWeights_NoRounding(
    GLOBAL float4* restrict weights,
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
    GLOBAL float4* restrict weights,
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

KERNEL
void ScaleX_1C8U(
    GLOBAL uchar* restrict dst_buf,
    GLOBAL float4 const* restrict weights,
    /* in bytes */ const int dst_offset, const int dst_width,
     const int dst_height, /* in bytes */ const int dst_pitch,
    GLOBAL uchar const* restrict src_buf,
    /* in bytes */ const int src_offset, const int src_width,
    const int src_height, /* in bytes */ const int src_pitch
    )
{
    int id = get_global_id(0);
    int dst_col = id % dst_width;
    int dst_row = (id - dst_col) / dst_width;
    int src_col = 2 * dst_col;
    int src_row = dst_row;

        // offsets in bytes
    int left_pixel = src_offset + (src_row * src_pitch + (src_col - 1));
    int center_pixel = src_offset + (src_row * src_pitch + src_col);
    int right_pixel = src_row * src_pitch + src_col + 1;
    int dst_pixel = dst_row * dst_pitch + dst_col;

    // first pixel in row
    if (dst_col == 0)
    {
        dst_buf[dst_pixel] = (uchar)(
                    weights[dst_col].y * ((float)src_buf[center_pixel]) +
                    weights[dst_col].z * ((float)src_buf[right_pixel]));
        return;
    }

    // last pixel in row
    if (dst_col == dst_width - 1)
    {
        dst_buf[dst_pixel] = (uchar)(
                    weights[dst_col].x * ((float)src_buf[left_pixel]) +
                    weights[dst_col].y * ((float)src_buf[center_pixel]));
        return;
    }

    if (id < dst_width * dst_height)
    {
        dst_buf[dst_pixel] = (uchar)(
                    weights[dst_col].x * ((float)src_buf[left_pixel]) + 
                    weights[dst_col].y * ((float)src_buf[center_pixel]) + 
                    weights[dst_col].z * ((float)src_buf[right_pixel]));
    }
}

KERNEL
void ScaleX_1C16U(
    GLOBAL uchar* restrict dst_buf,
    GLOBAL float4 const* restrict weights,
    /* in bytes */ const int dst_offset, const int dst_width,
     const int dst_height, /* in bytes */ const int dst_pitch,
    GLOBAL uchar const* restrict src_buf,
    /* in bytes */ const int src_offset, const int src_width,
    const int src_height, /* in bytes */ const int src_pitch
    )
{
    int id = get_global_id(0);
    int dst_col = id % dst_width;
    int dst_row = (id - dst_col) / dst_width;
    int src_col = 2 * dst_col;
    int src_row = dst_row;

        // offsets in bytes
    int left_pixel = src_offset + (src_row * src_pitch + (src_col - 1));
    int center_pixel = src_offset + (src_row * src_pitch + src_col);
    int right_pixel = src_row * src_pitch + src_col + 1;
    int dst_pixel = dst_row * dst_pitch + dst_col;

    __global half const* src_buf_16u = (__global half const*)src_buf;
    __global half * dst_buf_16u = (__global half *)dst_buf;

    // first pixel in row
    if (dst_col == 0)
    {
        dst_buf_16u[dst_pixel] = (uchar)(
                    weights[dst_col].y * ((float)src_buf_16u[center_pixel]) +
                    weights[dst_col].z * ((float)src_buf_16u[right_pixel]));
        return;
    }

    // last pixel in row
    if (dst_col == dst_width - 1)
    {
        dst_buf_16u[dst_pixel] = (uchar)(
                    weights[dst_col].x * ((float)src_buf_16u[left_pixel]) +
                    weights[dst_col].y * ((float)src_buf_16u[center_pixel]));
        return;
    }

    if (id < dst_width * dst_height)
    {
        dst_buf_16u[dst_pixel] = (uchar)(
                    weights[dst_col].x * ((float)src_buf_16u[left_pixel]) + 
                    weights[dst_col].y * ((float)src_buf_16u[center_pixel]) + 
                    weights[dst_col].z * ((float)src_buf_16u[right_pixel]));
    }
}

KERNEL
void ScaleX_1C32U(
    GLOBAL uchar* restrict dst_buf,
    GLOBAL float4 const* restrict weights,
    /* in bytes */ const int dst_offset, const int dst_width,
     const int dst_height, /* in bytes */ const int dst_pitch,
    GLOBAL uchar const* restrict src_buf,
    /* in bytes */ const int src_offset, const int src_width,
    const int src_height, /* in bytes */ const int src_pitch
    )
{
    int id = get_global_id(0);
    int dst_col = id % dst_width;
    int dst_row = (id - dst_col) / dst_width;
    int src_col = 2 * dst_col;
    int src_row = dst_row;

        // offsets in bytes
    int left_pixel = src_offset + (src_row * src_pitch + (src_col - 1));
    int center_pixel = src_offset + (src_row * src_pitch + src_col);
    int right_pixel = src_row * src_pitch + src_col + 1;
    int dst_pixel = dst_row * dst_pitch + dst_col;

    __global half const* src_buf_32u = (__global float3 const*)src_buf;
    __global half * dst_buf_32u = (__global float3 *)src_buf;

    // first pixel in row
    if (dst_col == 0)
    {
        dst_buf_32u[dst_pixel] = (uchar)(
                    weights[dst_col].y * ((float)src_buf_32u[center_pixel]) +
                    weights[dst_col].z * ((float)src_buf_32u[right_pixel]));
        return;
    }

    // last pixel in row
    if (dst_col == dst_width - 1)
    {
        dst_buf_32u[dst_pixel] = (uchar)(
                    weights[dst_col].x * ((float)src_buf_32u[left_pixel]) +
                    weights[dst_col].y * ((float)src_buf_32u[center_pixel]));
        return;
    }

    if (id < dst_width * dst_height)
    {
        dst_buf_32u[dst_pixel] = (uchar)(
                    weights[dst_col].x * ((float)src_buf_32u[left_pixel]) + 
                    weights[dst_col].y * ((float)src_buf_32u[center_pixel]) + 
                    weights[dst_col].z * ((float)src_buf_32u[right_pixel]));
    }
}


KERNEL
void ScaleY_1C8U(
    GLOBAL uchar* restrict dst_buf,
    GLOBAL float4 const* restrict weights,
    /* in bytes */ const int dst_offset, const int dst_width,
    const int dst_height, /* in bytes */ const int dst_pitch,
    GLOBAL uchar* restrict src_buf,
    /* in bytes */ const int src_offset, const int src_width,
    const int src_height, /* in bytes */ const int src_pitch
    )
{
    int id = get_global_id(0);
    int dst_col = id % dst_width;
    int dst_row = (int)((id - dst_col) / dst_width);
    int src_col = dst_col;
    int src_row = 2 * dst_row;

    // offsets in bytes
    int top_pixel = src_offset + (src_row - 1) * src_pitch +  src_col;
    int center_pixel = src_offset + src_row * src_pitch + src_col;
    int bottom_pixel = src_offset + (src_row + 1) * src_pitch + src_col;
    int dst_pixel = dst_offset + dst_row * dst_pitch + dst_col;

    // first row
    if (dst_row == 0)
    {
        dst_buf[dst_pixel] = (uchar)(
                    weights[dst_row].y * (float)(src_buf[center_pixel]) +
                    weights[dst_row].z * (float)(src_buf[bottom_pixel]));
        return;
    }

    // last row
    if (dst_row == dst_height - 1)
    {
        dst_buf[dst_pixel] = (uchar)(
                    weights[dst_row].x * (float)(src_buf[top_pixel]) +
                    weights[dst_row].y * (float)(src_buf[center_pixel]));
        return;
    }

    if (id < dst_width * dst_height)
    {
        dst_buf[dst_pixel] = (uchar)(
                    weights[dst_row].x * (float)(src_buf[top_pixel]) + 
                    weights[dst_row].y * (float)(src_buf[center_pixel]) + 
                    weights[dst_row].z * (float)(src_buf[bottom_pixel]));
    }
}

KERNEL
void ScaleY_1C16U(
    GLOBAL uchar* restrict dst_buf,
    GLOBAL float4 const* restrict weights,
    /* in bytes */ const int dst_offset, const int dst_width,
    const int dst_height, /* in bytes */ const int dst_pitch,
    GLOBAL uchar* restrict src_buf,
    /* in bytes */ const int src_offset, const int src_width,
    const int src_height, /* in bytes */ const int src_pitch
    )
{
    int id = get_global_id(0);
    int dst_col = id % dst_width;
    int dst_row = (int)((id - dst_col) / dst_width);
    int src_col = dst_col;
    int src_row = 2 * dst_row;

    // offsets in bytes
    int top_pixel = src_offset + (src_row - 1) * src_pitch +  src_col;
    int center_pixel = src_offset + src_row * src_pitch + src_col;
    int bottom_pixel = src_offset + (src_row + 1) * src_pitch + src_col;
    int dst_pixel = dst_offset + dst_row * dst_pitch + dst_col;

    __global half const* src_buf_16u = (__global half const*)src_buf;
    __global half * dst_buf_16u = (__global half *)dst_buf;

    // first row
    if (dst_row == 0)
    {
        dst_buf_16u[dst_pixel] = (uchar)(
                    weights[dst_row].y * (float)(src_buf_16u[center_pixel]) +
                    weights[dst_row].z * (float)(src_buf_16u[bottom_pixel]));
        return;
    }

    // last row
    if (dst_row == dst_height - 1)
    {
        dst_buf_16u[dst_pixel] = (uchar)(
                    weights[dst_row].x * (float)(src_buf_16u[top_pixel]) +
                    weights[dst_row].y * (float)(src_buf_16u[center_pixel]));
        return;
    }

    if (id < dst_width * dst_height)
    {
        dst_buf_16u[dst_pixel] = (uchar)(
                    weights[dst_row].x * (float)(src_buf_16u[top_pixel]) + 
                    weights[dst_row].y * (float)(src_buf_16u[center_pixel]) + 
                    weights[dst_row].z * (float)(src_buf_16u[bottom_pixel]));
    }
}

KERNEL
void ScaleY_1C32U(
    GLOBAL uchar* restrict dst_buf,
    GLOBAL float4 const* restrict weights,
    /* in bytes */ const int dst_offset, const int dst_width,
    const int dst_height, /* in bytes */ const int dst_pitch,
    GLOBAL uchar* restrict src_buf,
    /* in bytes */ const int src_offset, const int src_width,
    const int src_height, /* in bytes */ const int src_pitch
    )
{
    int id = get_global_id(0);
    int dst_col = id % dst_width;
    int dst_row = (int)((id - dst_col) / dst_width);
    int src_col = dst_col;
    int src_row = 2 * dst_row;

    // offsets in bytes
    int top_pixel = src_offset + (src_row - 1) * src_pitch +  src_col;
    int center_pixel = src_offset + src_row * src_pitch + src_col;
    int bottom_pixel = src_offset + (src_row + 1) * src_pitch + src_col;
    int dst_pixel = dst_offset + dst_row * dst_pitch + dst_col;

    __global float3 const* src_buf_16u = (__global float3 const*)src_buf;
    __global float3 * dst_buf_16u = (__global float3*)dst_buf;

    // first row
    if (dst_row == 0)
    {
        dst_buf_16u[dst_pixel] = (uchar)(
                    weights[dst_row].y * (float)(src_buf_16u[center_pixel]) +
                    weights[dst_row].z * (float)(src_buf_16u[bottom_pixel]));
        return;
    }

    // last row
    if (dst_row == dst_height - 1)
    {
        dst_buf_16u[dst_pixel] = (uchar)(
                    weights[dst_row].x * (float)(src_buf_16u[top_pixel]) +
                    weights[dst_row].y * (float)(src_buf_16u[center_pixel]));
        return;
    }

    if (id < dst_width * dst_height)
    {
        dst_buf_16u[dst_pixel] = (uchar)(
                    weights[dst_row].x * (float)(src_buf_16u[top_pixel]) + 
                    weights[dst_row].y * (float)(src_buf_16u[center_pixel]) + 
                    weights[dst_row].z * (float)(src_buf_16u[bottom_pixel]));
    }
}

KERNEL
void ScaleX_4C8U(
    GLOBAL uchar* restrict dst_buf,
    GLOBAL float4 const* restrict weights, int format,
    /* in bytes */ const int dst_offset, const int dst_width, 
    const int dst_height, /* in bytes */ const int dst_pitch,
    GLOBAL uchar const* restrict src_buf,
    /* in bytes */ const int src_offset, const int src_width,
     const int src_height, /* in bytes */ const int src_pitch
    )
{
    int id = get_global_id(0);
    int dst_col = id % dst_width;
    int dst_row = (id - dst_col) / dst_width;
    int src_col = 2 * dst_col;
    int src_row = dst_row;

    // offsets in bytes
    int left_pixel = src_offset + (src_row * src_pitch + 4 * (src_col - 1));
    int center_pixel = src_offset + (src_row * src_pitch + 4 * src_col);
    int right_pixel = src_offset + (src_row * src_pitch + 4 * (src_col + 1));
    int dst_pixel = dst_offset + (dst_row * dst_pitch + 4 * dst_col);

    // first pixel in row
    if (dst_col == 0)
    {
        dst_buf[dst_pixel] = (uchar)(
                    weights[dst_col].y * ((float)src_buf[center_pixel]) +
                    weights[dst_col].z * ((float)src_buf[right_pixel]));

        dst_buf[dst_pixel + 1] = (uchar)(
                    weights[dst_col].y * ((float)src_buf[center_pixel + 1]) +
                    weights[dst_col].z * ((float)src_buf[right_pixel + 1]));

        dst_buf[dst_pixel + 2] = (uchar)(
                    weights[dst_col].y * ((float)src_buf[center_pixel + 2]) +
                    weights[dst_col].z * ((float)src_buf[right_pixel + 2]));

        dst_buf[dst_pixel + 3] = (uchar)(
                    weights[dst_col].y * ((float)src_buf[center_pixel + 3]) +
                    weights[dst_col].z * ((float)src_buf[right_pixel + 3]));

        return;
    }

    // last pixel in row
    if (dst_col == dst_width - 1)
    {
        dst_buf[dst_pixel] = (uchar)(
                    weights[dst_col].x * ((float)src_buf[left_pixel]) +
                    weights[dst_col].y * ((float)src_buf[center_pixel]));

        dst_buf[dst_pixel + 1] = (uchar)(
                    weights[dst_col].x * ((float)src_buf[left_pixel + 1]) +
                    weights[dst_col].y * ((float)src_buf[center_pixel + 1]));

        dst_buf[dst_pixel + 2] = (uchar)(
                    weights[dst_col].x * ((float)src_buf[left_pixel + 2]) +
                    weights[dst_col].y * ((float)src_buf[center_pixel + 2]));

        dst_buf[dst_pixel + 3] = (uchar)(
                    weights[dst_col].x * ((float)src_buf[left_pixel + 3]) +
                    weights[dst_col].y * ((float)src_buf[center_pixel + 3]));

        return;
    }

    if (id < dst_width * dst_height)
    {

        dst_buf[dst_pixel] = (uchar)(
                    weights[dst_col].x * ((float)src_buf[left_pixel]) + 
                    weights[dst_col].y * ((float)src_buf[center_pixel]) +
                    weights[dst_col].z * ((float)src_buf[right_pixel]));

        dst_buf[dst_pixel + 1] = (uchar)(
                    weights[dst_col].x * ((float)src_buf[left_pixel + 1]) + 
                    weights[dst_col].y * ((float)src_buf[center_pixel + 1]) +
                    weights[dst_col].z * ((float)src_buf[right_pixel + 1]));

        dst_buf[dst_pixel + 2] = (uchar)(
                    weights[dst_col].x * ((float)src_buf[left_pixel + 2]) + 
                    weights[dst_col].y * ((float)src_buf[center_pixel + 2]) +
                    weights[dst_col].z * ((float)src_buf[right_pixel + 2]));

        dst_buf[dst_pixel + 3] = (uchar)(
                    weights[dst_col].x * ((float)src_buf[left_pixel + 3]) + 
                    weights[dst_col].y * ((float)src_buf[center_pixel + 3]) +
                    weights[dst_col].z * ((float)src_buf[right_pixel + 3]));
    }
}


KERNEL
void ScaleX_4C16U(
    GLOBAL uchar* restrict dst_buf,
    GLOBAL float4 const* restrict weights, int format,
    /* in bytes */ const int dst_offset, const int dst_width, 
    const int dst_height, /* in bytes */ const int dst_pitch,
    GLOBAL uchar const* restrict src_buf,
    /* in bytes */ const int src_offset, const int src_width,
     const int src_height, /* in bytes */ const int src_pitch
    )
{
    int id = get_global_id(0);
    int dst_col = id % dst_width;
    int dst_row = (id - dst_col) / dst_width;
    int src_col = 2 * dst_col;
    int src_row = dst_row;

    // offsets in bytes
    int left_pixel = src_offset + (src_row * src_pitch + 4 * (src_col - 1));
    int center_pixel = src_offset + (src_row * src_pitch + 4 * src_col);
    int right_pixel = src_offset + (src_row * src_pitch + 4 * (src_col + 1));
    int dst_pixel = dst_offset + (dst_row * dst_pitch + 4 * dst_col);

    __global half const* src_buf_16u = (__global half const*)src_buf;
    __global half * dst_buf_16u = (__global half*)dst_buf;

    // first pixel in row
    if (dst_col == 0)
    {
        dst_buf_16u[dst_pixel] = (uchar)(
                    weights[dst_col].y * ((float)src_buf_16u[center_pixel]) +
                    weights[dst_col].z * ((float)src_buf_16u[right_pixel]));

        dst_buf_16u[dst_pixel + 1] = (uchar)(
                    weights[dst_col].y * ((float)src_buf_16u[center_pixel + 1]) +
                    weights[dst_col].z * ((float)src_buf_16u[right_pixel + 1]));

        dst_buf_16u[dst_pixel + 2] = (uchar)(
                    weights[dst_col].y * ((float)src_buf_16u[center_pixel + 2]) +
                    weights[dst_col].z * ((float)src_buf_16u[right_pixel + 2]));

        dst_buf_16u[dst_pixel + 3] = (uchar)(
                    weights[dst_col].y * ((float)src_buf_16u[center_pixel + 3]) +
                    weights[dst_col].z * ((float)src_buf_16u[right_pixel + 3]));

        return;
    }

    // last pixel in row
    if (dst_col == dst_width - 1)
    {
        dst_buf_16u[dst_pixel] = (uchar)(
                    weights[dst_col].x * ((float)src_buf_16u[left_pixel]) +
                    weights[dst_col].y * ((float)src_buf_16u[center_pixel]));

        dst_buf_16u[dst_pixel + 1] = (uchar)(
                    weights[dst_col].x * ((float)src_buf_16u[left_pixel + 1]) +
                    weights[dst_col].y * ((float)src_buf_16u[center_pixel + 1]));

        dst_buf_16u[dst_pixel + 2] = (uchar)(
                    weights[dst_col].x * ((float)src_buf_16u[left_pixel + 2]) +
                    weights[dst_col].y * ((float)src_buf_16u[center_pixel + 2]));

        dst_buf_16u[dst_pixel + 3] = (uchar)(
                    weights[dst_col].x * ((float)src_buf_16u[left_pixel + 3]) +
                    weights[dst_col].y * ((float)src_buf_16u[center_pixel + 3]));

        return;
    }

    if (id < dst_width * dst_height)
    {

        dst_buf_16u[dst_pixel] = (uchar)(
                    weights[dst_col].x * ((float)src_buf_16u[left_pixel]) + 
                    weights[dst_col].y * ((float)src_buf_16u[center_pixel]) +
                    weights[dst_col].z * ((float)src_buf_16u[right_pixel]));

        dst_buf_16u[dst_pixel + 1] = (uchar)(
                    weights[dst_col].x * ((float)src_buf_16u[left_pixel + 1]) + 
                    weights[dst_col].y * ((float)src_buf_16u[center_pixel + 1]) +
                    weights[dst_col].z * ((float)src_buf_16u[right_pixel + 1]));

        dst_buf_16u[dst_pixel + 2] = (uchar)(
                    weights[dst_col].x * ((float)src_buf_16u[left_pixel + 2]) + 
                    weights[dst_col].y * ((float)src_buf_16u[center_pixel + 2]) +
                    weights[dst_col].z * ((float)src_buf_16u[right_pixel + 2]));

        dst_buf_16u[dst_pixel + 3] = (uchar)(
                    weights[dst_col].x * ((float)src_buf_16u[left_pixel + 3]) + 
                    weights[dst_col].y * ((float)src_buf_16u[center_pixel + 3]) +
                    weights[dst_col].z * ((float)src_buf_16u[right_pixel + 3]));
    }
}

KERNEL
void ScaleX_4C32U(
    GLOBAL uchar* restrict dst_buf,
    GLOBAL float4 const* restrict weights, int format,
    /* in bytes */ const int dst_offset, const int dst_width, 
    const int dst_height, /* in bytes */ const int dst_pitch,
    GLOBAL uchar const* restrict src_buf,
    /* in bytes */ const int src_offset, const int src_width,
     const int src_height, /* in bytes */ const int src_pitch
    )
{
    int id = get_global_id(0);
    int dst_col = id % dst_width;
    int dst_row = (id - dst_col) / dst_width;
    int src_col = 2 * dst_col;
    int src_row = dst_row;

    // offsets in bytes
    int left_pixel = src_offset + (src_row * src_pitch + 4 * (src_col - 1));
    int center_pixel = src_offset + (src_row * src_pitch + 4 * src_col);
    int right_pixel = src_offset + (src_row * src_pitch + 4 * (src_col + 1));
    int dst_pixel = dst_offset + (dst_row * dst_pitch + 4 * dst_col);

    __global float3 const* src_buf_32u = (__global float3 const*)src_buf;
    __global float3 * dst_buf_32u = (__global float3*)dst_buf;

    // first pixel in row
    if (dst_col == 0)
    {
        dst_buf_32u[dst_pixel] = (uchar)(
                    weights[dst_col].y * ((float)src_buf_32u[center_pixel]) +
                    weights[dst_col].z * ((float)src_buf_32u[right_pixel]));

        dst_buf_32u[dst_pixel + 1] = (uchar)(
                    weights[dst_col].y * ((float)src_buf_32u[center_pixel + 1]) +
                    weights[dst_col].z * ((float)src_buf_32u[right_pixel + 1]));

        dst_buf_32u[dst_pixel + 2] = (uchar)(
                    weights[dst_col].y * ((float)src_buf_32u[center_pixel + 2]) +
                    weights[dst_col].z * ((float)src_buf_32u[right_pixel + 2]));

        dst_buf_32u[dst_pixel + 3] = (uchar)(
                    weights[dst_col].y * ((float)src_buf_32u[center_pixel + 3]) +
                    weights[dst_col].z * ((float)src_buf_32u[right_pixel + 3]));

        return;
    }

    // last pixel in row
    if (dst_col == dst_width - 1)
    {
        dst_buf_32u[dst_pixel] = (uchar)(
                    weights[dst_col].x * ((float)src_buf_32u[left_pixel]) +
                    weights[dst_col].y * ((float)src_buf_32u[center_pixel]));

        dst_buf_32u[dst_pixel + 1] = (uchar)(
                    weights[dst_col].x * ((float)src_buf_32u[left_pixel + 1]) +
                    weights[dst_col].y * ((float)src_buf_32u[center_pixel + 1]));

        dst_buf_32u[dst_pixel + 2] = (uchar)(
                    weights[dst_col].x * ((float)src_buf_32u[left_pixel + 2]) +
                    weights[dst_col].y * ((float)src_buf_32u[center_pixel + 2]));

        dst_buf_32u[dst_pixel + 3] = (uchar)(
                    weights[dst_col].x * ((float)src_buf_32u[left_pixel + 3]) +
                    weights[dst_col].y * ((float)src_buf_32u[center_pixel + 3]));

        return;
    }

    if (id < dst_width * dst_height)
    {

        dst_buf_32u[dst_pixel] = (uchar)(
                    weights[dst_col].x * ((float)src_buf_32u[left_pixel]) + 
                    weights[dst_col].y * ((float)src_buf_32u[center_pixel]) +
                    weights[dst_col].z * ((float)src_buf_32u[right_pixel]));

        dst_buf_32u[dst_pixel + 1] = (uchar)(
                    weights[dst_col].x * ((float)src_buf_32u[left_pixel + 1]) + 
                    weights[dst_col].y * ((float)src_buf_32u[center_pixel + 1]) +
                    weights[dst_col].z * ((float)src_buf_32u[right_pixel + 1]));

        dst_buf_32u[dst_pixel + 2] = (uchar)(
                    weights[dst_col].x * ((float)src_buf_32u[left_pixel + 2]) + 
                    weights[dst_col].y * ((float)src_buf_32u[center_pixel + 2]) +
                    weights[dst_col].z * ((float)src_buf_32u[right_pixel + 2]));

        dst_buf_32u[dst_pixel + 3] = (uchar)(
                    weights[dst_col].x * ((float)src_buf_32u[left_pixel + 3]) + 
                    weights[dst_col].y * ((float)src_buf_32u[center_pixel + 3]) +
                    weights[dst_col].z * ((float)src_buf_32u[right_pixel + 3]));
    }
}



KERNEL
void ScaleY_4C8U(
    GLOBAL uchar* restrict dst_buf,
    GLOBAL float4 const* restrict weights,
    /* in bytes */ const int dst_offset, const int dst_width,
    const int dst_height, /* in bytes */const int dst_pitch,
    GLOBAL uchar const* restrict src_buf,
     /* in bytes */ const int src_offset, const int src_width,
      const int src_height, /* in bytes */ const int src_pitch
    )
{
    int id = get_global_id(0);
    int dst_col = id % dst_width;
    int dst_row = (id - dst_col) / dst_width;
    int src_col = dst_col;
    int src_row = 2 * dst_row;

    // offsets in bytes
    int top_pixel = src_offset + ((src_row - 1) * src_pitch + 4 * src_col);
    int center_pixel = src_offset + (src_row * src_pitch + 4 * src_col);
    int bottom_pixel = src_offset + ((src_row + 1) * src_pitch + 4 * src_col);
    int dst_pixel = dst_offset + (dst_row * dst_pitch + 4 * dst_col);

    // first row
    if (dst_row == 0)
    {
        dst_buf[dst_pixel] = (uchar)(
                    weights[dst_col].y * (float)(src_buf[center_pixel]) +
                    weights[dst_col].z * (float)(src_buf[bottom_pixel]));

        dst_buf[dst_pixel + 1] = (uchar)(
                    weights[dst_col].y * (float)(src_buf[center_pixel + 1]) +
                    weights[dst_col].z * (float)(src_buf[bottom_pixel + 1]));

        dst_buf[dst_pixel + 2] = (uchar)(
                    weights[dst_col].y * (float)(src_buf[center_pixel + 2]) +
                    weights[dst_col].z * (float)(src_buf[bottom_pixel + 2]));

        dst_buf[dst_pixel + 3] = (uchar)(
                    weights[dst_col].y * (float)(src_buf[center_pixel + 3]) +
                    weights[dst_col].z * (float)(src_buf[bottom_pixel + 3]));
                    
        return;
    }

    // last row
    if (dst_row == dst_height - 1)
    {
        dst_buf[dst_pixel] = (uchar)(
                    weights[dst_col].x * (float)(src_buf[top_pixel]) +
                    weights[dst_col].y * (float)(src_buf[center_pixel]));

        dst_buf[dst_pixel + 1] = (uchar)(
                    weights[dst_col].x * (float)(src_buf[top_pixel + 1]) +
                    weights[dst_col].y * (float)(src_buf[center_pixel + 1]));

        dst_buf[dst_pixel + 2] = (uchar)(
                    weights[dst_col].x * (float)(src_buf[top_pixel + 2]) +
                    weights[dst_col].y * (float)(src_buf[center_pixel + 2]));

        dst_buf[dst_pixel + 3] = (uchar)(
                    weights[dst_col].x * (float)(src_buf[top_pixel + 3]) +
                    weights[dst_col].y * (float)(src_buf[center_pixel + 3]));
                    
        return;
    }

    if (id < dst_width * dst_height)
    {
        dst_buf[dst_pixel] = (uchar)(
                    weights[dst_col].x * (float)(src_buf[top_pixel]) +
                    weights[dst_col].y * (float)(src_buf[center_pixel]) +
                    weights[dst_col].z * (float)(src_buf[bottom_pixel]));

        dst_buf[dst_pixel + 1] = (uchar)(
                    weights[dst_col].x * (float)(src_buf[top_pixel + 1]) +
                    weights[dst_col].y * (float)(src_buf[center_pixel + 1]) +
                    weights[dst_col].z * (float)(src_buf[bottom_pixel + 1]));

        dst_buf[dst_pixel + 2] = (uchar)(
                    weights[dst_col].x * (float)(src_buf[top_pixel + 2]) +
                    weights[dst_col].y * (float)(src_buf[center_pixel + 2]) +
                    weights[dst_col].z * (float)(src_buf[bottom_pixel + 2]));

        dst_buf[dst_pixel + 3] = (uchar)(
                    weights[dst_col].x * (float)(src_buf[top_pixel + 3]) +
                    weights[dst_col].y * (float)(src_buf[center_pixel + 3]) +
                    weights[dst_col].z * (float)(src_buf[bottom_pixel + 3]));
    }
}

KERNEL
void ScaleY_4C16U(
    GLOBAL uchar* restrict dst_buf,
    GLOBAL float4 const* restrict weights,
    /* in bytes */ const int dst_offset, const int dst_width,
    const int dst_height, /* in bytes */const int dst_pitch,
    GLOBAL uchar const* restrict src_buf,
     /* in bytes */ const int src_offset, const int src_width,
      const int src_height, /* in bytes */ const int src_pitch
    )
{
    int id = get_global_id(0);
    int dst_col = id % dst_width;
    int dst_row = (id - dst_col) / dst_width;
    int src_col = dst_col;
    int src_row = 2 * dst_row;

    // offsets in bytes
    int top_pixel = src_offset + ((src_row - 1) * src_pitch + 4 * src_col);
    int center_pixel = src_offset + (src_row * src_pitch + 4 * src_col);
    int bottom_pixel = src_offset + ((src_row + 1) * src_pitch + 4 * src_col);
    int dst_pixel = dst_offset + (dst_row * dst_pitch + 4 * dst_col);

    __global half const* src_buf_16u = (__global half const*)src_buf;
    __global half * dst_buf_16u = (__global half*)dst_buf;

    // first row
    if (dst_row == 0)
    {
        dst_buf_16u[dst_pixel] = (uchar)(
                    weights[dst_col].y * (float)(src_buf_16u[center_pixel]) +
                    weights[dst_col].z * (float)(src_buf_16u[bottom_pixel]));

        dst_buf_16u[dst_pixel + 1] = (uchar)(
                    weights[dst_col].y * (float)(src_buf_16u[center_pixel + 1]) +
                    weights[dst_col].z * (float)(src_buf_16u[bottom_pixel + 1]));

        dst_buf_16u[dst_pixel + 2] = (uchar)(
                    weights[dst_col].y * (float)(src_buf_16u[center_pixel + 2]) +
                    weights[dst_col].z * (float)(src_buf_16u[bottom_pixel + 2]));

        dst_buf_16u[dst_pixel + 3] = (uchar)(
                    weights[dst_col].y * (float)(src_buf_16u[center_pixel + 3]) +
                    weights[dst_col].z * (float)(src_buf_16u[bottom_pixel + 3]));
                    
        return;
    }

    // last row
    if (dst_row == dst_height - 1)
    {
        dst_buf_16u[dst_pixel] = (uchar)(
                    weights[dst_col].x * (float)(src_buf_16u[top_pixel]) +
                    weights[dst_col].y * (float)(src_buf_16u[center_pixel]));

        dst_buf_16u[dst_pixel + 1] = (uchar)(
                    weights[dst_col].x * (float)(src_buf_16u[top_pixel + 1]) +
                    weights[dst_col].y * (float)(src_buf_16u[center_pixel + 1]));

        dst_buf_16u[dst_pixel + 2] = (uchar)(
                    weights[dst_col].x * (float)(src_buf_16u[top_pixel + 2]) +
                    weights[dst_col].y * (float)(src_buf_16u[center_pixel + 2]));

        dst_buf_16u[dst_pixel + 3] = (uchar)(
                    weights[dst_col].x * (float)(src_buf_16u[top_pixel + 3]) +
                    weights[dst_col].y * (float)(src_buf_16u[center_pixel + 3]));
                    
        return;
    }

    if (id < dst_width * dst_height)
    {
        dst_buf_16u[dst_pixel] = (uchar)(
                    weights[dst_col].x * (float)(src_buf_16u[top_pixel]) +
                    weights[dst_col].y * (float)(src_buf_16u[center_pixel]) +
                    weights[dst_col].z * (float)(src_buf_16u[bottom_pixel]));

        dst_buf_16u[dst_pixel + 1] = (uchar)(
                    weights[dst_col].x * (float)(src_buf_16u[top_pixel + 1]) +
                    weights[dst_col].y * (float)(src_buf_16u[center_pixel + 1]) +
                    weights[dst_col].z * (float)(src_buf_16u[bottom_pixel + 1]));

        dst_buf_16u[dst_pixel + 2] = (uchar)(
                    weights[dst_col].x * (float)(src_buf_16u[top_pixel + 2]) +
                    weights[dst_col].y * (float)(src_buf_16u[center_pixel + 2]) +
                    weights[dst_col].z * (float)(src_buf_16u[bottom_pixel + 2]));

        dst_buf_16u[dst_pixel + 3] = (uchar)(
                    weights[dst_col].x * (float)(src_buf_16u[top_pixel + 3]) +
                    weights[dst_col].y * (float)(src_buf_16u[center_pixel + 3]) +
                    weights[dst_col].z * (float)(src_buf_16u[bottom_pixel + 3]));
    }
}

KERNEL
void ScaleY_4C32U(
    GLOBAL uchar* restrict dst_buf,
    GLOBAL float4 const* restrict weights,
    /* in bytes */ const int dst_offset, const int dst_width,
    const int dst_height, /* in bytes */const int dst_pitch,
    GLOBAL uchar const* restrict src_buf,
     /* in bytes */ const int src_offset, const int src_width,
      const int src_height, /* in bytes */ const int src_pitch
    )
{
    int id = get_global_id(0);
    int dst_col = id % dst_width;
    int dst_row = (id - dst_col) / dst_width;
    int src_col = dst_col;
    int src_row = 2 * dst_row;

    // offsets in bytes
    int top_pixel = src_offset + ((src_row - 1) * src_pitch + 4 * src_col);
    int center_pixel = src_offset + (src_row * src_pitch + 4 * src_col);
    int bottom_pixel = src_offset + ((src_row + 1) * src_pitch + 4 * src_col);
    int dst_pixel = dst_offset + (dst_row * dst_pitch + 4 * dst_col);

    __global float3 const* src_buf_32u = (__global float3 const*)src_buf;
    __global float3 * dst_buf_32u = (__global float3*)dst_buf;

    // first row
    if (dst_row == 0)
    {
        dst_buf_32u[dst_pixel] = (uchar)(
                    weights[dst_col].y * (float)(src_buf_32u[center_pixel]) +
                    weights[dst_col].z * (float)(src_buf_32u[bottom_pixel]));

        dst_buf_32u[dst_pixel + 1] = (uchar)(
                    weights[dst_col].y * (float)(src_buf_32u[center_pixel + 1]) +
                    weights[dst_col].z * (float)(src_buf_32u[bottom_pixel + 1]));

        dst_buf_32u[dst_pixel + 2] = (uchar)(
                    weights[dst_col].y * (float)(src_buf_32u[center_pixel + 2]) +
                    weights[dst_col].z * (float)(src_buf_32u[bottom_pixel + 2]));

        dst_buf_32u[dst_pixel + 3] = (uchar)(
                    weights[dst_col].y * (float)(src_buf_32u[center_pixel + 3]) +
                    weights[dst_col].z * (float)(src_buf_32u[bottom_pixel + 3]));
                    
        return;
    }

    // last row
    if (dst_row == dst_height - 1)
    {
        dst_buf_32u[dst_pixel] = (uchar)(
                    weights[dst_col].x * (float)(src_buf_32u[top_pixel]) +
                    weights[dst_col].y * (float)(src_buf_32u[center_pixel]));

        dst_buf_32u[dst_pixel + 1] = (uchar)(
                    weights[dst_col].x * (float)(src_buf_32u[top_pixel + 1]) +
                    weights[dst_col].y * (float)(src_buf_32u[center_pixel + 1]));

        dst_buf_32u[dst_pixel + 2] = (uchar)(
                    weights[dst_col].x * (float)(src_buf_32u[top_pixel + 2]) +
                    weights[dst_col].y * (float)(src_buf_32u[center_pixel + 2]));

        dst_buf_32u[dst_pixel + 3] = (uchar)(
                    weights[dst_col].x * (float)(src_buf_32u[top_pixel + 3]) +
                    weights[dst_col].y * (float)(src_buf_32u[center_pixel + 3]));
                    
        return;
    }

    if (id < dst_width * dst_height)
    {
        dst_buf_32u[dst_pixel] = (uchar)(
                    weights[dst_col].x * (float)(src_buf_32u[top_pixel]) +
                    weights[dst_col].y * (float)(src_buf_32u[center_pixel]) +
                    weights[dst_col].z * (float)(src_buf_32u[bottom_pixel]));

        dst_buf_32u[dst_pixel + 1] = (uchar)(
                    weights[dst_col].x * (float)(src_buf_32u[top_pixel + 1]) +
                    weights[dst_col].y * (float)(src_buf_32u[center_pixel + 1]) +
                    weights[dst_col].z * (float)(src_buf_32u[bottom_pixel + 1]));

        dst_buf_32u[dst_pixel + 2] = (uchar)(
                    weights[dst_col].x * (float)(src_buf_32u[top_pixel + 2]) +
                    weights[dst_col].y * (float)(src_buf_32u[center_pixel + 2]) +
                    weights[dst_col].z * (float)(src_buf_32u[bottom_pixel + 2]));

        dst_buf_32u[dst_pixel + 3] = (uchar)(
                    weights[dst_col].x * (float)(src_buf_32u[top_pixel + 3]) +
                    weights[dst_col].y * (float)(src_buf_32u[center_pixel + 3]) +
                    weights[dst_col].z * (float)(src_buf_32u[bottom_pixel + 3]));
    }
}

#endif // MIPMAP_LEVEL_SCALER_CL
