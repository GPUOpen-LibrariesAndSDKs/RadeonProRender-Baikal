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
#define GAUSS_KERNEL_SIZE 9
#define DENOM_EPS 1e-8f
#define FRAME_BLEND_ALPHA 0.2f

float4 GaussFilter3x3(
    GLOBAL float4 const* restrict buffer, 
    int2 buffer_size,
    int2 uv)
{
    const int2 kernel_offsets[GAUSS_KERNEL_SIZE] = {
        make_int2(-1, -1),  make_int2(0, -1),   make_int2(1, -1),
        make_int2(-1, 0),   make_int2(0, 0),    make_int2(1, 0),
        make_int2(-1, 1),   make_int2(0, 1),    make_int2(1, 1),
    };

    const float kernel_weights[GAUSS_KERNEL_SIZE] = {
        1.0 / 16.0, 1.0 / 8.0, 1.0 / 16.0,
        1.0 / 8.0, 1.0 / 4.0, 1.0 / 8.0,
        1.0 / 16.0, 1.0 / 8.0, 1.0 / 16.0
    };

    float4 sample_out = make_float4(0.f, 0.f, 0.f, 0.f);
    
    for (int i = 0; i < GAUSS_KERNEL_SIZE; i++)
    {
        const int cx = clamp(uv.x + kernel_offsets[i].x, 0, buffer_size.x - 1);
        const int cy = clamp(uv.y + kernel_offsets[i].y, 0, buffer_size.y - 1);
        const int ci = cy * buffer_size.x + cx;

        sample_out += kernel_weights[i] * buffer[ci];
    }

    return sample_out;
}

