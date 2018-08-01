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
#ifndef RAY_CL
#define RAY_CL

#include <../Baikal/Kernels/CL/common.cl>
#include <../Baikal/Kernels/CL/payload.cl>

// Ray descriptor
typedef struct
{
    // xyz - origin, w - max range
    float4 o;
    // xyz - direction, w - time
    float4 d;
    // x - ray mask, y - activity flag
    int2 extra;
    // Padding
    float2 padding;
} ray;

// Auxiliary rays descriptor
typedef struct
{
    //half3 o;
    //half3 d;

    // Origin
    float3 o;
    // Direction
    float3 d;
} aux_ray;

// Set ray activity flag
INLINE void Ray_SetInactive(GLOBAL ray* r)
{
    r->extra.y = 0;
}

INLINE bool Ray_IsActive(GLOBAL ray* r)
{
    return r->extra.y != 0;
}

// Set extra data for ray
INLINE void Ray_SetExtra(GLOBAL ray* r, float2 extra)
{
    r->padding = extra;
}

// Set mask
INLINE void Ray_SetMask(GLOBAL ray* r, int mask)
{
    r->extra.x = mask;
}

INLINE int Ray_GetMask(GLOBAL ray* r)
{
    return r->extra.x;
}

// Get extra data for ray
INLINE float2 Ray_GetExtra(GLOBAL ray const* r)
{
    return r->padding;
}

// Initialize ray structure
INLINE void Ray_Init(GLOBAL ray* r, float3 o, float3 d, float maxt, float time, int mask)
{
    r->o.xyz = o;
    r->d.xyz = d;
    r->o.w = maxt;
    r->d.w = time;
    r->extra.x = mask;
    r->extra.y = 0xFFFFFFFF;
}

INLINE void Aux_Ray_Init(GLOBAL aux_ray* r, float3 o, float3 d)
{
    //vstore_half3(o, 0, (GLOBAL half*)&r->o);
    //vstore_half3(d, 0, (GLOBAL half*)&r->d);
    r->o = o;
    r->d = d;
}

INLINE void Aux_Ray_SpecularReflect(
                // Input aux rays
                GLOBAL aux_ray* in_ray_x, GLOBAL aux_ray* in_ray_y,
                // Output reflected aux rays
                GLOBAL aux_ray* out_ray_x, GLOBAL aux_ray* out_ray_y,
                // Differential geometry in the point of reflection
                DifferentialGeometry* diffgeo,
                // Input, output ray direction
                float3 wo, float3 wi)
{
    float3 dndx = diffgeo->dndu * diffgeo->duvdx.x +
                  diffgeo->dndv * diffgeo->duvdx.y;
    float3 dndy = diffgeo->dndu * diffgeo->duvdy.x +
                  diffgeo->dndv * diffgeo->duvdy.y;

    float3 dwodx = -in_ray_x->d - wo;
    float3 dwody = -in_ray_y->d - wo;

    float dDNdx = dot(dwodx, diffgeo->n) + dot(wo, dndx);
    float dDNdy = dot(dwody, diffgeo->n) + dot(wo, dndy);

    Aux_Ray_Init(out_ray_x, diffgeo->p + diffgeo->dpdx, wi - dwodx + 2.0f * (dot(wo, diffgeo->n) * dndx + dDNdx * diffgeo->n));
    Aux_Ray_Init(out_ray_y, diffgeo->p + diffgeo->dpdy, wi - dwody + 2.0f * (dot(wo, diffgeo->n) * dndy + dDNdy * diffgeo->n));
}

INLINE void Aux_Ray_SpecularRefract(
                // Input aux rays
                GLOBAL aux_ray* in_ray_x, GLOBAL aux_ray* in_ray_y,
                // Output reflected aux rays
                GLOBAL aux_ray* out_ray_x, GLOBAL aux_ray* out_ray_y,
                // Differential geometry in the point of reflection
                DifferentialGeometry* diffgeo,
                // Input, output ray direction
                float3 wo, float3 wi,
                // Index of refraction
                float ior)
{
    float eta = ior;
    float3 w = -wo;
    if (dot(wo, diffgeo->n) < 0.0f)
    {
        eta = 1.0f / eta;
    }

    float3 dndx = diffgeo->dndu * diffgeo->duvdx.x +
                  diffgeo->dndv * diffgeo->duvdx.y;
    float3 dndy = diffgeo->dndu * diffgeo->duvdy.x +
                  diffgeo->dndv * diffgeo->duvdy.y;

    float3 dwodx = -in_ray_x->d - wo;
    float3 dwody = -in_ray_y->d - wo;

    float dDNdx = dot(dwodx, diffgeo->n) + dot(wo, dndx);
    float dDNdy = dot(dwody, diffgeo->n) + dot(wo, dndy);

    float mu = eta * dot(w, diffgeo->n) - dot(wi, diffgeo->n);
    float dmudx = (eta - (eta * eta * dot(w, diffgeo->n)) / dot(wi, diffgeo->n)) * dDNdx;
    float dmudy = (eta - (eta * eta * dot(w, diffgeo->n)) / dot(wi, diffgeo->n)) * dDNdy;

    Aux_Ray_Init(out_ray_x, diffgeo->p + diffgeo->dpdx, wi + eta * dwodx - (mu * dndx + dmudx * diffgeo->n));
    Aux_Ray_Init(out_ray_y, diffgeo->p + diffgeo->dpdy, wi + eta * dwody - (mu * dndy + dmudy * diffgeo->n));
}

#endif
