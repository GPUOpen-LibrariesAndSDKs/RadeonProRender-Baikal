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
#ifndef TEXTURE_CL
#define TEXTURE_CL


#include <../Baikal/Kernels/CL/payload.cl>
#include <../Baikal/Kernels/CL/utils.cl>
#include <../Baikal/Kernels/CL/ray.cl>


/// To simplify a bit
#define TEXTURE_ARG_LIST __global Texture const* textures, __global char const* texturedata, __global MipmapPyramid const* mipmap
#define TEXTURE_ARG_LIST_IDX(x) int x, __global Texture const* textures, __global char const* texturedata, __global MipmapPyramid const* mipmap
#define TEXTURE_ARGS textures, texturedata, mipmap
#define TEXTURE_ARGS_IDX(x) x, textures, texturedata, mipmap

inline float4 ReadValue(__global char const* data, int fmt, float2 const uv, int width, int height)
{
    // Calculate integer coordinates
    int x0 = clamp((int)floor(uv.x * width), 0, width - 1);
    int y0 = clamp((int)floor(uv.y * height), 0, height - 1);

    // Calculate samples for linear filtering
    int x1 = clamp(x0 + 1, 0,  width - 1);
    int y1 = clamp(y0 + 1, 0, height - 1);

    // Calculate weights for linear filtering
    float wx = uv.x * width - floor(uv.x * width);
    float wy = uv.y * height - floor(uv.y * height);

    switch (fmt)
    {
        case RGBA32:
        {
            __global float4 const* data_f = (__global float4 const*)data;
            
            // Get 4 values for linear filtering
            float4 val00 = *(data_f + width * y0 + x0);
            float4 val01 = *(data_f + width * y0 + x1);
            float4 val10 = *(data_f + width * y1 + x0);
            float4 val11 = *(data_f + width * y1 + x1);

            // Filter and return the result
            return lerp(lerp(val00, val01, wx), lerp(val10, val11, wx), wy);
        }

        case RGBA16:
        {
            __global half const* data_h = (__global half const*)data;

            // Get 4 values
            float4 val00 = vload_half4(width * y0 + x0, data_h);
            float4 val01 = vload_half4(width * y0 + x1, data_h);
            float4 val10 = vload_half4(width * y1 + x0, data_h);
            float4 val11 = vload_half4(width * y1 + x1, data_h);

            // Filter and return the result
            return lerp(lerp(val00, val01, wx), lerp(val10, val11, wx), wy);
        }

        case RGBA8:
        {
            __global uchar4 const* data_c = (__global uchar4 const*)data;

            // Get 4 values and convert to float
            uchar4 valu00 = *(data_c + width * y0 + x0);
            uchar4 valu01 = *(data_c + width * y0 + x1);
            uchar4 valu10 = *(data_c + width * y1 + x0);
            uchar4 valu11 = *(data_c + width * y1 + x1);

            float4 val00 = make_float4((float)valu00.x / 255.f, (float)valu00.y / 255.f, (float)valu00.z / 255.f, (float)valu00.w / 255.f);
            float4 val01 = make_float4((float)valu01.x / 255.f, (float)valu01.y / 255.f, (float)valu01.z / 255.f, (float)valu01.w / 255.f);
            float4 val10 = make_float4((float)valu10.x / 255.f, (float)valu10.y / 255.f, (float)valu10.z / 255.f, (float)valu10.w / 255.f);
            float4 val11 = make_float4((float)valu11.x / 255.f, (float)valu11.y / 255.f, (float)valu11.z / 255.f, (float)valu11.w / 255.f);

            // Filter and return the result
            return lerp(lerp(val00, val01, wx), lerp(val10, val11, wx), wy);
        }

        default:
        {
            return make_float4(0.f, 0.f, 0.f, 0.f);
        }
    }
}

/// Sample 2D texture
inline
float4 Texture_UVSample2D(float2 uv, TEXTURE_ARG_LIST_IDX(texidx))
{
        // Get width and height
    int width = textures[texidx].w;
    int height = textures[texidx].h;

    // Find the origin of the data in the pool
    __global char const* mydata = texturedata + textures[texidx].dataoffset;

    // Handle UV wrap
    // TODO: need UV mode support
    uv -= floor(uv);

    // Reverse Y:
    // it is needed as textures are loaded with Y axis going top to down
    // and our axis goes from down to top
    uv.y = 1.f - uv.y;

    return ReadValue(mydata, textures[texidx].fmt, uv, width, height);
}