KERNEL
void WaveletFilter_main(
    // Color data
    GLOBAL float4 const* restrict colors,
    // Normal data
    GLOBAL float4 const* restrict normals,
    // Positional data
    GLOBAL float4 const* restrict positions,
    // Variance
    GLOBAL float4 const* restrict variances,
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
    
    const int2 buffer_size  = make_int2(width, height);

    // From SVGF paper
    const float sigma_variance = 4.0f;
    sigma_normal = 128.f;
    
    // Check borders
    if (global_id.x < width && global_id.y < height)
    {
        const int idx = global_id.y * width + global_id.x;

        const float3 color = colors[idx].xyz / max(colors[idx].w, 1.f);
        const float3 position = positions[idx].xyz / max(positions[idx].w, 1.f);
        const float3 normal = normals[idx].xyz / max(normals[idx].w, 1.f);
        
        const float variance = sqrt(GaussFilter3x3(variances, buffer_size, global_id).z);
        const float step_width_2 = (float)(step_width * step_width);

        float3 color_sum = make_float3(0.f, 0.f, 0.f);
        float weight_sum = 0.f;

        const float3 luminance = make_float3(0.2126f, 0.7152f, 0.0722f);
        const float lum_color = dot(color, luminance);

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

                const float3 delta_position = position - sample_position;

                const float position_dist2 = dot(delta_position, delta_position);

                const float normal_weight = pow(max(0.f, dot(sample_normal, normal)), sigma_normal);
                const float position_weight = min(exp(-position_dist2 / sigma_position), 1.f);
                
                const float d = exp(-fabs(lum_color - dot(luminance, sample_color)) / (sigma_variance * variance + DENOM_EPS));

                float luminance_weight = isnan(d) ? 1.f : d;

                const float final_weight = luminance_weight * normal_weight * position_weight * kernel_weights[i];

                color_sum += final_weight * sample_color;
                weight_sum += final_weight;
            }

            out_colors[idx].xyz = color_sum / max(weight_sum, DENOM_EPS);
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
void WaveletGenerateMotionBuffer_main(
    GLOBAL float4 const* restrict positions,
    // Image resolution
    int width,
    int height,
    // View-projection matrix of current frame
    GLOBAL matrix4x4* restrict view_projection,
    // View-projection matrix of previous frame
    GLOBAL matrix4x4* restrict prev_view_projection,
    // Resulting motion buffer
    GLOBAL float4* restrict out_motion
)
{
    int2 global_id;
    global_id.x = get_global_id(0);
    global_id.y = get_global_id(1);
    
    // Check borders
    if (global_id.x < width && global_id.y < height)
    {
        const int idx = global_id.y * width + global_id.x;
        
        const float3 position_xyz = positions[idx].xyz / max(positions[idx].w, 1.f);

        if (length(position_xyz) > 0)
        {
            const float4 position = make_float4(position_xyz.x, position_xyz.y, position_xyz.z, 1.0f);

            float4 position_ps = matrix_mul_vector4(*view_projection, position);
            float3 position_cs = position_ps.xyz / position_ps.w;
            float2 position_ss = position_cs.xy * make_float2(0.5f, -0.5f) + make_float2(0.5f, 0.5f);

            float4 prev_position_ps = matrix_mul_vector4(*prev_view_projection, position);
            float2 prev_position_cs = prev_position_ps.xy / prev_position_ps.w;
            float2 prev_position_ss = prev_position_cs * make_float2(0.5f, -0.5f) + make_float2(0.5f, 0.5f);

            out_motion[idx] = (float4)(prev_position_ss - position_ss, 0.0f, 1.0f);
        }
        else
        {
            out_motion[idx] = make_float4(0.f, 0.f, 0.f, 1.f);
        }
    }
}

KERNEL
void CopyBuffers_main(
    GLOBAL float4 const* restrict colors,
    GLOBAL float4 const* restrict positions,
    GLOBAL float4 const* restrict normals,
    // Image resolution
    int width,
    int height,
    // Resulting color
    GLOBAL float4* restrict out_colors,
    GLOBAL float4* restrict out_positions,
    GLOBAL float4* restrict out_normals
)
{
    int2 global_id;
    global_id.x = get_global_id(0);
    global_id.y = get_global_id(1);

    // Check borders
    if (global_id.x < width && global_id.y < height)
    {
        const int idx = global_id.y * width + global_id.x;

        out_colors[idx] = colors[idx];
        out_positions[idx] = positions[idx];
        out_normals[idx] = normals[idx];
    }
}

KERNEL
void CopyBuffer_main(
    GLOBAL float4 const* restrict in_buffer,
    GLOBAL float4* restrict out_buffer,
    // Image resolution
    int width,
    int height
)
{
    int2 global_id;
    global_id.x = get_global_id(0);
    global_id.y = get_global_id(1);

    // Check borders
    if (global_id.x < width && global_id.y < height)
    {
        const int idx = global_id.y * width + global_id.x;
        out_buffer[idx] = in_buffer[idx];
    }
}

int ConvertToLinearAddress(int address_x, int address_y, int2 buffer_size)
{
    int max_buffer_size = buffer_size.x * buffer_size.y;
    return clamp(address_y * buffer_size.x + address_x, 0, max_buffer_size);
}

int ConvertToLinearAddressInt2(int2 address, int2 buffer_size)
{
    int max_buffer_size = buffer_size.x * buffer_size.y;
    return clamp(address.y * buffer_size.x + address.x, 0, max_buffer_size);
}

float4 Sampler2DBilinear(GLOBAL float4 const* restrict buffer, int2 buffer_size, float2 uv)
{
    uv = uv * make_float2(buffer_size.x, buffer_size.y) - make_float2(0.5f, 0.5f);

    int x = floor(uv.x);
    int y = floor(uv.y);

    float2 uv_ratio = uv - make_float2(x, y);
    float2 uv_inv   = make_float2(1.f, 1.f) - uv_ratio;

    int x1 = clamp(x + 1, 0, buffer_size.x - 1);
    int y1 = clamp(y + 1, 0, buffer_size.y - 1);

    float4 r =  (buffer[ConvertToLinearAddress(x, y, buffer_size)]   * uv_inv.x + buffer[ConvertToLinearAddress(x1, y, buffer_size)]   * uv_ratio.x) * uv_inv.y + 
                (buffer[ConvertToLinearAddress(x, y1, buffer_size)]  * uv_inv.x + buffer[ConvertToLinearAddress(x1, y1, buffer_size)]  * uv_ratio.x) * uv_ratio.y;

    return r;
}


float4 dFdx(GLOBAL float4 const* restrict buffer, float2 uv, int2 buffer_size)
{
    const float2 uv_x = uv + make_float2(1.0f / (float)buffer_size.x, 0.0);

    return Sampler2DBilinear(buffer, buffer_size, uv_x) - Sampler2DBilinear(buffer, buffer_size, uv);
}

float4 dFdy(GLOBAL float4 const* restrict buffer, float2 uv, int2 buffer_size)
{
    const float2 uv_y = uv + make_float2(0.0, 1.0f / (float)buffer_size.y);

    return Sampler2DBilinear(buffer, buffer_size, uv_y) - Sampler2DBilinear(buffer, buffer_size, uv);
}

bool IsNormalConsistent(float3 nq, float3 np)
{
    const float cos_pi_div_4  = cos(PI / 4.f);
    return dot(nq, np) > cos_pi_div_4;
}

bool IsDepthConsistent(
    GLOBAL float4 const* restrict buffer, 
    GLOBAL float4 const* restrict prev_buffer, 
    float2 uv, 
    float2 motion, 
    int2 buffer_size)
{
    float2 uv_prev = uv + motion;

    float dz_dx = fabs(dFdx(buffer, uv, buffer_size).z);
    float dz_dy = fabs(dFdy(buffer, uv, buffer_size).z);

    float z0 = Sampler2DBilinear(buffer, buffer_size, uv).z;
    float z0_min = z0 - dz_dx - dz_dy;
    float z0_max = z0 + dz_dx + dz_dy;

    float dz1_dx = fabs(dFdx(prev_buffer, uv_prev, buffer_size).z);
    float dz1_dy = fabs(dFdy(prev_buffer, uv_prev, buffer_size).z);

    float z1 = Sampler2DBilinear(prev_buffer, buffer_size, uv_prev).z;
    float z1_min = z1 - dz1_dx - dz1_dy;
    float z1_max = z1 + dz1_dx + dz1_dy;

    return z0_min <= z1_max && z1_min <= z0_max;
}

// Bilinear filterinng with geometry test on each tap
float4 SampleWithGeometryTest(GLOBAL float4 const* restrict buffer,
    float4 current_color,
    float3 current_positions,
    float3 current_normal,
    GLOBAL float4 const* restrict positions,
    GLOBAL float4 const* restrict normals,
    GLOBAL float4 const* restrict prev_positions,
    GLOBAL float4 const* restrict prev_normals,
    int2 buffer_size,
    float2 uv,
    float2 uv_prev)
{
    float2 scaled_uv = uv_prev * make_float2(buffer_size.x, buffer_size.y) - make_float2(0.5f, 0.5f);

    int x = floor(scaled_uv.x);
    int y = floor(scaled_uv.y);

    const int2 offsets[4] = {
        make_int2(x + 0, y + 0),
        make_int2(x + 1, y + 0),
        make_int2(x + 1, y + 1),
        make_int2(x + 0, y + 1)
    };

    const float4 buffer_samples[4] = {
        buffer[ConvertToLinearAddressInt2(offsets[0], buffer_size)],
        buffer[ConvertToLinearAddressInt2(offsets[1], buffer_size)],
        buffer[ConvertToLinearAddressInt2(offsets[2], buffer_size)],
        buffer[ConvertToLinearAddressInt2(offsets[3], buffer_size)]
    };

    const float3 normal_samples[4] = {
        prev_normals[ConvertToLinearAddressInt2(offsets[0], buffer_size)].xyz,
        prev_normals[ConvertToLinearAddressInt2(offsets[1], buffer_size)].xyz,
        prev_normals[ConvertToLinearAddressInt2(offsets[2], buffer_size)].xyz,
        prev_normals[ConvertToLinearAddressInt2(offsets[3], buffer_size)].xyz
    };

    const float3 position_samples[4] = {
        prev_positions[ConvertToLinearAddressInt2(offsets[0], buffer_size)].xyz,
        prev_positions[ConvertToLinearAddressInt2(offsets[1], buffer_size)].xyz,
        prev_positions[ConvertToLinearAddressInt2(offsets[2], buffer_size)].xyz,
        prev_positions[ConvertToLinearAddressInt2(offsets[3], buffer_size)].xyz
    };

    const bool is_normal_consistent[4] = {
        IsNormalConsistent(current_normal, normal_samples[0]),
        IsNormalConsistent(current_normal, normal_samples[1]),
        IsNormalConsistent(current_normal, normal_samples[2]),
        IsNormalConsistent(current_normal, normal_samples[3])
    };

    const float2 motion = uv_prev - uv;

    const float2 uv_offsets[4] = {
        motion,
        motion + make_float2(1.f / buffer_size.x, 0.f),
        motion + make_float2(1.f / buffer_size.x, 1.f / buffer_size.y),
        motion + make_float2(0.f, 1.f / buffer_size.y)
    };

    const bool is_depth_consistent[4] = {
        IsDepthConsistent(positions, prev_positions, uv, uv_offsets[0], buffer_size),
        IsDepthConsistent(positions, prev_positions, uv, uv_offsets[1], buffer_size),
        IsDepthConsistent(positions, prev_positions, uv, uv_offsets[2], buffer_size),
        IsDepthConsistent(positions, prev_positions, uv, uv_offsets[3], buffer_size)
    };

    int num_consistent_samples = 0;
    
    for (int i = 0; i < 4; i++) num_consistent_samples += is_normal_consistent[i] && is_depth_consistent[i] ? 1 : 0;

    // Bilinear resample if all samples are consistent
    if (num_consistent_samples == 4)
    {
        return Sampler2DBilinear(buffer, buffer_size, uv_prev);
    }

    // Box filter otherwise
    float weight = 1;
    float4 sample = current_color;

    for (int i = 0; i < 4; i++)
    {
        if (is_normal_consistent[i] && is_depth_consistent[i])
        {
            sample += buffer_samples[i];
            weight += 1.f;
        }
    }

    return sample / weight;
}

// Similarity function
inline float C(float3 x1, float3 x2, float sigma)
{
    float a = length(x1 - x2) / sigma;
    a *= a;
    return native_exp(-0.5f * a);
}

float4 BilateralVariance7x7(
    GLOBAL float4 const* restrict colors,
    GLOBAL float4 const* restrict positions,
    GLOBAL float4 const* restrict normals,
    int2 global_id,
    int2 buffer_size
)
{
    const float3 luminance_weight = make_float3(0.2126f, 0.7152f, 0.0722f);
    
    float local_mean = 0.f;
    float local_mean_2 = 0.f;
    float sum_weight = 0.0f;
    
    const int idx = global_id.y * buffer_size.x + global_id.x;

    float3 normal = normals[idx].w > 1 ? (normals[idx].xyz / normals[idx].w) : normals[idx].xyz;
    float3 position = positions[idx].w > 1 ? (positions[idx].xyz / positions[idx].w) : positions[idx].xyz;

    for (int i = -3; i <= 3; ++i)
    {
        for (int j = -3; j <= 3; ++j)
        {
            int cx = clamp(global_id.x + i, 0, buffer_size.x - 1);
            int cy = clamp(global_id.y + j, 0, buffer_size.y - 1);
            int ci = cy * buffer_size.x + cx;

            float3 c = colors[ci].xyz / colors[ci].w;
            float3 n = normals[ci].xyz / normals[ci].w;
            float3 p = positions[ci].xyz / positions[ci].w;
            
            float sigma_position = 0.1f;
            float sigma_normal = 0.1f;

            if (length(p) > 0.f && !any(isnan(c)))
            {
                const float weight = C(p, position, sigma_position) * C(n, normal, sigma_normal);
                const float luminance = dot(c, luminance_weight);

                local_mean      += luminance * weight;
                local_mean_2    += luminance * luminance * weight;
                sum_weight      += weight;
            }
        }
    }
    
    local_mean      = local_mean / max(DENOM_EPS, sum_weight);
    local_mean_2    = local_mean_2 / max(DENOM_EPS, sum_weight);;

    const float variance = local_mean_2 - local_mean * local_mean;
    
    return make_float4( local_mean, local_mean_2, variance, 1.f);
}

KERNEL
void TemporalAccumulation_main(
    GLOBAL float4 const* restrict prev_colors,
    GLOBAL float4 const* restrict prev_positions,
    GLOBAL float4 const* restrict prev_normals,
    GLOBAL float4*  restrict in_out_colors,
    GLOBAL float4 const*  restrict positions,
    GLOBAL float4 const*  restrict normals,
    GLOBAL float4 const* restrict motions,
    GLOBAL float4 const* restrict prev_moments_and_variance,
    GLOBAL float4* restrict moments_and_variance,
    // Image resolution
    int width,
    int height
)
{
    int2 global_id;
    global_id.x = get_global_id(0);
    global_id.y = get_global_id(1);

    // Check borders
    if (global_id.x < width && global_id.y < height)
    {
        const int idx = global_id.y * width + global_id.x;
        
        moments_and_variance[idx] = make_float4(0.f, 0.f, 0.f, 0.f);

        const float3 position_xyz = positions[idx].xyz / max(positions[idx].w, 1.f);
        const float3 normal = normals[idx].xyz / max(normals[idx].w, 1.f);
        const float3 color = in_out_colors[idx].xyz / max(in_out_colors[idx].w, 1.f);

        if (length(position_xyz) > 0 && !any(isnan(in_out_colors[idx])))
        {
            const float2 motion     = motions[idx].xy;
            const int2 buffer_size  = make_int2(width, height);
      
            const float2 uv = make_float2(global_id.x + 0.5f, global_id.y + 0.5f) / make_float2(width, height);
            const float2 prev_uv = uv + motion;

            const float3 prev_position_xyz  = Sampler2DBilinear(prev_positions, buffer_size, prev_uv).xyz;
            const float3 prev_normal        = Sampler2DBilinear(prev_normals, buffer_size, prev_uv).xyz;

            // Test for geometry consistency
            if (length(prev_position_xyz) > 0 && IsDepthConsistent(positions, prev_positions, uv, motion, buffer_size) && IsNormalConsistent(prev_normal, normal))
            {              
                // Temporal accumulation of moments
                const int prev_idx = global_id.y * width + global_id.x;

                float4 prev_moments_and_variance_sample  = Sampler2DBilinear(prev_moments_and_variance, buffer_size, prev_uv);

                const bool prev_moments_is_nan = any(isnan(prev_moments_and_variance_sample));

                float4 current_moments_and_variance_sample;

                if (prev_moments_and_variance_sample.w < 4 || prev_moments_is_nan)
                {
                    // Not enought accumulated samples - get bilateral estimate
                    current_moments_and_variance_sample = BilateralVariance7x7(in_out_colors, positions, normals, global_id, buffer_size);
                    prev_moments_and_variance_sample = prev_moments_is_nan ? current_moments_and_variance_sample : prev_moments_and_variance_sample;
                }
                else
                {
                    // Otherwise calculate moments for current color 
                    const float3 luminance_weight = make_float3(0.2126f, 0.7152f, 0.0722f);
                    
                    const float first_moment = dot(in_out_colors[idx].xyz, luminance_weight);
                    const float second_moment = first_moment * first_moment;
                    
                    current_moments_and_variance_sample = make_float4(first_moment, second_moment, 0.f, 1.f);
                }
                
                // Nan avoidance
                if (any(isnan(prev_moments_and_variance_sample)))
                {
                    prev_moments_and_variance_sample = make_float4(0,0,0,1);
                }

                // Accumulate current and previous moments
                moments_and_variance[idx].xy    = mix(prev_moments_and_variance_sample.xy, current_moments_and_variance_sample.xy, FRAME_BLEND_ALPHA);
                moments_and_variance[idx].w     = 1;

                const float mean          = moments_and_variance[idx].x;
                const float mean_2        = moments_and_variance[idx].y;
                
                moments_and_variance[idx].z     = mean_2 - mean * mean;

                // Temporal accumulation of color
                float3 prev_color = SampleWithGeometryTest(prev_colors, (float4)(color, 1.f), position_xyz, normal, positions, normals, prev_positions, prev_normals, buffer_size, uv, prev_uv).xyz;
                in_out_colors[idx].xyz = mix(prev_color, color, FRAME_BLEND_ALPHA);
            }
            else
            {
                // In case of disoclussion - calclulate variance by bilateral filter
                moments_and_variance[idx] = BilateralVariance7x7(in_out_colors, positions, normals, global_id, buffer_size);
            }
        }
    }
}

KERNEL
void UpdateVariance_main(
    // Color data
    GLOBAL float4 const* restrict colors,
    GLOBAL float4 const* restrict positions,
    GLOBAL float4 const* restrict normals,
    // Image resolution
    int width,
    int height,
    GLOBAL float4* restrict out_variance
)
{
    int2 global_id;
    global_id.x = get_global_id(0);
    global_id.y = get_global_id(1);
    
    const int2 buffer_size  = make_int2(width, height);

    // Check borders
    if (global_id.x < width && global_id.y < height)
    {
        const int idx = global_id.y * width + global_id.x;
        out_variance[idx] = BilateralVariance7x7(colors, positions, normals, global_id, buffer_size);
    } 
}
#endif