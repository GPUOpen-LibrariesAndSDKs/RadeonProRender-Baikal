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
#ifndef BXDF_CL
#define BXDF_CL

/// Schlick's approximation of Fresnel equtions
float SchlickFresnel(float eta, float ndotw)
{
    const float f = ((1.f - eta) / (1.f + eta)) * ((1.f - eta) / (1.f + eta));
    const float m = 1.f - fabs(ndotw);
    const float m2 = m*m;
    return f + (1.f - f) * m2 * m2 * m;
}

/// Full Fresnel equations
float FresnelDielectric(float etai, float etat, float ndotwi, float ndotwt)
{
    // Parallel and perpendicular polarization
    float rparl = ((etat * ndotwi) - (etai * ndotwt)) / ((etat * ndotwi) + (etai * ndotwt));
    float rperp = ((etai * ndotwi) - (etat * ndotwt)) / ((etai * ndotwi) + (etat * ndotwt));
    return (rparl*rparl + rperp*rperp) * 0.5f;
}

#define DENOM_EPS 1e-8f
#define ROUGHNESS_EPS 0.0001f


enum BxdfFlags
{
    kBxdfSingular = (1 << 0),
    kBxdfBrdf = (1 << 1),
    kBxdfEmissive = (1 << 2),
    kBxdfTransparency = (1 << 3)
};


#include <../Baikal/Kernels/CL/utils.cl>
#include <../Baikal/Kernels/CL/texture.cl>
#include <../Baikal/Kernels/CL/payload.cl>
#include <../Baikal/Kernels/CL/disney.cl>
#include <../Baikal/Kernels/CL/bxdf_basic.cl>
#include <../Baikal/Kernels/CL/bxdf_uberv2.cl>

/*
 Dispatch functions
 */
float3 Bxdf_Evaluate(
    // Geometry
    DifferentialGeometry const* dg,
    // Incoming direction
    float3 wi,
    // Outgoing direction
    float3 wo,
    // Texture args
    TEXTURE_ARG_LIST
    )
{
    // Transform vectors into tangent space
    float3 wi_t = matrix_mul_vector3(dg->world_to_tangent, wi);
    float3 wo_t = matrix_mul_vector3(dg->world_to_tangent, wo);

    int mattype = dg->mat.type;
    switch (mattype)
    {
    case kLambert:
        return Lambert_Evaluate(dg, wi_t, wo_t, TEXTURE_ARGS);
    case kMicrofacetGGX:
        return MicrofacetGGX_Evaluate(dg, wi_t, wo_t, TEXTURE_ARGS);
    case kMicrofacetBeckmann:
        return MicrofacetBeckmann_Evaluate(dg, wi_t, wo_t, TEXTURE_ARGS);
    case kIdealReflect:
        return IdealReflect_Evaluate(dg, wi_t, wo_t, TEXTURE_ARGS);
    case kIdealRefract:
        return IdealRefract_Evaluate(dg, wi_t, wo_t, TEXTURE_ARGS);
    case kTranslucent:
        return Translucent_Evaluate(dg, wi_t, wo_t, TEXTURE_ARGS);
    case kMicrofacetRefractionGGX:
        return MicrofacetRefractionGGX_Evaluate(dg, wi_t, wo_t, TEXTURE_ARGS);
    case kMicrofacetRefractionBeckmann:
        return MicrofacetRefractionBeckmann_Evaluate(dg, wi_t, wo_t, TEXTURE_ARGS);
    case kPassthrough:
        return 0.f;
#ifdef ENABLE_DISNEY
    case kDisney:
        return Disney_Evaluate(dg, wi_t, wo_t, TEXTURE_ARGS);
#endif
    case kUberV2:
        return UberV2_Evaluate(dg, wi_t, wo_t, TEXTURE_ARGS);
    }

    return 0.f;
}