/// Sample 2D texture
inline
float4 Texture_Sample2D(DifferentialGeometry const *diffgeo, TEXTURE_ARG_LIST_IDX(texidx))
{
    // Get width and height
    int width = textures[texidx].w;
    int height = textures[texidx].h;

    // Find the origin of the data in the pool
    __global char const* mydata = texturedata + textures[texidx].dataoffset;

    // Handle UV wrap
    // TODO: need UV mode support
    float2 uv = diffgeo->uv - floor(diffgeo->uv);

    // Reverse Y:
    // it is needed as textures are loaded with Y axis going top to down
    // and our axis goes from down to top
    uv.y = 1.f - uv.y;

    if (textures[texidx].mipmap_enabled && (diffgeo != NULL))
    {
        float delta_u_length = sqrt(diffgeo->dudx * diffgeo->dudx + diffgeo->dudy * diffgeo->dudy);
        float delta_v_length = sqrt(diffgeo->dvdx * diffgeo->dvdx + diffgeo->dvdy * diffgeo->dvdy);
        float max_delta = max(delta_u_length, delta_v_length);
        MipmapPyramid mip_pyramid = mipmap[textures[texidx].mipmap_index];
        float level = mip_pyramid.level_num - 1 + log2(max(max_delta, 1e-8f));

        if (level >= MAX_LEVEL_NUM)
        {
            switch (textures[texidx].fmt)
            {
                int level_offset = mip_pyramid.level_info[MAX_LEVEL_NUM - 1].offset;
                case RGBA32:
                {
                    return *((__global float4 const*)(texturedata + level_offset));
                }
                case RGBA16:
                {
                    return vload_half4(0, (__global half const*)(texturedata + level_offset));
                }
                case RGBA8:
                {
                    uchar4 val = *((__global uchar4 const*)(texturedata + level_offset));
                    return make_float4(
                        (float)val.x / 255.f,
                        (float)val.y / 255.f,
                        (float)val.z / 255.f,
                        (float)val.w / 255.f);
                }
                default:
                {
                    return make_float4(0.f, 0.f, 0.f, 0.f);
                }
            }
        }

        int int_level = floor(level);
        float alpha = level - int_level;

        if (level > 0)
        {
            // compute coords in level
            float f_x = uv.x * (float)mip_pyramid.level_info[int_level].width;
            float f_y = uv.y * (float)mip_pyramid.level_info[int_level].height;
            // compute integer coords
            int x0 = clamp((int)floor(f_x), 0, width - 1);
            int y0 = clamp((int)floor(f_y), 0, width - 1);
            int x1 = clamp(x0 + 1, 0, width - 1);
            int y1 = clamp(y0 + 1, 0, width - 1);
            // compute delta for filtering
            float dx = f_x - x0;
            float dy = f_y - y0;

            // level offsets
            int bottom_level_offset = mip_pyramid.level_info[int_level].offset;
            int top_level_offset = mip_pyramid.level_info[int_level + 1].offset;

            switch (textures[texidx].fmt)
            {
                case RGBA32:
                {
                    // get data
                    __global float4* bottom_level_data =
                        (__global float4* )(texturedata + bottom_level_offset);
                    __global float4* top_level_data =
                        (__global float4* )(texturedata + top_level_offset);

                    // triangle filter for bottom level
                    float4 bottom_val0 = bottom_level_data[y0 * mip_pyramid.level_info[int_level].pitch + x0];
                    float4 bottom_val1 = bottom_level_data[y1 * mip_pyramid.level_info[int_level].pitch + x0];
                    float4 bottom_val2 = bottom_level_data[y0 * mip_pyramid.level_info[int_level].pitch + x1];
                    float4 bottom_val3 = bottom_level_data[y1 * mip_pyramid.level_info[int_level].pitch + x1];
                    
                    float4 bottom_val = make_float4(
                        (1 - dx) * (1 - dy) * (bottom_val0.x + bottom_val1.x + bottom_val2.x + bottom_val3.x),
                        (1 - dx) * dy * (bottom_val0.y + bottom_val1.y + bottom_val2.y + bottom_val3.y),
                        dx * (1 - dy) * (bottom_val0.z + bottom_val1.z + bottom_val2.z + bottom_val3.z),
                        dx * dy * (bottom_val0.w + bottom_val1.w + bottom_val2.w + bottom_val3.w));

                    float4 top_val0 = top_level_data[y0 * mip_pyramid.level_info[int_level + 1].pitch + x0];
                    float4 top_val1 = top_level_data[y1 * mip_pyramid.level_info[int_level + 1].pitch + x0];
                    float4 top_val2 = top_level_data[y0 * mip_pyramid.level_info[int_level + 1].pitch + x1];
                    float4 top_val3 = top_level_data[y1 * mip_pyramid.level_info[int_level + 1].pitch + x1];
                    
                    float4 top_val =  make_float4(
                        (1 - dx) * (1 - dy) * (top_val0.x + top_val1.x + top_val2.x + top_val3.x),
                        (1 - dx) * dy * (top_val0.y + top_val1.y + top_val2.y + top_val3.y),
                        dx * (1 - dy) * (top_val0.z + top_val1.z + top_val2.z + top_val3.z),
                        dx * dy * (top_val0.w + top_val1.w + top_val2.w + top_val3.w));

                    return lerp(bottom_val, top_val, alpha);
                }
                case RGBA16:
                {
                    // get data
                    __global half4* bottom_level_data =
                        (__global half4* )(texturedata + bottom_level_offset);
                    __global half4* top_level_data =
                        (__global half4* )(texturedata + top_level_offset);

                    // triangle filter for bottom level
                    float4 bottom_val0 = vload_half4(y0 * mip_pyramid.level_info[int_level].pitch + x0, (__global half*)bottom_level_data);
                    float4 bottom_val1 = vload_half4(y1 * mip_pyramid.level_info[int_level].pitch + x0, (__global half*)bottom_level_data);
                    float4 bottom_val2 = vload_half4(y0 * mip_pyramid.level_info[int_level].pitch + x1, (__global half*)bottom_level_data);
                    float4 bottom_val3 = vload_half4(y1 * mip_pyramid.level_info[int_level].pitch + x1, (__global half*)bottom_level_data);
                    
                    float4 bottom_val = make_float4(
                        (1 - dx) * (1 - dy) * (bottom_val0.x + bottom_val1.x + bottom_val2.x + bottom_val3.x),
                        (1 - dx) * dy * (bottom_val0.y + bottom_val1.y + bottom_val2.y + bottom_val3.y),
                        dx * (1 - dy) * (bottom_val0.z + bottom_val1.z + bottom_val2.z + bottom_val3.z),
                        dx * dy * (bottom_val0.w + bottom_val1.w + bottom_val2.w + bottom_val3.w));

                   float4 top_val0 = vload_half4(y0 * mip_pyramid.level_info[int_level + 1].pitch + x0, (__global half*)bottom_level_data);
                   float4 top_val1 = vload_half4(y1 * mip_pyramid.level_info[int_level + 1].pitch + x0, (__global half*)bottom_level_data);
                   float4 top_val2 = vload_half4(y0 * mip_pyramid.level_info[int_level + 1].pitch + x1, (__global half*)bottom_level_data);
                   float4 top_val3 = vload_half4(y1 * mip_pyramid.level_info[int_level + 1].pitch + x1, (__global half*)bottom_level_data);
                    
                    float4 top_val =  make_float4(
                        (1 - dx) * (1 - dy) * (top_val0.x + top_val1.x + top_val2.x + top_val3.x),
                        (1 - dx) * dy * (top_val0.y + top_val1.y + top_val2.y + top_val3.y),
                        dx * (1 - dy) * (top_val0.z + top_val1.z + top_val2.z + top_val3.z),
                        dx * dy * (top_val0.w + top_val1.w + top_val2.w + top_val3.w));

                    return lerp(bottom_val, top_val, alpha);
                }
                case RGBA8:
                {
                    // get data
                    __global uchar4* bottom_level_data =
                        (__global uchar4* )(texturedata + bottom_level_offset);
                    __global uchar4* top_level_data =
                        (__global uchar4* )(texturedata + top_level_offset);

                    // triangle filter for bottom level
                    uchar4 bottom_val0 = bottom_level_data[y0 * mip_pyramid.level_info[int_level].pitch + x0];
                    uchar4 bottom_val1 = bottom_level_data[y1 * mip_pyramid.level_info[int_level].pitch + x0];
                    uchar4 bottom_val2 = bottom_level_data[y0 * mip_pyramid.level_info[int_level].pitch + x1];
                    uchar4 bottom_val3 = bottom_level_data[y1 * mip_pyramid.level_info[int_level].pitch + x1];
                    
                    float4 bottom_val = make_float4(
                        (1 - dx) * (1 - dy) * ((float)bottom_val0.x + (float)bottom_val1.x + (float)bottom_val2.x + (float)bottom_val3.x) / 255.f,
                        (1 - dx) * dy * ((float)bottom_val0.y + (float)bottom_val1.y + (float)bottom_val2.y +(float)bottom_val3.y) / 255.f,
                        dx * (1 - dy) * ((float)bottom_val0.z + (float)bottom_val1.z +(float)bottom_val2.z + (float)bottom_val3.z) / 255.f,
                        dx * dy * ((float)bottom_val0.w + (float)bottom_val1.w + (float)bottom_val2.w + (float)bottom_val3.w ) / 255.f);

                    uchar4 top_val0 = top_level_data[y0 * mip_pyramid.level_info[int_level + 1].pitch + x0];
                    uchar4 top_val1 = top_level_data[y1 * mip_pyramid.level_info[int_level + 1].pitch + x0];
                    uchar4 top_val2 = top_level_data[y0 * mip_pyramid.level_info[int_level + 1].pitch + x1];
                    uchar4 top_val3 = top_level_data[y1 * mip_pyramid.level_info[int_level + 1].pitch + x1];
                    
                    float4 top_val =  make_float4(
                        (1 - dx) * (1 - dy) * ((float)top_val0.x + (float)top_val1.x + (float)top_val2.x + (float)top_val3.x) / 255.f,
                        (1 - dx) * dy * ((float)top_val0.y + (float)top_val1.y + (float)top_val2.y + (float)top_val3.y) / 255.f,
                        dx * (1 - dy) * ((float)top_val0.z + (float)top_val1.z + (float)top_val2.z + (float)top_val3.z) / 255.f,
                        dx * dy * ((float)top_val0.w + (float)top_val1.w + (float)top_val2.w + (float)top_val3.w) / 255.f);

                    return lerp(bottom_val, top_val, alpha);
                }
                default:
                {
                    return make_float4(0.f, 0.f, 0.f, 0.f);
                }
            }
        }
        return ReadValue(mydata, textures[texidx].fmt, uv, width, height);
    }
}

