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
#ifndef WAVELETDENOISE_CL
#define WAVELETDENOISE_CL

#include <../Baikal/Kernels/CL/common.cl>
#include <../Baikal/Kernels/CL/utils.cl>

#define WAVELET_KERNEL_SIZE 25

KERNEL
void WaveletFilter_main(
    // Color data
    GLOBAL float4 const* restrict colors,
    // Normal data
    GLOBAL float4 const* restrict normals,
    // Positional data
    GLOBAL float4 const* restrict positions,
    // Image resolution
    int width,
    int height,
    // Filter width
    int step_width,
    // Filter kernel parameters
    float sigma_color,
    float sigma_normal,
    float sigma_position,
    // Resulting color
    GLOBAL float4* restrict out_colors
)
{
    int2 global_id;
    global_id.x = get_global_id(0);
    global_id.y = get_global_id(1);

    const float kernel_weights[WAVELET_KERNEL_SIZE] = {
        1.0 / 256.0, 1.0 / 64.0, 3.0 / 128.0, 1.0 / 64.0, 1.0 / 256.0,
        1.0 / 64.0, 1.0 / 16.0, 3.0 / 32.0, 1.0 / 16.0, 1.0 / 64.0,
        3.0 / 128.0, 3.0 / 32.0, 9.0 / 64.0, 3.0 / 32.0, 3.0 / 128.0,
        1.0 / 64.0, 1.0 / 16.0, 3.0 / 32.0, 1.0 / 16.0, 1.0 / 64.0,
        1.0 / 256.0, 1.0 / 64.0, 3.0 / 128.0, 1.0 / 64.0, 1.0 / 256.0 };

    const int2 kernel_offsets[WAVELET_KERNEL_SIZE] = {
        make_int2(-2, -2), make_int2(-1, -2), make_int2(0, -2),  make_int2(1, -2),  make_int2(2, -2),
        make_int2(-2, -1), make_int2(-1, -1), make_int2(0, -2),  make_int2(1, -1),  make_int2(2, -1),
        make_int2(-2, 0),  make_int2(-1, 0),  make_int2(0, 0),   make_int2(1, 0),   make_int2(2, 0),
        make_int2(-2, 1),  make_int2(-1, 1),  make_int2(0, 1),   make_int2(1, 1),   make_int2(2, 1),
        make_int2(-2, 2),  make_int2(-1, 2),  make_int2(0, 2),   make_int2(1, 2),   make_int2(2, 2) };

    // Check borders
    if (global_id.x < width && global_id.y < height)
    {
        const int idx = global_id.y * width + global_id.x;

        const float3 color = colors[idx].xyz / max(colors[idx].w, 1.f);
        const float3 position = positions[idx].xyz / max(positions[idx].w, 1.f);
        const float3 normal = normals[idx].xyz / max(normals[idx].w, 1.f);

        const float step_width_2 = (float)(step_width * step_width);

        float3 color_sum = make_float3(0.f, 0.f, 0.f);
        float weight_sum = 0.f;

        if (length(position) > 0.f)
        {
            for (int i = 0; i < WAVELET_KERNEL_SIZE; i++)
            {
                const int cx = clamp(global_id.x + step_width * kernel_offsets[i].x, 0, width - 1);
                const int cy = clamp(global_id.y + step_width * kernel_offsets[i].y, 0, height - 1);
                const int ci = cy * width + cx;

                const float3 sample_color = colors[ci].xyz / max(colors[ci].w, 1.f);
                const float3 sample_normal = normals[ci].xyz / max(normals[ci].w, 1.f);
                const float3 sample_position = positions[ci].xyz / max(positions[ci].w, 1.f);

                const float3 delta_color = color - sample_color;
                const float3 delta_normal = normal - sample_normal;
                const float3 delta_position = position - sample_position;

                const float color_dist2 = dot(delta_color, delta_color);
                const float normal_dist2 = max(dot(delta_normal, delta_normal) / step_width_2, 0.f);
                const float position_dist2 = dot(delta_position, delta_position);
                
                // First wavelet pass do not take into account color weights
                const float color_weight = (step_width == 1) ? 1.f : min(exp(-(color_dist2) / sigma_color), 1.f);
                const float normal_weight = min(exp(-(normal_dist2) / sigma_normal), 1.f);
                const float position_weight = min(exp(-(position_dist2) / sigma_position), 1.f);

                const float final_weight = color_weight * normal_weight * position_weight * kernel_weights[i];

                color_sum += final_weight * sample_color;
                weight_sum += final_weight;
            }

            out_colors[idx].xyz = color_sum / weight_sum;
            out_colors[idx].w = 1.f;
        }
        else
        {
            out_colors[idx].xyz = color;
            out_colors[idx].w = 1.f;
        }
    }
}

#endif
