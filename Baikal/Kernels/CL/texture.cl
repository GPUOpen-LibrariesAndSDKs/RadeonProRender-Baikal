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
#define TEXTURE_ARG_LIST __global Texture const* textures, __global MipLevel const* mip_levels, __global char const* texturedata
#define TEXTURE_ARG_LIST_IDX(x) int x, __global Texture const* textures, __global MipLevel const* mip_levels, __global char const* texturedata
#define TEXTURE_ARGS textures, mip_levels, texturedata
#define TEXTURE_ARGS_IDX(x) x, textures, mip_levels, texturedata

inline
int WrapTexel(float coord, float modulus)
{
    return (coord / modulus - floor(coord / modulus)) * modulus;
}

/// Sample 2D texture
inline
float4 Texture_Sample2DNoMip(float2 uv, TEXTURE_ARG_LIST_IDX(texidx))
{
    MipLevel texture_level = mip_levels[textures[texidx].mip_offset];

    // Get width, height of levels
    int w = texture_level.w;
    int h = texture_level.h;

    // Scale uv coordinates
    float x = uv.x * w - 0.5f;
    float y = uv.y * h - 0.5f;

    // Get texture space integer coordinates (with offsets for bilinear filtering)
    // >>> TODO: Need to implement UV mode support
    // Also reverse Y coordinate, it is needed as textures are loaded with Y axis
    // going top to down and our axis goes from down to top
    int xi0 = WrapTexel(x, w);
    int yi0 = (h - 1) - WrapTexel(y, h);
    int xi1 = WrapTexel(x + 1, w);
    int yi1 = (h - 1) - WrapTexel(y + 1, h);

    // Weights for bilinear interpolation
    float wx = x - floor(x);
    float wy = y - floor(y);

    __global char const* mydata = texturedata + texture_level.dataoffset;

    switch (textures[texidx].fmt)
    {
    case RGBA32:
    {
        __global float4 const* mydataf = (__global float4 const*)mydata;

        float4 val00 = *(mydataf + w * yi0 + xi0);
        float4 val01 = *(mydataf + w * yi0 + xi1);
        float4 val10 = *(mydataf + w * yi1 + xi0);
        float4 val11 = *(mydataf + w * yi1 + xi1);

        // Perform trilinear filtering and return the result
        return lerp(lerp(val00, val01, wx), lerp(val10, val11, wx), wy);
    }

    case RGBA16:
    {
        __global half const* mydatah = (__global half const*)mydata;

        float4 val00 = vload_half4(w * yi0 + xi0, mydatah);
        float4 val01 = vload_half4(w * yi0 + xi1, mydatah);
        float4 val10 = vload_half4(w * yi1 + xi0, mydatah);
        float4 val11 = vload_half4(w * yi1 + xi1, mydatah);

        // Perform trilinear filtering and return the result
        return lerp(lerp(val00, val01, wx), lerp(val10, val11, wx), wy);
    }

    case RGBA8:
    {
        __global uchar4 const* mydatac = (__global uchar4 const*)mydata;

        // Get 4 values and convert to float
        uchar4 valu00 = *(mydatac + w * yi0 + xi0);
        uchar4 valu01 = *(mydatac + w * yi0 + xi1);
        uchar4 valu10 = *(mydatac + w * yi1 + xi0);
        uchar4 valu11 = *(mydatac + w * yi1 + xi1);

        float4 val00 = make_float4((float)valu00.x / 255.f, (float)valu00.y / 255.f, (float)valu00.z / 255.f, (float)valu00.w / 255.f);
        float4 val01 = make_float4((float)valu01.x / 255.f, (float)valu01.y / 255.f, (float)valu01.z / 255.f, (float)valu01.w / 255.f);
        float4 val10 = make_float4((float)valu10.x / 255.f, (float)valu10.y / 255.f, (float)valu10.z / 255.f, (float)valu10.w / 255.f);
        float4 val11 = make_float4((float)valu11.x / 255.f, (float)valu11.y / 255.f, (float)valu11.z / 255.f, (float)valu11.w / 255.f);

        // Perform trilinear filtering and return the result
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
float4 Texture_Sample2D(DifferentialGeometry const *diffgeo, TEXTURE_ARG_LIST_IDX(texidx))
{
    // If texture has no mipmaps or derivatives are zero
    if (textures[texidx].mip_count == 1 || dot(diffgeo->duvdx, diffgeo->duvdx) * dot(diffgeo->duvdy, diffgeo->duvdy) == 0.0f)
    {
        return Texture_Sample2DNoMip(diffgeo->uv, TEXTURE_ARGS_IDX(texidx));
    }

    GLOBAL MipLevel const* texture_levels = mip_levels + textures[texidx].mip_offset;

    // Transform derivatives to texture space
    float2 dtexdx        = diffgeo->duvdx * texture_levels[0].w;
    float2 dtexdy        = diffgeo->duvdy * texture_levels[0].h;
    float  delta_max_sqr = max(dot(dtexdx, dtexdx), dot(dtexdy, dtexdy));
    float  mipmap_level  = clamp(0.5f * log2(delta_max_sqr), 0.0f, textures[texidx].mip_count - 2.0f);

    int mip_index = (int)floor(mipmap_level);

    // Get width, height of levels
    int w0 = texture_levels[mip_index].w;
    int h0 = texture_levels[mip_index].h;
    int w1 = texture_levels[mip_index + 1].w;
    int h1 = texture_levels[mip_index + 1].h;

    float2 uv = diffgeo->uv;
    // Scale uv coordinates
    // Level (n)
    float x0 = uv.x * w0 - 0.5f;
    float y0 = uv.y * h0 - 0.5f;
    // Level (n + 1)
    float x1 = uv.x * w1 - 0.5f;
    float y1 = uv.y * h1 - 0.5f;

    // Get texture space integer coordinates (with offsets for bilinear filtering)
    // >>> TODO: Need to implement UV mode support
    // Also reverse Y coordinate, it is needed as textures are loaded with Y axis
    // going top to down and our axis goes from down to top
    // Level (n)
    int xi00 = WrapTexel(x0, w0);
    int yi00 = (h0 - 1) - WrapTexel(y0, h0);
    int xi01 = WrapTexel(x0 + 1, w0);
    int yi01 = (h0 - 1) - WrapTexel(y0 + 1, h0);
    // Level (n + 1)
    int xi10 = WrapTexel(x1, w1);
    int yi10 = (h1 - 1) - WrapTexel(y1, h1);
    int xi11 = WrapTexel(x1 + 1, w1);
    int yi11 = (h1 - 1) - WrapTexel(y1 + 1, h1);

    // Weights for bilinear interpolation
    // Level (n)
    float wx0 = x0 - floor(x0);
    float wy0 = y0 - floor(y0);
    // Level (n + 1)
    float wx1 = x1 - floor(x1);
    float wy1 = y1 - floor(y1);

    // Weights for interpolation between neighbor mip levels
    float w01 = mipmap_level - floor(mipmap_level);

    __global char const* mydata0 = texturedata + texture_levels[mip_index].dataoffset;
    __global char const* mydata1 = texturedata + texture_levels[mip_index + 1].dataoffset;

    switch (textures[texidx].fmt)
    {
    case RGBA32:
    {
        __global float4 const* mydataf0 = (__global float4 const*)mydata0;
        __global float4 const* mydataf1 = (__global float4 const*)mydata1;

        float4 val000 = *(mydataf0 + w0 * yi00 + xi00);
        float4 val001 = *(mydataf0 + w0 * yi00 + xi01);
        float4 val010 = *(mydataf0 + w0 * yi01 + xi00);
        float4 val011 = *(mydataf0 + w0 * yi01 + xi01);

        float4 val100 = *(mydataf1 + w1 * yi10 + xi10);
        float4 val101 = *(mydataf1 + w1 * yi10 + xi11);
        float4 val110 = *(mydataf1 + w1 * yi11 + xi10);
        float4 val111 = *(mydataf1 + w1 * yi11 + xi11);

        // Perform trilinear filtering and return the result
        return lerp(lerp(lerp(val000, val001, wx0), lerp(val010, val011, wx0), wy0),
                    lerp(lerp(val100, val101, wx1), lerp(val110, val111, wx1), wy1),
                    w01);
    }

    case RGBA16:
    {
        __global half const* mydatah0 = (__global half const*)mydata0;
        __global half const* mydatah1 = (__global half const*)mydata1;

        float4 val000 = vload_half4(w0 * yi00 + xi00, mydatah0);
        float4 val001 = vload_half4(w0 * yi00 + xi01, mydatah0);
        float4 val010 = vload_half4(w0 * yi01 + xi00, mydatah0);
        float4 val011 = vload_half4(w0 * yi01 + xi01, mydatah0);

        float4 val100 = vload_half4(w1 * yi10 + xi10, mydatah1);
        float4 val101 = vload_half4(w1 * yi10 + xi11, mydatah1);
        float4 val110 = vload_half4(w1 * yi11 + xi10, mydatah1);
        float4 val111 = vload_half4(w1 * yi11 + xi11, mydatah1);

        // Perform trilinear filtering and return the result
        return lerp(lerp(lerp(val000, val001, wx0), lerp(val010, val011, wx0), wy0),
                    lerp(lerp(val100, val101, wx1), lerp(val110, val111, wx1), wy1),
                    w01);
    }

    case RGBA8:
    {
        __global uchar4 const* mydatac0 = (__global uchar4 const*)mydata0;
        __global uchar4 const* mydatac1 = (__global uchar4 const*)mydata1;

        // Get 4 values and convert to float
        uchar4 valu000 = *(mydatac0 + w0 * yi00 + xi00);
        uchar4 valu001 = *(mydatac0 + w0 * yi00 + xi01);
        uchar4 valu010 = *(mydatac0 + w0 * yi01 + xi00);
        uchar4 valu011 = *(mydatac0 + w0 * yi01 + xi01);

        uchar4 valu100 = *(mydatac1 + w1 * yi10 + xi10);
        uchar4 valu101 = *(mydatac1 + w1 * yi10 + xi11);
        uchar4 valu110 = *(mydatac1 + w1 * yi11 + xi10);
        uchar4 valu111 = *(mydatac1 + w1 * yi11 + xi11);

        float4 val000 = make_float4((float)valu000.x / 255.f, (float)valu000.y / 255.f, (float)valu000.z / 255.f, (float)valu000.w / 255.f);
        float4 val001 = make_float4((float)valu001.x / 255.f, (float)valu001.y / 255.f, (float)valu001.z / 255.f, (float)valu001.w / 255.f);
        float4 val010 = make_float4((float)valu010.x / 255.f, (float)valu010.y / 255.f, (float)valu010.z / 255.f, (float)valu010.w / 255.f);
        float4 val011 = make_float4((float)valu011.x / 255.f, (float)valu011.y / 255.f, (float)valu011.z / 255.f, (float)valu011.w / 255.f);

        float4 val100 = make_float4((float)valu100.x / 255.f, (float)valu100.y / 255.f, (float)valu100.z / 255.f, (float)valu100.w / 255.f);
        float4 val101 = make_float4((float)valu101.x / 255.f, (float)valu101.y / 255.f, (float)valu101.z / 255.f, (float)valu101.w / 255.f);
        float4 val110 = make_float4((float)valu110.x / 255.f, (float)valu110.y / 255.f, (float)valu110.z / 255.f, (float)valu110.w / 255.f);
        float4 val111 = make_float4((float)valu111.x / 255.f, (float)valu111.y / 255.f, (float)valu111.z / 255.f, (float)valu111.w / 255.f);

        // Perform trilinear filtering and return the result
        return lerp(lerp(lerp(val000, val001, wx0), lerp(val010, val011, wx0), wy0),
                    lerp(lerp(val100, val101, wx1), lerp(val110, val111, wx1), wy1),
                    w01);
    }

    default:
    {
        return make_float4(0.f, 0.f, 0.f, 0.f);
    }
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
    return Texture_Sample2DNoMip(uv, TEXTURE_ARGS_IDX(texidx)).xyz;
}

inline float3 TextureData_SampleNormalFromBump_uchar4(__global uchar4 const* mydatac, int width, int height, int s0, int t0)
{
    int s0minus = WrapTexel(s0 - 1, width);
    int s0plus  = WrapTexel(s0 + 1, width);
    int t0minus = WrapTexel(t0 - 1, height);
    int t0plus  = WrapTexel(t0 + 1, height);

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

inline float3 TextureData_SampleNormalFromBump_half4(__global half const* mydatah, int width, int height, int s0, int t0)
{
    int s0minus = WrapTexel(s0 - 1, width);
    int s0plus  = WrapTexel(s0 + 1, width);
    int t0minus = WrapTexel(t0 - 1, height);
    int t0plus  = WrapTexel(t0 + 1, height);

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

inline float3 TextureData_SampleNormalFromBump_float4(__global float4 const* mydataf, int width, int height, int s0, int t0)
{
    int s0minus = WrapTexel(s0 - 1, width);
    int s0plus  = WrapTexel(s0 + 1, width);
    int t0minus = WrapTexel(t0 - 1, height);
    int t0plus  = WrapTexel(t0 + 1, height);

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

/// Sample bump texture (not using mipmapping)
inline
float3 Texture_SampleBumpNoMip(float2 uv, TEXTURE_ARG_LIST_IDX(texidx))
{
    MipLevel texture_level = mip_levels[textures[texidx].mip_offset];

    // Get width, height of levels
    int w = texture_level.w;
    int h = texture_level.h;

    // Scale uv coordinates
    // Level (n)
    float x = uv.x * w - 0.5f;
    float y = uv.y * h - 0.5f;

    // Get texture space integer coordinates (with offsets for bilinear filtering)
    // >>> TODO: Need to implement UV mode support
    // Also reverse Y coordinate, it is needed as textures are loaded with Y axis
    // going top to down and our axis goes from down to top
    // Level (n)
    int xi0 = WrapTexel(x, w);
    int yi0 = (h - 1) - WrapTexel(y, h);
    int xi1 = WrapTexel(x + 1, w);
    int yi1 = (h - 1) - WrapTexel(y + 1, h);

    // Weights for bilinear interpolation
    float wx = x - floor(x);
    float wy = y - floor(y);

    __global char const* mydata = texturedata + texture_level.dataoffset;

    switch (textures[texidx].fmt)
    {
    case RGBA32:
    {
        __global float4 const* mydataf = (__global float4 const*)mydata;

        float3 n00 = TextureData_SampleNormalFromBump_float4(mydataf, w, h, xi0, yi0);
        float3 n01 = TextureData_SampleNormalFromBump_float4(mydataf, w, h, xi1, yi0);
        float3 n10 = TextureData_SampleNormalFromBump_float4(mydataf, w, h, xi0, yi1);
        float3 n11 = TextureData_SampleNormalFromBump_float4(mydataf, w, h, xi1, yi1);

        float3 n = lerp3(lerp3(n00, n01, wx), lerp3(n10, n11, wx), wy);

        return 0.5f * normalize(n) + make_float3(0.5f, 0.5f, 0.5f);
    }

    case RGBA16:
    {
        __global half const* mydatah = (__global half const*)mydata;

        float3 n00 = TextureData_SampleNormalFromBump_half4(mydatah, w, h, xi0, yi0);
        float3 n01 = TextureData_SampleNormalFromBump_half4(mydatah, w, h, xi1, yi0);
        float3 n10 = TextureData_SampleNormalFromBump_half4(mydatah, w, h, xi0, yi1);
        float3 n11 = TextureData_SampleNormalFromBump_half4(mydatah, w, h, xi1, yi1);

        float3 n = lerp3(lerp3(n00, n01, wx), lerp3(n10, n11, wx), wy);

        return 0.5f * normalize(n) + make_float3(0.5f, 0.5f, 0.5f);
    }

    case RGBA8:
    {
        __global uchar4 const* mydatac = (__global uchar4 const*)mydata;

        float3 n00 = TextureData_SampleNormalFromBump_uchar4(mydatac, w, h, xi0, yi0);
        float3 n01 = TextureData_SampleNormalFromBump_uchar4(mydatac, w, h, xi1, yi0);
        float3 n10 = TextureData_SampleNormalFromBump_uchar4(mydatac, w, h, xi0, yi1);
        float3 n11 = TextureData_SampleNormalFromBump_uchar4(mydatac, w, h, xi1, yi1);

        float3 n = lerp3(lerp3(n00, n01, wx), lerp3(n10, n11, wx), wy);

        return 0.5f * normalize(n) + make_float3(0.5f, 0.5f, 0.5f);
    }

    default:
    {
        return make_float3(0.f, 0.f, 0.f);
    }
    }
}

/// Sample bump texture
inline
float3 Texture_SampleBump(DifferentialGeometry const *diffgeo, TEXTURE_ARG_LIST_IDX(texidx))
{
    // If texture has no mipmaps or derivatives are zero
    if (textures[texidx].mip_count == 1 || dot(diffgeo->duvdx, diffgeo->duvdx) * dot(diffgeo->duvdy, diffgeo->duvdy) == 0.0f)
    {
        return Texture_SampleBumpNoMip(diffgeo->uv, TEXTURE_ARGS_IDX(texidx));
    }

    GLOBAL MipLevel const* texture_levels = mip_levels + textures[texidx].mip_offset;

    // Transform derivatives to texture space
    float2 dtexdx        = diffgeo->duvdx * texture_levels[0].w;
    float2 dtexdy        = diffgeo->duvdy * texture_levels[0].h;
    float  delta_max_sqr = max(dot(dtexdx, dtexdx), dot(dtexdy, dtexdy));
    float  mipmap_level  = clamp(0.5f * log2(delta_max_sqr), 0.0f, textures[texidx].mip_count - 2.0f);

    int mip_index = (int)floor(mipmap_level);

    // Get width, height of levels
    int w0 = texture_levels[mip_index].w;
    int h0 = texture_levels[mip_index].h;
    int w1 = texture_levels[mip_index + 1].w;
    int h1 = texture_levels[mip_index + 1].h;

    float2 uv = diffgeo->uv;
    // Scale uv coordinates
    // Level (n)
    float x0 = uv.x * w0 - 0.5f;
    float y0 = uv.y * h0 - 0.5f;
    // Level (n + 1)
    float x1 = uv.x * w1 - 0.5f;
    float y1 = uv.y * h1 - 0.5f;

    // Get texture space integer coordinates (with offsets for bilinear filtering)
    // >>> TODO: Need to implement UV mode support
    // Also reverse Y coordinate, it is needed as textures are loaded with Y axis
    // going top to down and our axis goes from down to top
    // Level (n)
    int xi00 = WrapTexel(x0, w0);
    int yi00 = (h0 - 1) - WrapTexel(y0, h0);
    int xi01 = WrapTexel(x0 + 1, w0);
    int yi01 = (h0 - 1) - WrapTexel(y0 + 1, h0);
    // Level (n + 1)
    int xi10 = WrapTexel(x1, w1);
    int yi10 = (h1 - 1) - WrapTexel(y1, h1);
    int xi11 = WrapTexel(x1 + 1, w1);
    int yi11 = (h1 - 1) - WrapTexel(y1 + 1, h1);

    // Weights for bilinear interpolation
    // Level (n)
    float wx0 = x0 - floor(x0);
    float wy0 = y0 - floor(y0);
    // Level (n + 1)
    float wx1 = x1 - floor(x1);
    float wy1 = y1 - floor(y1);

    // Weights for interpolation between neighbor mip levels
    float w01 = mipmap_level - floor(mipmap_level);

    __global char const* mydata0 = texturedata + texture_levels[mip_index].dataoffset;
    __global char const* mydata1 = texturedata + texture_levels[mip_index + 1].dataoffset;

    switch (textures[texidx].fmt)
    {
    case RGBA32:
    {
        __global float4 const* mydataf0 = (__global float4 const*)mydata0;
        __global float4 const* mydataf1 = (__global float4 const*)mydata1;

        float3 n000 = TextureData_SampleNormalFromBump_float4(mydataf0, w0, h0, xi00, yi00);
        float3 n001 = TextureData_SampleNormalFromBump_float4(mydataf0, w0, h0, xi01, yi00);
        float3 n010 = TextureData_SampleNormalFromBump_float4(mydataf0, w0, h0, xi00, yi01);
        float3 n011 = TextureData_SampleNormalFromBump_float4(mydataf0, w0, h0, xi01, yi01);

        float3 n100 = TextureData_SampleNormalFromBump_float4(mydataf1, w1, h1, xi10, yi10);
        float3 n101 = TextureData_SampleNormalFromBump_float4(mydataf1, w1, h1, xi11, yi10);
        float3 n110 = TextureData_SampleNormalFromBump_float4(mydataf1, w1, h1, xi10, yi11);
        float3 n111 = TextureData_SampleNormalFromBump_float4(mydataf1, w1, h1, xi11, yi11);

        float3 n = lerp3(lerp3(lerp3(n000, n001, wx0), lerp3(n010, n011, wx0), wy0),
                   lerp3(lerp3(n100, n101, wx1), lerp3(n110, n111, wx1), wy1),
                   w01);

        return 0.5f * normalize(n) + make_float3(0.5f, 0.5f, 0.5f);
    }

    case RGBA16:
    {
        __global half const* mydatah0 = (__global half const*)mydata0;
        __global half const* mydatah1 = (__global half const*)mydata1;

        float3 n000 = TextureData_SampleNormalFromBump_half4(mydatah0, w0, h0, xi00, yi00);
        float3 n001 = TextureData_SampleNormalFromBump_half4(mydatah0, w0, h0, xi01, yi00);
        float3 n010 = TextureData_SampleNormalFromBump_half4(mydatah0, w0, h0, xi00, yi01);
        float3 n011 = TextureData_SampleNormalFromBump_half4(mydatah0, w0, h0, xi01, yi01);

        float3 n100 = TextureData_SampleNormalFromBump_half4(mydatah1, w1, h1, xi10, yi10);
        float3 n101 = TextureData_SampleNormalFromBump_half4(mydatah1, w1, h1, xi11, yi10);
        float3 n110 = TextureData_SampleNormalFromBump_half4(mydatah1, w1, h1, xi10, yi11);
        float3 n111 = TextureData_SampleNormalFromBump_half4(mydatah1, w1, h1, xi11, yi11);

        float3 n = lerp3(lerp3(lerp3(n000, n001, wx0), lerp3(n010, n011, wx0), wy0),
                   lerp3(lerp3(n100, n101, wx1), lerp3(n110, n111, wx1), wy1),
                   w01);

        return 0.5f * normalize(n) + make_float3(0.5f, 0.5f, 0.5f);
    }

    case RGBA8:
    {
        __global uchar4 const* mydatac0 = (__global uchar4 const*)mydata0;
        __global uchar4 const* mydatac1 = (__global uchar4 const*)mydata1;

        float3 n000 = TextureData_SampleNormalFromBump_uchar4(mydatac0, w0, h0, xi00, yi00);
        float3 n001 = TextureData_SampleNormalFromBump_uchar4(mydatac0, w0, h0, xi01, yi00);
        float3 n010 = TextureData_SampleNormalFromBump_uchar4(mydatac0, w0, h0, xi00, yi01);
        float3 n011 = TextureData_SampleNormalFromBump_uchar4(mydatac0, w0, h0, xi01, yi01);

        float3 n100 = TextureData_SampleNormalFromBump_uchar4(mydatac1, w1, h1, xi10, yi10);
        float3 n101 = TextureData_SampleNormalFromBump_uchar4(mydatac1, w1, h1, xi11, yi10);
        float3 n110 = TextureData_SampleNormalFromBump_uchar4(mydatac1, w1, h1, xi10, yi11);
        float3 n111 = TextureData_SampleNormalFromBump_uchar4(mydatac1, w1, h1, xi11, yi11);

        float3 n = lerp3(lerp3(lerp3(n000, n001, wx0), lerp3(n010, n011, wx0), wy0),
                   lerp3(lerp3(n100, n101, wx1), lerp3(n110, n111, wx1), wy1),
                   w01);

        return 0.5f * normalize(n) + make_float3(0.5f, 0.5f, 0.5f);
    }

    default:
    {
        return make_float3(0.f, 0.f, 0.f);
    }
    }
}

#endif // TEXTURE_CL