/// Sample lattitue-longitude environment map using 3d vector
inline
float3 Texture_SampleEnvMap(float3 d, TEXTURE_ARG_LIST_IDX(texidx), bool mirror_x)
{
    // Transform to spherical coords
    float r, phi, theta;
    CartesianToSpherical(d, &r, &phi, &theta);

    // Map to [0,1]x[0,1] range and reverse Y axis
    float2 uv;
    uv.x = (mirror_x) ? (1.f - phi / (2 * PI)) : phi / (2 * PI);
    uv.y = 1.f - theta / PI;

    // Sample the texture
    return Texture_UVSample2D(uv, TEXTURE_ARGS_IDX(texidx)).xyz;
}

inline
float3 Texture_UV_GetValue3f(
                // Value
                float3 v,
                // uv coordinates
                float2 uv,
                // Texture args
                TEXTURE_ARG_LIST_IDX(texidx)
                )
{
    // If texture present sample from texture
    if (texidx != -1)
    {
        // Sample texture
        return native_powr(Texture_UVSample2D(uv, TEXTURE_ARGS_IDX(texidx)).xyz, 2.2f);
    }

    // Return fixed color otherwise
    return v;
}

/// Get data from parameter value or texture
inline
float3 Texture_GetValue3f(
                // Value
                float3 v,
                // Differential geometry
                DifferentialGeometry const *diffgeo,
                // Texture args
                TEXTURE_ARG_LIST_IDX(texidx)
                )
{
    // If texture present sample from texture
    if (texidx != -1)
    {
        // Sample texture
        return native_powr(Texture_Sample2D(diffgeo, TEXTURE_ARGS_IDX(texidx)).xyz, 2.2f);
    }

    // Return fixed color otherwise
    return v;
}

