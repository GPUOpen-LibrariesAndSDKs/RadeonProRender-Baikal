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

#pragma OPENCL EXTENSION cl_khr_fp16 : enable

// Downscale image in x dimension by 2, with rounding up
KERNEL
void ScaleX(
    // destination image
    GLOBAL uchar* dst_img,
    // source image
    GLOBAL uchar* src_img,
    // weights for convolution with image rows
    float3* weights,
    // src image width
    int width,
    // src image height
    int height,
    // src image step
    int step
)
{
    int id = get_global_id(0);     
    int x = id % step;

    if (x > width)
    {
        return;
    }

    // power of two case
    if (width % 2 == 0)
    {
        dst_img[id] =  weights[x].y * src_img[2 * id] + weights[x].z * src_img[2 * id + 1];
        return;
    }

    // rounding up case
    // if there is first pixel in row
    if (x == 0)
    {
        dst_img[id] = weights[0].y * src_img[2 * id] + weights[0].z * src_img[2 * id + 1];
        return;
    }

    // if there is the last pixel in row
    if (x == width - 1)
    {
        dst_img[id] = weights[width - 1].x * src_img[2 * id - 1] + weights[width - 1].y * src_img[2 * id];
        return;
    }

    // if there is the middle pixel in row
    dst_img[id] = 
        weights[x].x * src_img[2 * id - 1] +
        weights[x].y * src_img[2 * id] +
        weights[x].z * src_img[2 * id + 1];
}

// Downscale image in y dimension by 2, with rounding up
KERNEL
void ScaleY(
    // destination image
    GLOBAL uchar* dst_img,
    // source image
    GLOBAL uchar* src_img,
    // weights for convolution with image rows
    float3* weights,
    // src image width
    int width,
    // src image height
    int height,
    // src image step
    int step
)
{
    int id = get_global_id(0);
    int x = id % step;
    int y = (int)floor(id / step);

    if (x > width)
    {
        return;
    }

    // if there is the first row
    if (y == 0)
    {
        dst_img[x] = weights[0].w1 * src_img[x] + weights[0].w2 * (src_img + step)[x]);
        return;
    }

    // if there is the last row
    if (y == height - 1)
    {
        dst_img[id] = 
            weights[height - 1].x * src_img[(height - 2) * step + x] +
            weights[height - 1].y * src_img[(height - 1) * step + x];
        return;
    }

    // process middle rows
    dst_img[id] =
        weights[y].x * src_img[(y - 1) * step + x] +
        weights[y].y * src_img[y * step + x] +
        weights[y].x * src_img[(y + 1) * step + x]
}

#endif // MIPMAP_LEVEL_SCALER_CL
