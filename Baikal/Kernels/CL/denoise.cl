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
#ifndef DENOISE_CL
#define DENOISE_CL

#include <../Baikal/Kernels/CL/common.cl>
#include <../Baikal/Kernels/CL/utils.cl>

// Similarity function
inline float C(float3 x1, float3 x2, float sigma)
{
    float a = length(x1 - x2) / sigma;
    a *= a;
    return native_exp(-0.5f * a);
}

// TODO: Optimize this kernel using local memory
KERNEL
void BilateralDenoise_main(
    // Color data
    GLOBAL float4 const* restrict colors,
    // Normal data
    GLOBAL float4 const* restrict normals,
    // Positional data
    GLOBAL float4 const* restrict positions,
    // Albedo data
    GLOBAL float4 const* restrict albedos,
    // Image resolution
    int width,
    int height,
    // Filter radius
    int radius,
    // Filter kernel width
    float sigma_color,
    float sigma_normal,
    float sigma_position,
    float sigma_albedo,
    // Resulting color
    GLOBAL float4* restrict out_colors
)
{
    int2 global_id;
    global_id.x = get_global_id(0);
    global_id.y = get_global_id(1);

    // Check borders
    if (global_id.x < width && global_id.y < height)
    {
        int idx = global_id.y * width + global_id.x;

        float3 color = colors[idx].w > 1 ? (colors[idx].xyz / colors[idx].w) : colors[idx].xyz;
        float3 normal = normals[idx].w > 1 ? (normals[idx].xyz / normals[idx].w) : normals[idx].xyz;
        float3 position = positions[idx].w > 1 ? (positions[idx].xyz / positions[idx].w) : positions[idx].xyz;
        float3 albedo = albedos[idx].w > 1 ? (albedos[idx].xyz / albedos[idx].w) : albedos[idx].xyz;

        float3 filtered_color = 0.f;
        float sum = 0.f;
        if (length(position) > 0.f)
        {
            for (int i = -radius; i <= radius; ++i)
            {
                for (int j = -radius; j <= radius; ++j)
                {
                    int cx = clamp(global_id.x + i, 0, width - 1);
                    int cy = clamp(global_id.y + j, 0, height - 1);
                    int ci = cy * width + cx;

                    float3 c = colors[ci].xyz / colors[ci].w;
                    float3 n = normals[ci].xyz / normals[ci].w;
                    float3 p = positions[ci].xyz / positions[ci].w;
                    float3 a = albedos[ci].xyz / albedos[ci].w;

                    if (length(p) > 0.f)
                    {
                        filtered_color += c * C(p, position, sigma_position) *
                            C(c, color, sigma_color) *
                            C(n, normal, sigma_normal) *
                            C(a, albedo, sigma_albedo);
                        sum += C(p, position, sigma_position) * C(c, color, sigma_color) *
                            C(n, normal, sigma_normal) *
                            C(a, albedo, sigma_albedo);
                    }
                }
            }

            out_colors[idx].xyz = sum > 0 ? filtered_color / sum : color;
            out_colors[idx].w = 1.f;
        }
        else
        {
            out_colors[idx].xyz = color;
            out_colors[idx].w = 1.f;
        }
    }
}

KERNEL
void ToneMappingExponential(GLOBAL float4* restrict dst,
                        GLOBAL float4 const* restrict src,
                        int elems_num)
{
    int id = get_global_id(0);

    if (id >= elems_num)
    {
        return;
    }

    if (src[id].w != 0.0f)
    {
        dst[id].xyz = 1.f - exp(-1.2f * src[id].xyz / src[id].w);
    }
    else
    {
        dst[id] = make_float4(0.0f, 0.0f, 0.0f, 1.0f);
    }
}


// perform division on w component
KERNEL
void DivideBySampleCount(GLOBAL float4* restrict dst,
                         GLOBAL float4 const* restrict src,
                         int elems_num)
{
    int id = get_global_id(0);

    if (id >= elems_num)
    {
        return;
    }

    if (src[id].w != 0.0f)
    {
        dst[id].xyz = src[id].xyz / src[id].w;
        dst[id].w = src[id].w;
    }
    else
    {
        dst[id] = make_float4(0.0f, 0.0f, 0.0f, 1.0f);
    }

}

KERNEL
void CopyInterleaved(GLOBAL float4* restrict dst,
                     GLOBAL float4 const* restrict src,
                     int dst_width,
                     int dst_height,
                     int dst_channels_offset, // offset inside pixel in channels (not bytes)
                     int dst_channels_num,
                     int src_width,
                     int src_height,
                     int src_channels_offset, // offset inside pixel in channels (not bytes)
                     int src_channels_num,
                     int channels_to_copy)
{
    int global_id = get_global_id(0);

    int x = global_id % dst_width;
    int y = global_id / dst_width;

    if ((x > dst_width) || (global_id > dst_width * dst_height))
    {
        return;
    }

    int src_offset = src_channels_num * (y * src_width + x) + src_channels_offset;
    int dst_offset = dst_channels_num * (y * dst_width + x) + dst_channels_offset;

    GLOBAL float* dst_pixel = (GLOBAL float*)dst + dst_offset;
    GLOBAL float const* src_pixel = (GLOBAL float const*)src + src_offset;

    for (int i = 0; i < channels_to_copy; i++)
    {
        dst_pixel[i] = src_pixel[i];
    }
}

static float3 BicubicConvolutionCompute(float4 f0, float4 f1, float4 f2, float4 f3)
{
    const float t = 0.5f;
    float4 a0 = f1;
    float4 a1 = (f0 - f2) / 2.f;
    float4 a2 = -f0 - 3.5f * f1 + 4 * f2 + .5f * f3;
    float4 a3 = .5f * f0 + 2.5f * f1 - 2.5f * f2 - .5f * f3;

    return (a3 * t * t * t + a2 * t * t + a1 * t + a0).xyz;
}

KERNEL
void BicubicUpscale2x_X(// size of the dst buffer should be enough
                        // to store 2 * sizeof(float3) * width * height
                        GLOBAL float4* restrict dst,
                        GLOBAL float4 const* restrict src,
                        int width,
                        int height)
{
    int idx = get_global_id(0);
    int src_idx = idx / 2;

    dst[idx].w = src[0].w;

    if (idx % 2 == 0 || (idx + 1) % (2 * width) == 0)
    {
        dst[idx].xyz = src[idx / 2].xyz;
        return;
    }

    dst[idx].xyz = BicubicConvolutionCompute(src[src_idx - 1],
                                      src[src_idx],
                                      src[src_idx + 1],
                                      src[src_idx + 2]);


}


KERNEL
void BicubicUpscale2x_Y(// size of the dst buffer should be enough to store
                        // 2 * sizeof(float3) * width * height
                        GLOBAL float4* restrict dst,
                        GLOBAL float4 const* restrict src,
                        int width,
                        int height)
{
    int idx = get_global_id(0);

    int x_coord = idx % width; // same for dst and src buffers
    int dst_y = (idx - x_coord) / width;
    int src_y = dst_y / 2;
    int src_idx = src_y * width + x_coord;

    dst[idx].w = src[0].w;

    if (dst_y % 2 == 0 || dst_y == 1 || dst_y > height - 2)
    {
        dst[idx].xyz = src[src_idx].xyz;
        return;
    }

    dst[idx].xyz = BicubicConvolutionCompute(src[src_idx - width],
                                      src[src_idx],
                                      src[src_idx + width],
                                      src[src_idx + 2 * width]);
}

#endif