/// Get data from parameter value or texture
inline
float4 Texture_GetValue4f(
                // Value
                float4 v,
                // Differential geometry
                DifferentialGeometry const *diffgeo,
                // Texture args
                TEXTURE_ARG_LIST_IDX(texidx)
                )
{
    // If texture present sample from texture
    if (texidx != -1)
    {
        // Sample texture
        return native_powr(Texture_Sample2D(diffgeo, TEXTURE_ARGS_IDX(texidx)), 2.2f);
    }

    // Return fixed color otherwise
    return v;
}

/// Get data from parameter value or texture
inline
float Texture_GetValue1f(
                        // Value
                        float v,
                        // Differential geometry
                        DifferentialGeometry const *diffgeo,
                        // Texture args
                        TEXTURE_ARG_LIST_IDX(texidx)
                        )
{
    // If texture present sample from texture
    if (texidx != -1)
    {
        // Sample texture
        return Texture_Sample2D(diffgeo, TEXTURE_ARGS_IDX(texidx)).x;
    }

    // Return fixed color otherwise
    return v;
}

inline float3 TextureData_SampleNormalFromBump_uchar4(__global uchar4 const* mydatac, int width, int height, int t0, int s0)
{
	int t0minus = clamp(t0 - 1, 0, height - 1);
	int t0plus = clamp(t0 + 1, 0, height - 1);
	int s0minus = clamp(s0 - 1, 0, width - 1);
	int s0plus = clamp(s0 + 1, 0, width - 1);

	const uchar utex00 = (*(mydatac + width * t0minus + s0minus)).x;
	const uchar utex10 = (*(mydatac + width * t0minus + (s0))).x;
	const uchar utex20 = (*(mydatac + width * t0minus + s0plus)).x;

	const uchar utex01 = (*(mydatac + width * (t0)+s0minus)).x;
	const uchar utex21 = (*(mydatac + width * (t0)+(s0 + 1))).x;

	const uchar utex02 = (*(mydatac + width * t0plus + s0minus)).x;
	const uchar utex12 = (*(mydatac + width * t0plus + (s0))).x;
	const uchar utex22 = (*(mydatac + width * t0plus + s0plus)).x;

	const float tex00 = (float)utex00 / 255.f;
	const float tex10 = (float)utex10 / 255.f;
	const float tex20 = (float)utex20 / 255.f;

	const float tex01 = (float)utex01 / 255.f;
	const float tex21 = (float)utex21 / 255.f;

	const float tex02 = (float)utex02 / 255.f;
	const float tex12 = (float)utex12 / 255.f;
	const float tex22 = (float)utex22 / 255.f;

	const float Gx = tex00 - tex20 + 2.0f * tex01 - 2.0f * tex21 + tex02 - tex22;
	const float Gy = tex00 + 2.0f * tex10 + tex20 - tex02 - 2.0f * tex12 - tex22;
	const float3 n = make_float3(Gx, Gy, 1.f);

	return n;
}