float3 Bxdf_Sample(
    // Geometry
    DifferentialGeometry const* dg,
    // Incoming direction
    float3 wi,
    // Texture args
    TEXTURE_ARG_LIST,
    // RNG
    float2 sample,
    // Outgoing  direction
    float3* wo,
    // PDF at w
    float* pdf
    )
{
    // Transform vectors into tangent space
    float3 wi_t = matrix_mul_vector3(dg->world_to_tangent, wi);
    float3 wo_t;

    float3 res = 0.f;

    int mattype = dg->mat.type;
    switch (mattype)
    {
    case kLambert:
        res = Lambert_Sample(dg, wi_t, TEXTURE_ARGS, sample, &wo_t, pdf);
        break;
    case kMicrofacetGGX:
        res = MicrofacetGGX_Sample(dg, wi_t, TEXTURE_ARGS, sample, &wo_t, pdf);
        break;
    case kMicrofacetBeckmann:
        res = MicrofacetBeckmann_Sample(dg, wi_t, TEXTURE_ARGS, sample, &wo_t, pdf);
        break;
    case kIdealReflect:
        res = IdealReflect_Sample(dg, wi_t, TEXTURE_ARGS, sample, &wo_t, pdf);
        break;
    case kIdealRefract:
        res = IdealRefract_Sample(dg, wi_t, TEXTURE_ARGS, sample, &wo_t, pdf);
        break;
    case kTranslucent:
        res = Translucent_Sample(dg, wi_t, TEXTURE_ARGS, sample, &wo_t, pdf);
        break;
    case kPassthrough:
        res = Passthrough_Sample(dg, wi_t, TEXTURE_ARGS, sample, &wo_t, pdf);
        break;
    case kMicrofacetRefractionGGX:
        res = MicrofacetRefractionGGX_Sample(dg, wi_t, TEXTURE_ARGS, sample, &wo_t, pdf);
        break;
    case kMicrofacetRefractionBeckmann:
        res = MicrofacetRefractionBeckmann_Sample(dg, wi_t, TEXTURE_ARGS, sample, &wo_t, pdf);
        break;
#ifdef ENABLE_DISNEY
    case kDisney:
        res = Disney_Sample(dg, wi_t, TEXTURE_ARGS, sample, &wo_t, pdf);
        break;
#endif
    case kUberV2:
        res = UberV2_Sample(dg, wi_t, TEXTURE_ARGS, sample, &wo_t, pdf);
        break;

    default:
        *pdf = 0.f;
        break;
    }

    *wo = matrix_mul_vector3(dg->tangent_to_world, wo_t);

    return res;
}

float Bxdf_GetPdf(
    // Geometry
    DifferentialGeometry const* dg,
    // Incoming direction
    float3 wi,
    // Outgoing direction
    float3 wo,
    // Texture args
    TEXTURE_ARG_LIST
    )
{
    // Transform vectors into tangent space
    float3 wi_t = matrix_mul_vector3(dg->world_to_tangent, wi);
    float3 wo_t = matrix_mul_vector3(dg->world_to_tangent, wo);

    int mattype = dg->mat.type;
    switch (mattype)
    {
    case kLambert:
        return Lambert_GetPdf(dg, wi_t, wo_t, TEXTURE_ARGS);
    case kMicrofacetGGX:
        return MicrofacetGGX_GetPdf(dg, wi_t, wo_t, TEXTURE_ARGS);
    case kMicrofacetBeckmann:
        return MicrofacetBeckmann_GetPdf(dg, wi_t, wo_t, TEXTURE_ARGS);
    case kIdealReflect:
        return IdealReflect_GetPdf(dg, wi_t, wo_t, TEXTURE_ARGS);
    case kIdealRefract:
        return IdealRefract_GetPdf(dg, wi_t, wo_t, TEXTURE_ARGS);
    case kTranslucent:
        return Translucent_GetPdf(dg, wi_t, wo_t, TEXTURE_ARGS);
    case kPassthrough:
        return 0.f;
    case kMicrofacetRefractionGGX:
        return MicrofacetRefractionGGX_GetPdf(dg, wi_t, wo_t, TEXTURE_ARGS);
    case kMicrofacetRefractionBeckmann:
        return MicrofacetRefractionBeckmann_GetPdf(dg, wi_t, wo_t, TEXTURE_ARGS);
#ifdef ENABLE_DISNEY
    case kDisney:
        return Disney_GetPdf(dg, wi_t, wo_t, TEXTURE_ARGS);
#endif
    case kUberV2:
        return UberV2_GetPdf(dg, wi_t, wo_t, TEXTURE_ARGS);
    }

    return 0.f;
}

/// Emissive BRDF sampling
float3 Emissive_GetLe(
    // Geometry
    DifferentialGeometry const* dg,
    // Texture args
    TEXTURE_ARG_LIST)
{
    const float3 kd = Texture_GetValue3f(dg->mat.simple.kx.xyz, dg->uv, TEXTURE_ARGS_IDX(dg->mat.simple.kxmapidx));
    return kd;
}

/// BxDF singularity check
bool Bxdf_IsSingular(DifferentialGeometry const* dg)
{
    return (dg->mat.bxdf_flags & kBxdfSingular) == kBxdfSingular;
}

/// BxDF emission check
bool Bxdf_IsEmissive(DifferentialGeometry const* dg)
{
    return (dg->mat.bxdf_flags & kBxdfEmissive) == kBxdfEmissive;
}

/// BxDF singularity check
bool Bxdf_IsBtdf(DifferentialGeometry const* dg)
{
    return (dg->mat.bxdf_flags & kBxdfBrdf) == 0;
}

bool Bxdf_IsRefraction(DifferentialGeometry const* dg)
{
    return Bxdf_IsBtdf(dg) && ((dg->mat.uberv2.layers & kRefractionLayer) == kRefractionLayer);
}

bool Bxdf_IsReflection(DifferentialGeometry const* dg)
{
    return (dg->mat.bxdf_flags & kBxdfBrdf) == kBxdfBrdf;
}

bool Bxdf_IsTransparency(DifferentialGeometry const* dg)
{
    return (dg->mat.bxdf_flags & kBxdfTransparency) == kBxdfTransparency;
}

#endif // BXDF_CL
