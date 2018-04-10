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
    GLOBAL float* restrict weights,
    // size of weight vector
    const int size)
{
    int id = get_global_id(0);

    if (id < size)
    {
        weights[3 * id] = .0f;
        weights[3 * id + 1] = .5f;
        weights[3 * id + 2] = .5f;
    }
}

KERNEL
void ComputeWeights_RoundingUp(
    GLOBAL float* restrict weights,
    // size of weight vector
    const int size)
{
    int id = get_global_id(0);

    float denominator = 2.f * size - 1.f;

    // first weight
    if (id == 0)
    {
        weights[3 * id] = .0f;
        weights[3 * id + 1] = ((float)size) / denominator;
        weights[3 * id + 2] = ((float)size - 1.f) / denominator;
        return;
    }

    // last weight
    if (id == size - 1)
    {
        weights[3 * id] = ((float)size - 1.f) / denominator;
        weights[3 * id + 1] = ((float)size) / denominator;
        weights[3 * id + 2] = .0f;
        return;
    }

    if (id < size - 1)
    {
        weights[3 * id] = ((float)size - (float)id - 1.f) / denominator;
        weights[3 * id + 1] = ((float)size) / denominator;
        weights[3 * id + 2] = ((float)id) / denominator;
    }
}

KERNEL
void ScaleX_1C(
    GLOBAL uchar* restrict dst_buf,
    GLOBAL float const* restrict weights,
    const int dst_width, const int dst_height,
    GLOBAL uchar const* restrict src_buf,
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
        dst_buf[id] = (uchar)(
                    weights[3 * dst_col + 1] * ((float)src_buf[src_row * src_width + src_col]) +
                    weights[3 * dst_col + 2] * ((float)src_buf[src_row * src_width + src_col + 1]));
        return;
    }

    // last pixel in row
    if (dst_col == dst_width - 1)
    {
        dst_buf[id] = (uchar)(
                    weights[3 * dst_col] * ((float)src_buf[src_row * src_width + src_col - 1]) +
                    weights[3 * dst_col + 1] * ((float)src_buf[src_row * src_width + src_col]));
        return;
    }

    if (id < dst_width * dst_height)
    {
        dst_buf[id] = (uchar)(
                    weights[3 * dst_col] * ((float)src_buf[src_row * src_width + src_col - 1]) + 
                    weights[3 * dst_col + 1] * ((float)src_buf[src_row * src_width + src_col]) + 
                    weights[3 * dst_col + 2] * ((float)src_buf[src_row * src_width + src_col + 1]));
    }
}


KERNEL
void ScaleY_1C(
    GLOBAL uchar* restrict dst_buf,
    GLOBAL float const* restrict weights,
    const int dst_width, const int dst_height,
    GLOBAL uchar* restrict src_buf,
    const int src_width, const int src_height
    )
{
    int id = get_global_id(0);
    int dst_col = id % dst_width;
    int dst_row = (int)((id - dst_col) / dst_width);
    int src_col = dst_col;
    int src_row = 2 * dst_row;

    // first row
    if (dst_row == 0)
    {
        dst_buf[id] = (uchar)(
                    weights[3 * dst_row + 1] * (float)(src_buf[src_row * src_width + src_col]) +
                    weights[3 * dst_row + 2] * (float)(src_buf[(src_row + 1) * src_width + src_col]));
        return;
    }

    // last row
    if (dst_row == dst_height - 1)
    {
        dst_buf[id] = (uchar)(
                    weights[3 * dst_row] * (float)(src_buf[(src_row - 1)* src_width + src_col]) +
                    weights[3 * dst_row + 1] * (float)(src_buf[src_row * src_width + src_col]));
        return;
    }

    if (id < dst_width * dst_height)
    {
        dst_buf[id] = (uchar)(
                    weights[3 * dst_row] * (float)(src_buf[(src_row - 1) * src_width + src_col]) + 
                    weights[3 * dst_row + 1] * (float)(src_buf[src_row * src_width + src_col]) + 
                    weights[3 * dst_row + 2] * (float)(src_buf[(src_row + 1) * src_width + src_col]));
    }
}