inline float3 TextureData_SampleNormalFromBump_half4(__global half const* mydatah, int width, int height, int t0, int s0)
{
	int t0minus = clamp(t0 - 1, 0, height - 1);
	int t0plus = clamp(t0 + 1, 0, height - 1);
	int s0minus = clamp(s0 - 1, 0, width - 1);
	int s0plus = clamp(s0 + 1, 0, width - 1);

	const float tex00 = vload_half4(width * t0minus + s0minus, mydatah).x;
	const float tex10 = vload_half4(width * t0minus + (s0), mydatah).x;
	const float tex20 = vload_half4(width * t0minus + s0plus, mydatah).x;

	const float tex01 = vload_half4(width * (t0)+s0minus, mydatah).x;
	const float tex21 = vload_half4(width * (t0)+s0plus, mydatah).x;

	const float tex02 = vload_half4(width * t0plus + s0minus, mydatah).x;
	const float tex12 = vload_half4(width * t0plus + (s0), mydatah).x;
	const float tex22 = vload_half4(width * t0plus + s0plus, mydatah).x;

	const float Gx = tex00 - tex20 + 2.0f * tex01 - 2.0f * tex21 + tex02 - tex22;
	const float Gy = tex00 + 2.0f * tex10 + tex20 - tex02 - 2.0f * tex12 - tex22;
	const float3 n = make_float3(Gx, Gy, 1.f);

	return n;
}

