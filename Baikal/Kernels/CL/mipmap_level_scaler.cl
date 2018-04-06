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

    float denominator = 2.f * size - 1;

    // first weight
    if (id == 0)
    {
        weights[id].x = .0f;
        weights[id].y = size / denominator;
        weights[id].z = (size - 1) / denominator;
        return;
    }

    // last weight
    if (id == size - 1)
    {
        weights[id].x = (size - 1) / denominator;
        weights[id].y = size / denominator;
        weights[id].z = 0;
        return;
    }

    if (id < size - 1)
    {
        weights[id].x = (size - id - 1) / denominator;
        weights[id].y = size / denominator;
        weights[id].z = id / denominator;
    }
}

KERNEL
void ScaleX(
    GLOBAL uchar* restrict dst_buf,
    GLOBAL float3 const* restrict weights,
    const int dst_width, const int dst_height,
    GLOBAL uchar* restrict src_buf,
    const int src_width, const int src_height
    )
{
    int id = get_global_id(0);
    int dst_col = id % dst_width;
    int dst_row = (id - dst_col) / dst_width;
    int src_col = 2 * dst_col;
    int src_row = dst_row;

    // first pixel in row
    if (dst_col == 0)
    {
        dst_buf[id] = 
                    weights[dst_col].y * src_buf[src_row * src_width + src_col] +
                    weights[dst_col].z * src_buf[src_row * src_width + src_col + 1];
        return;
    }

    // last pixel in row
    if (dst_col == dst_width - 1)
    {
        dst_buf[id] = 
                    weights[dst_col].x * src_buf[src_row * src_width + src_col - 1] +
                    weights[dst_col].y * src_buf[src_row * src_width + src_col];
        return;
    }

    if (id < dst_width * dst_height)
    {
        dst_buf[id] = 
                    weights[dst_col].x * src_buf[src_row * src_width + src_col - 1] + 
                    weights[dst_col].y * src_buf[src_row * src_width + src_col] + 
                    weights[dst_col].z * src_buf[src_row * src_width + src_col + 1];
    }
}


KERNEL
void ScaleY(
    GLOBAL uchar* restrict dst_buf,
    GLOBAL float3 const* restrict weights,
    const int dst_width, const int dst_height,
    GLOBAL uchar* restrict src_buf,
    const int src_width, const int src_height
    )
{
    int id = get_global_id(0);
    int dst_col = id % dst_width;
    int dst_row = (id - dst_col) / dst_width;
    int src_col = 2 * dst_col;
    int src_row = dst_row;

    // first row
    if (dst_row == 0)
    {
        dst_buf[id] = 
                    weights[dst_row].y * src_buf[src_row * src_width + src_col] +
                    weights[dst_row].z * src_buf[(src_row + 1) * src_width + src_col];
        return;
    }

    // last row
    if (dst_col == dst_width - 1)
    {
        dst_buf[id] = 
                    weights[dst_row].x * src_buf[(src_row - 1)* src_width + src_col] +
                    weights[dst_row].y * src_buf[src_row * src_width + src_col];
        return;
    }

    if (id < dst_width * dst_height)
    {
        dst_buf[id] = 
                    weights[dst_row].x * src_buf[(src_row - 1)* src_width + src_col] + 
                    weights[dst_row].y * src_buf[src_row * src_width + src_col] + 
                    weights[dst_row].z * src_buf[(src_row + 1) * src_width + src_col];
    }
}

#endif // MIPMAP_LEVEL_SCALER_CL