KERNEL
void ScaleX_4C(
    GLOBAL uchar* restrict dst_buf,
    GLOBAL float const* restrict weights,
    const int dst_width, const int dst_height, /* in bytes */const int dst_pitch,
    GLOBAL uchar const* restrict src_buf,
    const int src_width, const int src_height, /* in bytes */ const int src_pitch
    )
{
    int id = get_global_id(0);
    int dst_col = id % dst_width;
    int dst_row = (id - dst_col) / dst_width;
    int src_col = 2 * dst_col;
    int src_row = dst_row;

    // offsets in bytes
    int left_pixel = src_row * src_pitch + 4 * (src_col - 1);
    int center_pixel = src_row * src_pitch + 4 * src_col;
    int right_pixel = src_row * src_pitch + 4 * (src_col + 1);
    int dst_pixel = dst_row * dst_pitch + 4 * dst_col;

    // first pixel in row
    if (dst_col == 0)
    {
        dst_buf[dst_pixel] = (uchar)(
                    weights[3 * dst_col + 1] * ((float)src_buf[center_pixel]) +
                    weights[3 * dst_col + 2] * ((float)src_buf[right_pixel]));

        dst_buf[dst_pixel + 1] = (uchar)(
                    weights[3 * dst_col + 1] * ((float)src_buf[center_pixel + 1]) +
                    weights[3 * dst_col + 2] * ((float)src_buf[right_pixel + 1]));

        dst_buf[dst_pixel + 2] = (uchar)(
                    weights[3 * dst_col + 1] * ((float)src_buf[center_pixel + 2]) +
                    weights[3 * dst_col + 2] * ((float)src_buf[right_pixel + 2]));

        dst_buf[dst_pixel + 3] = (uchar)(
                    weights[3 * dst_col + 1] * ((float)src_buf[center_pixel + 3]) +
                    weights[3 * dst_col + 2] * ((float)src_buf[right_pixel + 3]));

        return;
    }

    // last pixel in row
    if (dst_col == dst_width - 1)
    {
        dst_buf[dst_pixel] = (uchar)(
                    weights[3 * dst_col] * ((float)src_buf[left_pixel]) +
                    weights[3 * dst_col + 1] * ((float)src_buf[center_pixel]));

        dst_buf[dst_pixel + 1] = (uchar)(
                    weights[3 * dst_col] * ((float)src_buf[left_pixel + 1]) +
                    weights[3 * dst_col + 1] * ((float)src_buf[center_pixel + 1]));

        dst_buf[dst_pixel + 2] = (uchar)(
                    weights[3 * dst_col] * ((float)src_buf[left_pixel + 2]) +
                    weights[3 * dst_col + 1] * ((float)src_buf[center_pixel + 2]));

        dst_buf[dst_pixel + 3] = (uchar)(
                    weights[3 * dst_col] * ((float)src_buf[left_pixel + 3]) +
                    weights[3 * dst_col + 1] * ((float)src_buf[center_pixel + 3]));

        return;
    }

    if (id < dst_width * dst_height)
    {

        dst_buf[dst_pixel] = (uchar)(
                    weights[3 * dst_col] * ((float)src_buf[left_pixel]) + 
                    weights[3 * dst_col + 1] * ((float)src_buf[center_pixel]) +
                    weights[3 * dst_col + 2] * ((float)src_buf[right_pixel]));

        dst_buf[dst_pixel + 1] = (uchar)(
                    weights[3 * dst_col] * ((float)src_buf[left_pixel + 1]) + 
                    weights[3 * dst_col + 1] * ((float)src_buf[center_pixel + 1]) +
                    weights[3 * dst_col + 2] * ((float)src_buf[right_pixel + 1]));

        dst_buf[dst_pixel + 2] = (uchar)(
                    weights[3 * dst_col] * ((float)src_buf[left_pixel + 2]) + 
                    weights[3 * dst_col + 1] * ((float)src_buf[center_pixel + 2]) +
                    weights[3 * dst_col + 2] * ((float)src_buf[right_pixel + 2]));

        dst_buf[dst_pixel + 3] = (uchar)(
                    weights[3 * dst_col] * ((float)src_buf[left_pixel + 3]) + 
                    weights[3 * dst_col + 1] * ((float)src_buf[center_pixel + 3]) +
                    weights[3 * dst_col + 2] * ((float)src_buf[right_pixel + 3]));
    }
}