inline float3 TextureData_SampleNormalFromBump_float4(__global float4 const* mydataf, int width, int height, int t0, int s0)
{
	int t0minus = clamp(t0 - 1, 0, height - 1);
	int t0plus = clamp(t0 + 1, 0, height - 1);
	int s0minus = clamp(s0 - 1, 0, width - 1);
	int s0plus = clamp(s0 + 1, 0, width - 1);

	const float tex00 = (*(mydataf + width * t0minus + s0minus)).x;
	const float tex10 = (*(mydataf + width * t0minus + (s0))).x;
	const float tex20 = (*(mydataf + width * t0minus + s0plus)).x;

	const float tex01 = (*(mydataf + width * (t0)+s0minus)).x;
	const float tex21 = (*(mydataf + width * (t0)+s0plus)).x;

	const float tex02 = (*(mydataf + width * t0plus + s0minus)).x;
	const float tex12 = (*(mydataf + width * t0plus + (s0))).x;
	const float tex22 = (*(mydataf + width * t0plus + s0plus)).x;

	const float Gx = tex00 - tex20 + 2.0f * tex01 - 2.0f * tex21 + tex02 - tex22;
	const float Gy = tex00 + 2.0f * tex10 + tex20 - tex02 - 2.0f * tex12 - tex22;
	const float3 n = make_float3(Gx, Gy, 1.f);

	return n;
}

/// Sample 2D texture
inline
float3 Texture_SampleBump(float2 uv, TEXTURE_ARG_LIST_IDX(texidx))
{
    // Get width and height
    int width = textures[texidx].w;
    int height = textures[texidx].h;

    // Find the origin of the data in the pool
    __global char const* mydata = texturedata + textures[texidx].dataoffset;

    // Handle UV wrap
    // TODO: need UV mode support
    uv -= floor(uv);

    // Reverse Y:
    // it is needed as textures are loaded with Y axis going top to down
    // and our axis goes from down to top
    uv.y = 1.f - uv.y;

    // Calculate integer coordinates
    int s0 = clamp((int)floor(uv.x * width), 0, width - 1);
    int t0 = clamp((int)floor(uv.y * height), 0, height - 1);

	int s1 = clamp(s0 + 1, 0, width - 1);
	int t1 = clamp(t0 + 1, 0, height - 1);

	// Calculate weights for linear filtering
	float wx = uv.x * width - floor(uv.x * width);
	float wy = uv.y * height - floor(uv.y * height);

    switch (textures[texidx].fmt)
    {
    case RGBA32:
    {
        __global float3 const* mydataf = (__global float3 const*)mydata;

		float3 n00 = TextureData_SampleNormalFromBump_float4(mydataf, width, height, t0, s0);
		float3 n01 = TextureData_SampleNormalFromBump_float4(mydataf, width, height, t0, s1);
		float3 n10 = TextureData_SampleNormalFromBump_float4(mydataf, width, height, t1, s0);
		float3 n11 = TextureData_SampleNormalFromBump_float4(mydataf, width, height, t1, s1);

		float3 n = lerp3(lerp3(n00, n01, wx), lerp3(n10, n11, wx), wy);

		return 0.5f * normalize(n) + make_float3(0.5f, 0.5f, 0.5f);
    }

    case RGBA16:
    {
        __global half const* mydatah = (__global half const*)mydata;

		float3 n00 = TextureData_SampleNormalFromBump_half4(mydatah, width, height, t0, s0);
		float3 n01 = TextureData_SampleNormalFromBump_half4(mydatah, width, height, t0, s1);
		float3 n10 = TextureData_SampleNormalFromBump_half4(mydatah, width, height, t1, s0);
		float3 n11 = TextureData_SampleNormalFromBump_half4(mydatah, width, height, t1, s1);

		float3 n = lerp3(lerp3(n00, n01, wx), lerp3(n10, n11, wx), wy);

		return 0.5f * normalize(n) + make_float3(0.5f, 0.5f, 0.5f);
    }

    case RGBA8:
    {
        __global uchar4 const* mydatac = (__global uchar4 const*)mydata;

		float3 n00 = TextureData_SampleNormalFromBump_uchar4(mydatac, width, height, t0, s0);
		float3 n01 = TextureData_SampleNormalFromBump_uchar4(mydatac, width, height, t0, s1);
		float3 n10 = TextureData_SampleNormalFromBump_uchar4(mydatac, width, height, t1, s0);
		float3 n11 = TextureData_SampleNormalFromBump_uchar4(mydatac, width, height, t1, s1);

		float3 n = lerp3(lerp3(n00, n01, wx), lerp3(n10, n11, wx), wy);

		return 0.5f * normalize(n) + make_float3(0.5f, 0.5f, 0.5f);
    }

    default:
    {
        return make_float3(0.f, 0.f, 0.f);
    }
    }
}



#endif // TEXTURE_CL