KERNEL
void ScaleY_4C(
    GLOBAL uchar* restrict dst_buf,
    GLOBAL float const* restrict weights,
    const int dst_width, const int dst_height, /* in bytes */const int dst_pitch,
    GLOBAL uchar const* restrict src_buf,
    const int src_width, const int src_height, /* in bytes */ const int src_pitch
    )
{
    int id = get_global_id(0);
    int dst_col = id % dst_width;
    int dst_row = (id - dst_col) / dst_width;
    int src_col = dst_col;
    int src_row = 2 * dst_row;

    // offsets in bytes
    int top_pixel = (src_row - 1) * src_pitch + 4 * src_col;
    int center_pixel = src_row * src_pitch + 4 * src_col;
    int bottom_pixel = (src_row + 1) * src_pitch + 4 * src_col;
    int dst_pixel = dst_row * dst_pitch + 4 * dst_col;

    // first row
    if (dst_row == 0)
    {
        dst_buf[dst_pixel] = (uchar)(
                    weights[3 * dst_row + 1] * (float)(src_buf[center_pixel]) +
                    weights[3 * dst_row + 2] * (float)(src_buf[bottom_pixel]));

        dst_buf[dst_pixel + 1] = (uchar)(
                    weights[3 * dst_row + 1] * (float)(src_buf[center_pixel + 1]) +
                    weights[3 * dst_row + 2] * (float)(src_buf[bottom_pixel + 1]));

        dst_buf[dst_pixel + 2] = (uchar)(
                    weights[3 * dst_row + 1] * (float)(src_buf[center_pixel + 2]) +
                    weights[3 * dst_row + 2] * (float)(src_buf[bottom_pixel + 2]));

        dst_buf[dst_pixel + 3] = (uchar)(
                    weights[3 * dst_row + 1] * (float)(src_buf[center_pixel + 3]) +
                    weights[3 * dst_row + 2] * (float)(src_buf[bottom_pixel + 3]));
                    
        return;
    }

    // last row
    if (dst_row == dst_height - 1)
    {
        dst_buf[dst_pixel] = (uchar)(
                    weights[3 * dst_row] * (float)(src_buf[top_pixel]) +
                    weights[3 * dst_row + 1] * (float)(src_buf[center_pixel]));

        dst_buf[dst_pixel + 1] = (uchar)(
                    weights[3 * dst_row] * (float)(src_buf[top_pixel + 1]) +
                    weights[3 * dst_row + 1] * (float)(src_buf[center_pixel + 1]));

        dst_buf[dst_pixel + 2] = (uchar)(
                    weights[3 * dst_row] * (float)(src_buf[top_pixel + 2]) +
                    weights[3 * dst_row + 1] * (float)(src_buf[center_pixel + 2]));

        dst_buf[dst_pixel + 3] = (uchar)(
                    weights[3 * dst_row] * (float)(src_buf[top_pixel + 3]) +
                    weights[3 * dst_row + 1] * (float)(src_buf[center_pixel + 3]));
                    
        return;
    }

    if (id < dst_width * dst_height)
    {
        dst_buf[dst_pixel] = (uchar)(
                    weights[3 * dst_row] * (float)(src_buf[top_pixel]) +
                    weights[3 * dst_row + 1] * (float)(src_buf[center_pixel]) +
                    weights[3 * dst_row + 2] * (float)(src_buf[bottom_pixel]));

        dst_buf[dst_pixel + 1] = (uchar)(
                    weights[3 * dst_row] * (float)(src_buf[top_pixel + 1]) +
                    weights[3 * dst_row + 1] * (float)(src_buf[center_pixel + 1]) +
                    weights[3 * dst_row + 2] * (float)(src_buf[bottom_pixel + 1]));

        dst_buf[dst_pixel + 2] = (uchar)(
                    weights[3 * dst_row] * (float)(src_buf[top_pixel + 2]) +
                    weights[3 * dst_row + 1] * (float)(src_buf[center_pixel + 2]) +
                    weights[3 * dst_row + 2] * (float)(src_buf[bottom_pixel + 2]));

        dst_buf[dst_pixel + 3] = (uchar)(
                    weights[3 * dst_row] * (float)(src_buf[top_pixel + 3]) +
                    weights[3 * dst_row + 1] * (float)(src_buf[center_pixel + 3]) +
                    weights[3 * dst_row + 2] * (float)(src_buf[bottom_pixel + 3]));
    }
}
#endif // MIPMAP_LEVEL_SCALER_CL
