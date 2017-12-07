/* This is an auto-generated file. Do not edit manually*/

static const char g_bxdf_opencl[]= \
"/********************************************************************** \n"\
"Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved. \n"\
" \n"\
"Permission is hereby granted, free of charge, to any person obtaining a copy \n"\
"of this software and associated documentation files (the \"Software\"), to deal \n"\
"in the Software without restriction, including without limitation the rights \n"\
"to use, copy, modify, merge, publish, distribute, sublicense, and/or sell \n"\
"copies of the Software, and to permit persons to whom the Software is \n"\
"furnished to do so, subject to the following conditions: \n"\
" \n"\
"The above copyright notice and this permission notice shall be included in \n"\
"all copies or substantial portions of the Software. \n"\
" \n"\
"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR \n"\
"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, \n"\
"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE \n"\
"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER \n"\
"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, \n"\
"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN \n"\
"THE SOFTWARE. \n"\
"********************************************************************/ \n"\
"#ifndef BXDF_CL \n"\
"#define BXDF_CL \n"\
" \n"\
" \n"\
"#define DENOM_EPS 1e-8f \n"\
"#define ROUGHNESS_EPS 0.0001f \n"\
" \n"\
" \n"\
"enum BxdfFlags \n"\
"{ \n"\
"    kReflection = (1 << 0), \n"\
"    kTransmission = (1 << 1), \n"\
"    kDiffuse = (1 << 2), \n"\
"    kSpecular = (1 << 3), \n"\
"    kGlossy = (1 << 4), \n"\
"    kAllReflection = kReflection | kDiffuse | kSpecular | kGlossy, \n"\
"    kAllTransmission = kTransmission | kDiffuse | kSpecular | kGlossy, \n"\
"    kAll = kReflection | kTransmission | kDiffuse | kSpecular | kGlossy \n"\
"}; \n"\
" \n"\
" \n"\
"/// Schlick's approximation of Fresnel equtions \n"\
"float SchlickFresnel(float eta, float ndotw) \n"\
"{ \n"\
"    const float f = ((1.f - eta) / (1.f + eta)) * ((1.f - eta) / (1.f + eta)); \n"\
"    const float m = 1.f - fabs(ndotw); \n"\
"    const float m2 = m*m; \n"\
"    return f + (1.f - f) * m2 * m2 * m; \n"\
"} \n"\
" \n"\
"/// Full Fresnel equations \n"\
"float FresnelDielectric(float etai, float etat, float ndotwi, float ndotwt) \n"\
"{ \n"\
"    // Parallel and perpendicular polarization \n"\
"    float rparl = ((etat * ndotwi) - (etai * ndotwt)) / ((etat * ndotwi) + (etai * ndotwt)); \n"\
"    float rperp = ((etai * ndotwi) - (etat * ndotwt)) / ((etai * ndotwi) + (etat * ndotwt)); \n"\
"    return (rparl*rparl + rperp*rperp) * 0.5f; \n"\
"} \n"\
" \n"\
"/* \n"\
" Microfacet Beckmann \n"\
" */ \n"\
" \n"\
" // Distribution fucntion \n"\
"float MicrofacetDistribution_Beckmann_D(float roughness, float3 m) \n"\
"{ \n"\
"    float ndotm = fabs(m.y); \n"\
"    float ndotm2 = ndotm * ndotm; \n"\
"    float sinmn = native_sqrt(1.f - clamp(ndotm * ndotm, 0.f, 1.f)); \n"\
"    float tanmn = ndotm > DENOM_EPS ? sinmn / ndotm : 0.f; \n"\
"    float a2 = roughness * roughness; \n"\
" \n"\
"    return ndotm > DENOM_EPS ? (1.f / (PI * a2 * ndotm2 * ndotm2)) * native_exp(-tanmn * tanmn / a2) : 0.f; \n"\
"} \n"\
" \n"\
"// PDF of the given direction \n"\
"float MicrofacetDistribution_Beckmann_GetPdf( \n"\
"    // Rougness \n"\
"    float roughness, \n"\
"    // Geometry \n"\
"    DifferentialGeometry const* dg, \n"\
"    // Incoming direction \n"\
"    float3 wi, \n"\
"    // Outgoing direction \n"\
"    float3 wo, \n"\
"    // Texture args \n"\
"    TEXTURE_ARG_LIST \n"\
"    ) \n"\
"{ \n"\
"    // We need to convert pdf(wh)->pdf(wo) \n"\
"    float3 m = normalize(wi + wo); \n"\
"    float wodotm = fabs(dot(wo, m)); \n"\
" \n"\
"    float mpdf = MicrofacetDistribution_Beckmann_D(roughness, m) * fabs(m.y); \n"\
"    // See Humphreys and Pharr for derivation \n"\
"    return mpdf / (4.f * wodotm); \n"\
"} \n"\
" \n"\
"// Sample the distribution \n"\
"void MicrofacetDistribution_Beckmann_SampleNormal(// Roughness \n"\
"    float roughness, \n"\
"    // Geometry \n"\
"    DifferentialGeometry const* dg, \n"\
"    // Texture args \n"\
"    TEXTURE_ARG_LIST, \n"\
"    // Sample \n"\
"    float2 sample, \n"\
"    // Outgoing  direction \n"\
"    float3* wh \n"\
"    ) \n"\
"{ \n"\
"    float r1 = sample.x; \n"\
"    float r2 = sample.y; \n"\
" \n"\
"    // Sample halfway vector first, then reflect wi around that \n"\
"    float theta = atan(native_sqrt(-roughness*roughness*native_log(max(DENOM_EPS, 1.f - r1)))); \n"\
"    float costheta = native_cos(theta); \n"\
"    float sintheta = native_sin(theta); \n"\
" \n"\
"    // phi = 2*PI*ksi2 \n"\
"    float phi = 2.f*PI*r2; \n"\
"    float cosphi = native_cos(phi); \n"\
"    float sinphi = native_sin(phi); \n"\
" \n"\
"    // Reflect wi around wh \n"\
"    *wh = make_float3(sintheta * cosphi, costheta, sintheta * sinphi); \n"\
"} \n"\
" \n"\
"float MicrofacetDistribution_Beckmann_G1(float roughness, float3 v, float3 m) \n"\
"{ \n"\
"    float ndotv = fabs(v.y); \n"\
"    float mdotv = fabs(dot(m, v)); \n"\
"    float sinnv = native_sqrt(1.f - clamp(ndotv * ndotv, 0.f, 1.f)); \n"\
"    float tannv = ndotv > DENOM_EPS ? sinnv / ndotv : 0.f; \n"\
"    float a = tannv > DENOM_EPS ? 1.f / (roughness * tannv) : 0.f;  \n"\
"    float a2 = a * a; \n"\
" \n"\
"    if (a > 1.6f) \n"\
"        return 1.f; \n"\
" \n"\
"    return (3.535f * a + 2.181f * a2) / (1.f + 2.276f * a + 2.577f * a2); \n"\
"} \n"\
" \n"\
"// Shadowing function also depends on microfacet distribution \n"\
"float MicrofacetDistribution_Beckmann_G(float roughness, float3 wi, float3 wo, float3 wh) \n"\
"{ \n"\
"    return MicrofacetDistribution_Beckmann_G1(roughness, wi, wh) * MicrofacetDistribution_Beckmann_G1(roughness, wo, wh); \n"\
"} \n"\
" \n"\
" \n"\
"float3 MicrofacetBeckmann_Evaluate( \n"\
"    // Geometry \n"\
"    DifferentialGeometry const* dg, \n"\
"    // Incoming direction \n"\
"    float3 wi, \n"\
"    // Outgoing direction \n"\
"    float3 wo, \n"\
"    // Texture args \n"\
"    TEXTURE_ARG_LIST \n"\
"    ) \n"\
"{ \n"\
"    const float3 ks = Texture_GetValue3f(dg->mat.simple.kx.xyz, dg->uv, TEXTURE_ARGS_IDX(dg->mat.simple.kxmapidx)); \n"\
"    const float roughness = Texture_GetValue1f(dg->mat.simple.ns, dg->uv, TEXTURE_ARGS_IDX(dg->mat.simple.nsmapidx)); \n"\
"    const float eta = dg->mat.simple.ni; \n"\
" \n"\
"    // Incident and reflected zenith angles \n"\
"    float costhetao = fabs(wo.y); \n"\
"    float costhetai = fabs(wi.y); \n"\
" \n"\
"    // Calc halfway vector \n"\
"    float3 wh = normalize(wi + wo); \n"\
" \n"\
"    float F = dg->mat.simple.fresnel; \n"\
" \n"\
"    float denom = 4.f * costhetao * costhetai; \n"\
" \n"\
"    // F(eta) * D * G * ks / (4 * cosa * cosi) \n"\
"    return denom > DENOM_EPS ? F * ks * MicrofacetDistribution_Beckmann_G(roughness, wi, wo, wh) * MicrofacetDistribution_Beckmann_D(roughness, wh) / denom : 0.f; \n"\
"} \n"\
" \n"\
" \n"\
"float MicrofacetBeckmann_GetPdf( \n"\
"    // Geometry \n"\
"    DifferentialGeometry const* dg, \n"\
"    // Incoming direction \n"\
"    float3 wi, \n"\
"    // Outgoing direction \n"\
"    float3 wo, \n"\
"    // Texture args \n"\
"    TEXTURE_ARG_LIST \n"\
"    ) \n"\
"{ \n"\
"    const float roughness = Texture_GetValue1f(dg->mat.simple.ns, dg->uv, TEXTURE_ARGS_IDX(dg->mat.simple.nsmapidx)); \n"\
"    return MicrofacetDistribution_Beckmann_GetPdf(roughness, dg, wi, wo, TEXTURE_ARGS); \n"\
"} \n"\
" \n"\
"float3 MicrofacetBeckmann_Sample( \n"\
"    // Geometry \n"\
"    DifferentialGeometry const* dg, \n"\
"    // Incoming direction \n"\
"    float3 wi, \n"\
"    // Texture args \n"\
"    TEXTURE_ARG_LIST, \n"\
"    // Sample \n"\
"    float2 sample, \n"\
"    // Outgoing  direction \n"\
"    float3* wo, \n"\
"    // PDF at wo \n"\
"    float* pdf \n"\
"    ) \n"\
"{ \n"\
"    const float roughness = Texture_GetValue1f(dg->mat.simple.ns, dg->uv, TEXTURE_ARGS_IDX(dg->mat.simple.nsmapidx)); \n"\
" \n"\
"    float3 wh; \n"\
"    MicrofacetDistribution_Beckmann_SampleNormal(roughness, dg, TEXTURE_ARGS, sample, &wh); \n"\
" \n"\
"    *wo = -wi + 2.f*fabs(dot(wi, wh)) * wh; \n"\
" \n"\
"    *pdf = MicrofacetDistribution_Beckmann_GetPdf(roughness, dg, wi, *wo, TEXTURE_ARGS); \n"\
" \n"\
"    return MicrofacetBeckmann_Evaluate(dg, wi, *wo, TEXTURE_ARGS); \n"\
"} \n"\
" \n"\
"/* \n"\
" Microfacet GGX \n"\
" */ \n"\
" // Distribution fucntion \n"\
"float MicrofacetDistribution_GGX_D(float roughness, float3 m) \n"\
"{ \n"\
"    float ndotm = fabs(m.y); \n"\
"    float ndotm2 = ndotm * ndotm; \n"\
"    float sinmn = native_sqrt(1.f - clamp(ndotm * ndotm, 0.f, 1.f)); \n"\
"    float tanmn = ndotm > DENOM_EPS ? sinmn / ndotm : 0.f; \n"\
"    float a2 = roughness * roughness; \n"\
"    float denom = (PI * ndotm2 * ndotm2 * (a2 + tanmn * tanmn) * (a2 + tanmn * tanmn)); \n"\
"    return denom > DENOM_EPS ? (a2 / denom) : 1.f; \n"\
"} \n"\
" \n"\
"// PDF of the given direction \n"\
"float MicrofacetDistribution_GGX_GetPdf( \n"\
"    // Halfway vector \n"\
"    float3 m, \n"\
"    // Rougness \n"\
"    float roughness, \n"\
"    // Geometry \n"\
"    DifferentialGeometry const* dg, \n"\
"    // Incoming direction \n"\
"    float3 wi, \n"\
"    // Outgoing direction \n"\
"    float3 wo, \n"\
"    // Texture args \n"\
"    TEXTURE_ARG_LIST \n"\
"    ) \n"\
"{ \n"\
"    float mpdf = MicrofacetDistribution_GGX_D(roughness, m) * fabs(m.y); \n"\
"    // See Humphreys and Pharr for derivation \n"\
"    float denom = (4.f * fabs(dot(wo, m))); \n"\
" \n"\
"    return denom > DENOM_EPS ? mpdf / denom : 0.f; \n"\
"} \n"\
" \n"\
"// Sample the distribution \n"\
"void MicrofacetDistribution_GGX_SampleNormal( \n"\
"    // Roughness \n"\
"    float roughness, \n"\
"    // Differential geometry \n"\
"    DifferentialGeometry const* dg, \n"\
"    // Texture args \n"\
"    TEXTURE_ARG_LIST, \n"\
"    // Sample \n"\
"    float2 sample, \n"\
"    // Outgoing  direction \n"\
"    float3* wh \n"\
"    ) \n"\
"{ \n"\
"    float r1 = sample.x; \n"\
"    float r2 = sample.y; \n"\
" \n"\
"    // Sample halfway vector first, then reflect wi around that \n"\
"    float theta = atan2(roughness * native_sqrt(r1), native_sqrt(1.f - r1)); \n"\
"    float costheta = native_cos(theta); \n"\
"    float sintheta = native_sin(theta); \n"\
" \n"\
"    // phi = 2*PI*ksi2 \n"\
"    float phi = 2.f * PI * r2; \n"\
"    float cosphi = native_cos(phi); \n"\
"    float sinphi = native_sin(phi); \n"\
" \n"\
"    // Calculate wh \n"\
"    *wh = make_float3(sintheta * cosphi, costheta, sintheta * sinphi); \n"\
"} \n"\
" \n"\
"// \n"\
"float MicrofacetDistribution_GGX_G1(float roughness, float3 v, float3 m) \n"\
"{ \n"\
"    float ndotv = fabs(v.y); \n"\
"    float mdotv = fabs(dot(m, v)); \n"\
" \n"\
"    float sinnv = native_sqrt(1.f - clamp(ndotv * ndotv, 0.f, 1.f)); \n"\
"    float tannv = ndotv > DENOM_EPS ? sinnv / ndotv : 0.f; \n"\
"    float a2 = roughness * roughness; \n"\
"    return 2.f / (1.f + native_sqrt(1.f + a2 * tannv * tannv)); \n"\
"} \n"\
" \n"\
"// Shadowing function also depends on microfacet distribution \n"\
"float MicrofacetDistribution_GGX_G(float roughness, float3 wi, float3 wo, float3 wh) \n"\
"{ \n"\
"    return MicrofacetDistribution_GGX_G1(roughness, wi, wh) * MicrofacetDistribution_GGX_G1(roughness, wo, wh); \n"\
"} \n"\
" \n"\
"float3 MicrofacetGGX_Evaluate( \n"\
"    // Geometry \n"\
"    DifferentialGeometry const* dg,  \n"\
"    // Incoming direction \n"\
"    float3 wi, \n"\
"    // Outgoing direction \n"\
"    float3 wo, \n"\
"    // Texture args \n"\
"    TEXTURE_ARG_LIST \n"\
"    ) \n"\
"{ \n"\
"    const float3 ks = Texture_GetValue3f(dg->mat.simple.kx.xyz, dg->uv, TEXTURE_ARGS_IDX(dg->mat.simple.kxmapidx)); \n"\
"    const float roughness = Texture_GetValue1f(dg->mat.simple.ns, dg->uv, TEXTURE_ARGS_IDX(dg->mat.simple.nsmapidx)); \n"\
" \n"\
"    // Incident and reflected zenith angles \n"\
"    float costhetao = fabs(wo.y); \n"\
"    float costhetai = fabs(wi.y); \n"\
" \n"\
"    // Calc halfway vector \n"\
"    float3 wh = normalize(wi + wo); \n"\
" \n"\
"    float F = dg->mat.simple.fresnel; \n"\
" \n"\
"    float denom = (4.f * costhetao * costhetai); \n"\
"     \n"\
"    return denom > DENOM_EPS ? F * ks * MicrofacetDistribution_GGX_G(roughness, wi, wo, wh) * MicrofacetDistribution_GGX_D(roughness, wh) / denom : 0.f; \n"\
"} \n"\
" \n"\
" \n"\
"float MicrofacetGGX_GetPdf( \n"\
"    // Geometry \n"\
"    DifferentialGeometry const* dg, \n"\
"    // Incoming direction \n"\
"    float3 wi, \n"\
"    // Outgoing direction \n"\
"    float3 wo, \n"\
"    // Texture args \n"\
"    TEXTURE_ARG_LIST \n"\
"    ) \n"\
"{ \n"\
"    const float roughness = Texture_GetValue1f(dg->mat.simple.ns, dg->uv, TEXTURE_ARGS_IDX(dg->mat.simple.nsmapidx)); \n"\
" \n"\
"    float3 wh = normalize(wo + wi); \n"\
" \n"\
"    return MicrofacetDistribution_GGX_GetPdf(wh, roughness, dg, wi, wo, TEXTURE_ARGS); \n"\
"} \n"\
" \n"\
"float3 MicrofacetGGX_Sample( \n"\
"    // Geometry \n"\
"    DifferentialGeometry const* dg, \n"\
"    // Incoming direction \n"\
"    float3 wi, \n"\
"    // Texture args \n"\
"    TEXTURE_ARG_LIST, \n"\
"    // Sample \n"\
"    float2 sample, \n"\
"    // Outgoing  direction \n"\
"    float3* wo, \n"\
"    // PDF at wo \n"\
"    float* pdf \n"\
"    ) \n"\
"{ \n"\
"    const float roughness = Texture_GetValue1f(dg->mat.simple.ns, dg->uv, TEXTURE_ARGS_IDX(dg->mat.simple.nsmapidx)); \n"\
" \n"\
"    float3 wh; \n"\
"    MicrofacetDistribution_GGX_SampleNormal(roughness, dg, TEXTURE_ARGS, sample, &wh); \n"\
" \n"\
"    *wo = -wi + 2.f*fabs(dot(wi, wh)) * wh; \n"\
" \n"\
"    *pdf = MicrofacetDistribution_GGX_GetPdf(wh, roughness, dg, wi, *wo, TEXTURE_ARGS); \n"\
" \n"\
"    return MicrofacetGGX_Evaluate(dg, wi, *wo, TEXTURE_ARGS); \n"\
"} \n"\
" \n"\
" \n"\
"/* \n"\
" Lambert BRDF \n"\
" */ \n"\
" /// Lambert BRDF evaluation \n"\
"float3 Lambert_Evaluate( \n"\
"    // Geometry \n"\
"    DifferentialGeometry const* dg, \n"\
"    // Incoming direction \n"\
"    float3 wi, \n"\
"    // Outgoing direction \n"\
"    float3 wo, \n"\
"    // Texture args \n"\
"    TEXTURE_ARG_LIST \n"\
"    ) \n"\
"{ \n"\
"    const float3 kd = Texture_GetValue3f(dg->mat.simple.kx.xyz, dg->uv, TEXTURE_ARGS_IDX(dg->mat.simple.kxmapidx)); \n"\
" \n"\
"    float F = dg->mat.simple.fresnel; \n"\
" \n"\
"    return F * kd / PI; \n"\
"} \n"\
" \n"\
"/// Lambert BRDF PDF \n"\
"float Lambert_GetPdf( \n"\
"    // Geometry \n"\
"    DifferentialGeometry const* dg, \n"\
"    // Incoming direction \n"\
"    float3 wi, \n"\
"    // Outgoing direction \n"\
"    float3 wo, \n"\
"    // Texture args \n"\
"    TEXTURE_ARG_LIST \n"\
"    ) \n"\
"{ \n"\
"    return fabs(wo.y) / PI; \n"\
"} \n"\
" \n"\
"/// Lambert BRDF sampling \n"\
"float3 Lambert_Sample( \n"\
"    // Geometry \n"\
"    DifferentialGeometry const* dg, \n"\
"    // Incoming direction \n"\
"    float3 wi, \n"\
"    // Texture args \n"\
"    TEXTURE_ARG_LIST, \n"\
"    // Sample \n"\
"    float2 sample, \n"\
"    // Outgoing  direction \n"\
"    float3* wo, \n"\
"    // PDF at wo \n"\
"    float* pdf \n"\
"    ) \n"\
"{ \n"\
"    const float3 kd = Texture_GetValue3f(dg->mat.simple.kx.xyz, dg->uv, TEXTURE_ARGS_IDX(dg->mat.simple.kxmapidx)); \n"\
" \n"\
"    *wo = Sample_MapToHemisphere(sample, make_float3(0.f, 1.f, 0.f) , 1.f); \n"\
" \n"\
"    float F = dg->mat.simple.fresnel; \n"\
" \n"\
"    *pdf = fabs((*wo).y) / PI; \n"\
" \n"\
"    return F * kd / PI; \n"\
"} \n"\
" \n"\
"/* \n"\
" Ideal reflection BRDF \n"\
" */ \n"\
"float3 IdealReflect_Evaluate( \n"\
"    // Geometry \n"\
"    DifferentialGeometry const* dg, \n"\
"    // Incoming direction \n"\
"    float3 wi, \n"\
"    // Outgoing direction \n"\
"    float3 wo, \n"\
"    // Texture args \n"\
"    TEXTURE_ARG_LIST \n"\
"    ) \n"\
"{ \n"\
"    return 0.f; \n"\
"} \n"\
" \n"\
"/// Lambert BRDF sampling \n"\
"float3 Translucent_Sample( \n"\
"    // Geometry \n"\
"    DifferentialGeometry const* dg, \n"\
"    // Incoming direction \n"\
"    float3 wi, \n"\
"    // Texture args \n"\
"    TEXTURE_ARG_LIST, \n"\
"    // Sample \n"\
"    float2 sample, \n"\
"    // Outgoing  direction \n"\
"    float3* wo, \n"\
"    // PDF at wo \n"\
"    float* pdf \n"\
"    ) \n"\
"{ \n"\
"    const float3 kd = Texture_GetValue3f(dg->mat.simple.kx.xyz, dg->uv, TEXTURE_ARGS_IDX(dg->mat.simple.kxmapidx)); \n"\
" \n"\
"    float ndotwi = wi.y; \n"\
" \n"\
"    float3 n = ndotwi > DENOM_EPS ? make_float3(0.f, -1.f, 0.f) : make_float3(0.f, 1.f, 0.f); \n"\
" \n"\
"    *wo = normalize(Sample_MapToHemisphere(sample, n, 1.f)); \n"\
" \n"\
"    *pdf = fabs((*wo).y) / PI; \n"\
" \n"\
"    return kd / PI; \n"\
"} \n"\
" \n"\
"// Lambert BRDF evaluation \n"\
"float3 Translucent_Evaluate( \n"\
"    // Geometry \n"\
"    DifferentialGeometry const* dg, \n"\
"    // Incoming direction \n"\
"    float3 wi, \n"\
"    // Outgoing direction \n"\
"    float3 wo, \n"\
"    // Texture args \n"\
"    TEXTURE_ARG_LIST \n"\
"    ) \n"\
"{ \n"\
"    const float3 kd = Texture_GetValue3f(dg->mat.simple.kx.xyz, dg->uv, TEXTURE_ARGS_IDX(dg->mat.simple.kxmapidx)); \n"\
" \n"\
"    float ndotwi = wi.y; \n"\
"    float ndotwo = wo.y; \n"\
" \n"\
"    if (ndotwi * ndotwo > 0.f) \n"\
"        return 0.f; \n"\
" \n"\
"    return kd / PI; \n"\
"} \n"\
" \n"\
"/// Lambert BRDF PDF \n"\
"float Translucent_GetPdf( \n"\
"    // Geometry \n"\
"    DifferentialGeometry const* dg, \n"\
"    // Incoming direction \n"\
"    float3 wi, \n"\
"    // Outgoing direction \n"\
"    float3 wo, \n"\
"    // Texture args \n"\
"    TEXTURE_ARG_LIST \n"\
"    ) \n"\
"{ \n"\
"    float ndotwi = wi.y; \n"\
"    float ndotwo = wo.y; \n"\
" \n"\
"    if (ndotwi * ndotwo > 0) \n"\
"        return 0.f; \n"\
" \n"\
"    return fabs(ndotwo) / PI; \n"\
"} \n"\
" \n"\
"float IdealReflect_GetPdf( \n"\
"    // Geometry \n"\
"    DifferentialGeometry const* dg, \n"\
"    // Incoming direction \n"\
"    float3 wi, \n"\
"    // Outgoing direction \n"\
"    float3 wo, \n"\
"    // Texture args \n"\
"    TEXTURE_ARG_LIST \n"\
"    ) \n"\
"{ \n"\
"    return 0.f; \n"\
"} \n"\
" \n"\
"float3 IdealReflect_Sample( \n"\
"    // Geometry \n"\
"    DifferentialGeometry const* dg, \n"\
"    // Incoming direction \n"\
"    float3 wi, \n"\
"    // Texture args \n"\
"    TEXTURE_ARG_LIST, \n"\
"    // Sample \n"\
"    float2 sample, \n"\
"    // Outgoing  direction \n"\
"    float3* wo, \n"\
"    // PDF at wo \n"\
"    float* pdf \n"\
"    ) \n"\
"{ \n"\
"    const float3 ks = Texture_GetValue3f(dg->mat.simple.kx.xyz, dg->uv, TEXTURE_ARGS_IDX(dg->mat.simple.kxmapidx)); \n"\
"    const float eta = dg->mat.simple.ni; \n"\
" \n"\
"    // Mirror reflect wi \n"\
"    *wo = normalize(make_float3(-wi.x, wi.y, -wi.z)); \n"\
" \n"\
"    // PDF is infinite at that point, but deltas are going to cancel out while evaluating \n"\
"    // so set it to 1.f \n"\
"    *pdf = 1.f; \n"\
" \n"\
"    float F = dg->mat.simple.fresnel; \n"\
" \n"\
"    float coswo = fabs((*wo).y); \n"\
" \n"\
"    // Return reflectance value \n"\
"    return coswo > DENOM_EPS ? (F * ks * (1.f / coswo)) : 0.f; \n"\
"} \n"\
" \n"\
"/* \n"\
" Ideal refraction BTDF \n"\
" */ \n"\
" \n"\
"float3 IdealRefract_Evaluate( \n"\
"    // Geometry \n"\
"    DifferentialGeometry const* dg, \n"\
"    // Incoming direction \n"\
"    float3 wi, \n"\
"    // Outgoing direction \n"\
"    float3 wo, \n"\
"    // Texture args \n"\
"    TEXTURE_ARG_LIST \n"\
"    ) \n"\
"{ \n"\
"    return 0.f; \n"\
"} \n"\
" \n"\
"float IdealRefract_GetPdf( \n"\
"    // Geometry \n"\
"    DifferentialGeometry const* dg, \n"\
"    // Incoming direction \n"\
"    float3 wi, \n"\
"    // Outgoing direction \n"\
"    float3 wo, \n"\
"    // Texture args \n"\
"    TEXTURE_ARG_LIST \n"\
"    ) \n"\
"{ \n"\
"    return 0.f; \n"\
"} \n"\
" \n"\
"float3 IdealRefract_Sample( \n"\
"    // Geometry \n"\
"    DifferentialGeometry const* dg, \n"\
"    // Incoming direction \n"\
"    float3 wi, \n"\
"    // Texture args \n"\
"    TEXTURE_ARG_LIST, \n"\
"    // Sample \n"\
"    float2 sample, \n"\
"    // Outgoing  direction \n"\
"    float3* wo, \n"\
"    // PDF at wo \n"\
"    float* pdf \n"\
"    ) \n"\
"{ \n"\
"    const float3 ks = Texture_GetValue3f(dg->mat.simple.kx.xyz, dg->uv, TEXTURE_ARGS_IDX(dg->mat.simple.kxmapidx)); \n"\
" \n"\
"    float etai = 1.f; \n"\
"    float etat = dg->mat.simple.ni; \n"\
"    float cosi = wi.y; \n"\
" \n"\
"    bool entering = cosi > 0.f; \n"\
" \n"\
"    // Revert normal and eta if needed \n"\
"    if (!entering) \n"\
"    { \n"\
"        float tmp = etai; \n"\
"        etai = etat; \n"\
"        etat = tmp; \n"\
"    } \n"\
" \n"\
"    float eta = etai / etat; \n"\
"    float sini2 = 1.f - cosi * cosi; \n"\
"    float sint2 = eta * eta * sini2; \n"\
" \n"\
"    if (sint2 >= 1.f) \n"\
"    { \n"\
"        *pdf = 0.f; \n"\
"        return 0.f; \n"\
"    } \n"\
" \n"\
"    float cost = native_sqrt(max(0.f, 1.f - sint2)); \n"\
" \n"\
"    // Transmitted ray \n"\
"    float F = dg->mat.simple.fresnel; \n"\
" \n"\
"    *wo = normalize(make_float3(eta * -wi.x, entering ? -cost : cost, eta * -wi.z)); \n"\
" \n"\
"    *pdf = 1.f; \n"\
" \n"\
"    return cost > DENOM_EPS ? (F * eta * eta * ks / cost) : 0.f; \n"\
"} \n"\
" \n"\
" \n"\
"float3 MicrofacetRefractionGGX_Evaluate( \n"\
"    // Geometry \n"\
"    DifferentialGeometry const* dg, \n"\
"    // Incoming direction \n"\
"    float3 wi, \n"\
"    // Outgoing direction \n"\
"    float3 wo, \n"\
"    // Texture args \n"\
"    TEXTURE_ARG_LIST \n"\
"    ) \n"\
"{ \n"\
"    const float3 ks = Texture_GetValue3f(dg->mat.simple.kx.xyz, dg->uv, TEXTURE_ARGS_IDX(dg->mat.simple.kxmapidx)); \n"\
"    const float roughness = max(Texture_GetValue1f(dg->mat.simple.ns, dg->uv, TEXTURE_ARGS_IDX(dg->mat.simple.nsmapidx)), ROUGHNESS_EPS); \n"\
" \n"\
"    float ndotwi = wi.y; \n"\
"    float ndotwo = wo.y; \n"\
" \n"\
"    if (ndotwi * ndotwo >= 0.f) \n"\
"    { \n"\
"        return 0.f; \n"\
"    } \n"\
" \n"\
"    float etai = 1.f; \n"\
"    float etat = dg->mat.simple.ni; \n"\
" \n"\
"    // Revert normal and eta if needed \n"\
"    if (ndotwi < 0.f) \n"\
"    { \n"\
"        float tmp = etai; \n"\
"        etai = etat; \n"\
"        etat = tmp; \n"\
"    } \n"\
" \n"\
"    // Calc halfway vector \n"\
"    float3 ht = -(etai * wi + etat * wo); \n"\
"    float3 wh = normalize(ht); \n"\
" \n"\
"    float widotwh = fabs(dot(wh, wi)); \n"\
"    float wodotwh = fabs(dot(wh, wo)); \n"\
" \n"\
"    float F = dg->mat.simple.fresnel; \n"\
" \n"\
"    float denom = dot(ht, ht); \n"\
"    denom *= (fabs(ndotwi) * fabs(ndotwo)); \n"\
" \n"\
"    return denom > DENOM_EPS ? (F * ks * (widotwh * wodotwh)  * (etat)* (etat)* \n"\
"        MicrofacetDistribution_GGX_G(roughness, wi, wo, wh) * MicrofacetDistribution_GGX_D(roughness, wh) / denom) : 0.f; \n"\
"} \n"\
" \n"\
" \n"\
" \n"\
"float MicrofacetRefractionGGX_GetPdf( \n"\
"    // Geometry \n"\
"    DifferentialGeometry const* dg, \n"\
"    // Incoming direction \n"\
"    float3 wi, \n"\
"    // Outgoing direction \n"\
"    float3 wo, \n"\
"    // Texture args \n"\
"    TEXTURE_ARG_LIST \n"\
"    ) \n"\
"{ \n"\
"    const float roughness = max(Texture_GetValue1f(dg->mat.simple.ns, dg->uv, TEXTURE_ARGS_IDX(dg->mat.simple.nsmapidx)), ROUGHNESS_EPS); \n"\
"    float ndotwi = wi.y; \n"\
"    float ndotwo = wo.y; \n"\
" \n"\
"    float etai = 1.f; \n"\
"    float etat = dg->mat.simple.ni; \n"\
" \n"\
"    if (ndotwi * ndotwo >= 0.f) \n"\
"    { \n"\
"        return 0.f; \n"\
"    } \n"\
" \n"\
"    // Revert normal and eta if needed \n"\
"    if (ndotwi < 0.f) \n"\
"    { \n"\
"        float tmp = etai; \n"\
"        etai = etat; \n"\
"        etat = tmp; \n"\
"    } \n"\
" \n"\
"    // Calc halfway vector \n"\
"    float3 ht = -(etai * wi + etat * wo); \n"\
" \n"\
"    float3 wh = normalize(ht); \n"\
" \n"\
"    float wodotwh = fabs(dot(wo, wh)); \n"\
" \n"\
"    float whpdf = MicrofacetDistribution_GGX_D(roughness, wh) * fabs(wh.y); \n"\
" \n"\
"    float whwo = wodotwh * etat * etat; \n"\
" \n"\
"    float denom = dot(ht, ht); \n"\
" \n"\
"    return denom > DENOM_EPS ? whpdf * whwo / denom : 0.f; \n"\
"} \n"\
" \n"\
"float3 MicrofacetRefractionGGX_Sample( \n"\
"    // Geometry \n"\
"    DifferentialGeometry const* dg, \n"\
"    // Incoming direction \n"\
"    float3 wi, \n"\
"    // Texture args \n"\
"    TEXTURE_ARG_LIST, \n"\
"    // Sample \n"\
"    float2 sample, \n"\
"    // Outgoing  direction \n"\
"    float3* wo, \n"\
"    // PDF at wo \n"\
"    float* pdf \n"\
"    ) \n"\
"{ \n"\
"    const float3 ks = Texture_GetValue3f(dg->mat.simple.kx.xyz, dg->uv, TEXTURE_ARGS_IDX(dg->mat.simple.kxmapidx)); \n"\
"    const float roughness = max(Texture_GetValue1f(dg->mat.simple.ns, dg->uv, TEXTURE_ARGS_IDX(dg->mat.simple.nsmapidx)), ROUGHNESS_EPS); \n"\
" \n"\
"    float ndotwi = wi.y; \n"\
" \n"\
"    if (ndotwi == 0.f) \n"\
"    { \n"\
"        *pdf = 0.f; \n"\
"        return 0.f; \n"\
"    } \n"\
" \n"\
"    float etai = 1.f; \n"\
"    float etat = dg->mat.simple.ni; \n"\
"    float s = 1.f; \n"\
" \n"\
"    // Revert normal and eta if needed \n"\
"    if (ndotwi < 0.f) \n"\
"    { \n"\
"        float tmp = etai; \n"\
"        etai = etat; \n"\
"        etat = tmp; \n"\
"        s = -s; \n"\
"    } \n"\
" \n"\
"    float3 wh; \n"\
"    MicrofacetDistribution_GGX_SampleNormal(roughness, dg, TEXTURE_ARGS, sample, &wh); \n"\
" \n"\
"    float c = dot(wi, wh); \n"\
"    float eta = etai / etat; \n"\
" \n"\
"    float d = 1 + eta * (c * c - 1); \n"\
" \n"\
"    if (d <= 0.f) \n"\
"    { \n"\
"        *pdf = 0.f; \n"\
"        return 0.f; \n"\
"    } \n"\
" \n"\
"    *wo = normalize((eta * c - s * native_sqrt(d)) * wh - eta * wi); \n"\
" \n"\
"    *pdf = MicrofacetRefractionGGX_GetPdf(dg, wi, *wo, TEXTURE_ARGS); \n"\
" \n"\
"    return MicrofacetRefractionGGX_Evaluate(dg, wi, *wo, TEXTURE_ARGS); \n"\
"} \n"\
" \n"\
"float3 MicrofacetRefractionBeckmann_Evaluate( \n"\
"    // Geometry \n"\
"    DifferentialGeometry const* dg, \n"\
"    // Incoming direction \n"\
"    float3 wi, \n"\
"    // Outgoing direction \n"\
"    float3 wo, \n"\
"    // Texture args \n"\
"    TEXTURE_ARG_LIST \n"\
"    ) \n"\
"{ \n"\
"    const float3 ks = Texture_GetValue3f(dg->mat.simple.kx.xyz, dg->uv, TEXTURE_ARGS_IDX(dg->mat.simple.kxmapidx)); \n"\
"    const float roughness = max(Texture_GetValue1f(dg->mat.simple.ns, dg->uv, TEXTURE_ARGS_IDX(dg->mat.simple.nsmapidx)), ROUGHNESS_EPS); \n"\
" \n"\
"    float ndotwi = wi.y; \n"\
"    float ndotwo = wo.y; \n"\
" \n"\
"    float etai = 1.f; \n"\
"    float etat = dg->mat.simple.ni; \n"\
" \n"\
"    // Revert normal and eta if needed \n"\
"    if (ndotwi < 0.f) \n"\
"    { \n"\
"        float tmp = etai; \n"\
"        etai = etat; \n"\
"        etat = tmp; \n"\
"    } \n"\
" \n"\
"    // Calc halfway vector \n"\
"    float3 ht = -(etai * wi + etat * wo); \n"\
"    float3 wh = normalize(ht); \n"\
" \n"\
"    float widotwh = fabs(dot(wh, wi)); \n"\
"    float wodotwh = fabs(dot(wh, wo)); \n"\
" \n"\
"    float F = dg->mat.simple.fresnel; \n"\
" \n"\
"    float denom = dot(ht, ht); \n"\
"    denom *= (fabs(ndotwi) * fabs(ndotwo)); \n"\
" \n"\
"    return denom > DENOM_EPS ? (F * ks * (widotwh * wodotwh)  * (etat)* (etat)* \n"\
"        MicrofacetDistribution_Beckmann_G(roughness, wi, wo, wh) * MicrofacetDistribution_Beckmann_D(roughness, wh) / denom) : 0.f; \n"\
"} \n"\
" \n"\
" \n"\
" \n"\
"float MicrofacetRefractionBeckmann_GetPdf( \n"\
"    // Geometry \n"\
"    DifferentialGeometry const* dg, \n"\
"    // Incoming direction \n"\
"    float3 wi, \n"\
"    // Outgoing direction \n"\
"    float3 wo, \n"\
"    // Texture args \n"\
"    TEXTURE_ARG_LIST \n"\
"    ) \n"\
"{ \n"\
"    const float roughness = Texture_GetValue1f(dg->mat.simple.ns, dg->uv, TEXTURE_ARGS_IDX(dg->mat.simple.nsmapidx)); \n"\
"    float ndotwi = wi.y; \n"\
"    float ndotwo = wo.y; \n"\
" \n"\
"    float etai = 1.f; \n"\
"    float etat = dg->mat.simple.ni; \n"\
" \n"\
"    // Revert normal and eta if needed \n"\
"    if (ndotwi < 0.f) \n"\
"    { \n"\
"        float tmp = etai; \n"\
"        etai = etat; \n"\
"        etat = tmp; \n"\
"    } \n"\
" \n"\
"    // Calc halfway vector \n"\
"    float3 ht = -(etai * wi + etat * wo); \n"\
" \n"\
"    float3 wh = normalize(ht); \n"\
" \n"\
"    float wodotwh = fabs(dot(wo, wh)); \n"\
" \n"\
"    float whpdf = MicrofacetDistribution_Beckmann_D(roughness, wh) * fabs(wh.y); \n"\
" \n"\
"    float whwo = wodotwh * etat * etat; \n"\
" \n"\
"    float denom = dot(ht, ht); \n"\
" \n"\
"    return denom > DENOM_EPS ? whpdf * whwo / denom : 0.f; \n"\
"} \n"\
" \n"\
"float3 MicrofacetRefractionBeckmann_Sample( \n"\
"    // Geometry \n"\
"    DifferentialGeometry const* dg, \n"\
"    // Incoming direction \n"\
"    float3 wi, \n"\
"    // Texture args \n"\
"    TEXTURE_ARG_LIST, \n"\
"    // Sample \n"\
"    float2 sample, \n"\
"    // Outgoing  direction \n"\
"    float3* wo, \n"\
"    // PDF at wo \n"\
"    float* pdf \n"\
"    ) \n"\
"{ \n"\
"    const float3 ks = Texture_GetValue3f(dg->mat.simple.kx.xyz, dg->uv, TEXTURE_ARGS_IDX(dg->mat.simple.kxmapidx)); \n"\
"    const float roughness = Texture_GetValue1f(dg->mat.simple.ns, dg->uv, TEXTURE_ARGS_IDX(dg->mat.simple.nsmapidx)); \n"\
" \n"\
"    float ndotwi = wi.y; \n"\
" \n"\
"    float etai = 1.f; \n"\
"    float etat = dg->mat.simple.ni; \n"\
"    float s = 1.f; \n"\
" \n"\
"    // Revert normal and eta if needed \n"\
"    if (ndotwi < 0.f) \n"\
"    { \n"\
"        float tmp = etai; \n"\
"        etai = etat; \n"\
"        etat = tmp; \n"\
"        s = -s; \n"\
"    } \n"\
" \n"\
"    float3 wh; \n"\
"    MicrofacetDistribution_Beckmann_SampleNormal(roughness, dg, TEXTURE_ARGS, sample, &wh); \n"\
" \n"\
"    float c = dot(wi, wh); \n"\
"    float eta = etai / etat; \n"\
" \n"\
"    float d = 1 + eta * (c * c - 1); \n"\
" \n"\
"    if (d <= 0) \n"\
"    { \n"\
"        *pdf = 0.f; \n"\
"        return 0.f; \n"\
"    } \n"\
" \n"\
"    *wo = normalize((eta * c - s * native_sqrt(d)) * wh - eta * wi); \n"\
" \n"\
"    *pdf = MicrofacetRefractionBeckmann_GetPdf(dg, wi, *wo, TEXTURE_ARGS); \n"\
" \n"\
"    return MicrofacetRefractionBeckmann_Evaluate(dg, wi, *wo, TEXTURE_ARGS); \n"\
"} \n"\
" \n"\
"float3 Passthrough_Sample( \n"\
"    // Geometry \n"\
"    DifferentialGeometry const* dg, \n"\
"    // Incoming direction \n"\
"    float3 wi, \n"\
"    // Texture args \n"\
"    TEXTURE_ARG_LIST, \n"\
"    // Sample \n"\
"    float2 sample, \n"\
"    // Outgoing  direction \n"\
"    float3* wo, \n"\
"    // PDF at wo \n"\
"    float* pdf \n"\
"    ) \n"\
"{ \n"\
" \n"\
"    *wo = -wi; \n"\
"    float coswo = fabs((*wo).y); \n"\
" \n"\
"    // PDF is infinite at that point, but deltas are going to cancel out while evaluating \n"\
"    // so set it to 1.f \n"\
"    *pdf = 1.f; \n"\
" \n"\
"    // \n"\
"    return coswo > 1e-5f ? (1.f / coswo) : 0.f; \n"\
"} \n"\
" \n"\
"/* \n"\
" Dispatch functions \n"\
" */ \n"\
"float3 Bxdf_Evaluate( \n"\
"    // Geometry \n"\
"    DifferentialGeometry const* dg, \n"\
"    // Incoming direction \n"\
"    float3 wi, \n"\
"    // Outgoing direction \n"\
"    float3 wo, \n"\
"    // Texture args \n"\
"    TEXTURE_ARG_LIST \n"\
"    ) \n"\
"{ \n"\
"    // Transform vectors into tangent space \n"\
"    float3 wi_t = matrix_mul_vector3(dg->world_to_tangent, wi); \n"\
"    float3 wo_t = matrix_mul_vector3(dg->world_to_tangent, wo); \n"\
" \n"\
"    int mattype = dg->mat.type; \n"\
"    switch (mattype) \n"\
"    { \n"\
"    case kLambert: \n"\
"        return Lambert_Evaluate(dg, wi_t, wo_t, TEXTURE_ARGS); \n"\
"    case kMicrofacetGGX: \n"\
"        return MicrofacetGGX_Evaluate(dg, wi_t, wo_t, TEXTURE_ARGS); \n"\
"    case kMicrofacetBeckmann: \n"\
"        return MicrofacetBeckmann_Evaluate(dg, wi_t, wo_t, TEXTURE_ARGS); \n"\
"    case kIdealReflect: \n"\
"        return IdealReflect_Evaluate(dg, wi_t, wo_t, TEXTURE_ARGS); \n"\
"    case kIdealRefract: \n"\
"        return IdealRefract_Evaluate(dg, wi_t, wo_t, TEXTURE_ARGS); \n"\
"    case kTranslucent: \n"\
"        return Translucent_Evaluate(dg, wi_t, wo_t, TEXTURE_ARGS); \n"\
"    case kMicrofacetRefractionGGX: \n"\
"        return MicrofacetRefractionGGX_Evaluate(dg, wi_t, wo_t, TEXTURE_ARGS); \n"\
"    case kMicrofacetRefractionBeckmann: \n"\
"        return MicrofacetRefractionBeckmann_Evaluate(dg, wi_t, wo_t, TEXTURE_ARGS); \n"\
"    case kPassthrough: \n"\
"        return 0.f; \n"\
"#ifdef ENABLE_DISNEY \n"\
"    case kDisney: \n"\
"        return Disney_Evaluate(dg, wi_t, wo_t, TEXTURE_ARGS); \n"\
"#endif \n"\
"    } \n"\
" \n"\
"    return 0.f; \n"\
"} \n"\
" \n"\
"float3 Bxdf_Sample( \n"\
"    // Geometry \n"\
"    DifferentialGeometry const* dg, \n"\
"    // Incoming direction \n"\
"    float3 wi, \n"\
"    // Texture args \n"\
"    TEXTURE_ARG_LIST, \n"\
"    // RNG \n"\
"    float2 sample, \n"\
"    // Outgoing  direction \n"\
"    float3* wo, \n"\
"    // PDF at w \n"\
"    float* pdf \n"\
"    ) \n"\
"{ \n"\
"    // Transform vectors into tangent space \n"\
"    float3 wi_t = matrix_mul_vector3(dg->world_to_tangent, wi); \n"\
"    float3 wo_t; \n"\
" \n"\
"    float3 res = 0.f; \n"\
" \n"\
"    int mattype = dg->mat.type; \n"\
"    switch (mattype) \n"\
"    { \n"\
"    case kLambert: \n"\
"        res = Lambert_Sample(dg, wi_t, TEXTURE_ARGS, sample, &wo_t, pdf); \n"\
"        break; \n"\
"    case kMicrofacetGGX: \n"\
"        res = MicrofacetGGX_Sample(dg, wi_t, TEXTURE_ARGS, sample, &wo_t, pdf); \n"\
"        break; \n"\
"    case kMicrofacetBeckmann: \n"\
"        res = MicrofacetBeckmann_Sample(dg, wi_t, TEXTURE_ARGS, sample, &wo_t, pdf); \n"\
"        break; \n"\
"    case kIdealReflect: \n"\
"        res = IdealReflect_Sample(dg, wi_t, TEXTURE_ARGS, sample, &wo_t, pdf); \n"\
"        break; \n"\
"    case kIdealRefract: \n"\
"        res = IdealRefract_Sample(dg, wi_t, TEXTURE_ARGS, sample, &wo_t, pdf); \n"\
"        break; \n"\
"    case kTranslucent: \n"\
"        res = Translucent_Sample(dg, wi_t, TEXTURE_ARGS, sample, &wo_t, pdf); \n"\
"        break; \n"\
"    case kPassthrough: \n"\
"        res = Passthrough_Sample(dg, wi_t, TEXTURE_ARGS, sample, &wo_t, pdf); \n"\
"        break; \n"\
"    case kMicrofacetRefractionGGX: \n"\
"        res = MicrofacetRefractionGGX_Sample(dg, wi_t, TEXTURE_ARGS, sample, &wo_t, pdf); \n"\
"        break; \n"\
"    case kMicrofacetRefractionBeckmann: \n"\
"        res = MicrofacetRefractionBeckmann_Sample(dg, wi_t, TEXTURE_ARGS, sample, &wo_t, pdf); \n"\
"        break; \n"\
"#ifdef ENABLE_DISNEY \n"\
"    case kDisney: \n"\
"        res = Disney_Sample(dg, wi_t, TEXTURE_ARGS, sample, &wo_t, pdf); \n"\
"        break; \n"\
"#endif \n"\
"    default: \n"\
"        *pdf = 0.f; \n"\
"        break; \n"\
"    } \n"\
" \n"\
"    *wo = matrix_mul_vector3(dg->tangent_to_world, wo_t); \n"\
" \n"\
"    return res; \n"\
"} \n"\
" \n"\
"float Bxdf_GetPdf( \n"\
"    // Geometry \n"\
"    DifferentialGeometry const* dg, \n"\
"    // Incoming direction \n"\
"    float3 wi, \n"\
"    // Outgoing direction \n"\
"    float3 wo, \n"\
"    // Texture args \n"\
"    TEXTURE_ARG_LIST \n"\
"    ) \n"\
"{ \n"\
"    // Transform vectors into tangent space \n"\
"    float3 wi_t = matrix_mul_vector3(dg->world_to_tangent, wi); \n"\
"    float3 wo_t = matrix_mul_vector3(dg->world_to_tangent, wo); \n"\
" \n"\
"    int mattype = dg->mat.type; \n"\
"    switch (mattype) \n"\
"    { \n"\
"    case kLambert: \n"\
"        return Lambert_GetPdf(dg, wi_t, wo_t, TEXTURE_ARGS); \n"\
"    case kMicrofacetGGX: \n"\
"        return MicrofacetGGX_GetPdf(dg, wi_t, wo_t, TEXTURE_ARGS); \n"\
"    case kMicrofacetBeckmann: \n"\
"        return MicrofacetBeckmann_GetPdf(dg, wi_t, wo_t, TEXTURE_ARGS); \n"\
"    case kIdealReflect: \n"\
"        return IdealReflect_GetPdf(dg, wi_t, wo_t, TEXTURE_ARGS); \n"\
"    case kIdealRefract: \n"\
"        return IdealRefract_GetPdf(dg, wi_t, wo_t, TEXTURE_ARGS); \n"\
"    case kTranslucent: \n"\
"        return Translucent_GetPdf(dg, wi_t, wo_t, TEXTURE_ARGS); \n"\
"    case kPassthrough: \n"\
"        return 0.f; \n"\
"    case kMicrofacetRefractionGGX: \n"\
"        return MicrofacetRefractionGGX_GetPdf(dg, wi_t, wo_t, TEXTURE_ARGS); \n"\
"    case kMicrofacetRefractionBeckmann: \n"\
"        return MicrofacetRefractionBeckmann_GetPdf(dg, wi_t, wo_t, TEXTURE_ARGS); \n"\
"#ifdef ENABLE_DISNEY \n"\
"    case kDisney: \n"\
"        return Disney_GetPdf(dg, wi_t, wo_t, TEXTURE_ARGS); \n"\
"#endif \n"\
"    } \n"\
" \n"\
"    return 0.f; \n"\
"} \n"\
" \n"\
"/// Emissive BRDF sampling \n"\
"float3 Emissive_GetLe( \n"\
"    // Geometry \n"\
"    DifferentialGeometry const* dg, \n"\
"    // Texture args \n"\
"    TEXTURE_ARG_LIST) \n"\
"{ \n"\
"    const float3 kd = Texture_GetValue3f(dg->mat.simple.kx.xyz, dg->uv, TEXTURE_ARGS_IDX(dg->mat.simple.kxmapidx)); \n"\
"    return kd; \n"\
"} \n"\
" \n"\
" \n"\
"/// BxDF singularity check \n"\
"bool Bxdf_IsSingular(DifferentialGeometry const* dg) \n"\
"{ \n"\
"    return dg->mat.type == kIdealReflect || dg->mat.type == kIdealRefract || dg->mat.type == kPassthrough; \n"\
"} \n"\
" \n"\
"/// BxDF emission check \n"\
"bool Bxdf_IsEmissive(DifferentialGeometry const* dg) \n"\
"{ \n"\
"    return dg->mat.type == kEmissive; \n"\
"} \n"\
" \n"\
"/// BxDF singularity check \n"\
"bool Bxdf_IsBtdf(DifferentialGeometry const* dg) \n"\
"{ \n"\
"    return dg->mat.type == kIdealRefract || dg->mat.type == kPassthrough || dg->mat.type == kTranslucent || \n"\
"        dg->mat.type == kMicrofacetRefractionGGX || dg->mat.type == kMicrofacetRefractionBeckmann; \n"\
"} \n"\
" \n"\
"#endif // BXDF_CL \n"\
;
static const char g_common_opencl[]= \
"/********************************************************************** \n"\
"Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved. \n"\
" \n"\
"Permission is hereby granted, free of charge, to any person obtaining a copy \n"\
"of this software and associated documentation files (the \"Software\"), to deal \n"\
"in the Software without restriction, including without limitation the rights \n"\
"to use, copy, modify, merge, publish, distribute, sublicense, and/or sell \n"\
"copies of the Software, and to permit persons to whom the Software is \n"\
"furnished to do so, subject to the following conditions: \n"\
" \n"\
"The above copyright notice and this permission notice shall be included in \n"\
"all copies or substantial portions of the Software. \n"\
" \n"\
"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR \n"\
"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, \n"\
"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE \n"\
"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER \n"\
"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, \n"\
"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN \n"\
"THE SOFTWARE. \n"\
"********************************************************************/ \n"\
"#ifndef COMMON_CL \n"\
"#define COMMON_CL \n"\
" \n"\
"#define PI 3.14159265358979323846f \n"\
"#define KERNEL __kernel \n"\
"#define GLOBAL __global \n"\
" \n"\
"#ifndef APPLE \n"\
"#define INLINE __attribute__((always_inline)) \n"\
"#endif \n"\
" \n"\
"#define HIT_MARKER 1 \n"\
"#define MISS_MARKER -1 \n"\
"#define INVALID_IDX -1 \n"\
" \n"\
"#define CRAZY_LOW_THROUGHPUT 0.0f \n"\
"#define CRAZY_HIGH_RADIANCE 30.f \n"\
"#define CRAZY_HIGH_DISTANCE 1000000.f \n"\
"#define CRAZY_LOW_DISTANCE 0.001f \n"\
"#define REASONABLE_RADIANCE(x) (clamp((x), 0.f, CRAZY_HIGH_RADIANCE)) \n"\
"#define NON_BLACK(x) (length(x) > 0.f) \n"\
" \n"\
"#define MULTISCATTER \n"\
" \n"\
"#define RANDOM 1 \n"\
"#define SOBOL 2 \n"\
"#define CMJ 3 \n"\
" \n"\
"#define SAMPLER CMJ \n"\
" \n"\
"#define CMJ_DIM 16 \n"\
" \n"\
"#define BDPT_MAX_SUBPATH_LEN 3 \n"\
" \n"\
"#ifdef BAIKAL_ATOMIC_RESOLVE \n"\
"#define ADD_FLOAT3(x,y) atomic_add_float3((x),(y)) \n"\
"#define ADD_FLOAT4(x,y) atomic_add_float4((x),(y)) \n"\
"#else \n"\
"#define ADD_FLOAT3(x,y) add_float3((x),(y)) \n"\
"#define ADD_FLOAT4(x,y) add_float4((x),(y)) \n"\
"#endif \n"\
" \n"\
"#define VISIBILITY_MASK_PRIMARY (0x1) \n"\
"#define VISIBILITY_MASK_SHADOW (0x1 << 15) \n"\
"#define VISIBILITY_MASK_ALL (0xffffffffu) \n"\
"#define VISIBILITY_MASK_NONE (0x0u) \n"\
"#define VISIBILITY_MASK_BOUNCE(i) (VISIBILITY_MASK_PRIMARY << i) \n"\
"#define VISIBILITY_MASK_BOUNCE_SHADOW(i) (VISIBILITY_MASK_SHADOW << i) \n"\
" \n"\
"#endif // COMMON_CL \n"\
;
static const char g_denoise_opencl[]= \
"/********************************************************************** \n"\
"Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved. \n"\
" \n"\
"Permission is hereby granted, free of charge, to any person obtaining a copy \n"\
"of this software and associated documentation files (the \"Software\"), to deal \n"\
"in the Software without restriction, including without limitation the rights \n"\
"to use, copy, modify, merge, publish, distribute, sublicense, and/or sell \n"\
"copies of the Software, and to permit persons to whom the Software is \n"\
"furnished to do so, subject to the following conditions: \n"\
" \n"\
"The above copyright notice and this permission notice shall be included in \n"\
"all copies or substantial portions of the Software. \n"\
" \n"\
"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR \n"\
"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, \n"\
"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE \n"\
"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER \n"\
"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, \n"\
"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN \n"\
"THE SOFTWARE. \n"\
"********************************************************************/ \n"\
"#ifndef DENOISE_CL \n"\
"#define DENOISE_CL \n"\
" \n"\
" \n"\
"// Similarity function \n"\
"inline float C(float3 x1, float3 x2, float sigma) \n"\
"{ \n"\
"    float a = length(x1 - x2) / sigma; \n"\
"    a *= a; \n"\
"    return native_exp(-0.5f * a); \n"\
"} \n"\
" \n"\
"// TODO: Optimize this kernel using local memory \n"\
"KERNEL \n"\
"void BilateralDenoise_main( \n"\
"    // Color data \n"\
"    GLOBAL float4 const* restrict colors, \n"\
"    // Normal data \n"\
"    GLOBAL float4 const* restrict normals, \n"\
"    // Positional data \n"\
"    GLOBAL float4 const* restrict positions, \n"\
"    // Albedo data \n"\
"    GLOBAL float4 const* restrict albedos, \n"\
"    // Image resolution \n"\
"    int width, \n"\
"    int height, \n"\
"    // Filter radius \n"\
"    int radius, \n"\
"    // Filter kernel width \n"\
"    float sigma_color, \n"\
"    float sigma_normal, \n"\
"    float sigma_position, \n"\
"    float sigma_albedo, \n"\
"    // Resulting color \n"\
"    GLOBAL float4* restrict out_colors \n"\
") \n"\
"{ \n"\
"    int2 global_id; \n"\
"    global_id.x = get_global_id(0); \n"\
"    global_id.y = get_global_id(1); \n"\
" \n"\
"    // Check borders \n"\
"    if (global_id.x < width && global_id.y < height) \n"\
"    { \n"\
"        int idx = global_id.y * width + global_id.x; \n"\
" \n"\
"        float3 color = colors[idx].w > 1 ? (colors[idx].xyz / colors[idx].w) : colors[idx].xyz; \n"\
"        float3 normal = normals[idx].w > 1 ? (normals[idx].xyz / normals[idx].w) : normals[idx].xyz; \n"\
"        float3 position = positions[idx].w > 1 ? (positions[idx].xyz / positions[idx].w) : positions[idx].xyz; \n"\
"        float3 albedo = albedos[idx].w > 1 ? (albedos[idx].xyz / albedos[idx].w) : albedos[idx].xyz; \n"\
" \n"\
"        float3 filtered_color = 0.f; \n"\
"        float sum = 0.f; \n"\
"        if (length(position) > 0.f) \n"\
"        { \n"\
"            for (int i = -radius; i <= radius; ++i) \n"\
"            { \n"\
"                for (int j = -radius; j <= radius; ++j) \n"\
"                { \n"\
"                    int cx = clamp(global_id.x + i, 0, width - 1); \n"\
"                    int cy = clamp(global_id.y + j, 0, height - 1); \n"\
"                    int ci = cy * width + cx; \n"\
" \n"\
"                    float3 c = colors[ci].xyz / colors[ci].w; \n"\
"                    float3 n = normals[ci].xyz / normals[ci].w; \n"\
"                    float3 p = positions[ci].xyz / positions[ci].w; \n"\
"                    float3 a = albedos[ci].xyz / albedos[ci].w; \n"\
" \n"\
"                    if (length(p) > 0.f) \n"\
"                    { \n"\
"                        filtered_color += c * C(p, position, sigma_position) * \n"\
"                            C(c, color, sigma_color) * \n"\
"                            C(n, normal, sigma_normal) * \n"\
"                            C(a, albedo, sigma_albedo); \n"\
"                        sum += C(p, position, sigma_position) * C(c, color, sigma_color) * \n"\
"                            C(n, normal, sigma_normal) * \n"\
"                            C(a, albedo, sigma_albedo); \n"\
"                    } \n"\
"                } \n"\
"            } \n"\
" \n"\
"            out_colors[idx].xyz = sum > 0 ? filtered_color / sum : color; \n"\
"            out_colors[idx].w = 1.f; \n"\
"        } \n"\
"        else \n"\
"        { \n"\
"            out_colors[idx].xyz = color; \n"\
"            out_colors[idx].w = 1.f; \n"\
"        } \n"\
"    } \n"\
"} \n"\
" \n"\
"#endif \n"\
;
static const char g_disney_opencl[]= \
"#ifndef DISNEY_CL \n"\
"#define DISNEY_CL \n"\
" \n"\
" \n"\
"#define DENOM_EPS 1e-8f \n"\
"#define ROUGHNESS_EPS 0.0001f \n"\
" \n"\
"#define WHITE make_float3(1.f, 1.f, 1.f) \n"\
" \n"\
"INLINE float SchlickFresnelReflectance(float u) \n"\
"{ \n"\
"    float m = clamp(1.f - u, 0.f, 1.f); \n"\
"    float m2 = m * m; \n"\
"    return m2 * m2 * m; \n"\
"} \n"\
" \n"\
"float GTR1(float ndoth, float a) \n"\
"{ \n"\
"    if (a >= 1.f) return 1.f / PI; \n"\
"     \n"\
"    float a2 = a * a; \n"\
"    float t = 1.f + (a2 - 1.f) * ndoth * ndoth; \n"\
"    return (a2 - 1.f) / (PI * log(a2) * t); \n"\
"} \n"\
" \n"\
"float GTR2(float ndoth, float a) \n"\
"{ \n"\
"    float a2 = a * a; \n"\
"    float t = 1.f + (a2 - 1.f) * ndoth * ndoth; \n"\
"    return a2 / (PI * t * t); \n"\
"} \n"\
" \n"\
"float GTR2_Aniso(float ndoth, float hdotx, float hdoty, float ax, float ay) \n"\
"{ \n"\
"    float hdotxa2 = (hdotx / ax); \n"\
"    hdotxa2 *= hdotxa2; \n"\
"    float hdotya2 = (hdoty / ay); \n"\
"    hdotya2 *= hdotya2; \n"\
"    float denom = hdotxa2 + hdotya2 + ndoth * ndoth; \n"\
"    return denom > 1e-5 ? (1.f / (PI * ax * ay * denom * denom)) : 0.f; \n"\
"} \n"\
" \n"\
" \n"\
"float SmithGGX_G(float ndotv, float a) \n"\
"{ \n"\
"    float a2 = a * a; \n"\
"    float b = ndotv * ndotv; \n"\
"    return 1.f / (ndotv + native_sqrt(a2 + b - a2 * b)); \n"\
"} \n"\
" \n"\
"float SmithGGX_G_Aniso(float ndotv, float vdotx, float vdoty, float ax, float ay) \n"\
"{ \n"\
"    float vdotxax2 = (vdotx * ax) * (vdotx * ax); \n"\
"    float vdotyay2 = (vdoty * ay) * (vdoty * ay); \n"\
"    return 1.f / (ndotv + native_sqrt( vdotxax2 + vdotyay2 + ndotv * ndotv)); \n"\
"} \n"\
" \n"\
" \n"\
"INLINE float Disney_GetPdf( \n"\
"    // Geometry \n"\
"    DifferentialGeometry const* dg, \n"\
"    // Incoming direction \n"\
"    float3 wi, \n"\
"    // Outgoing direction \n"\
"    float3 wo, \n"\
"    // Texture args \n"\
"    TEXTURE_ARG_LIST \n"\
"    ) \n"\
"{ \n"\
"    float3 base_color = Texture_GetValue3f(dg->mat.disney.base_color.xyz, dg->uv, TEXTURE_ARGS_IDX(dg->mat.disney.base_color_map_idx)); \n"\
"    float metallic = Texture_GetValue1f(dg->mat.disney.metallic, dg->uv, TEXTURE_ARGS_IDX(dg->mat.disney.metallic_map_idx)); \n"\
"    float specular = Texture_GetValue1f(dg->mat.disney.specular, dg->uv, TEXTURE_ARGS_IDX(dg->mat.disney.specular_map_idx)); \n"\
"    float anisotropy = Texture_GetValue1f(dg->mat.disney.anisotropy, dg->uv, TEXTURE_ARGS_IDX(dg->mat.disney.anisotropy_map_idx)); \n"\
"    float roughness = Texture_GetValue1f(dg->mat.disney.roughness, dg->uv, TEXTURE_ARGS_IDX(dg->mat.disney.roughness_map_idx)); \n"\
"    float specular_tint = Texture_GetValue1f(dg->mat.disney.specular_tint, dg->uv, TEXTURE_ARGS_IDX(dg->mat.disney.specular_tint_map_idx)); \n"\
"    float sheen_tint = Texture_GetValue1f(dg->mat.disney.sheen_tint, dg->uv, TEXTURE_ARGS_IDX(dg->mat.disney.sheen_tint_map_idx)); \n"\
"    float sheen = Texture_GetValue1f(dg->mat.disney.sheen, dg->uv, TEXTURE_ARGS_IDX(dg->mat.disney.sheen_map_idx)); \n"\
"    float clearcoat_gloss = Texture_GetValue1f(dg->mat.disney.clearcoat_gloss, dg->uv, TEXTURE_ARGS_IDX(dg->mat.disney.clearcoat_gloss_map_idx)); \n"\
"    float clearcoat = Texture_GetValue1f(dg->mat.disney.clearcoat, dg->uv, TEXTURE_ARGS_IDX(dg->mat.disney.clearcoat_map_idx)); \n"\
"    float subsurface = dg->mat.disney.subsurface; \n"\
"     \n"\
"    float aspect = native_sqrt(1.f - anisotropy * 0.9f); \n"\
"     \n"\
"    float ax = max(0.001f, roughness * roughness * ( 1.f + anisotropy)); \n"\
"    float ay = max(0.001f, roughness * roughness * ( 1.f - anisotropy)); \n"\
"    float3 wh = normalize(wo + wi); \n"\
"    float ndotwh = fabs(wh.y); \n"\
"    float hdotwo = fabs(dot(wh, wo)); \n"\
" \n"\
"     \n"\
"    float d_pdf = fabs(wo.y) / PI; \n"\
"    float r_pdf = GTR2_Aniso(ndotwh, wh.x, wh.z, ax, ay) * ndotwh / (4.f * hdotwo); \n"\
"    float c_pdf = GTR1(ndotwh, mix(0.1f,0.001f, clearcoat_gloss)) * ndotwh / (4.f * hdotwo); \n"\
"     \n"\
"    float3 cd_lin = native_powr(base_color, 2.2f); \n"\
"    // Luminance approximmation \n"\
"    float cd_lum = dot(cd_lin, make_float3(0.3f, 0.6f, 0.1f)); \n"\
"     \n"\
"    // Normalize lum. to isolate hue+sat \n"\
"    float3 c_tint = cd_lum > 0.f ? (cd_lin / cd_lum) : WHITE; \n"\
"     \n"\
"    float3 c_spec0 = mix(specular * 0.1f * mix(WHITE, \n"\
"                                        c_tint, specular_tint), \n"\
"                         cd_lin, metallic); \n"\
"     \n"\
"    float cs_lum = dot(c_spec0, make_float3(0.3f, 0.6f, 0.1f)); \n"\
"     \n"\
"    float cs_w = cs_lum / (cs_lum + (1.f - metallic) * cd_lum); \n"\
"     \n"\
"    return c_pdf * clearcoat + (1.f - clearcoat) * (cs_w * r_pdf + (1.f - cs_w) * d_pdf); \n"\
"} \n"\
" \n"\
" \n"\
"INLINE float3 Disney_Evaluate( \n"\
"    // Geometry \n"\
"    DifferentialGeometry const* dg, \n"\
"    // Incoming direction \n"\
"    float3 wi, \n"\
"    // Outgoing direction \n"\
"    float3 wo, \n"\
"    // Texture args \n"\
"    TEXTURE_ARG_LIST \n"\
"    ) \n"\
"{ \n"\
"    float3 base_color = Texture_GetValue3f(dg->mat.disney.base_color.xyz, dg->uv, TEXTURE_ARGS_IDX(dg->mat.disney.base_color_map_idx)); \n"\
"    float metallic = Texture_GetValue1f(dg->mat.disney.metallic, dg->uv, TEXTURE_ARGS_IDX(dg->mat.disney.metallic_map_idx)); \n"\
"    float specular = Texture_GetValue1f(dg->mat.disney.specular, dg->uv, TEXTURE_ARGS_IDX(dg->mat.disney.specular_map_idx)); \n"\
"    float anisotropy = Texture_GetValue1f(dg->mat.disney.anisotropy, dg->uv, TEXTURE_ARGS_IDX(dg->mat.disney.anisotropy_map_idx)); \n"\
"    float roughness = Texture_GetValue1f(dg->mat.disney.roughness, dg->uv, TEXTURE_ARGS_IDX(dg->mat.disney.roughness_map_idx)); \n"\
"    float specular_tint = Texture_GetValue1f(dg->mat.disney.specular_tint, dg->uv, TEXTURE_ARGS_IDX(dg->mat.disney.specular_tint_map_idx)); \n"\
"    float sheen_tint = Texture_GetValue1f(dg->mat.disney.sheen_tint, dg->uv, TEXTURE_ARGS_IDX(dg->mat.disney.sheen_tint_map_idx)); \n"\
"    float sheen = Texture_GetValue1f(dg->mat.disney.sheen, dg->uv, TEXTURE_ARGS_IDX(dg->mat.disney.sheen_map_idx)); \n"\
"    float clearcoat_gloss = Texture_GetValue1f(dg->mat.disney.clearcoat_gloss, dg->uv, TEXTURE_ARGS_IDX(dg->mat.disney.clearcoat_gloss_map_idx)); \n"\
"    float clearcoat = Texture_GetValue1f(dg->mat.disney.clearcoat, dg->uv, TEXTURE_ARGS_IDX(dg->mat.disney.clearcoat_map_idx)); \n"\
"    float subsurface = dg->mat.disney.subsurface; \n"\
"     \n"\
"    float ndotwi = fabs(wi.y); \n"\
"    float ndotwo = fabs(wo.y); \n"\
"     \n"\
"    float3 h = normalize(wi + wo); \n"\
"    float ndoth = fabs(h.y); \n"\
"    float hdotwo = fabs(dot(h, wo)); \n"\
"     \n"\
"    float3 cd_lin = native_powr(base_color, 2.2f); \n"\
"    // Luminance approximmation \n"\
"    float cd_lum = dot(cd_lin, make_float3(0.3f, 0.6f, 0.1f)); \n"\
"     \n"\
"    // Normalize lum. to isolate hue+sat \n"\
"    float3 c_tint = cd_lum > 0.f ? (cd_lin / cd_lum) : WHITE; \n"\
"     \n"\
"    float3 c_spec0 = mix(specular * 0.1f * mix(WHITE, \n"\
"                                    c_tint, specular_tint), \n"\
"                                    cd_lin, metallic); \n"\
"     \n"\
"    float3 c_sheen = mix(WHITE, c_tint, sheen_tint); \n"\
"     \n"\
"    // Diffuse fresnel - go from 1 at normal incidence to 0.5 at grazing \n"\
"    // and mix in diffuse retro-reflection based on roughness \n"\
"    float f_wo = SchlickFresnelReflectance(ndotwo); \n"\
"    float f_wi = SchlickFresnelReflectance(ndotwi); \n"\
"     \n"\
"    float fd90 = 0.5f + 2 * hdotwo * hdotwo * roughness; \n"\
"    float fd = mix(1.f, fd90, f_wo) * mix(1.f, fd90, f_wi); \n"\
"     \n"\
"    // Based on Hanrahan-Krueger brdf approximation of isotropic bssrdf \n"\
"    // 1.25 scale is used to (roughly) preserve albedo \n"\
"    // fss90 used to \"flatten\" retroreflection based on roughness \n"\
"    float fss90 = hdotwo * hdotwo * roughness; \n"\
"    float fss = mix(1.f, fss90, f_wo) * mix(1.f, fss90, f_wi); \n"\
"    float ss = 1.25f * (fss * (1.f / (ndotwo + ndotwi) - 0.5f) + 0.5f); \n"\
"     \n"\
"    // Specular \n"\
"    float ax = max(0.001f, roughness * roughness * ( 1.f + anisotropy)); \n"\
"    float ay = max(0.001f, roughness * roughness * ( 1.f - anisotropy)); \n"\
"    float ds = GTR2_Aniso(ndoth, h.x, h.z, ax, ay); \n"\
"    float fh = SchlickFresnelReflectance(hdotwo); \n"\
"    float3 fs = mix(c_spec0, WHITE, fh); \n"\
"     \n"\
"    float gs; \n"\
"    gs  = SmithGGX_G_Aniso(ndotwo, wo.x, wo.z, ax, ay); \n"\
"    gs *= SmithGGX_G_Aniso(ndotwi, wi.x, wi.z, ax, ay); \n"\
"     \n"\
"    // Sheen \n"\
"    float3 f_sheen = fh * sheen * c_sheen; \n"\
"     \n"\
"    // clearcoat (ior = 1.5 -> F0 = 0.04) \n"\
"    float dr = GTR1(ndoth, mix(0.1f,0.001f, clearcoat_gloss)); \n"\
"    float fr = mix(0.04f, 1.f, fh); \n"\
"    float gr = SmithGGX_G(ndotwo, 0.25f) * SmithGGX_G(ndotwi, 0.25f); \n"\
"     \n"\
"    return ((1.f / PI) * mix(fd, ss, subsurface) * cd_lin + f_sheen) * \n"\
"            (1.f - metallic) + gs * fs * ds + clearcoat * gr * fr * dr; \n"\
"} \n"\
" \n"\
"INLINE float3 Disney_Sample( \n"\
"                            // Geometry \n"\
"                            DifferentialGeometry const* dg, \n"\
"                            // Incoming direction \n"\
"                            float3 wi, \n"\
"                            // Texture args \n"\
"                            TEXTURE_ARG_LIST, \n"\
"                            // Sample \n"\
"                            float2 sample, \n"\
"                            // Outgoing  direction \n"\
"                            float3* wo, \n"\
"                            // PDF at wo \n"\
"                            float* pdf \n"\
"                            ) \n"\
"{ \n"\
"    float3 base_color = Texture_GetValue3f(dg->mat.disney.base_color.xyz, dg->uv, TEXTURE_ARGS_IDX(dg->mat.disney.base_color_map_idx)); \n"\
"    float metallic = Texture_GetValue1f(dg->mat.disney.metallic, dg->uv, TEXTURE_ARGS_IDX(dg->mat.disney.metallic_map_idx)); \n"\
"    float specular = Texture_GetValue1f(dg->mat.disney.specular, dg->uv, TEXTURE_ARGS_IDX(dg->mat.disney.specular_map_idx)); \n"\
"    float anisotropy = Texture_GetValue1f(dg->mat.disney.anisotropy, dg->uv, TEXTURE_ARGS_IDX(dg->mat.disney.anisotropy_map_idx)); \n"\
"    float roughness = Texture_GetValue1f(dg->mat.disney.roughness, dg->uv, TEXTURE_ARGS_IDX(dg->mat.disney.roughness_map_idx)); \n"\
"    float specular_tint = Texture_GetValue1f(dg->mat.disney.specular_tint, dg->uv, TEXTURE_ARGS_IDX(dg->mat.disney.specular_tint_map_idx)); \n"\
"    float sheen_tint = Texture_GetValue1f(dg->mat.disney.sheen_tint, dg->uv, TEXTURE_ARGS_IDX(dg->mat.disney.sheen_tint_map_idx)); \n"\
"    float sheen = Texture_GetValue1f(dg->mat.disney.sheen, dg->uv, TEXTURE_ARGS_IDX(dg->mat.disney.sheen_map_idx)); \n"\
"    float clearcoat_gloss = Texture_GetValue1f(dg->mat.disney.clearcoat_gloss, dg->uv, TEXTURE_ARGS_IDX(dg->mat.disney.clearcoat_gloss_map_idx)); \n"\
"    float clearcoat = Texture_GetValue1f(dg->mat.disney.clearcoat, dg->uv, TEXTURE_ARGS_IDX(dg->mat.disney.clearcoat_map_idx)); \n"\
"    float subsurface = dg->mat.disney.subsurface; \n"\
"     \n"\
"    float ax = max(0.001f, roughness * roughness * ( 1.f + anisotropy)); \n"\
"    float ay = max(0.001f, roughness * roughness * ( 1.f - anisotropy)); \n"\
"     \n"\
"    float mis_weight = 1.f; \n"\
"    float3 wh; \n"\
"     \n"\
"     \n"\
"    if (sample.x < clearcoat) \n"\
"    { \n"\
"        sample.x /= (clearcoat); \n"\
"         \n"\
"        float a = mix(0.1f,0.001f, clearcoat_gloss); \n"\
"        float ndotwh = native_sqrt((1.f - native_powr(a*a, 1.f - sample.y)) / (1.f - a*a)); \n"\
"        float sintheta = native_sqrt(1.f - ndotwh * ndotwh); \n"\
"        wh = normalize(make_float3(native_cos(2.f * PI * sample.x) * sintheta, \n"\
"                                   ndotwh, \n"\
"                                   native_sin(2.f * PI * sample.x) * sintheta)); \n"\
"         \n"\
"        *wo = -wi + 2.f*fabs(dot(wi, wh)) * wh; \n"\
"         \n"\
"    } \n"\
"    else \n"\
"    { \n"\
"        sample.x -= (clearcoat); \n"\
"        sample.x /= (1.f - clearcoat); \n"\
" \n"\
"        float3 cd_lin = native_powr(base_color, 2.2f); \n"\
"        // Luminance approximmation \n"\
"        float cd_lum = dot(cd_lin, make_float3(0.3f, 0.6f, 0.1f)); \n"\
" \n"\
"        // Normalize lum. to isolate hue+sat \n"\
"        float3 c_tint = cd_lum > 0.f ? (cd_lin / cd_lum) : WHITE; \n"\
" \n"\
"        float3 c_spec0 = mix(specular * 0.3f * mix(WHITE, \n"\
"            c_tint, specular_tint), \n"\
"            cd_lin, metallic); \n"\
" \n"\
"        float cs_lum = dot(c_spec0, make_float3(0.3f, 0.6f, 0.1f)); \n"\
" \n"\
"        float cs_w = cs_lum / (cs_lum + (1.f - metallic) * cd_lum); \n"\
"         \n"\
"        if (sample.y < cs_w) \n"\
"        { \n"\
"            sample.y /= cs_w; \n"\
"             \n"\
"            float t = native_sqrt(sample.y / (1.f - sample.y)); \n"\
"            wh = normalize(make_float3(t * ax * native_cos(2.f * PI * sample.x), \n"\
"                                              1.f, \n"\
"                                              t * ay * native_sin(2.f * PI * sample.x))); \n"\
"             \n"\
"            *wo = -wi + 2.f*fabs(dot(wi, wh)) * wh; \n"\
"        } \n"\
"        else \n"\
"        { \n"\
"            sample.y -= cs_w; \n"\
"            sample.y /= (1.f - cs_w); \n"\
"             \n"\
"            *wo = Sample_MapToHemisphere(sample, make_float3(0.f, 1.f, 0.f) , 1.f); \n"\
"             \n"\
"            wh = normalize(*wo + wi); \n"\
"        } \n"\
"    } \n"\
" \n"\
"    //float ndotwh = fabs(wh.y); \n"\
"    //float hdotwo = fabs(dot(wh, *wo)); \n"\
"     \n"\
"    *pdf = Disney_GetPdf(dg, wi, *wo, TEXTURE_ARGS); \n"\
"     \n"\
"    return Disney_Evaluate(dg, wi, *wo, TEXTURE_ARGS); \n"\
"} \n"\
" \n"\
"#endif \n"\
" \n"\
;
static const char g_integrator_bdpt_opencl[]= \
"/********************************************************************** \n"\
"Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved. \n"\
" \n"\
"Permission is hereby granted, free of charge, to any person obtaining a copy \n"\
"of this software and associated documentation files (the \"Software\"), to deal \n"\
"in the Software without restriction, including without limitation the rights \n"\
"to use, copy, modify, merge, publish, distribute, sublicense, and/or sell \n"\
"copies of the Software, and to permit persons to whom the Software is \n"\
"furnished to do so, subject to the following conditions: \n"\
" \n"\
"The above copyright notice and this permission notice shall be included in \n"\
"all copies or substantial portions of the Software. \n"\
" \n"\
"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR \n"\
"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, \n"\
"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE \n"\
"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER \n"\
"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, \n"\
"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN \n"\
"THE SOFTWARE. \n"\
"********************************************************************/ \n"\
" \n"\
"// Convert PDF of sampling point po from point p from solid angle measure to area measure \n"\
"INLINE \n"\
"float Pdf_ConvertSolidAngleToArea(float pdf, float3 po, float3 p, float3 n) \n"\
"{ \n"\
"    float3 v = po - p; \n"\
"    float dist = length(v); \n"\
"    return pdf * fabs(dot(normalize(v), n)) / (dist * dist); \n"\
"} \n"\
" \n"\
"KERNEL void GenerateLightVertices( \n"\
"    // Number of subpaths to generate \n"\
"    int num_subpaths, \n"\
"    // Vertices \n"\
"    GLOBAL float3 const* restrict vertices, \n"\
"    // Normals \n"\
"    GLOBAL float3 const* restrict normals, \n"\
"    // UVs \n"\
"    GLOBAL float2 const* restrict uvs, \n"\
"    // Indices \n"\
"    GLOBAL int const* restrict indices, \n"\
"    // Shapes \n"\
"    GLOBAL Shape const* restrict shapes, \n"\
"    // Material IDs \n"\
"    GLOBAL int const* restrict material_ids, \n"\
"    // Materials \n"\
"    GLOBAL Material const* restrict materials, \n"\
"    // Textures \n"\
"    TEXTURE_ARG_LIST, \n"\
"    // Environment texture index \n"\
"    int env_light_idx, \n"\
"    // Emissives \n"\
"    GLOBAL Light const* restrict lights, \n"\
"    // Number of emissive objects \n"\
"    int num_lights, \n"\
"    // RNG seed value \n"\
"    uint rngseed, \n"\
"    int frame, \n"\
"    // RNG data \n"\
"    GLOBAL uint const* restrict random, \n"\
"    GLOBAL uint const* restrict sobolmat, \n"\
"    // Output rays \n"\
"    GLOBAL ray* restrict rays, \n"\
"    // Light subpath \n"\
"    GLOBAL PathVertex* restrict light_subpath, \n"\
"    // Light subpath length \n"\
"    GLOBAL int* restrict light_subpath_length, \n"\
"    // Path buffer \n"\
"    GLOBAL Path* restrict paths \n"\
") \n"\
"{ \n"\
"    Scene scene = \n"\
"    { \n"\
"        vertices, \n"\
"        normals, \n"\
"        uvs, \n"\
"        indices, \n"\
"        shapes, \n"\
"        material_ids, \n"\
"        materials, \n"\
"        lights, \n"\
"        env_light_idx, \n"\
"        num_lights \n"\
"    }; \n"\
" \n"\
"    int global_id = get_global_id(0);  \n"\
" \n"\
"    // Check borders \n"\
"    if (global_id < num_subpaths) \n"\
"    { \n"\
"        // Get pointer to ray to handle \n"\
"        GLOBAL ray* my_ray = rays + global_id; \n"\
"        GLOBAL PathVertex* my_vertex = light_subpath + BDPT_MAX_SUBPATH_LEN * global_id; \n"\
"        GLOBAL int* my_count = light_subpath_length + global_id; \n"\
"        GLOBAL Path* my_path = paths + global_id; \n"\
" \n"\
"        Sampler sampler; \n"\
"#if SAMPLER == SOBOL \n"\
"        uint scramble = random[global_id] * 0x1fe3434f; \n"\
" \n"\
"        if (frame & 0xF) \n"\
"        { \n"\
"            random[global_id] = WangHash(scramble); \n"\
"        } \n"\
" \n"\
"        Sampler_Init(&sampler, frame, SAMPLE_DIM_CAMERA_OFFSET, scramble); \n"\
"#elif SAMPLER == RANDOM \n"\
"        uint scramble = global_id * rngseed; \n"\
"        Sampler_Init(&sampler, scramble); \n"\
"#elif SAMPLER == CMJ \n"\
"        uint rnd = random[global_id]; \n"\
"        uint scramble = rnd * 0x1fe3434f * ((frame + 133 * rnd) / (CMJ_DIM * CMJ_DIM)); \n"\
"        Sampler_Init(&sampler, frame % (CMJ_DIM * CMJ_DIM), SAMPLE_DIM_CAMERA_OFFSET, scramble); \n"\
"#endif \n"\
" \n"\
"        // Generate sample \n"\
"        float2 sample0 = Sampler_Sample2D(&sampler, SAMPLER_ARGS); \n"\
"        float2 sample1 = Sampler_Sample2D(&sampler, SAMPLER_ARGS); \n"\
" \n"\
"        float selection_pdf; \n"\
"        int idx = Scene_SampleLight(&scene, Sampler_Sample1D(&sampler, SAMPLER_ARGS), &selection_pdf); \n"\
" \n"\
"        float3 p, n, wo; \n"\
"        float light_pdf; \n"\
"        float3 ke = Light_SampleVertex(idx, &scene, TEXTURE_ARGS, sample0, sample1, &p, &n, &wo, &light_pdf); \n"\
" \n"\
"        // Calculate direction to image plane \n"\
"        my_ray->d.xyz = normalize(wo); \n"\
"        // Origin == camera position + nearz * d \n"\
"        my_ray->o.xyz = p + CRAZY_LOW_DISTANCE * n; \n"\
"        // Max T value = zfar - znear since we moved origin to znear \n"\
"        my_ray->o.w = CRAZY_HIGH_DISTANCE; \n"\
"        // Generate random time from 0 to 1 \n"\
"        my_ray->d.w = sample0.x; \n"\
"        // Set ray max \n"\
"        my_ray->extra.x = 0xFFFFFFFF; \n"\
"        my_ray->extra.y = 0xFFFFFFFF; \n"\
"        Ray_SetExtra(my_ray, 1.f); \n"\
" \n"\
"        PathVertex v; \n"\
"        PathVertex_Init(&v, \n"\
"            p, \n"\
"            n, \n"\
"            n, \n"\
"            0.f, \n"\
"            selection_pdf * light_pdf, \n"\
"            1.f, \n"\
"            100.f,//ke * fabs(dot(n, my_ray->d.xyz)) / selection_pdf * light_pdf, \n"\
"            kLight, \n"\
"            -1); \n"\
" \n"\
"        *my_count = 1; \n"\
"        *my_vertex = v; \n"\
" \n"\
"        // Initlize path data \n"\
"        my_path->throughput = ke * fabs(dot(n, my_ray->d.xyz)) / selection_pdf * light_pdf; \n"\
"        my_path->volume = -1; \n"\
"        my_path->flags = 0; \n"\
"        my_path->active = 0xFF; \n"\
"    } \n"\
"} \n"\
" \n"\
"KERNEL void SampleSurface(  \n"\
"    // Ray batch \n"\
"    GLOBAL ray const* rays, \n"\
"    // Intersection data \n"\
"    GLOBAL Intersection const* intersections, \n"\
"    // Hit indices \n"\
"    GLOBAL int const* hit_indices, \n"\
"    // Pixel indices \n"\
"    GLOBAL int const* pixel_indices, \n"\
"    // Number of rays \n"\
"    GLOBAL int const* num_hits, \n"\
"    // Vertices \n"\
"    GLOBAL float3 const* vertices, \n"\
"    // Normals \n"\
"    GLOBAL float3 const* normals, \n"\
"    // UVs \n"\
"    GLOBAL float2 const* uvs, \n"\
"    // Indices \n"\
"    GLOBAL int const* indices, \n"\
"    // Shapes \n"\
"    GLOBAL Shape const* shapes, \n"\
"    // Material IDs \n"\
"    GLOBAL int const* material_ids, \n"\
"    // Materials \n"\
"    GLOBAL Material const* materials, \n"\
"    // Textures \n"\
"    TEXTURE_ARG_LIST, \n"\
"    // Environment texture index \n"\
"    int env_light_idx, \n"\
"    // Emissives \n"\
"    GLOBAL Light const* lights, \n"\
"    // Number of emissive objects \n"\
"    int num_lights, \n"\
"    // RNG seed \n"\
"    uint rngseed, \n"\
"    // Sampler states \n"\
"    GLOBAL uint* random, \n"\
"    // Sobol matrices \n"\
"    GLOBAL uint const* sobolmat, \n"\
"    // Current bounce \n"\
"    int bounce, \n"\
"    // Frame \n"\
"    int frame, \n"\
"    // Radiance or importance \n"\
"    int transfer_mode, \n"\
"    // Path throughput \n"\
"    GLOBAL Path* paths, \n"\
"    // Indirect rays \n"\
"    GLOBAL ray* extension_rays, \n"\
"    // Vertices \n"\
"    GLOBAL PathVertex* subpath, \n"\
"    GLOBAL int* subpath_length \n"\
") \n"\
"    { \n"\
"        int global_id = get_global_id(0); \n"\
" \n"\
"        Scene scene = \n"\
"        { \n"\
"            vertices, \n"\
"            normals, \n"\
"            uvs, \n"\
"            indices, \n"\
"            shapes, \n"\
"            material_ids, \n"\
"            materials, \n"\
"            lights, \n"\
"            env_light_idx, \n"\
"            num_lights \n"\
"        }; \n"\
" \n"\
"        // Only applied to active rays after compaction \n"\
"        if (global_id < *num_hits) \n"\
"        { \n"\
"            // Fetch index \n"\
"            int hit_idx = hit_indices[global_id]; \n"\
"            int pixel_idx = pixel_indices[global_id]; \n"\
"            Intersection isect = intersections[hit_idx]; \n"\
" \n"\
"            GLOBAL Path* path = paths + pixel_idx; \n"\
" \n"\
"            // Fetch incoming ray direction \n"\
"            float3 wi = -normalize(rays[hit_idx].d.xyz); \n"\
" \n"\
"            Sampler sampler; \n"\
"#if SAMPLER == SOBOL \n"\
"            uint scramble = random[pixel_idx] * 0x1fe3434f; \n"\
"            Sampler_Init(&sampler, frame, SAMPLE_DIM_SURFACE_OFFSET + bounce * SAMPLE_DIMS_PER_BOUNCE, scramble); \n"\
"#elif SAMPLER == RANDOM \n"\
"            uint scramble = pixel_idx * rngseed; \n"\
"            Sampler_Init(&sampler, scramble); \n"\
"#elif SAMPLER == CMJ \n"\
"            uint rnd = random[pixel_idx]; \n"\
"            uint scramble = rnd * 0x1fe3434f * ((frame + 331 * rnd) / (CMJ_DIM * CMJ_DIM)); \n"\
"            Sampler_Init(&sampler, frame % (CMJ_DIM * CMJ_DIM), SAMPLE_DIM_SURFACE_OFFSET + bounce * SAMPLE_DIMS_PER_BOUNCE, scramble); \n"\
"#endif \n"\
" \n"\
"            // Fill surface data \n"\
"            DifferentialGeometry diffgeo; \n"\
"            Scene_FillDifferentialGeometry(&scene, &isect, &diffgeo); \n"\
"            diffgeo.transfer_mode = transfer_mode;  \n"\
" \n"\
"            // Check if we are hitting from the inside \n"\
"            float backfacing = dot(diffgeo.ng, wi) < 0.f; \n"\
"            int twosided = false; // diffgeo.mat.twosided; \n"\
"            if (twosided && backfacing) \n"\
"            { \n"\
"                // Reverse normal and tangents in this case \n"\
"                // but not for BTDFs, since BTDFs rely \n"\
"                // on normal direction in order to arrange \n"\
"                // indices of refraction \n"\
"                diffgeo.n = -diffgeo.n; \n"\
"                diffgeo.dpdu = -diffgeo.dpdu; \n"\
"                diffgeo.dpdv = -diffgeo.dpdv; \n"\
"            } \n"\
" \n"\
"            float ndotwi = dot(diffgeo.n, wi); \n"\
" \n"\
"            // Select BxDF \n"\
"            Material_Select(&scene, wi, &sampler, TEXTURE_ARGS, SAMPLER_ARGS, &diffgeo); \n"\
" \n"\
"            // Terminate if emissive \n"\
"            if (Bxdf_IsEmissive(&diffgeo))  \n"\
"            { \n"\
"                Path_Kill(path); \n"\
"                return; \n"\
"            } \n"\
" \n"\
"            float s = Bxdf_IsBtdf(&diffgeo) ? (-sign(ndotwi)) : 1.f; \n"\
"            if (!twosided && backfacing && !Bxdf_IsBtdf(&diffgeo)) \n"\
"            { \n"\
"                //Reverse normal and tangents in this case \n"\
"                //but not for BTDFs, since BTDFs rely \n"\
"                //on normal direction in order to arrange \n"\
"                //indices of refraction \n"\
"                diffgeo.n = -diffgeo.n; \n"\
"                diffgeo.dpdu = -diffgeo.dpdu; \n"\
"                diffgeo.dpdv = -diffgeo.dpdv; \n"\
"            } \n"\
" \n"\
"            // Check if we need to apply normal map \n"\
"            DifferentialGeometry_ApplyBumpNormalMap(&diffgeo, TEXTURE_ARGS); \n"\
"            DifferentialGeometry_CalculateTangentTransforms(&diffgeo); \n"\
" \n"\
"            float lightpdf = 0.f; \n"\
"            float bxdflightpdf = 0.f; \n"\
"            float bxdfpdf = 0.f; \n"\
"            float lightbxdfpdf = 0.f; \n"\
"            float selection_pdf = 0.f; \n"\
"            float3 radiance = 0.f; \n"\
"            float3 lightwo; \n"\
"            float3 bxdfwo; \n"\
"            float3 wo; \n"\
"            float bxdfweight = 1.f; \n"\
"            float lightweight = 1.f; \n"\
" \n"\
"            float3 throughput = Path_GetThroughput(path); \n"\
" \n"\
"            // Sample bxdf \n"\
"            float3 bxdf = Bxdf_Sample(&diffgeo, wi, TEXTURE_ARGS, Sampler_Sample2D(&sampler, SAMPLER_ARGS), &bxdfwo, &bxdfpdf); \n"\
" \n"\
"            bxdfwo = normalize(bxdfwo); \n"\
"            float3 t = bxdf * fabs(dot(diffgeo.n, bxdfwo)); \n"\
" \n"\
"            // Write path vertex \n"\
"            GLOBAL int* my_counter = subpath_length + pixel_idx; \n"\
"            int idx = atom_inc(my_counter); \n"\
" \n"\
"            if (idx < BDPT_MAX_SUBPATH_LEN) \n"\
"            { \n"\
"                GLOBAL PathVertex* my_vertex = subpath + BDPT_MAX_SUBPATH_LEN * pixel_idx + idx; \n"\
"                GLOBAL PathVertex* my_prev_vertex = subpath + BDPT_MAX_SUBPATH_LEN * pixel_idx + idx - 1; \n"\
"                // Fill vertex \n"\
"                // Calculate forward PDF of a current vertex \n"\
"                float pdf_solid_angle = Ray_GetExtra(&rays[hit_idx]).x; \n"\
"                float pdf_fwd = Pdf_ConvertSolidAngleToArea(pdf_solid_angle, rays[hit_idx].o.xyz, diffgeo.p, diffgeo.n); \n"\
" \n"\
"                PathVertex v; \n"\
"                PathVertex_Init(&v, \n"\
"                    diffgeo.p, \n"\
"                    diffgeo.n, \n"\
"                    diffgeo.ng, \n"\
"                    diffgeo.uv, \n"\
"                    pdf_fwd, \n"\
"                    0.f, \n"\
"                    my_prev_vertex->flow * t / bxdfpdf, \n"\
"                    kSurface, \n"\
"                    diffgeo.material_index); \n"\
" \n"\
"                *my_vertex = v; \n"\
" \n"\
"                // TODO: wrong \n"\
"                if (my_prev_vertex->type == kSurface) \n"\
"                { \n"\
"                    float pdf_bwd = Pdf_ConvertSolidAngleToArea(Bxdf_GetPdf(&diffgeo, bxdfwo, wi, TEXTURE_ARGS), diffgeo.p, my_prev_vertex->position, my_prev_vertex->shading_normal); \n"\
"                    my_prev_vertex->pdf_backward = pdf_bwd; \n"\
"                } \n"\
"            } \n"\
"            else \n"\
"            { \n"\
"                atom_dec(my_counter); \n"\
"                Path_Kill(path); \n"\
"                Ray_SetInactive(extension_rays + global_id); \n"\
"                return; \n"\
"            } \n"\
" \n"\
"            // Only continue if we have non-zero throughput & pdf \n"\
"            if (NON_BLACK(t) && bxdfpdf > 0.f) \n"\
"            { \n"\
"                // Update the throughput \n"\
"                Path_MulThroughput(path, t / bxdfpdf); \n"\
" \n"\
"                // Generate ray \n"\
"                float3 new_ray_dir = bxdfwo; \n"\
"                float3 new_ray_o = diffgeo.p + CRAZY_LOW_DISTANCE * s * diffgeo.n; \n"\
" \n"\
"                Ray_Init(extension_rays + global_id, new_ray_o, new_ray_dir, CRAZY_HIGH_DISTANCE, 0.f, 0xFFFFFFFF); \n"\
"                Ray_SetExtra(extension_rays + global_id, make_float2(bxdfpdf, 0.f)); \n"\
"            } \n"\
"            else \n"\
"            { \n"\
"                // Otherwise kill the path \n"\
"                Path_Kill(path); \n"\
"                Ray_SetInactive(extension_rays + global_id); \n"\
"            } \n"\
"        } \n"\
"    } \n"\
" \n"\
"    KERNEL void ConnectDirect( \n"\
"        int eye_vertex_index, \n"\
"        int num_rays, \n"\
"        GLOBAL int const* pixel_indices, \n"\
"        GLOBAL PathVertex const* restrict eye_subpath, \n"\
"        GLOBAL int const* restrict eye_subpath_length, \n"\
"        GLOBAL float3 const* restrict vertices, \n"\
"        GLOBAL float3 const* restrict normals, \n"\
"        GLOBAL float2 const* restrict uvs, \n"\
"        GLOBAL int const* restrict indices, \n"\
"        GLOBAL Shape const* restrict shapes, \n"\
"        GLOBAL int const* restrict materialids, \n"\
"        GLOBAL Material const* restrict materials, \n"\
"        TEXTURE_ARG_LIST, \n"\
"        int env_light_idx, \n"\
"        GLOBAL Light const* restrict lights, \n"\
"        int num_lights, \n"\
"        uint rngseed, \n"\
"        GLOBAL uint const* restrict random, \n"\
"        GLOBAL uint const* restrict sobolmat, \n"\
"        int frame, \n"\
"        GLOBAL ray* restrict shadowrays, \n"\
"        GLOBAL float3* restrict lightsamples \n"\
"    ) \n"\
"    { \n"\
"        int global_id = get_global_id(0); \n"\
" \n"\
"        if (global_id < num_rays) \n"\
"        { \n"\
"            int pixel_idx = pixel_indices[global_id]; \n"\
"            int num_eye_vertices = *(eye_subpath_length + pixel_idx); \n"\
" \n"\
"            GLOBAL PathVertex const* my_eye_vertex = eye_subpath + BDPT_MAX_SUBPATH_LEN * pixel_idx + eye_vertex_index; \n"\
"            GLOBAL PathVertex const* my_prev_eye_vertex = eye_subpath + BDPT_MAX_SUBPATH_LEN * pixel_idx + eye_vertex_index - 1; \n"\
" \n"\
"            DifferentialGeometry diffgeo; \n"\
"            diffgeo.p = my_eye_vertex->position; \n"\
"            diffgeo.n = my_eye_vertex->shading_normal; \n"\
"            diffgeo.ng = my_eye_vertex->geometric_normal; \n"\
"            diffgeo.uv = my_eye_vertex->uv; \n"\
"            diffgeo.dpdu = GetOrthoVector(diffgeo.n); \n"\
"            diffgeo.dpdv = cross(diffgeo.n, diffgeo.dpdu); \n"\
"            diffgeo.mat = materials[my_eye_vertex->material_index]; \n"\
"            diffgeo.mat.fresnel = 1.f; \n"\
"            DifferentialGeometry_CalculateTangentTransforms(&diffgeo); \n"\
" \n"\
"            float3 wi = normalize(my_prev_eye_vertex->position - diffgeo.p); \n"\
"            float3 throughput = my_prev_eye_vertex->flow; \n"\
" \n"\
"            if (eye_vertex_index >= num_eye_vertices) \n"\
"            { \n"\
"                lightsamples[global_id].xyz = 0.f; \n"\
"                Ray_SetInactive(shadowrays + global_id); \n"\
"                return; \n"\
"            } \n"\
" \n"\
"            Scene scene =     \n"\
"            { \n"\
"                vertices, \n"\
"                normals, \n"\
"                uvs, \n"\
"                indices, \n"\
"                shapes, \n"\
"                materialids, \n"\
"                materials, \n"\
"                lights, \n"\
"                env_light_idx, \n"\
"                num_lights \n"\
"            }; \n"\
" \n"\
"            Sampler sampler; \n"\
"#if SAMPLER == SOBOL \n"\
"            uint scramble = random[global_id] * 0x1fe3434f; \n"\
"            Sampler_Init(&sampler, frame, SAMPLE_DIM_SURFACE_OFFSET + eye_vertex_index * SAMPLE_DIMS_PER_BOUNCE, scramble);  \n"\
"#elif SAMPLER == RANDOM \n"\
"            uint scramble = global_id * rngseed; \n"\
"            Sampler_Init(&sampler, scramble); \n"\
"#elif SAMPLER == CMJ \n"\
"            uint rnd = random[global_id]; \n"\
"            uint scramble = rnd * 0x1fe3434f * ((frame + 331 * rnd) / (CMJ_DIM * CMJ_DIM)); \n"\
"            Sampler_Init(&sampler, frame % (CMJ_DIM * CMJ_DIM), SAMPLE_DIM_SURFACE_OFFSET + eye_vertex_index * SAMPLE_DIMS_PER_BOUNCE, scramble); \n"\
"#endif \n"\
" \n"\
"            float selection_pdf; \n"\
"            int light_idx = Scene_SampleLight(&scene, Sampler_Sample1D(&sampler, SAMPLER_ARGS), &selection_pdf); \n"\
" \n"\
"            // Sample light \n"\
"            float lightpdf = 0.f; \n"\
"            float lightbxdfpdf = 0.f; \n"\
"            float bxdfpdf = 0.f; \n"\
"            float bxdflightpdf = 0.f; \n"\
"            float lightweight = 0.f; \n"\
"            float bxdfweight = 0.f; \n"\
" \n"\
" \n"\
"            float3 lightwo; \n"\
"            float3 bxdfwo; \n"\
"            float3 le = Light_Sample(light_idx, &scene, &diffgeo, TEXTURE_ARGS, Sampler_Sample2D(&sampler, SAMPLER_ARGS), &lightwo, &lightpdf); \n"\
"            float3 bxdf = Bxdf_Sample(&diffgeo, normalize(wi), TEXTURE_ARGS, Sampler_Sample2D(&sampler, SAMPLER_ARGS), &bxdfwo, &bxdfpdf); \n"\
"            lightbxdfpdf = Bxdf_GetPdf(&diffgeo, normalize(wi), normalize(lightwo), TEXTURE_ARGS); \n"\
"            bxdflightpdf = Light_GetPdf(light_idx, &scene, &diffgeo, normalize(bxdfwo), TEXTURE_ARGS); \n"\
" \n"\
"            bool singular_light = Light_IsSingular(&scene.lights[light_idx]); \n"\
"            bool singular_bxdf = Bxdf_IsSingular(&diffgeo); \n"\
"            lightweight = singular_light ? 1.f : BalanceHeuristic(1, lightpdf * selection_pdf, 1, lightbxdfpdf); \n"\
"            bxdfweight = singular_bxdf ? 1.f : BalanceHeuristic(1, bxdfpdf, 1, bxdflightpdf * selection_pdf); \n"\
" \n"\
"            float3 radiance = 0.f;  \n"\
"            float3 wo; \n"\
" \n"\
"            if (singular_light && singular_bxdf)  \n"\
"            { \n"\
"                // Otherwise save some intersector cycles \n"\
"                Ray_SetInactive(shadowrays + global_id);   \n"\
"                lightsamples[global_id] = 0;  \n"\
"                return; \n"\
"            } \n"\
" \n"\
"            float split = Sampler_Sample1D(&sampler, SAMPLER_ARGS); \n"\
" \n"\
"            if (((singular_bxdf && !singular_light) || split < 0.5f) && bxdfpdf > 0.f) \n"\
"            { \n"\
"                wo = CRAZY_HIGH_DISTANCE * bxdfwo;  \n"\
"                float ndotwo = fabs(dot(diffgeo.n, normalize(wo))); \n"\
"                le = Light_GetLe(light_idx, &scene, &diffgeo, &wo, TEXTURE_ARGS); \n"\
"                radiance = 2.f * bxdfweight * le * throughput * bxdf * ndotwo / bxdfpdf; \n"\
"            } \n"\
" \n"\
"            if (((!singular_bxdf && singular_light) || split >= 0.5f) && lightpdf > 0.f)  \n"\
"            { \n"\
"                wo = lightwo; \n"\
"                float ndotwo = fabs(dot(diffgeo.n, normalize(wo)));   \n"\
"                radiance = 2.f * lightweight * le * Bxdf_Evaluate(&diffgeo, wi, normalize(wo), TEXTURE_ARGS) * throughput * ndotwo / lightpdf / selection_pdf; \n"\
"            } \n"\
" \n"\
"            // If we have some light here generate a shadow ray \n"\
"            if (NON_BLACK(radiance))  \n"\
"            { \n"\
"                // Generate shadow ray \n"\
"                float3 shadow_ray_o = diffgeo.p + CRAZY_LOW_DISTANCE * diffgeo.n;  \n"\
"                float3 temp = diffgeo.p + wo - shadow_ray_o; \n"\
"                float3 shadow_ray_dir = normalize(temp); \n"\
"                float shadow_ray_length = 0.999f * length(temp); \n"\
"                int shadow_ray_mask = 0xFFFFFFFF; \n"\
" \n"\
"                Ray_Init(shadowrays + global_id, shadow_ray_o, shadow_ray_dir, shadow_ray_length, 0.f, shadow_ray_mask); \n"\
" \n"\
"                // And write the light sample \n"\
"                lightsamples[global_id] = REASONABLE_RADIANCE(radiance); \n"\
"            } \n"\
"            else \n"\
"            { \n"\
"                // Otherwise save some intersector cycles \n"\
"                Ray_SetInactive(shadowrays + global_id); \n"\
"                lightsamples[global_id] = 0; \n"\
"            } \n"\
"        } \n"\
"    } \n"\
" \n"\
"    KERNEL void Connect( \n"\
"        int num_rays, \n"\
"        // Index of eye vertex we are trying to connect \n"\
"        int eye_vertex_index, \n"\
"        // Index of light vertex we are trying to connect \n"\
"        int light_vertex_index, \n"\
"        GLOBAL int const* pixel_indices, \n"\
"        // Vertex arrays \n"\
"        GLOBAL PathVertex const* restrict eye_subpath, \n"\
"        GLOBAL int const* restrict eye_subpath_length, \n"\
"        GLOBAL PathVertex const* restrict light_subpath, \n"\
"        GLOBAL int const* restrict light_subpath_length, \n"\
"        GLOBAL Material const* restrict materials, \n"\
"        // Textures \n"\
"        TEXTURE_ARG_LIST, \n"\
"        GLOBAL ray* connection_rays, \n"\
"        // Path contriburions to final image \n"\
"        GLOBAL float4* contributions \n"\
"    ) \n"\
"    { \n"\
"        int global_id = get_global_id(0); \n"\
" \n"\
" \n"\
"        if (global_id >= num_rays) \n"\
"        { \n"\
"            return; \n"\
"        } \n"\
" \n"\
"        int pixel_idx = pixel_indices[global_id]; \n"\
" \n"\
"        // Check if we our indices are within subpath index range \n"\
"        if (eye_vertex_index >= eye_subpath_length[pixel_idx] || \n"\
"            light_vertex_index >= light_subpath_length[global_id]) \n"\
"        { \n"\
"            contributions[global_id].xyz = 0.f; \n"\
"            Ray_SetInactive(&connection_rays[global_id]); \n"\
"            return; \n"\
"        } \n"\
" \n"\
"        // Prepare data pointers \n"\
"        GLOBAL PathVertex const* my_eye_vertex = eye_subpath + BDPT_MAX_SUBPATH_LEN * pixel_idx + eye_vertex_index; \n"\
"        GLOBAL PathVertex const* my_prev_eye_vertex = eye_subpath + BDPT_MAX_SUBPATH_LEN * pixel_idx + eye_vertex_index - 1; \n"\
" \n"\
"        GLOBAL PathVertex const* my_light_vertex = light_subpath + BDPT_MAX_SUBPATH_LEN * global_id + light_vertex_index; \n"\
"        GLOBAL PathVertex const* my_prev_light_vertex = light_subpath + BDPT_MAX_SUBPATH_LEN * global_id + light_vertex_index - 1; \n"\
" \n"\
"        // Fill differential geometries for both eye and light vertices \n"\
"        DifferentialGeometry eye_dg; \n"\
"        eye_dg.p = my_eye_vertex->position; \n"\
"        eye_dg.n = my_eye_vertex->shading_normal; \n"\
"        eye_dg.ng = my_eye_vertex->geometric_normal; \n"\
"        eye_dg.uv = my_eye_vertex->uv; \n"\
"        eye_dg.dpdu = GetOrthoVector(eye_dg.n); \n"\
"        eye_dg.dpdv = cross(eye_dg.n, eye_dg.dpdu); \n"\
"        eye_dg.mat = materials[my_eye_vertex->material_index]; \n"\
"        eye_dg.mat.fresnel = 1.f; \n"\
"        DifferentialGeometry_CalculateTangentTransforms(&eye_dg); \n"\
" \n"\
"        DifferentialGeometry light_dg; \n"\
"        light_dg.p = my_light_vertex->position; \n"\
"        light_dg.n = my_light_vertex->shading_normal; \n"\
"        light_dg.ng = my_light_vertex->geometric_normal; \n"\
"        light_dg.uv = my_light_vertex->uv; \n"\
"        light_dg.dpdu = GetOrthoVector(light_dg.n); \n"\
"        light_dg.dpdv = cross(light_dg.dpdu, light_dg.n); \n"\
"        light_dg.mat = materials[my_light_vertex->material_index]; \n"\
"        light_dg.mat.fresnel = 1.f; \n"\
"        DifferentialGeometry_CalculateTangentTransforms(&light_dg); \n"\
" \n"\
"        // Vector from eye subpath vertex to previous eye subpath vertex (incoming vector) \n"\
"        float3 eye_wi = normalize(my_prev_eye_vertex->position - eye_dg.p); \n"\
"        // Vector from camera subpath vertex to light subpath vertex (connection vector) \n"\
"        float3 eye_wo = normalize(light_dg.p - eye_dg.p); \n"\
"        // Distance between vertices we are connecting \n"\
"        float dist = length(light_dg.p - eye_dg.p); \n"\
"        // BXDF at eye subpath vertex \n"\
"        float3 eye_bxdf = Bxdf_Evaluate(&eye_dg, eye_wi, eye_wo, TEXTURE_ARGS); \n"\
"        // Eye subpath vertex cosine factor \n"\
"        float eye_dotnl = fabs(dot(eye_dg.n, eye_wo)); \n"\
" \n"\
"        // Calculate througput of a segment eye->eyevetex0->...->eyevertexN->lightvertexM->lightvertexM-1...->light \n"\
"        // First eye subpath throughput eye->eyevertex0->...eyevertexN \n"\
"        float3 eye_contribution = my_prev_eye_vertex->flow; \n"\
"        eye_contribution *= (eye_bxdf * eye_dotnl); \n"\
" \n"\
"        // Vector from light subpath vertex to prev light subpath vertex (incoming light vector) \n"\
"        float3 light_wi = normalize(my_prev_light_vertex->position - light_dg.p); \n"\
"        // Outgoing light subpath vector \n"\
"        float3 light_wo = -eye_wo; \n"\
"        // BXDF at light subpath vertex \n"\
"        float3 light_bxdf = Bxdf_Evaluate(&light_dg, light_wi, light_wo, TEXTURE_ARGS); \n"\
"        // Light subpath vertex cosine factor \n"\
"        float light_dotnl = fabs(dot(light_dg.n, light_wo)); \n"\
" \n"\
"        // Light subpath throughput lightvertexM->lightvertexM-1->light \n"\
"        float3 light_contribution = my_prev_light_vertex->flow; \n"\
"        light_contribution *= (light_bxdf * light_dotnl); \n"\
" \n"\
"        // Now we need to figure out MIS weight for the path we are trying to build  \n"\
"        // connecting these vertices. \n"\
"        // MIS weight should account for all possible paths of the same length using  \n"\
"        // same vertices, but generated using different subpath lengths, for example \n"\
"        // Original: eye0 eye1 eye2 eye3 | connect | light3 light2 light1 light0 \n"\
"        // Variant 1: eye0 eye1 eye2 | connect | eye3 light3 light2 light1 light0 \n"\
"        // as if only eye0->eye1->eye2 was generated using eye walk,  \n"\
"        // and eye3->light3->light2->light1->light0 was generated using light walk. \n"\
" \n"\
"        // This is PDF of sampling eye subpath vertex from previous eye subpath vertex using eye walk strategy. \n"\
"        float eye_vertex_pdf_fwd = my_eye_vertex->pdf_forward; \n"\
"        // This is PDF of sampling light subpath vertex from previous light subpath vertex using light walk strategy. \n"\
"        float light_vertex_pdf_fwd = my_light_vertex->pdf_forward; \n"\
" \n"\
"        // Important: this is PDF of sampling eye subpath vertex from light subpath vertex using light walk strategy. \n"\
"        float eye_vertex_pdf_bwd = Pdf_ConvertSolidAngleToArea(Bxdf_GetPdf(&light_dg, light_wi, light_wo, TEXTURE_ARGS), \n"\
"            eye_dg.p, light_dg.p, light_dg.n); \n"\
"        // Important: this is PDF of sampling prev eye subpath vertex from eye subpath vertex using light walk strategy. \n"\
"        float eye_prev_vertex_pdf_bwd = Pdf_ConvertSolidAngleToArea(Bxdf_GetPdf(&eye_dg, eye_wi, eye_wo, TEXTURE_ARGS), \n"\
"            my_prev_eye_vertex->position, eye_dg.p, eye_dg.n); \n"\
" \n"\
"        // Important: this is PDF of sampling light subpath vertex from eye subpath vertex using eye walk strategy. \n"\
"        float light_vertex_pdf_bwd = Pdf_ConvertSolidAngleToArea(Bxdf_GetPdf(&eye_dg, eye_wo, eye_wi, TEXTURE_ARGS), \n"\
"            light_dg.p, eye_dg.p, eye_dg.n); \n"\
"        // Important: this is PDF of sampling prev light subpath vertex from light subpath vertex using eye walk strategy. \n"\
"        float light_prev_vertex_pdf_bwd = Pdf_ConvertSolidAngleToArea(Bxdf_GetPdf(&light_dg, light_wo, light_wi, TEXTURE_ARGS), \n"\
"            my_prev_light_vertex->position, light_dg.p, light_dg.n); \n"\
" \n"\
"        // We need sum of all weights \n"\
"        float weights_sum = (eye_vertex_pdf_bwd / eye_vertex_pdf_fwd); \n"\
"        float weights_eye = (eye_vertex_pdf_bwd / eye_vertex_pdf_fwd) * (eye_prev_vertex_pdf_bwd / my_prev_eye_vertex->pdf_forward); \n"\
"        weights_sum += weights_eye; \n"\
" \n"\
"        weights_sum += (light_vertex_pdf_bwd / light_vertex_pdf_fwd); \n"\
"        float weights_light = (light_vertex_pdf_bwd / light_vertex_pdf_fwd) * (light_prev_vertex_pdf_bwd / my_prev_light_vertex->pdf_forward); \n"\
"        weights_sum += weights_light; \n"\
" \n"\
"        // Iterate eye subpath \n"\
"        // (we already accounted for prev vertex, so start from one before, that's why -2) \n"\
"        for (int i = eye_vertex_index - 2; i >= 0; --i) \n"\
"        { \n"\
"            GLOBAL PathVertex const* v = &eye_subpath[BDPT_MAX_SUBPATH_LEN * pixel_idx + i]; \n"\
"            weights_eye *= (v->pdf_backward / v->pdf_forward); \n"\
"            weights_sum += weights_eye; \n"\
"        } \n"\
" \n"\
"        // Iterate light subpath \n"\
"        // (we already accounted for prev vertex, so start from one before, that's why -2) \n"\
"        for (int i = light_vertex_index - 2; i >= 0; --i) \n"\
"        { \n"\
"            GLOBAL PathVertex const* v = &light_subpath[BDPT_MAX_SUBPATH_LEN * global_id + i]; \n"\
"            weights_light *= (v->pdf_backward / v->pdf_forward); \n"\
"            weights_sum += weights_light;  \n"\
"        } \n"\
" \n"\
"        float mis_weight = 1.f / (1.f + weights_sum); \n"\
"        contributions[global_id].xyz = REASONABLE_RADIANCE(mis_weight * (eye_contribution * light_contribution) / (dist * dist)); \n"\
" \n"\
" \n"\
"        float3 connect_ray_o = eye_dg.p + CRAZY_LOW_DISTANCE * eye_dg.n; \n"\
"        float3 temp = light_dg.p - connect_ray_o; \n"\
"        float  connect_ray_length = 0.999f * length(temp); \n"\
"        float3 connect_ray_dir = eye_wo; \n"\
" \n"\
"        Ray_Init(&connection_rays[global_id], connect_ray_o, connect_ray_dir, connect_ray_length, 0.f, 0xFFFFFFFF); \n"\
"    } \n"\
" \n"\
" \n"\
"// Generate pixel indices withing a tile \n"\
"KERNEL void GenerateTileDomain( \n"\
"    // Output buffer width \n"\
"    int output_width, \n"\
"    // Output buffer height \n"\
"    int output_height, \n"\
"    // Tile offset within an output buffer \n"\
"    int tile_offset_x, \n"\
"    int tile_offset_y, \n"\
"    // Tile size \n"\
"    int tile_width, \n"\
"    int tile_height, \n"\
"    // Subtile size (tile can be organized in subtiles) \n"\
"    int subtile_width, \n"\
"    int subtile_height, \n"\
"    // Pixel indices \n"\
"    GLOBAL int* restrict pixel_idx0, \n"\
"    GLOBAL int* restrict pixel_idx1, \n"\
"    // Pixel count \n"\
"    GLOBAL int* restrict count \n"\
") \n"\
"{ \n"\
"    int tile_x = get_global_id(0); \n"\
"    int tile_y = get_global_id(1); \n"\
"    int tile_start_idx = output_width * tile_offset_y + tile_offset_x; \n"\
" \n"\
"    if (tile_x < tile_width && tile_y < tile_height) \n"\
"    { \n"\
"        // TODO: implement subtile support \n"\
"        int idx = tile_start_idx + tile_y * output_width + tile_x; \n"\
"        pixel_idx0[tile_y * tile_width + tile_x] = idx; \n"\
"        pixel_idx1[tile_y * tile_width + tile_x] = idx;  \n"\
"    } \n"\
" \n"\
"    if (tile_x == 0 && tile_y == 0) \n"\
"    { \n"\
"        *count = tile_width * tile_height; \n"\
"    } \n"\
"} \n"\
" \n"\
"///< Restore pixel indices after compaction \n"\
"KERNEL void FilterPathStream( \n"\
"    GLOBAL Intersection const* restrict intersections, \n"\
"    // Number of compacted indices \n"\
"    GLOBAL int const* restrict num_items, \n"\
"    GLOBAL int const* restrict pixel_indices, \n"\
"    GLOBAL Path* restrict paths, \n"\
"    // 1 or 0 for each item (include or exclude) \n"\
"    GLOBAL int* restrict predicate \n"\
") \n"\
"{ \n"\
"    int global_id = get_global_id(0); \n"\
" \n"\
"    // Handle only working subset \n"\
"    if (global_id < *num_items) \n"\
"    { \n"\
"        int pixelidx = pixel_indices[global_id]; \n"\
" \n"\
"        GLOBAL Path* path = paths + pixelidx; \n"\
" \n"\
"        if (Path_IsAlive(path)) \n"\
"        { \n"\
"            bool kill = (length(Path_GetThroughput(path)) < CRAZY_LOW_THROUGHPUT); \n"\
" \n"\
"            if (!kill) \n"\
"            { \n"\
"                predicate[global_id] = intersections[global_id].shapeid >= 0 ? 1 : 0; \n"\
"            } \n"\
"            else \n"\
"            { \n"\
"                Path_Kill(path); \n"\
"                predicate[global_id] = 0; \n"\
"            } \n"\
"        } \n"\
"        else \n"\
"        { \n"\
"            predicate[global_id] = 0; \n"\
"        } \n"\
"    } \n"\
"} \n"\
" \n"\
"KERNEL void IncrementSampleCounter( \n"\
"    // Number of rays \n"\
"    int num_rays, \n"\
"    GLOBAL int const* restrict pixel_indices, \n"\
"    // Radiance sample buffer \n"\
"    GLOBAL float4* restrict output \n"\
") \n"\
"{ \n"\
"    int global_id = get_global_id(0); \n"\
" \n"\
"    if (global_id < num_rays) \n"\
"    { \n"\
"        int pixel_idx = pixel_indices[global_id]; \n"\
"        // Start collecting samples \n"\
"        { \n"\
"            output[pixel_idx].w += 1.f; \n"\
"        } \n"\
"    } \n"\
"} \n"\
" \n"\
"KERNEL void GatherContributions( \n"\
"    // Number of rays \n"\
"    int num_rays, \n"\
"    GLOBAL int const* restrict pixel_indices, \n"\
"    // Shadow rays hits \n"\
"    GLOBAL int const* restrict shadow_hits, \n"\
"    // Light samples \n"\
"    GLOBAL float3 const* restrict contributions, \n"\
"    // Radiance sample buffer \n"\
"    GLOBAL float4* output \n"\
") \n"\
"{ \n"\
"    int global_id = get_global_id(0); \n"\
" \n"\
"    if (global_id < num_rays) \n"\
"    { \n"\
"        int pixel_idx = pixel_indices[global_id]; \n"\
"        // Prepare accumulator variable \n"\
"        float3 radiance = make_float3(0.f, 0.f, 0.f); \n"\
" \n"\
"        // Start collecting samples \n"\
"        { \n"\
"            // If shadow ray global_id't hit anything and reached skydome \n"\
"            if (shadow_hits[global_id] == -1) \n"\
"            { \n"\
"                // Add its contribution to radiance accumulator \n"\
"                radiance += contributions[global_id]; \n"\
"                output[pixel_idx].xyz += radiance; \n"\
"            } \n"\
"        } \n"\
"    } \n"\
"} \n"\
" \n"\
"KERNEL void GatherCausticContributions( \n"\
"    int num_items, \n"\
"    int width, \n"\
"    int height, \n"\
"    GLOBAL int const* restrict shadow_hits, \n"\
"    GLOBAL float3 const* restrict contributions, \n"\
"    GLOBAL float2 const* restrict image_plane_positions, \n"\
"    GLOBAL float4* restrict output \n"\
") \n"\
"{ \n"\
"    int global_id = get_global_id(0); \n"\
" \n"\
"    if (global_id < num_items) \n"\
"    { \n"\
"        // Start collecting samples \n"\
"        { \n"\
"            // If shadow ray didn't hit anything and reached skydome \n"\
"            if (shadow_hits[global_id] == -1) \n"\
"            { \n"\
"                // Add its contribution to radiance accumulator \n"\
"                float3 radiance = contributions[global_id]; \n"\
"                float2 uv = image_plane_positions[global_id]; \n"\
"                int x = clamp((int)(uv.x * width), 0, width - 1); \n"\
"                int y = clamp((int)(uv.y * height), 0, height - 1); \n"\
"                output[y * width + x].xyz += radiance; \n"\
"            } \n"\
"        } \n"\
"    } \n"\
"} \n"\
" \n"\
"// Restore pixel indices after compaction \n"\
"KERNEL void RestorePixelIndices( \n"\
"    // Compacted indices \n"\
"    GLOBAL int const* restrict compacted_indices, \n"\
"    // Number of compacted indices \n"\
"    GLOBAL int* restrict num_items, \n"\
"    // Previous pixel indices \n"\
"    GLOBAL int const* restrict prev_indices, \n"\
"    // New pixel indices \n"\
"    GLOBAL int* restrict new_indices \n"\
") \n"\
"{ \n"\
"    int global_id = get_global_id(0); \n"\
" \n"\
"    // Handle only working subset \n"\
"    if (global_id < *num_items) \n"\
"    { \n"\
"        new_indices[global_id] = prev_indices[compacted_indices[global_id]]; \n"\
"    } \n"\
"} \n"\
" \n"\
"// Copy data to interop texture if supported \n"\
"KERNEL void ApplyGammaAndCopyData( \n"\
"    // \n"\
"    GLOBAL float4 const* data, \n"\
"    int width, \n"\
"    int height, \n"\
"    float gamma, \n"\
"    write_only image2d_t img \n"\
") \n"\
"{ \n"\
"    int global_id = get_global_id(0); \n"\
"    int x = global_id % width; \n"\
"    int y = global_id / width; \n"\
" \n"\
"    if (x < width && y < height) \n"\
"    { \n"\
"        float4 v = data[global_id]; \n"\
"        float4 val = clamp(native_powr(v / v.w, 1.f / gamma), 0.f, 1.f); \n"\
"        write_imagef(img, make_int2(x, y), val); \n"\
"    } \n"\
"} \n"\
" \n"\
" \n"\
"INLINE void atomic_add_float(volatile __global float* addr, float val) \n"\
"{ \n"\
"    union { \n"\
"        unsigned int u32; \n"\
"        float        f32; \n"\
"    } next, expected, current; \n"\
"    current.f32 = *addr; \n"\
"    do { \n"\
"        expected.f32 = current.f32; \n"\
"        next.f32 = expected.f32 + val; \n"\
"        current.u32 = atomic_cmpxchg((volatile __global unsigned int *)addr, \n"\
"            expected.u32, next.u32); \n"\
"    } while (current.u32 != expected.u32); \n"\
"} \n"\
" \n"\
"KERNEL void ConnectCaustics( \n"\
"    int num_items, \n"\
"    int eye_vertex_index, \n"\
"    GLOBAL PathVertex const* restrict light_subpath, \n"\
"    GLOBAL int const* restrict light_subpath_length, \n"\
"    GLOBAL Camera const* restrict camera, \n"\
"    GLOBAL Material const* restrict materials, \n"\
"    TEXTURE_ARG_LIST, \n"\
"    GLOBAL ray* restrict connection_rays, \n"\
"    GLOBAL float4* restrict contributions, \n"\
"    GLOBAL float2* restrict image_plane_positions \n"\
") \n"\
"{ \n"\
"    int global_id = get_global_id(0); \n"\
" \n"\
"    if (global_id >= num_items) \n"\
"        return; \n"\
" \n"\
"    if (eye_vertex_index >= light_subpath_length[global_id]) \n"\
"    { \n"\
"        contributions[global_id].xyz = 0.f; \n"\
"        Ray_SetInactive(connection_rays + global_id); \n"\
"        return; \n"\
"    } \n"\
" \n"\
"    GLOBAL PathVertex const* my_light_vertex = light_subpath + BDPT_MAX_SUBPATH_LEN * global_id + eye_vertex_index; \n"\
"    GLOBAL PathVertex const* my_prev_light_vertex = light_subpath + BDPT_MAX_SUBPATH_LEN * global_id + eye_vertex_index - 1; \n"\
" \n"\
"    DifferentialGeometry eye_dg; \n"\
"    eye_dg.p = camera->p; \n"\
"    eye_dg.n = camera->forward; \n"\
"    eye_dg.ng = camera->forward; \n"\
"    eye_dg.uv = 0.f; \n"\
"    eye_dg.dpdu = GetOrthoVector(eye_dg.n); \n"\
"    eye_dg.dpdv = cross(eye_dg.n, eye_dg.dpdu); \n"\
"    DifferentialGeometry_CalculateTangentTransforms(&eye_dg); \n"\
" \n"\
"    DifferentialGeometry light_dg; \n"\
"    light_dg.p = my_light_vertex->position; \n"\
"    light_dg.n = my_light_vertex->shading_normal; \n"\
"    light_dg.ng = my_light_vertex->geometric_normal; \n"\
"    light_dg.uv = my_light_vertex->uv; \n"\
"    light_dg.dpdu = GetOrthoVector(light_dg.n); \n"\
"    light_dg.dpdv = cross(light_dg.dpdu, light_dg.n); \n"\
"    light_dg.mat = materials[my_light_vertex->material_index]; \n"\
"    light_dg.mat.fresnel = 1.f; \n"\
"    DifferentialGeometry_CalculateTangentTransforms(&light_dg); \n"\
" \n"\
"    float3 eye_wo = normalize(light_dg.p - eye_dg.p); \n"\
"    float3 light_wi = normalize(my_prev_light_vertex->position - light_dg.p); \n"\
"    float3 light_wo = -eye_wo; \n"\
" \n"\
"    float3 light_bxdf = Bxdf_Evaluate(&light_dg, light_wi, light_wo, TEXTURE_ARGS); \n"\
"    float light_pdf = Bxdf_GetPdf(&light_dg, light_wi, light_wo, TEXTURE_ARGS); \n"\
"    float light_dotnl = max(dot(light_dg.n, light_wo), 0.f); \n"\
" \n"\
"    float3 light_contribution = my_prev_light_vertex->flow * (light_bxdf) / my_light_vertex->pdf_forward; \n"\
" \n"\
"    //float w0 = camera_pdf / (camera_pdf + light_pdf); \n"\
"    //float w1 = light_pdf / (camera_pdf + light_pdf); \n"\
" \n"\
"    float dist = length(light_dg.p - eye_dg.p); \n"\
"    float3 val = (light_contribution) / (dist * dist); \n"\
"    contributions[global_id].xyz = REASONABLE_RADIANCE(val); \n"\
" \n"\
"    float  connect_ray_length = 0.99f * length(light_dg.p - eye_dg.p); \n"\
"    float3 connect_ray_dir = eye_wo; \n"\
"    float3 connect_ray_o = eye_dg.p; \n"\
" \n"\
"    Ray_Init(connection_rays + global_id, connect_ray_o, connect_ray_dir, connect_ray_length, 0.f, 0xFFFFFFFF); \n"\
" \n"\
"    ray r; \n"\
"    r.o.xyz = eye_dg.p; \n"\
"    r.o.w = CRAZY_HIGH_DISTANCE; \n"\
"    r.d.xyz = eye_wo; \n"\
" \n"\
"    float3 v0, v1, v2, v3; \n"\
"    float2 ext = 0.5f * camera->dim; \n"\
"    v0 = camera->p + camera->focal_length * camera->forward - ext.x * camera->right - ext.y * camera->up; \n"\
"    v1 = camera->p + camera->focal_length * camera->forward + ext.x * camera->right - ext.y * camera->up; \n"\
"    v2 = camera->p + camera->focal_length * camera->forward + ext.x * camera->right + ext.y * camera->up; \n"\
"    v3 = camera->p + camera->focal_length * camera->forward - ext.x * camera->right + ext.y * camera->up; \n"\
" \n"\
"    float a, b; \n"\
"    float3 p; \n"\
"    if (IntersectTriangle(&r, v0, v1, v2, &a, &b)) \n"\
"    { \n"\
"        p = (1.f - a - b) * v0 + a * v1 + b * v2; \n"\
"    } \n"\
"    else if (IntersectTriangle(&r, v0, v2, v3, &a, &b)) \n"\
"    { \n"\
"        p = (1.f - a - b) * v0 + a * v2 + b * v3; \n"\
"    } \n"\
"    else \n"\
"    { \n"\
"        contributions[global_id].xyz = 0.f; \n"\
"        Ray_SetInactive(connection_rays + global_id); \n"\
"        return; \n"\
"    } \n"\
" \n"\
"    float3 imgp = p - v0; \n"\
"    float2 impuv; \n"\
"    impuv.x = clamp(dot(imgp, v1 - v0) / dot(v1 - v0, v1 - v0), 0.f, 1.f); \n"\
"    impuv.y = clamp(dot(imgp, v3 - v0) / dot(v3 - v0, v3 - v0), 0.f, 1.f); \n"\
"    image_plane_positions[global_id] = impuv; \n"\
"} \n"\
" \n"\
" \n"\
" \n"\
;
static const char g_isect_opencl[]= \
"/********************************************************************** \n"\
" Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved. \n"\
"  \n"\
" Permission is hereby granted, free of charge, to any person obtaining a copy \n"\
" of this software and associated documentation files (the \"Software\"), to deal \n"\
" in the Software without restriction, including without limitation the rights \n"\
" to use, copy, modify, merge, publish, distribute, sublicense, and/or sell \n"\
" copies of the Software, and to permit persons to whom the Software is \n"\
" furnished to do so, subject to the following conditions: \n"\
"  \n"\
" The above copyright notice and this permission notice shall be included in \n"\
" all copies or substantial portions of the Software. \n"\
"  \n"\
" THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR \n"\
" IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, \n"\
" FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE \n"\
" AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER \n"\
" LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, \n"\
" OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN \n"\
" THE SOFTWARE. \n"\
" ********************************************************************/ \n"\
"#ifndef ISECT_CL \n"\
"#define ISECT_CL \n"\
" \n"\
"/// Intersection data returned by RadeonRays \n"\
"typedef struct _Intersection \n"\
"{ \n"\
"    // id of a shape \n"\
"    int shapeid; \n"\
"    // Primitive index \n"\
"    int primid; \n"\
"    // Padding elements \n"\
"    int padding0; \n"\
"    int padding1; \n"\
"         \n"\
"    // uv - hit barycentrics, w - ray distance \n"\
"    float4 uvwt; \n"\
"} Intersection; \n"\
" \n"\
"float Intersection_GetDistance(__global Intersection const* isect) \n"\
"{ \n"\
"    return isect->uvwt.w; \n"\
"} \n"\
" \n"\
"float2 Intersection_GetBarycentrics(__global Intersection const* isect) \n"\
"{ \n"\
"    return isect->uvwt.xy; \n"\
"} \n"\
" \n"\
"#endif \n"\
;
static const char g_light_opencl[]= \
"/********************************************************************** \n"\
"Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved. \n"\
" \n"\
"Permission is hereby granted, free of charge, to any person obtaining a copy \n"\
"of this software and associated documentation files (the \"Software\"), to deal \n"\
"in the Software without restriction, including without limitation the rights \n"\
"to use, copy, modify, merge, publish, distribute, sublicense, and/or sell \n"\
"copies of the Software, and to permit persons to whom the Software is \n"\
"furnished to do so, subject to the following conditions: \n"\
" \n"\
"The above copyright notice and this permission notice shall be included in \n"\
"all copies or substantial portions of the Software. \n"\
" \n"\
"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR \n"\
"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, \n"\
"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE \n"\
"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER \n"\
"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, \n"\
"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN \n"\
"THE SOFTWARE. \n"\
"********************************************************************/ \n"\
"#ifndef LIGHT_CL \n"\
"#define LIGHT_CL \n"\
" \n"\
" \n"\
" \n"\
"INLINE \n"\
"bool IntersectTriangle(ray const* r, float3 v1, float3 v2, float3 v3, float* a, float* b) \n"\
"{ \n"\
"    const float3 e1 = v2 - v1; \n"\
"    const float3 e2 = v3 - v1; \n"\
"    const float3 s1 = cross(r->d.xyz, e2); \n"\
"    const float  invd = native_recip(dot(s1, e1)); \n"\
"    const float3 d = r->o.xyz - v1; \n"\
"    const float  b1 = dot(d, s1) * invd; \n"\
"    const float3 s2 = cross(d, e1); \n"\
"    const float  b2 = dot(r->d.xyz, s2) * invd; \n"\
"    const float temp = dot(e2, s2) * invd; \n"\
" \n"\
"    if (b1 < 0.f || b1 > 1.f || b2 < 0.f || b1 + b2 > 1.f) \n"\
"    { \n"\
"        return false; \n"\
"    } \n"\
"    else \n"\
"    { \n"\
"        *a = b1; \n"\
"        *b = b2; \n"\
"        return true; \n"\
"    } \n"\
"} \n"\
" \n"\
"/* \n"\
" Environment light \n"\
" */ \n"\
"/// Get intensity for a given direction \n"\
"float3 EnvironmentLight_GetLe(// Light \n"\
"                              Light const* light, \n"\
"                              // Scene \n"\
"                              Scene const* scene, \n"\
"                              // Geometry \n"\
"                              DifferentialGeometry const* dg, \n"\
"                              // Direction to light source \n"\
"                              float3* wo, \n"\
"                              // Textures \n"\
"                              TEXTURE_ARG_LIST \n"\
"                              ) \n"\
"{ \n"\
"    // Sample envmap \n"\
"    *wo *= 100000.f; \n"\
"    // \n"\
"    return light->multiplier * Texture_SampleEnvMap(normalize(*wo), TEXTURE_ARGS_IDX(light->tex)); \n"\
"} \n"\
" \n"\
"/// Sample direction to the light \n"\
"float3 EnvironmentLight_Sample(// Light \n"\
"                               Light const* light, \n"\
"                               // Scene \n"\
"                               Scene const* scene, \n"\
"                               // Geometry \n"\
"                               DifferentialGeometry const* dg, \n"\
"                               // Textures \n"\
"                               TEXTURE_ARG_LIST, \n"\
"                               // Sample \n"\
"                               float2 sample, \n"\
"                               // Direction to light source \n"\
"                               float3* wo, \n"\
"                               // PDF \n"\
"                               float* pdf \n"\
"                              ) \n"\
"{ \n"\
"    float3 d = Sample_MapToHemisphere(sample, dg->n, 0.f); \n"\
" \n"\
"    // Generate direction \n"\
"    *wo = 100000.f * d; \n"\
" \n"\
"    // Envmap PDF \n"\
"    *pdf = 1.f / (2.f * PI); \n"\
" \n"\
"    // Sample envmap \n"\
"    return light->multiplier * Texture_SampleEnvMap(d, TEXTURE_ARGS_IDX(light->tex)); \n"\
"} \n"\
" \n"\
"/// Get PDF for a given direction \n"\
"float EnvironmentLight_GetPdf( \n"\
"                              // Light \n"\
"                              Light const* light, \n"\
"                              // Scene \n"\
"                              Scene const* scene, \n"\
"                              // Geometry \n"\
"                              DifferentialGeometry const* dg, \n"\
"                              // Direction to light source \n"\
"                              float3 wo, \n"\
"                              // Textures \n"\
"                              TEXTURE_ARG_LIST \n"\
"                              ) \n"\
"{ \n"\
"    return 1.f / (2.f * PI); \n"\
"} \n"\
" \n"\
" \n"\
"/* \n"\
" Area light \n"\
" */ \n"\
"// Get intensity for a given direction \n"\
"float3 AreaLight_GetLe(// Emissive object \n"\
"                       Light const* light, \n"\
"                       // Scene \n"\
"                       Scene const* scene, \n"\
"                       // Geometry \n"\
"                       DifferentialGeometry const* dg, \n"\
"                       // Direction to light source \n"\
"                       float3* wo, \n"\
"                       // Textures \n"\
"                       TEXTURE_ARG_LIST \n"\
"                       ) \n"\
"{ \n"\
"    ray r; \n"\
"    r.o.xyz = dg->p; \n"\
"    r.d.xyz = *wo; \n"\
" \n"\
"    int shapeidx = light->shapeidx; \n"\
"    int primidx = light->primidx; \n"\
" \n"\
"    float3 v0, v1, v2; \n"\
"    Scene_GetTriangleVertices(scene, shapeidx, primidx, &v0, &v1, &v2); \n"\
" \n"\
"    float a, b; \n"\
"    if (IntersectTriangle(&r, v0, v1, v2, &a, &b)) \n"\
"    { \n"\
"        float3 n; \n"\
"        float3 p; \n"\
"        float2 tx; \n"\
"        float area; \n"\
"        Scene_InterpolateAttributes(scene, shapeidx, primidx, make_float2(a, b), &p, &n, &tx, &area); \n"\
" \n"\
"        float3 d = p - dg->p; \n"\
"        *wo = d; \n"\
" \n"\
"        int mat_idx = Scene_GetMaterialIndex(scene, shapeidx, primidx); \n"\
"        Material mat = scene->materials[mat_idx]; \n"\
" \n"\
"        const float3 ke = Texture_GetValue3f(mat.simple.kx.xyz, tx, TEXTURE_ARGS_IDX(mat.simple.kxmapidx)); \n"\
"        return ke; \n"\
"    } \n"\
"    else \n"\
"    { \n"\
"        return make_float3(0.f, 0.f, 0.f); \n"\
"    } \n"\
"} \n"\
" \n"\
"/// Sample direction to the light \n"\
"float3 AreaLight_Sample(// Emissive object \n"\
"                        Light const* light, \n"\
"                        // Scene \n"\
"                        Scene const* scene, \n"\
"                        // Geometry \n"\
"                        DifferentialGeometry const* dg, \n"\
"                        // Textures \n"\
"                        TEXTURE_ARG_LIST, \n"\
"                        // Sample \n"\
"                        float2 sample, \n"\
"                        // Direction to light source \n"\
"                        float3* wo, \n"\
"                        // PDF \n"\
"                        float* pdf) \n"\
"{ \n"\
"    int shapeidx = light->shapeidx; \n"\
"    int primidx = light->primidx; \n"\
" \n"\
"    // Generate sample on triangle \n"\
"    float r0 = sample.x; \n"\
"    float r1 = sample.y; \n"\
" \n"\
"    // Convert random to barycentric coords \n"\
"    float2 uv; \n"\
"    uv.x = native_sqrt(r0) * (1.f - r1); \n"\
"    uv.y = native_sqrt(r0) * r1; \n"\
" \n"\
"    float3 n; \n"\
"    float3 p; \n"\
"    float2 tx; \n"\
"    float area; \n"\
"    Scene_InterpolateAttributes(scene, shapeidx, primidx, uv, &p, &n, &tx, &area); \n"\
" \n"\
"    *wo = p - dg->p; \n"\
" \n"\
"    int mat_idx = Scene_GetMaterialIndex(scene, shapeidx, primidx); \n"\
"    Material mat = scene->materials[mat_idx]; \n"\
" \n"\
"    const float3 ke = Texture_GetValue3f(mat.simple.kx.xyz, tx, TEXTURE_ARGS_IDX(mat.simple.kxmapidx)); \n"\
" \n"\
"    float3 v = -normalize(*wo); \n"\
" \n"\
"    float ndotv = dot(n, v); \n"\
" \n"\
"    if (ndotv > 0.f) \n"\
"    { \n"\
"        float dist2 = dot(*wo, *wo); \n"\
"        float denom = fabs(ndotv) * area; \n"\
"        *pdf = denom > 0.f ? dist2 / denom : 0.f; \n"\
"        return dist2 > 0.f ? ke * ndotv / dist2 : 0.f; \n"\
"    } \n"\
"    else \n"\
"    { \n"\
"        *pdf = 0.f; \n"\
"        return 0.f; \n"\
"    } \n"\
"} \n"\
" \n"\
"/// Get PDF for a given direction \n"\
"float AreaLight_GetPdf(// Emissive object \n"\
"                       Light const* light, \n"\
"                       // Scene \n"\
"                       Scene const* scene, \n"\
"                       // Geometry \n"\
"                       DifferentialGeometry const* dg, \n"\
"                       // Direction to light source \n"\
"                       float3 wo, \n"\
"                       // Textures \n"\
"                       TEXTURE_ARG_LIST \n"\
"                       ) \n"\
"{ \n"\
"    ray r; \n"\
"    r.o.xyz = dg->p; \n"\
"    r.d.xyz = wo; \n"\
" \n"\
"    int shapeidx = light->shapeidx; \n"\
"    int primidx = light->primidx; \n"\
" \n"\
"    float3 v0, v1, v2; \n"\
"    Scene_GetTriangleVertices(scene, shapeidx, primidx, &v0, &v1, &v2); \n"\
" \n"\
"    // Intersect ray against this area light \n"\
"    float a, b; \n"\
"    if (IntersectTriangle(&r, v0, v1, v2, &a, &b)) \n"\
"    { \n"\
"        float3 n; \n"\
"        float3 p; \n"\
"        float2 tx; \n"\
"        float area; \n"\
"        Scene_InterpolateAttributes(scene, shapeidx, primidx, make_float2(a, b), &p, &n, &tx, &area); \n"\
" \n"\
"        float3 d = p - dg->p; \n"\
"        float dist2 = dot(d, d) ; \n"\
"        float denom = (fabs(dot(-normalize(d), n)) * area); \n"\
" \n"\
"        return denom > 0.f ? dist2 / denom : 0.f; \n"\
"    } \n"\
"    else \n"\
"    { \n"\
"        return 0.f; \n"\
"    } \n"\
"} \n"\
" \n"\
"float3 AreaLight_SampleVertex( \n"\
"    // Emissive object \n"\
"    Light const* light, \n"\
"    // Scene \n"\
"    Scene const* scene, \n"\
"    // Textures \n"\
"    TEXTURE_ARG_LIST, \n"\
"    // Sample \n"\
"    float2 sample0, \n"\
"    float2 sample1, \n"\
"    // Direction to light source \n"\
"    float3* p, \n"\
"    float3* n, \n"\
"    float3* wo, \n"\
"    // PDF \n"\
"    float* pdf) \n"\
"{ \n"\
"    int shapeidx = light->shapeidx; \n"\
"    int primidx = light->primidx; \n"\
" \n"\
"    // Generate sample on triangle \n"\
"    float r0 = sample0.x; \n"\
"    float r1 = sample0.y; \n"\
" \n"\
"    // Convert random to barycentric coords \n"\
"    float2 uv; \n"\
"    uv.x = native_sqrt(r0) * (1.f - r1); \n"\
"    uv.y = native_sqrt(r0) * r1; \n"\
" \n"\
"    float2 tx; \n"\
"    float area; \n"\
"    Scene_InterpolateAttributes(scene, shapeidx, primidx, uv, p, n, &tx, &area); \n"\
" \n"\
"    int mat_idx = Scene_GetMaterialIndex(scene, shapeidx, primidx); \n"\
"    Material mat = scene->materials[mat_idx]; \n"\
" \n"\
"    const float3 ke = Texture_GetValue3f(mat.simple.kx.xyz, tx, TEXTURE_ARGS_IDX(mat.simple.kxmapidx)); \n"\
" \n"\
"    *wo = Sample_MapToHemisphere(sample1, *n, 1.f); \n"\
"    *pdf = (1.f / area) * fabs(dot(*n, *wo)) / PI; \n"\
" \n"\
"    return ke; \n"\
"} \n"\
" \n"\
"/* \n"\
"Directional light \n"\
"*/ \n"\
"// Get intensity for a given direction \n"\
"float3 DirectionalLight_GetLe(// Emissive object \n"\
"    Light const* light, \n"\
"    // Scene \n"\
"    Scene const* scene, \n"\
"    // Geometry \n"\
"    DifferentialGeometry const* dg, \n"\
"    // Direction to light source \n"\
"    float3* wo, \n"\
"    // Textures \n"\
"    TEXTURE_ARG_LIST \n"\
") \n"\
"{ \n"\
"    return 0.f; \n"\
"} \n"\
" \n"\
"/// Sample direction to the light \n"\
"float3 DirectionalLight_Sample(// Emissive object \n"\
"    Light const* light, \n"\
"    // Scene \n"\
"    Scene const* scene, \n"\
"    // Geometry \n"\
"    DifferentialGeometry const* dg, \n"\
"    // Textures \n"\
"    TEXTURE_ARG_LIST, \n"\
"    // Sample \n"\
"    float2 sample, \n"\
"    // Direction to light source \n"\
"    float3* wo, \n"\
"    // PDF \n"\
"    float* pdf) \n"\
"{ \n"\
"    *wo = 100000.f * -light->d; \n"\
"    *pdf = 1.f; \n"\
"    return light->intensity; \n"\
"} \n"\
" \n"\
"/// Get PDF for a given direction \n"\
"float DirectionalLight_GetPdf(// Emissive object \n"\
"    Light const* light, \n"\
"    // Scene \n"\
"    Scene const* scene, \n"\
"    // Geometry \n"\
"    DifferentialGeometry const* dg, \n"\
"    // Direction to light source \n"\
"    float3 wo, \n"\
"    // Textures \n"\
"    TEXTURE_ARG_LIST \n"\
") \n"\
"{ \n"\
"    return 0.f; \n"\
"} \n"\
" \n"\
"/* \n"\
" Point light \n"\
" */ \n"\
"// Get intensity for a given direction \n"\
"float3 PointLight_GetLe(// Emissive object \n"\
"                              Light const* light, \n"\
"                              // Scene \n"\
"                              Scene const* scene, \n"\
"                              // Geometry \n"\
"                              DifferentialGeometry const* dg, \n"\
"                              // Direction to light source \n"\
"                              float3* wo, \n"\
"                              // Textures \n"\
"                              TEXTURE_ARG_LIST \n"\
"                              ) \n"\
"{ \n"\
"    return 0.f; \n"\
"} \n"\
" \n"\
"/// Sample direction to the light \n"\
"float3 PointLight_Sample(// Emissive object \n"\
"                               Light const* light, \n"\
"                               // Scene \n"\
"                               Scene const* scene, \n"\
"                               // Geometry \n"\
"                               DifferentialGeometry const* dg, \n"\
"                               // Textures \n"\
"                               TEXTURE_ARG_LIST, \n"\
"                               // Sample \n"\
"                               float2 sample, \n"\
"                               // Direction to light source \n"\
"                               float3* wo, \n"\
"                               // PDF \n"\
"                               float* pdf) \n"\
"{ \n"\
"    *wo = light->p - dg->p; \n"\
"    *pdf = 1.f; \n"\
"    return light->intensity / dot(*wo, *wo); \n"\
"} \n"\
" \n"\
"/// Get PDF for a given direction \n"\
"float PointLight_GetPdf(// Emissive object \n"\
"                              Light const* light, \n"\
"                              // Scene \n"\
"                              Scene const* scene, \n"\
"                              // Geometry \n"\
"                              DifferentialGeometry const* dg, \n"\
"                              // Direction to light source \n"\
"                              float3 wo, \n"\
"                              // Textures \n"\
"                              TEXTURE_ARG_LIST \n"\
"                              ) \n"\
"{ \n"\
"    return 0.f; \n"\
"} \n"\
" \n"\
"/// Sample vertex on the light \n"\
"float3 PointLight_SampleVertex( \n"\
"    // Light object \n"\
"    Light const* light, \n"\
"    // Scene \n"\
"    Scene const* scene, \n"\
"    // Textures \n"\
"    TEXTURE_ARG_LIST, \n"\
"    // Sample \n"\
"    float2 sample0, \n"\
"    float2 sample1, \n"\
"    // Direction to light source \n"\
"    float3* p, \n"\
"    float3* n, \n"\
"    float3* wo, \n"\
"    // PDF \n"\
"    float* pdf) \n"\
"{ \n"\
"    *p = light->p; \n"\
"    *n = make_float3(0.f, 1.f, 0.f); \n"\
"    *wo = Sample_MapToSphere(sample0); \n"\
"    *pdf = 1.f / (4.f * PI); \n"\
"    return light->intensity; \n"\
"} \n"\
" \n"\
"/* \n"\
" Spot light \n"\
" */ \n"\
"// Get intensity for a given direction \n"\
"float3 SpotLight_GetLe(// Emissive object \n"\
"                        Light const* light, \n"\
"                        // Scene \n"\
"                        Scene const* scene, \n"\
"                        // Geometry \n"\
"                        DifferentialGeometry const* dg, \n"\
"                        // Direction to light source \n"\
"                        float3* wo, \n"\
"                        // Textures \n"\
"                        TEXTURE_ARG_LIST \n"\
"                        ) \n"\
"{ \n"\
"    return 0.f; \n"\
"} \n"\
" \n"\
"/// Sample direction to the light \n"\
"float3 SpotLight_Sample(// Emissive object \n"\
"                         Light const* light, \n"\
"                         // Scene \n"\
"                         Scene const* scene, \n"\
"                         // Geometry \n"\
"                         DifferentialGeometry const* dg, \n"\
"                         // Textures \n"\
"                         TEXTURE_ARG_LIST, \n"\
"                         // Sample \n"\
"                         float2 sample, \n"\
"                         // Direction to light source \n"\
"                         float3* wo, \n"\
"                         // PDF \n"\
"                         float* pdf) \n"\
"{ \n"\
"    *wo = light->p - dg->p; \n"\
"    float ddotwo = dot(-normalize(*wo), light->d); \n"\
"     \n"\
"    if (ddotwo > light->oa) \n"\
"    { \n"\
"        float3 intensity = light->intensity / dot(*wo, *wo); \n"\
"        *pdf = 1.f; \n"\
"        return ddotwo > light->ia ? intensity : intensity * (1.f - (light->ia - ddotwo) / (light->ia - light->oa)); \n"\
"    } \n"\
"    else \n"\
"    { \n"\
"        *pdf = 0.f; \n"\
"        return 0.f; \n"\
"    } \n"\
"} \n"\
" \n"\
"/// Get PDF for a given direction \n"\
"float SpotLight_GetPdf(// Emissive object \n"\
"                        Light const* light, \n"\
"                        // Scene \n"\
"                        Scene const* scene, \n"\
"                        // Geometry \n"\
"                        DifferentialGeometry const* dg, \n"\
"                        // Direction to light source \n"\
"                        float3 wo, \n"\
"                        // Textures \n"\
"                        TEXTURE_ARG_LIST \n"\
"                        ) \n"\
"{ \n"\
"    return 0.f; \n"\
"} \n"\
" \n"\
" \n"\
"/* \n"\
" Dispatch calls \n"\
" */ \n"\
" \n"\
"/// Get intensity for a given direction \n"\
"float3 Light_GetLe(// Light index \n"\
"                   int idx, \n"\
"                   // Scene \n"\
"                   Scene const* scene, \n"\
"                   // Geometry \n"\
"                   DifferentialGeometry const* dg, \n"\
"                   // Direction to light source \n"\
"                   float3* wo, \n"\
"                   // Textures \n"\
"                   TEXTURE_ARG_LIST \n"\
"                   ) \n"\
"{ \n"\
"    Light light = scene->lights[idx]; \n"\
" \n"\
"    switch(light.type) \n"\
"    { \n"\
"        case kIbl: \n"\
"            return EnvironmentLight_GetLe(&light, scene, dg, wo, TEXTURE_ARGS); \n"\
"        case kArea: \n"\
"            return AreaLight_GetLe(&light, scene, dg, wo, TEXTURE_ARGS); \n"\
"        case kDirectional: \n"\
"            return DirectionalLight_GetLe(&light, scene, dg, wo, TEXTURE_ARGS); \n"\
"        case kPoint: \n"\
"            return PointLight_GetLe(&light, scene, dg, wo, TEXTURE_ARGS); \n"\
"        case kSpot: \n"\
"            return SpotLight_GetLe(&light, scene, dg, wo, TEXTURE_ARGS); \n"\
"    } \n"\
" \n"\
"    return make_float3(0.f, 0.f, 0.f); \n"\
"} \n"\
" \n"\
"/// Sample direction to the light \n"\
"float3 Light_Sample(// Light index \n"\
"                    int idx, \n"\
"                    // Scene \n"\
"                    Scene const* scene, \n"\
"                    // Geometry \n"\
"                    DifferentialGeometry const* dg, \n"\
"                    // Textures \n"\
"                    TEXTURE_ARG_LIST, \n"\
"                    // Sample \n"\
"                    float2 sample, \n"\
"                    // Direction to light source \n"\
"                    float3* wo, \n"\
"                    // PDF \n"\
"                    float* pdf) \n"\
"{ \n"\
"    Light light = scene->lights[idx]; \n"\
" \n"\
"    switch(light.type) \n"\
"    { \n"\
"        case kIbl: \n"\
"            return EnvironmentLight_Sample(&light, scene, dg, TEXTURE_ARGS, sample, wo, pdf); \n"\
"        case kArea: \n"\
"            return AreaLight_Sample(&light, scene, dg, TEXTURE_ARGS, sample, wo, pdf); \n"\
"        case kDirectional: \n"\
"            return DirectionalLight_Sample(&light, scene, dg, TEXTURE_ARGS, sample, wo, pdf); \n"\
"        case kPoint: \n"\
"            return PointLight_Sample(&light, scene, dg, TEXTURE_ARGS, sample, wo, pdf); \n"\
"        case kSpot: \n"\
"            return SpotLight_Sample(&light, scene, dg, TEXTURE_ARGS, sample, wo, pdf); \n"\
"    } \n"\
" \n"\
"    *pdf = 0.f; \n"\
"    return make_float3(0.f, 0.f, 0.f); \n"\
"} \n"\
" \n"\
"/// Get PDF for a given direction \n"\
"float Light_GetPdf(// Light index \n"\
"                   int idx, \n"\
"                   // Scene \n"\
"                   Scene const* scene, \n"\
"                   // Geometry \n"\
"                   DifferentialGeometry const* dg, \n"\
"                   // Direction to light source \n"\
"                   float3 wo, \n"\
"                   // Textures \n"\
"                   TEXTURE_ARG_LIST \n"\
"                   ) \n"\
"{ \n"\
"    Light light = scene->lights[idx]; \n"\
" \n"\
"    switch(light.type) \n"\
"    { \n"\
"        case kIbl: \n"\
"            return EnvironmentLight_GetPdf(&light, scene, dg, wo, TEXTURE_ARGS); \n"\
"        case kArea: \n"\
"            return AreaLight_GetPdf(&light, scene, dg, wo, TEXTURE_ARGS); \n"\
"        case kDirectional: \n"\
"            return DirectionalLight_GetPdf(&light, scene, dg, wo, TEXTURE_ARGS); \n"\
"        case kPoint: \n"\
"            return PointLight_GetPdf(&light, scene, dg, wo, TEXTURE_ARGS); \n"\
"        case kSpot: \n"\
"            return SpotLight_GetPdf(&light, scene, dg, wo, TEXTURE_ARGS); \n"\
"    } \n"\
" \n"\
"    return 0.f; \n"\
"} \n"\
" \n"\
"/// Sample vertex on the light \n"\
"float3 Light_SampleVertex( \n"\
"    // Light index \n"\
"    int idx, \n"\
"    // Scene \n"\
"    Scene const* scene, \n"\
"    // Textures \n"\
"    TEXTURE_ARG_LIST, \n"\
"    // Sample \n"\
"    float2 sample0, \n"\
"    float2 sample1, \n"\
"    // Point on the light \n"\
"    float3* p, \n"\
"    // Normal at light vertex \n"\
"    float3* n, \n"\
"    // Direction \n"\
"    float3* wo, \n"\
"    // PDF \n"\
"    float* pdf) \n"\
"{ \n"\
"    Light light = scene->lights[idx]; \n"\
" \n"\
"    switch (light.type) \n"\
"    { \n"\
"        case kArea: \n"\
"            return AreaLight_SampleVertex(&light, scene, TEXTURE_ARGS, sample0, sample1, p, n, wo, pdf); \n"\
"        case kPoint: \n"\
"            return PointLight_SampleVertex(&light, scene, TEXTURE_ARGS, sample0, sample1, p, n, wo, pdf); \n"\
"    } \n"\
" \n"\
"    *pdf = 0.f; \n"\
"    return make_float3(0.f, 0.f, 0.f); \n"\
"} \n"\
" \n"\
"/// Check if the light is singular \n"\
"bool Light_IsSingular(__global Light const* light) \n"\
"{ \n"\
"    return light->type == kPoint || \n"\
"        light->type == kSpot || \n"\
"        light->type == kDirectional; \n"\
"} \n"\
" \n"\
"#endif // LIGHT_CLnv \n"\
;
static const char g_material_opencl[]= \
"/********************************************************************** \n"\
"Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved. \n"\
" \n"\
"Permission is hereby granted, free of charge, to any person obtaining a copy \n"\
"of this software and associated documentation files (the \"Software\"), to deal \n"\
"in the Software without restriction, including without limitation the rights \n"\
"to use, copy, modify, merge, publish, distribute, sublicense, and/or sell \n"\
"copies of the Software, and to permit persons to whom the Software is \n"\
"furnished to do so, subject to the following conditions: \n"\
" \n"\
"The above copyright notice and this permission notice shall be included in \n"\
"all copies or substantial portions of the Software. \n"\
" \n"\
"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR \n"\
"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, \n"\
"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE \n"\
"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER \n"\
"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, \n"\
"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN \n"\
"THE SOFTWARE. \n"\
"********************************************************************/ \n"\
"#ifndef MATERIAL_CL \n"\
"#define MATERIAL_CL \n"\
" \n"\
" \n"\
"void Material_Select( \n"\
"    // Scene data \n"\
"    Scene const* scene, \n"\
"    // Incoming direction \n"\
"    float3 wi, \n"\
"    // Sampler \n"\
"    Sampler* sampler, \n"\
"    // Texture args \n"\
"    TEXTURE_ARG_LIST, \n"\
"    // Sampler args \n"\
"    SAMPLER_ARG_LIST, \n"\
"    // Geometry \n"\
"    DifferentialGeometry* dg \n"\
") \n"\
"{ \n"\
"    // Check material type \n"\
"    int type = dg->mat.type; \n"\
"    int idx = dg->material_index; \n"\
" \n"\
"    float ndotwi = dot(dg->n, wi); \n"\
" \n"\
"    if (ndotwi < 0.f && dg->mat.thin) \n"\
"    { \n"\
"        ndotwi = -ndotwi; \n"\
"    } \n"\
" \n"\
"    // If material is regular BxDF we do not have to sample it \n"\
"    if (type != kFresnelBlend && type != kMix) \n"\
"    { \n"\
"        // If fresnel > 0 here we need to calculate Frensle factor (remove this workaround) \n"\
"        if (dg->mat.simple.fresnel > 0.f) \n"\
"        { \n"\
"            float etai = 1.f; \n"\
"            float etat = dg->mat.simple.ni; \n"\
"            float cosi = ndotwi; \n"\
" \n"\
"            // Revert normal and eta if needed \n"\
"            if (cosi < 0.f) \n"\
"            { \n"\
"                float tmp = etai; \n"\
"                etai = etat; \n"\
"                etat = tmp; \n"\
"                cosi = -cosi; \n"\
"            } \n"\
" \n"\
"            float eta = etai / etat; \n"\
"            float sini2 = 1.f - cosi * cosi; \n"\
"            float sint2 = eta * eta * sini2; \n"\
" \n"\
"            float fresnel = 1.f; \n"\
" \n"\
"            if (sint2 < 1.f) \n"\
"            { \n"\
"                float cost = native_sqrt(max(0.f, 1.f - sint2)); \n"\
"                fresnel = FresnelDielectric(etai, etat, cosi, cost); \n"\
"            } \n"\
" \n"\
"            dg->mat.simple.fresnel = Bxdf_IsBtdf(dg) ? (1.f - fresnel) : fresnel; \n"\
"        } \n"\
"        else \n"\
"        { \n"\
"            // Otherwise set multiplier to 1 \n"\
"            dg->mat.simple.fresnel = 1.f; \n"\
"        } \n"\
"    } \n"\
"    // Here we deal with combined material and we have to sample \n"\
"    else \n"\
"    { \n"\
"        // Prefetch current material \n"\
"        Material mat = dg->mat; \n"\
"        int iter = 0; \n"\
" \n"\
"        // Might need several passes of sampling \n"\
"        while (mat.type == kFresnelBlend || mat.type == kMix) \n"\
"        { \n"\
"            if (mat.type == kFresnelBlend) \n"\
"            { \n"\
"                float etai = 1.f; \n"\
"                float etat = mat.compound.weight; \n"\
"                float cosi = ndotwi; \n"\
" \n"\
"                // Revert normal and eta if needed \n"\
"                if (cosi < 0.f) \n"\
"                { \n"\
"                    float tmp = etai; \n"\
"                    etai = etat; \n"\
"                    etat = tmp; \n"\
"                    cosi = -cosi; \n"\
"                } \n"\
" \n"\
"                float eta = etai / etat; \n"\
"                float sini2 = 1.f - cosi * cosi; \n"\
"                float sint2 = eta * eta * sini2; \n"\
"                float fresnel = 1.f; \n"\
" \n"\
"                if (sint2 < 1.f) \n"\
"                { \n"\
"                    float cost = native_sqrt(max(0.f, 1.f - sint2)); \n"\
"                    fresnel = FresnelDielectric(etai, etat, cosi, cost); \n"\
"                } \n"\
" \n"\
"                float sample = Sampler_Sample1D(sampler, SAMPLER_ARGS); \n"\
" \n"\
"                if (sample < fresnel) \n"\
"                { \n"\
"                    // Sample top \n"\
"                    idx = mat.compound.top_brdf_idx; \n"\
"                    // \n"\
"                    mat = scene->materials[idx]; \n"\
"                    mat.simple.fresnel = 1.f; \n"\
"                } \n"\
"                else \n"\
"                { \n"\
"                    // Sample base \n"\
"                    idx = mat.compound.base_brdf_idx; \n"\
"                    //  \n"\
"                    mat = scene->materials[idx]; \n"\
"                    mat.simple.fresnel = 1.f; \n"\
"                } \n"\
"            } \n"\
"            else \n"\
"            { \n"\
"                float sample = Sampler_Sample1D(sampler, SAMPLER_ARGS); \n"\
" \n"\
"                float weight = Texture_GetValue1f(mat.compound.weight, dg->uv, TEXTURE_ARGS_IDX(mat.compound.weight_map_idx)); \n"\
" \n"\
"                if (sample < weight) \n"\
"                { \n"\
"                    // Sample top \n"\
"                    idx = mat.compound.top_brdf_idx; \n"\
"                    // \n"\
"                    mat = scene->materials[idx]; \n"\
"                    mat.simple.fresnel = 1.f; \n"\
"                } \n"\
"                else \n"\
"                { \n"\
"                    // Sample base \n"\
"                    idx = mat.compound.base_brdf_idx; \n"\
"                    // \n"\
"                    mat = scene->materials[idx]; \n"\
"                    mat.simple.fresnel = 1.f; \n"\
"                } \n"\
"            } \n"\
"        } \n"\
" \n"\
"        dg->material_index = idx; \n"\
"        dg->mat = mat; \n"\
"    } \n"\
"} \n"\
" \n"\
" \n"\
"#endif // MATERIAL_CL \n"\
;
static const char g_monte_carlo_renderer_opencl[]= \
"/********************************************************************** \n"\
"Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved. \n"\
" \n"\
"Permission is hereby granted, free of charge, to any person obtaining a copy \n"\
"of this software and associated documentation files (the \"Software\"), to deal \n"\
"in the Software without restriction, including without limitation the rights \n"\
"to use, copy, modify, merge, publish, distribute, sublicense, and/or sell \n"\
"copies of the Software, and to permit persons to whom the Software is \n"\
"furnished to do so, subject to the following conditions: \n"\
" \n"\
"The above copyright notice and this permission notice shall be included in \n"\
"all copies or substantial portions of the Software. \n"\
" \n"\
"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR \n"\
"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, \n"\
"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE \n"\
"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER \n"\
"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, \n"\
"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN \n"\
"THE SOFTWARE. \n"\
"********************************************************************/ \n"\
"#ifndef MONTE_CARLO_RENDERER_CL \n"\
"#define MONTE_CARLO_RENDERER_CL \n"\
" \n"\
" \n"\
"// Pinhole camera implementation. \n"\
"// This kernel is being used if aperture value = 0. \n"\
"KERNEL \n"\
"void PerspectiveCamera_GeneratePaths( \n"\
"    // Camera \n"\
"    GLOBAL Camera const* restrict camera,  \n"\
"    // Image resolution \n"\
"    int output_width, \n"\
"    int output_height, \n"\
"    // Pixel domain buffer \n"\
"    GLOBAL int const* restrict pixel_idx, \n"\
"    // Size of pixel domain buffer \n"\
"    GLOBAL int const* restrict num_pixels, \n"\
"    // RNG seed value \n"\
"    uint rng_seed, \n"\
"    // Current frame \n"\
"    uint frame, \n"\
"    // Rays to generate \n"\
"    GLOBAL ray* restrict rays, \n"\
"    // RNG data \n"\
"    GLOBAL uint* restrict random, \n"\
"    GLOBAL uint const* restrict sobol_mat \n"\
") \n"\
"{ \n"\
"    int global_id = get_global_id(0); \n"\
" \n"\
"    // Check borders \n"\
"    if (global_id < *num_pixels) \n"\
"    { \n"\
"        int idx = pixel_idx[global_id]; \n"\
"        int y = idx / output_width; \n"\
"        int x = idx % output_width; \n"\
" \n"\
"        // Get pointer to ray & path handles \n"\
"        GLOBAL ray* my_ray = rays + global_id; \n"\
" \n"\
"        // Initialize sampler \n"\
"        Sampler sampler; \n"\
"#if SAMPLER == SOBOL \n"\
"        uint scramble = random[x + output_width * y] * 0x1fe3434f; \n"\
" \n"\
"        if (frame & 0xF) \n"\
"        { \n"\
"            random[x + output_width * y] = WangHash(scramble); \n"\
"        } \n"\
" \n"\
"        Sampler_Init(&sampler, frame, SAMPLE_DIM_CAMERA_OFFSET, scramble); \n"\
"#elif SAMPLER == RANDOM \n"\
"        uint scramble = x + output_width * y * rng_seed; \n"\
"        Sampler_Init(&sampler, scramble); \n"\
"#elif SAMPLER == CMJ \n"\
"        uint rnd = random[x + output_width * y]; \n"\
"        uint scramble = rnd * 0x1fe3434f * ((frame + 133 * rnd) / (CMJ_DIM * CMJ_DIM)); \n"\
"        Sampler_Init(&sampler, frame % (CMJ_DIM * CMJ_DIM), SAMPLE_DIM_CAMERA_OFFSET, scramble); \n"\
"#endif \n"\
" \n"\
"        // Generate sample \n"\
"#ifndef BAIKAL_GENERATE_SAMPLE_AT_PIXEL_CENTER \n"\
"        float2 sample0 = Sampler_Sample2D(&sampler, SAMPLER_ARGS); \n"\
"#else \n"\
"        float2 sample0 = make_float2(0.5f, 0.5f); \n"\
"#endif \n"\
" \n"\
"        // Calculate [0..1] image plane sample \n"\
"        float2 img_sample; \n"\
"        img_sample.x = (float)x / output_width + sample0.x / output_width; \n"\
"        img_sample.y = (float)y / output_height + sample0.y / output_height; \n"\
" \n"\
"        // Transform into [-0.5, 0.5] \n"\
"        float2 h_sample = img_sample - make_float2(0.5f, 0.5f); \n"\
"        // Transform into [-dim/2, dim/2] \n"\
"        float2 c_sample = h_sample * camera->dim; \n"\
" \n"\
"        // Calculate direction to image plane \n"\
"        my_ray->d.xyz = normalize(camera->focal_length * camera->forward + c_sample.x * camera->right + c_sample.y * camera->up); \n"\
"        // Origin == camera position + nearz * d \n"\
"        my_ray->o.xyz = camera->p + camera->zcap.x * my_ray->d.xyz; \n"\
"        // Max T value = zfar - znear since we moved origin to znear \n"\
"        my_ray->o.w = camera->zcap.y - camera->zcap.x; \n"\
"        // Generate random time from 0 to 1 \n"\
"        my_ray->d.w = sample0.x; \n"\
"        // Set ray max \n"\
"        my_ray->extra.x = 0xFFFFFFFF; \n"\
"        my_ray->extra.y = 0xFFFFFFFF; \n"\
"        Ray_SetExtra(my_ray, 1.f); \n"\
"        Ray_SetMask(my_ray, VISIBILITY_MASK_PRIMARY); \n"\
"    } \n"\
"} \n"\
" \n"\
"// Physical camera implemenation. \n"\
"// This kernel is being used if aperture > 0. \n"\
"KERNEL void PerspectiveCameraDof_GeneratePaths( \n"\
"    // Camera \n"\
"    GLOBAL Camera const* restrict camera, \n"\
"    // Image resolution \n"\
"    int output_width, \n"\
"    int output_height, \n"\
"    // Pixel domain buffer \n"\
"    GLOBAL int const* restrict pixel_idx, \n"\
"    // Size of pixel domain buffer \n"\
"    GLOBAL int const* restrict num_pixels, \n"\
"    // RNG seed value \n"\
"    uint rng_seed, \n"\
"    // Current frame \n"\
"    uint frame, \n"\
"    // Rays to generate \n"\
"    GLOBAL ray* restrict rays, \n"\
"    // RNG data \n"\
"    GLOBAL uint* restrict random, \n"\
"    GLOBAL uint const* restrict sobol_mat \n"\
") \n"\
"{ \n"\
"    int global_id = get_global_id(0); \n"\
" \n"\
"    // Check borders \n"\
"    if (global_id < *num_pixels) \n"\
"    { \n"\
"        int idx = pixel_idx[global_id]; \n"\
"        int y = idx / output_width; \n"\
"        int x = idx % output_width; \n"\
" \n"\
"        // Get pointer to ray & path handles \n"\
"        GLOBAL ray* my_ray = rays + global_id; \n"\
" \n"\
"        // Initialize sampler \n"\
"        Sampler sampler; \n"\
"#if SAMPLER == SOBOL \n"\
"        uint scramble = random[x + output_width * y] * 0x1fe3434f; \n"\
" \n"\
"        if (frame & 0xF) \n"\
"        { \n"\
"            random[x + output_width * y] = WangHash(scramble); \n"\
"        } \n"\
" \n"\
"        Sampler_Init(&sampler, frame, SAMPLE_DIM_CAMERA_OFFSET, scramble); \n"\
"#elif SAMPLER == RANDOM \n"\
"        uint scramble = x + output_width * y * rng_seed; \n"\
"        Sampler_Init(&sampler, scramble); \n"\
"#elif SAMPLER == CMJ \n"\
"        uint rnd = random[x + output_width * y]; \n"\
"        uint scramble = rnd * 0x1fe3434f * ((frame + 133 * rnd) / (CMJ_DIM * CMJ_DIM)); \n"\
"        Sampler_Init(&sampler, frame % (CMJ_DIM * CMJ_DIM), SAMPLE_DIM_CAMERA_OFFSET, scramble); \n"\
"#endif \n"\
" \n"\
"        // Generate pixel and lens samples \n"\
"#ifndef BAIKAL_GENERATE_SAMPLE_AT_PIXEL_CENTER \n"\
"        float2 sample0 = Sampler_Sample2D(&sampler, SAMPLER_ARGS); \n"\
"#else \n"\
"        float2 sample0 = make_float2(0.5f, 0.5f); \n"\
"#endif \n"\
"        float2 sample1 = Sampler_Sample2D(&sampler, SAMPLER_ARGS); \n"\
" \n"\
"        // Calculate [0..1] image plane sample \n"\
"        float2 img_sample; \n"\
"        img_sample.x = (float)x / output_width + sample0.x / output_width; \n"\
"        img_sample.y = (float)y / output_height + sample0.y / output_height; \n"\
" \n"\
"        // Transform into [-0.5, 0.5] \n"\
"        float2 h_sample = img_sample - make_float2(0.5f, 0.5f); \n"\
"        // Transform into [-dim/2, dim/2] \n"\
"        float2 c_sample = h_sample * camera->dim; \n"\
" \n"\
"        // Generate sample on the lens \n"\
"        float2 lens_sample = camera->aperture * Sample_MapToDiskConcentric(sample1); \n"\
"        // Calculate position on focal plane \n"\
"        float2 focal_plane_sample = c_sample * camera->focus_distance / camera->focal_length; \n"\
"        // Calculate ray direction \n"\
"        float2 camera_dir = focal_plane_sample - lens_sample; \n"\
" \n"\
"        // Calculate direction to image plane \n"\
"        my_ray->d.xyz = normalize(camera->forward * camera->focus_distance + camera->right * camera_dir.x + camera->up * camera_dir.y); \n"\
"        // Origin == camera position + nearz * d \n"\
"        my_ray->o.xyz = camera->p + lens_sample.x * camera->right + lens_sample.y * camera->up; \n"\
"        // Max T value = zfar - znear since we moved origin to znear \n"\
"        my_ray->o.w = camera->zcap.y - camera->zcap.x; \n"\
"        // Generate random time from 0 to 1 \n"\
"        my_ray->d.w = sample0.x; \n"\
"        // Set ray max \n"\
"        my_ray->extra.x = 0xFFFFFFFF; \n"\
"        my_ray->extra.y = 0xFFFFFFFF; \n"\
"        Ray_SetExtra(my_ray, 1.f); \n"\
"        Ray_SetMask(my_ray, VISIBILITY_MASK_PRIMARY); \n"\
"    } \n"\
"} \n"\
" \n"\
" \n"\
"KERNEL \n"\
"void PerspectiveCamera_GenerateVertices( \n"\
"    // Camera \n"\
"    GLOBAL Camera const* restrict camera, \n"\
"    // Image resolution \n"\
"    int output_width, \n"\
"    int output_height, \n"\
"    // Pixel domain buffer \n"\
"    GLOBAL int const* restrict pixel_idx, \n"\
"    // Size of pixel domain buffer \n"\
"    GLOBAL int const* restrict num_pixels, \n"\
"    // RNG seed value \n"\
"    uint rng_seed, \n"\
"    // Current frame \n"\
"    uint frame, \n"\
"    // RNG data \n"\
"    GLOBAL uint* restrict random, \n"\
"    GLOBAL uint const* restrict sobol_mat, \n"\
"    // Rays to generate \n"\
"    GLOBAL ray* restrict rays, \n"\
"    // Eye subpath vertices \n"\
"    GLOBAL PathVertex* restrict eye_subpath, \n"\
"    // Eye subpath length \n"\
"    GLOBAL int* restrict eye_subpath_length, \n"\
"    // Path buffer \n"\
"    GLOBAL Path* restrict paths \n"\
") \n"\
" \n"\
"{ \n"\
"    int global_id = get_global_id(0); \n"\
" \n"\
"    // Check borders \n"\
"    if (global_id < *num_pixels) \n"\
"    { \n"\
"        int idx = pixel_idx[global_id]; \n"\
"        int y = idx / output_width; \n"\
"        int x = idx % output_width; \n"\
" \n"\
"        // Get pointer to ray & path handles \n"\
"        GLOBAL ray* my_ray = rays + global_id; \n"\
" \n"\
"        GLOBAL PathVertex* my_vertex = eye_subpath + BDPT_MAX_SUBPATH_LEN * idx; \n"\
"        GLOBAL int* my_count = eye_subpath_length + idx; \n"\
"        GLOBAL Path* my_path = paths + idx; \n"\
" \n"\
"        // Initialize sampler \n"\
"        Sampler sampler; \n"\
"#if SAMPLER == SOBOL \n"\
"        uint scramble = random[x + output_width * y] * 0x1fe3434f; \n"\
" \n"\
"        if (frame & 0xF) \n"\
"        { \n"\
"            random[x + output_width * y] = WangHash(scramble); \n"\
"        } \n"\
" \n"\
"        Sampler_Init(&sampler, frame, SAMPLE_DIM_CAMERA_OFFSET, scramble); \n"\
"#elif SAMPLER == RANDOM \n"\
"        uint scramble = x + output_width * y * rng_seed; \n"\
"        Sampler_Init(&sampler, scramble); \n"\
"#elif SAMPLER == CMJ \n"\
"        uint rnd = random[x + output_width * y]; \n"\
"        uint scramble = rnd * 0x1fe3434f * ((frame + 133 * rnd) / (CMJ_DIM * CMJ_DIM)); \n"\
"        Sampler_Init(&sampler, frame % (CMJ_DIM * CMJ_DIM), SAMPLE_DIM_CAMERA_OFFSET, scramble); \n"\
"#endif \n"\
" \n"\
"        // Generate sample \n"\
"        float2 sample0 = Sampler_Sample2D(&sampler, SAMPLER_ARGS); \n"\
" \n"\
"        // Calculate [0..1] image plane sample \n"\
"        float2 img_sample; \n"\
"        img_sample.x = (float)x / output_width + sample0.x / output_width; \n"\
"        img_sample.y = (float)y / output_height + sample0.y / output_height; \n"\
" \n"\
"        // Transform into [-0.5, 0.5] \n"\
"        float2 h_sample = img_sample - make_float2(0.5f, 0.5f); \n"\
"        // Transform into [-dim/2, dim/2] \n"\
"        float2 c_sample = h_sample * camera->dim; \n"\
" \n"\
"        // Calculate direction to image plane \n"\
"        my_ray->d.xyz = normalize(camera->focal_length * camera->forward + c_sample.x * camera->right + c_sample.y * camera->up); \n"\
"        // Origin == camera position + nearz * d \n"\
"        my_ray->o.xyz = camera->p + camera->zcap.x * my_ray->d.xyz; \n"\
"        // Max T value = zfar - znear since we moved origin to znear \n"\
"        my_ray->o.w = camera->zcap.y - camera->zcap.x; \n"\
"        // Generate random time from 0 to 1 \n"\
"        my_ray->d.w = sample0.x; \n"\
"        // Set ray max \n"\
"        my_ray->extra.x = 0xFFFFFFFF; \n"\
"        my_ray->extra.y = 0xFFFFFFFF; \n"\
"        Ray_SetExtra(my_ray, 1.f); \n"\
" \n"\
"        PathVertex v; \n"\
"        PathVertex_Init(&v, \n"\
"            camera->p, \n"\
"            camera->forward, \n"\
"            camera->forward, \n"\
"            0.f, \n"\
"            1.f, \n"\
"            1.f, \n"\
"            1.f, \n"\
"            kCamera, \n"\
"            -1); \n"\
" \n"\
"        *my_count = 1; \n"\
"        *my_vertex = v; \n"\
" \n"\
"        // Initlize path data \n"\
"        my_path->throughput = make_float3(1.f, 1.f, 1.f); \n"\
"        my_path->volume = -1; \n"\
"        my_path->flags = 0; \n"\
"        my_path->active = 0xFF; \n"\
"    } \n"\
"} \n"\
" \n"\
"KERNEL \n"\
"void PerspectiveCameraDof_GenerateVertices( \n"\
"    // Camera \n"\
"    GLOBAL Camera const* restrict camera, \n"\
"    // Image resolution \n"\
"    int output_width, \n"\
"    int output_height, \n"\
"    // Pixel domain buffer \n"\
"    GLOBAL int const* restrict pixel_idx, \n"\
"    // Size of pixel domain buffer \n"\
"    GLOBAL int const* restrict num_pixels, \n"\
"    // RNG seed value \n"\
"    uint rng_seed, \n"\
"    // Current frame \n"\
"    uint frame, \n"\
"    // RNG data \n"\
"    GLOBAL uint* restrict random, \n"\
"    GLOBAL uint const* restrict sobol_mat, \n"\
"    // Rays to generate \n"\
"    GLOBAL ray* restrict rays, \n"\
"    // Eye subpath vertices \n"\
"    GLOBAL PathVertex* restrict eye_subpath, \n"\
"    // Eye subpath length \n"\
"    GLOBAL int* restrict eye_subpath_length, \n"\
"    // Path buffer \n"\
"    GLOBAL Path* restrict paths \n"\
") \n"\
" \n"\
"{ \n"\
"    int global_id = get_global_id(0); \n"\
" \n"\
"    // Check borders \n"\
"    if (global_id < *num_pixels) \n"\
"    { \n"\
"        int idx = pixel_idx[global_id]; \n"\
"        int y = idx / output_width; \n"\
"        int x = idx % output_width; \n"\
" \n"\
"        // Get pointer to ray & path handles \n"\
"        GLOBAL ray* my_ray = rays + global_id; \n"\
"        GLOBAL PathVertex* my_vertex = eye_subpath + BDPT_MAX_SUBPATH_LEN * (y * output_width + x); \n"\
"        GLOBAL int* my_count = eye_subpath_length + y * output_width + x; \n"\
"        GLOBAL Path* my_path = paths + y * output_width + x; \n"\
" \n"\
"        // Initialize sampler \n"\
"        Sampler sampler; \n"\
"#if SAMPLER == SOBOL \n"\
"        uint scramble = random[x + output_width * y] * 0x1fe3434f; \n"\
" \n"\
"        if (frame & 0xF) \n"\
"        { \n"\
"            random[x + output_width * y] = WangHash(scramble); \n"\
"        } \n"\
" \n"\
"        Sampler_Init(&sampler, frame, SAMPLE_DIM_CAMERA_OFFSET, scramble); \n"\
"#elif SAMPLER == RANDOM \n"\
"        uint scramble = x + output_width * y * rng_seed; \n"\
"        Sampler_Init(&sampler, scramble); \n"\
"#elif SAMPLER == CMJ \n"\
"        uint rnd = random[x + output_width * y]; \n"\
"        uint scramble = rnd * 0x1fe3434f * ((frame + 133 * rnd) / (CMJ_DIM * CMJ_DIM)); \n"\
"        Sampler_Init(&sampler, frame % (CMJ_DIM * CMJ_DIM), SAMPLE_DIM_CAMERA_OFFSET, scramble); \n"\
"#endif \n"\
" \n"\
"        // Generate pixel and lens samples \n"\
"        float2 sample0 = Sampler_Sample2D(&sampler, SAMPLER_ARGS); \n"\
"        float2 sample1 = Sampler_Sample2D(&sampler, SAMPLER_ARGS); \n"\
" \n"\
"        // Calculate [0..1] image plane sample \n"\
"        float2 img_sample; \n"\
"        img_sample.x = (float)x / output_width + sample0.x / output_width; \n"\
"        img_sample.y = (float)y / output_height + sample0.y / output_height; \n"\
" \n"\
"        // Transform into [-0.5, 0.5] \n"\
"        float2 h_sample = img_sample - make_float2(0.5f, 0.5f); \n"\
"        // Transform into [-dim/2, dim/2] \n"\
"        float2 c_sample = h_sample * camera->dim; \n"\
" \n"\
"        // Generate sample on the lens \n"\
"        float2 lens_sample = camera->aperture * Sample_MapToDiskConcentric(sample1); \n"\
"        // Calculate position on focal plane \n"\
"        float2 focal_plane_sample = c_sample * camera->focus_distance / camera->focal_length; \n"\
"        // Calculate ray direction \n"\
"        float2 camera_dir = focal_plane_sample - lens_sample; \n"\
" \n"\
"        // Calculate direction to image plane \n"\
"        my_ray->d.xyz = normalize(camera->forward * camera->focus_distance + camera->right * camera_dir.x + camera->up * camera_dir.y); \n"\
"        // Origin == camera position + nearz * d \n"\
"        my_ray->o.xyz = camera->p + lens_sample.x * camera->right + lens_sample.y * camera->up; \n"\
"        // Max T value = zfar - znear since we moved origin to znear \n"\
"        my_ray->o.w = camera->zcap.y - camera->zcap.x; \n"\
"        // Generate random time from 0 to 1 \n"\
"        my_ray->d.w = sample0.x; \n"\
"        // Set ray max \n"\
"        my_ray->extra.x = 0xFFFFFFFF; \n"\
"        my_ray->extra.y = 0xFFFFFFFF; \n"\
"        Ray_SetExtra(my_ray, 1.f); \n"\
" \n"\
"        PathVertex v; \n"\
"        PathVertex_Init(&v, \n"\
"            camera->p, \n"\
"            camera->forward, \n"\
"            camera->forward, \n"\
"            0.f, \n"\
"            1.f, \n"\
"            1.f, \n"\
"            1.f, \n"\
"            kCamera, \n"\
"            -1); \n"\
" \n"\
"        *my_count = 1; \n"\
"        *my_vertex = v; \n"\
" \n"\
"        // Initlize path data \n"\
"        my_path->throughput = make_float3(1.f, 1.f, 1.f); \n"\
"        my_path->volume = -1; \n"\
"        my_path->flags = 0; \n"\
"        my_path->active = 0xFF; \n"\
"    } \n"\
"} \n"\
" \n"\
"uint Part1By1(uint x) \n"\
"{ \n"\
"    x &= 0x0000ffff;                  // x = ---- ---- ---- ---- fedc ba98 7654 3210 \n"\
"    x = (x ^ (x << 8)) & 0x00ff00ff; // x = ---- ---- fedc ba98 ---- ---- 7654 3210 \n"\
"    x = (x ^ (x << 4)) & 0x0f0f0f0f; // x = ---- fedc ---- ba98 ---- 7654 ---- 3210 \n"\
"    x = (x ^ (x << 2)) & 0x33333333; // x = --fe --dc --ba --98 --76 --54 --32 --10 \n"\
"    x = (x ^ (x << 1)) & 0x55555555; // x = -f-e -d-c -b-a -9-8 -7-6 -5-4 -3-2 -1-0 \n"\
"    return x; \n"\
"} \n"\
" \n"\
"uint Morton2D(uint x, uint y) \n"\
"{ \n"\
"    return (Part1By1(y) << 1) + Part1By1(x); \n"\
"} \n"\
" \n"\
"KERNEL void GenerateTileDomain( \n"\
"    int output_width, \n"\
"    int output_height, \n"\
"    int offset_x, \n"\
"    int offset_y, \n"\
"    int width, \n"\
"    int height, \n"\
"    uint rng_seed, \n"\
"    uint frame, \n"\
"    GLOBAL uint* restrict random, \n"\
"    GLOBAL uint const* restrict sobol_mat, \n"\
"    GLOBAL int* restrict indices, \n"\
"    GLOBAL int* restrict count \n"\
") \n"\
"{ \n"\
"    int2 global_id; \n"\
"    global_id.x = get_global_id(0); \n"\
"    global_id.y = get_global_id(1); \n"\
" \n"\
"    int2 local_id; \n"\
"    local_id.x = get_local_id(0); \n"\
"    local_id.y = get_local_id(1); \n"\
" \n"\
"    int2 group_id; \n"\
"    group_id.x = get_group_id(0); \n"\
"    group_id.y = get_group_id(1); \n"\
" \n"\
"    int2 tile_size; \n"\
"    tile_size.x = get_local_size(0); \n"\
"    tile_size.y = get_local_size(1); \n"\
" \n"\
"    int num_tiles_x = output_width / tile_size.x; \n"\
"    int num_tiles_y = output_height / tile_size.y; \n"\
" \n"\
"    int start_idx = output_width * offset_y + offset_x; \n"\
" \n"\
"    if (global_id.x < width && global_id.y < height) \n"\
"    { \n"\
"        int idx = start_idx + \n"\
"            (group_id.y * tile_size.y + local_id.y) * output_width + \n"\
"            (group_id.x * tile_size.x + local_id.x); \n"\
" \n"\
"        indices[global_id.y * width + global_id.x] = idx; \n"\
"    } \n"\
" \n"\
"    if (global_id.x == 0 && global_id.y == 0) \n"\
"    { \n"\
"        *count = width * height; \n"\
"    } \n"\
"} \n"\
" \n"\
"KERNEL void GenerateTileDomain_Adaptive( \n"\
"    int output_width, \n"\
"    int output_height, \n"\
"    int offset_x, \n"\
"    int offset_y, \n"\
"    int width, \n"\
"    int height, \n"\
"    uint rng_seed, \n"\
"    uint frame, \n"\
"    GLOBAL uint* restrict random, \n"\
"    GLOBAL uint const* restrict sobol_mat, \n"\
"    GLOBAL int const* restrict tile_distribution, \n"\
"    GLOBAL int* restrict indices, \n"\
"    GLOBAL int* restrict count \n"\
") \n"\
"{ \n"\
"    int2 global_id; \n"\
"    global_id.x = get_global_id(0); \n"\
"    global_id.y = get_global_id(1); \n"\
" \n"\
"    int2 local_id; \n"\
"    local_id.x = get_local_id(0); \n"\
"    local_id.y = get_local_id(1); \n"\
" \n"\
"    int2 group_id; \n"\
"    group_id.x = get_group_id(0); \n"\
"    group_id.y = get_group_id(1); \n"\
" \n"\
"    int2 tile_size; \n"\
"    tile_size.x = get_local_size(0); \n"\
"    tile_size.y = get_local_size(1); \n"\
" \n"\
" \n"\
"    // Initialize sampler   \n"\
"    Sampler sampler; \n"\
"    int x = global_id.x; \n"\
"    int y = global_id.y; \n"\
"#if SAMPLER == SOBOL \n"\
"    uint scramble = random[x + output_width * y] * 0x1fe3434f; \n"\
" \n"\
"    if (frame & 0xF) \n"\
"    { \n"\
"        random[x + output_width * y] = WangHash(scramble); \n"\
"    } \n"\
" \n"\
"    Sampler_Init(&sampler, frame, SAMPLE_DIM_IMG_PLANE_EVALUATE_OFFSET, scramble); \n"\
"#elif SAMPLER == RANDOM \n"\
"    uint scramble = x + output_width * y * rng_seed; \n"\
"    Sampler_Init(&sampler, scramble); \n"\
"#elif SAMPLER == CMJ \n"\
"    uint rnd = random[group_id.x + output_width *group_id.y]; \n"\
"    uint scramble = rnd * 0x1fe3434f * ((frame + 133 * rnd) / (CMJ_DIM * CMJ_DIM)); \n"\
"    Sampler_Init(&sampler, frame % (CMJ_DIM * CMJ_DIM), SAMPLE_DIM_IMG_PLANE_EVALUATE_OFFSET, scramble); \n"\
"#endif \n"\
" \n"\
"    float2 sample = Sampler_Sample2D(&sampler, SAMPLER_ARGS); \n"\
" \n"\
"    float pdf; \n"\
"    int tile = Distribution1D_SampleDiscrete(sample.x, tile_distribution, &pdf); \n"\
" \n"\
"    int num_tiles_x = output_width / tile_size.x; \n"\
"    int num_tiles_y = output_height / tile_size.y; \n"\
" \n"\
"    int tile_y = clamp(tile / num_tiles_x , 0, num_tiles_y - 1); \n"\
"    int tile_x = clamp(tile % num_tiles_x, 0, num_tiles_x - 1); \n"\
" \n"\
"    int start_idx = output_width * offset_y + offset_x; \n"\
" \n"\
"    if (global_id.x < width && global_id.y < height) \n"\
"    { \n"\
"        int idx = start_idx + \n"\
"            (tile_y * tile_size.y + local_id.y) * output_width + \n"\
"            (tile_x * tile_size.x + local_id.x); \n"\
" \n"\
"        indices[global_id.y * width + global_id.x] = idx; \n"\
"    } \n"\
" \n"\
"    if (global_id.x == 0 && global_id.y == 0) \n"\
"    { \n"\
"        *count = width * height; \n"\
"    } \n"\
"} \n"\
" \n"\
" \n"\
"// Fill AOVs \n"\
"KERNEL void FillAOVs( \n"\
"    // Ray batch \n"\
"    GLOBAL ray const* restrict rays, \n"\
"    // Intersection data \n"\
"    GLOBAL Intersection const* restrict isects, \n"\
"    // Pixel indices \n"\
"    GLOBAL int const* restrict pixel_idx, \n"\
"    // Number of pixels \n"\
"    GLOBAL int const* restrict num_items, \n"\
"    // Vertices \n"\
"    GLOBAL float3 const*restrict  vertices, \n"\
"    // Normals \n"\
"    GLOBAL float3 const* restrict normals, \n"\
"    // UVs \n"\
"    GLOBAL float2 const* restrict uvs, \n"\
"    // Indices \n"\
"    GLOBAL int const* restrict indices, \n"\
"    // Shapes \n"\
"    GLOBAL Shape const* restrict shapes, \n"\
"    // Materials \n"\
"    GLOBAL Material const* restrict materials, \n"\
"    // Textures \n"\
"    TEXTURE_ARG_LIST, \n"\
"    // Environment texture index \n"\
"    int env_light_idx, \n"\
"    // Emissives \n"\
"    GLOBAL Light const* restrict lights, \n"\
"    // Number of emissive objects \n"\
"    int num_lights, \n"\
"    // RNG seed \n"\
"    uint rngseed, \n"\
"    // Sampler states \n"\
"    GLOBAL uint* restrict random, \n"\
"    // Sobol matrices \n"\
"    GLOBAL uint const* restrict sobol_mat,  \n"\
"    // Frame \n"\
"    int frame, \n"\
"    // World position flag \n"\
"    int world_position_enabled,  \n"\
"    // World position AOV \n"\
"    GLOBAL float4* restrict aov_world_position, \n"\
"    // World normal flag \n"\
"    int world_shading_normal_enabled, \n"\
"    // World normal AOV \n"\
"    GLOBAL float4* restrict aov_world_shading_normal, \n"\
"    // World true normal flag \n"\
"    int world_geometric_normal_enabled, \n"\
"    // World true normal AOV \n"\
"    GLOBAL float4* restrict aov_world_geometric_normal, \n"\
"    // UV flag \n"\
"    int uv_enabled, \n"\
"    // UV AOV \n"\
"    GLOBAL float4* restrict aov_uv, \n"\
"    // Wireframe flag \n"\
"    int wireframe_enabled, \n"\
"    // Wireframe AOV \n"\
"    GLOBAL float4* restrict aov_wireframe, \n"\
"    // Albedo flag \n"\
"    int albedo_enabled, \n"\
"    // Wireframe AOV \n"\
"    GLOBAL float4* restrict aov_albedo, \n"\
"    // World tangent flag \n"\
"    int world_tangent_enabled, \n"\
"    // World tangent AOV \n"\
"    GLOBAL float4* restrict aov_world_tangent, \n"\
"    // World bitangent flag \n"\
"    int world_bitangent_enabled, \n"\
"    // World bitangent AOV \n"\
"    GLOBAL float4* restrict aov_world_bitangent, \n"\
"    // Gloss enabled flag \n"\
"    int gloss_enabled, \n"\
"    // Specularity map \n"\
"    GLOBAL float4* restrict aov_gloss, \n"\
"    // Depth enabled flag \n"\
"    int depth_enabled, \n"\
"    // Depth map \n"\
"    GLOBAL float4* restrict aov_depth, \n"\
"    // NOTE: following are fake parameters, handled outside \n"\
"    int visibility_enabled, \n"\
"    GLOBAL float4* restrict aov_visibility \n"\
") \n"\
"{ \n"\
"    int global_id = get_global_id(0); \n"\
" \n"\
"    Scene scene = \n"\
"    { \n"\
"        vertices, \n"\
"        normals, \n"\
"        uvs, \n"\
"        indices, \n"\
"        shapes, \n"\
"        materials, \n"\
"        lights, \n"\
"        env_light_idx, \n"\
"        num_lights \n"\
"    }; \n"\
" \n"\
"    // Only applied to active rays after compaction \n"\
"    if (global_id < *num_items) \n"\
"    { \n"\
"        Intersection isect = isects[global_id]; \n"\
"        int idx = pixel_idx[global_id]; \n"\
" \n"\
"        if (isect.shapeid > -1) \n"\
"        { \n"\
"            // Fetch incoming ray direction \n"\
"            float3 wi = -normalize(rays[global_id].d.xyz); \n"\
" \n"\
"            Sampler sampler; \n"\
"#if SAMPLER == SOBOL  \n"\
"            uint scramble = random[global_id] * 0x1fe3434f; \n"\
"            Sampler_Init(&sampler, frame, SAMPLE_DIM_SURFACE_OFFSET, scramble); \n"\
"#elif SAMPLER == RANDOM \n"\
"            uint scramble = global_id * rngseed; \n"\
"            Sampler_Init(&sampler, scramble); \n"\
"#elif SAMPLER == CMJ \n"\
"            uint rnd = random[global_id]; \n"\
"            uint scramble = rnd * 0x1fe3434f * ((frame + 331 * rnd) / (CMJ_DIM * CMJ_DIM)); \n"\
"            Sampler_Init(&sampler, frame % (CMJ_DIM * CMJ_DIM), SAMPLE_DIM_SURFACE_OFFSET, scramble); \n"\
"#endif \n"\
" \n"\
"            // Fill surface data \n"\
"            DifferentialGeometry diffgeo; \n"\
"            Scene_FillDifferentialGeometry(&scene, &isect, &diffgeo); \n"\
" \n"\
"            if (world_position_enabled) \n"\
"            { \n"\
"                aov_world_position[idx].xyz += diffgeo.p; \n"\
"                aov_world_position[idx].w += 1.f; \n"\
"            } \n"\
" \n"\
"            if (world_shading_normal_enabled) \n"\
"            { \n"\
"                float ngdotwi = dot(diffgeo.ng, wi); \n"\
"                bool backfacing = ngdotwi < 0.f; \n"\
" \n"\
"                // Select BxDF \n"\
"                Material_Select(&scene, wi, &sampler, TEXTURE_ARGS, SAMPLER_ARGS, &diffgeo); \n"\
" \n"\
"                float s = Bxdf_IsBtdf(&diffgeo) ? (-sign(ngdotwi)) : 1.f; \n"\
"                if (backfacing && !Bxdf_IsBtdf(&diffgeo)) \n"\
"                { \n"\
"                    //Reverse normal and tangents in this case \n"\
"                    //but not for BTDFs, since BTDFs rely \n"\
"                    //on normal direction in order to arrange    \n"\
"                    //indices of refraction \n"\
"                    diffgeo.n = -diffgeo.n; \n"\
"                    diffgeo.dpdu = -diffgeo.dpdu; \n"\
"                    diffgeo.dpdv = -diffgeo.dpdv; \n"\
"                } \n"\
" \n"\
"                DifferentialGeometry_ApplyBumpNormalMap(&diffgeo, TEXTURE_ARGS); \n"\
"                DifferentialGeometry_CalculateTangentTransforms(&diffgeo); \n"\
" \n"\
"                aov_world_shading_normal[idx].xyz += diffgeo.n; \n"\
"                aov_world_shading_normal[idx].w += 1.f; \n"\
"            } \n"\
" \n"\
"            if (world_geometric_normal_enabled) \n"\
"            { \n"\
"                aov_world_geometric_normal[idx].xyz += diffgeo.ng; \n"\
"                aov_world_geometric_normal[idx].w += 1.f; \n"\
"            } \n"\
" \n"\
"            if (wireframe_enabled) \n"\
"            { \n"\
"                bool hit = (isect.uvwt.x < 1e-3) || (isect.uvwt.y < 1e-3) || (1.f - isect.uvwt.x - isect.uvwt.y < 1e-3); \n"\
"                float3 value = hit ? make_float3(1.f, 1.f, 1.f) : make_float3(0.f, 0.f, 0.f); \n"\
"                aov_wireframe[idx].xyz += value; \n"\
"                aov_wireframe[idx].w += 1.f; \n"\
"            } \n"\
" \n"\
"            if (uv_enabled) \n"\
"            { \n"\
"                aov_uv[idx].xy += diffgeo.uv.xy; \n"\
"                aov_uv[idx].w += 1.f; \n"\
"            } \n"\
" \n"\
"            if (albedo_enabled) \n"\
"            { \n"\
"                float ngdotwi = dot(diffgeo.ng, wi); \n"\
"                bool backfacing = ngdotwi < 0.f; \n"\
" \n"\
"                // Select BxDF \n"\
"                Material_Select(&scene, wi, &sampler, TEXTURE_ARGS, SAMPLER_ARGS, &diffgeo); \n"\
" \n"\
"                const float3 kd = Texture_GetValue3f(diffgeo.mat.simple.kx.xyz, diffgeo.uv, TEXTURE_ARGS_IDX(diffgeo.mat.simple.kxmapidx)); \n"\
" \n"\
"                aov_albedo[idx].xyz += kd; \n"\
"                aov_albedo[idx].w += 1.f; \n"\
"            } \n"\
" \n"\
"            if (world_tangent_enabled) \n"\
"            { \n"\
"                float ngdotwi = dot(diffgeo.ng, wi); \n"\
"                bool backfacing = ngdotwi < 0.f; \n"\
" \n"\
"                // Select BxDF \n"\
"                Material_Select(&scene, wi, &sampler, TEXTURE_ARGS, SAMPLER_ARGS, &diffgeo); \n"\
" \n"\
"                float s = Bxdf_IsBtdf(&diffgeo) ? (-sign(ngdotwi)) : 1.f; \n"\
"                if (backfacing && !Bxdf_IsBtdf(&diffgeo)) \n"\
"                { \n"\
"                    //Reverse normal and tangents in this case \n"\
"                    //but not for BTDFs, since BTDFs rely \n"\
"                    //on normal direction in order to arrange \n"\
"                    //indices of refraction \n"\
"                    diffgeo.n = -diffgeo.n; \n"\
"                    diffgeo.dpdu = -diffgeo.dpdu; \n"\
"                    diffgeo.dpdv = -diffgeo.dpdv; \n"\
"                } \n"\
" \n"\
"                DifferentialGeometry_ApplyBumpNormalMap(&diffgeo, TEXTURE_ARGS); \n"\
"                DifferentialGeometry_CalculateTangentTransforms(&diffgeo); \n"\
" \n"\
"                aov_world_tangent[idx].xyz += diffgeo.dpdu; \n"\
"                aov_world_tangent[idx].w += 1.f; \n"\
"            } \n"\
" \n"\
"            if (world_bitangent_enabled) \n"\
"            { \n"\
"                float ngdotwi = dot(diffgeo.ng, wi); \n"\
"                bool backfacing = ngdotwi < 0.f; \n"\
" \n"\
"                // Select BxDF \n"\
"                Material_Select(&scene, wi, &sampler, TEXTURE_ARGS, SAMPLER_ARGS, &diffgeo); \n"\
" \n"\
"                float s = Bxdf_IsBtdf(&diffgeo) ? (-sign(ngdotwi)) : 1.f; \n"\
"                if (backfacing && !Bxdf_IsBtdf(&diffgeo)) \n"\
"                { \n"\
"                    //Reverse normal and tangents in this case \n"\
"                    //but not for BTDFs, since BTDFs rely \n"\
"                    //on normal direction in order to arrange \n"\
"                    //indices of refraction \n"\
"                    diffgeo.n = -diffgeo.n; \n"\
"                    diffgeo.dpdu = -diffgeo.dpdu; \n"\
"                    diffgeo.dpdv = -diffgeo.dpdv; \n"\
"                } \n"\
" \n"\
"                DifferentialGeometry_ApplyBumpNormalMap(&diffgeo, TEXTURE_ARGS); \n"\
"                DifferentialGeometry_CalculateTangentTransforms(&diffgeo); \n"\
" \n"\
"                aov_world_bitangent[idx].xyz += diffgeo.dpdv; \n"\
"                aov_world_bitangent[idx].w += 1.f; \n"\
"            } \n"\
" \n"\
"            if (gloss_enabled) \n"\
"            { \n"\
"                float ngdotwi = dot(diffgeo.ng, wi); \n"\
"                bool backfacing = ngdotwi < 0.f; \n"\
" \n"\
"                // Select BxDF \n"\
"                Material_Select(&scene, wi, &sampler, TEXTURE_ARGS, SAMPLER_ARGS, &diffgeo); \n"\
" \n"\
"                float gloss = 0.f; \n"\
" \n"\
"                int type = diffgeo.mat.type; \n"\
"                if (type == kIdealReflect || type == kIdealReflect || type == kPassthrough) \n"\
"                { \n"\
"                    gloss = 1.f; \n"\
"                } \n"\
"                else if (type == kMicrofacetGGX || type == kMicrofacetBeckmann || \n"\
"                    type == kMicrofacetRefractionGGX || type == kMicrofacetRefractionBeckmann) \n"\
"                { \n"\
"                    gloss = 1.f - Texture_GetValue1f(diffgeo.mat.simple.ns, diffgeo.uv, TEXTURE_ARGS_IDX(diffgeo.mat.simple.nsmapidx)); \n"\
"                } \n"\
" \n"\
" \n"\
"                aov_gloss[idx].xyz += gloss; \n"\
"                aov_gloss[idx].w += 1.f; \n"\
"            } \n"\
" \n"\
"            if (depth_enabled) \n"\
"            { \n"\
"                float w = aov_depth[idx].w; \n"\
"                if (w == 0.f) \n"\
"                { \n"\
"                    aov_depth[idx].xyz = isect.uvwt.w; \n"\
"                    aov_depth[idx].w = 1.f; \n"\
"                } \n"\
"                else \n"\
"                { \n"\
"                    aov_depth[idx].xyz += isect.uvwt.w; \n"\
"                    aov_depth[idx].w += 1.f; \n"\
"                } \n"\
"            } \n"\
"        } \n"\
"    } \n"\
"} \n"\
" \n"\
" \n"\
"// Copy data to interop texture if supported \n"\
"KERNEL void AccumulateData( \n"\
"    GLOBAL float4 const* src_data, \n"\
"    int num_elements, \n"\
"    GLOBAL float4* dst_data \n"\
") \n"\
"{ \n"\
"    int global_id = get_global_id(0); \n"\
" \n"\
"    if (global_id < num_elements) \n"\
"    { \n"\
"        float4 v = src_data[global_id]; \n"\
"        dst_data[global_id] += v; \n"\
"    } \n"\
"} \n"\
" \n"\
"//#define ADAPTIVITY_DEBUG \n"\
"// Copy data to interop texture if supported \n"\
"KERNEL void ApplyGammaAndCopyData( \n"\
"    GLOBAL float4 const* data, \n"\
"    int img_width, \n"\
"    int img_height, \n"\
"    float gamma, \n"\
"    write_only image2d_t img \n"\
") \n"\
"{ \n"\
"    int global_id = get_global_id(0); \n"\
" \n"\
"    int global_idx = global_id % img_width; \n"\
"    int global_idy = global_id / img_width; \n"\
" \n"\
"    if (global_idx < img_width && global_idy < img_height) \n"\
"    { \n"\
"        float4 v = data[global_id]; \n"\
"#ifdef ADAPTIVITY_DEBUG \n"\
"        float a = v.w < 1024 ? min(1.f, v.w / 1024.f) : 0.f; \n"\
"        float4 mul_color = make_float4(1.f, 1.f - a, 1.f - a, 1.f); \n"\
"        v *= mul_color; \n"\
"#endif \n"\
" \n"\
"        float4 val = clamp(native_powr(v / v.w, 1.f / gamma), 0.f, 1.f); \n"\
"        write_imagef(img, make_int2(global_idx, global_idy), val); \n"\
"    } \n"\
"}  \n"\
" \n"\
"KERNEL void AccumulateSingleSample( \n"\
"    GLOBAL float4 const* restrict src_sample_data, \n"\
"    GLOBAL float4* restrict dst_accumulation_data, \n"\
"    GLOBAL int* restrict scatter_indices, \n"\
"    int num_elements \n"\
") \n"\
"{ \n"\
"    int global_id = get_global_id(0); \n"\
" \n"\
"    if (global_id < num_elements) \n"\
"    { \n"\
"        int idx = scatter_indices[global_id]; \n"\
"        float4 sample = src_sample_data[global_id]; \n"\
"        dst_accumulation_data[idx].xyz += sample.xyz; \n"\
"        dst_accumulation_data[idx].w += 1.f; \n"\
"    } \n"\
"} \n"\
" \n"\
"INLINE void group_reduce_add(__local float* lds, int size, int lid) \n"\
"{ \n"\
"    for (int offset = (size >> 1); offset > 0; offset >>= 1) \n"\
"    { \n"\
"        if (lid < offset) \n"\
"        { \n"\
"            lds[lid] += lds[lid + offset]; \n"\
"        } \n"\
"        barrier(CLK_LOCAL_MEM_FENCE); \n"\
"    } \n"\
"} \n"\
" \n"\
"INLINE void group_reduce_min(__local float* lds, int size, int lid) \n"\
"{ \n"\
"    for (int offset = (size >> 1); offset > 0; offset >>= 1) \n"\
"    { \n"\
"        if (lid < offset) \n"\
"        { \n"\
"            lds[lid] = min(lds[lid], lds[lid + offset]); \n"\
"        } \n"\
"        barrier(CLK_LOCAL_MEM_FENCE); \n"\
"    } \n"\
"} \n"\
" \n"\
" \n"\
"INLINE void group_reduce_max(__local float* lds, int size, int lid) \n"\
"{ \n"\
"    for (int offset = (size >> 1); offset > 0; offset >>= 1) \n"\
"    { \n"\
"        if (lid < offset) \n"\
"        { \n"\
"            lds[lid] = max(lds[lid], lds[lid + offset]); \n"\
"        } \n"\
"        barrier(CLK_LOCAL_MEM_FENCE); \n"\
"    } \n"\
"} \n"\
" \n"\
" \n"\
"KERNEL void EstimateVariance( \n"\
"    GLOBAL float4 const* restrict image_buffer, \n"\
"    GLOBAL float* restrict variance_buffer, \n"\
"    int width, \n"\
"    int height \n"\
") \n"\
"{ \n"\
"    __local float lds[256]; \n"\
" \n"\
"    int x = get_global_id(0); \n"\
"    int y = get_global_id(1); \n"\
"    int lx = get_local_id(0); \n"\
"    int ly = get_local_id(1); \n"\
"    int gx = get_group_id(0); \n"\
"    int gy = get_group_id(1); \n"\
"    int wx = get_local_size(0); \n"\
"    int wy = get_local_size(1); \n"\
"    int num_tiles = (width + wx - 1) / wx; \n"\
"    int lid = ly * wx + lx; \n"\
" \n"\
"    float value = 0.f; \n"\
"    if (x < width && y < height) \n"\
"    { \n"\
"        float4 rw = image_buffer[y * width + x]; rw /= rw.w; \n"\
"        value = 4*luminance(clamp(rw.xyz, 0.f, 1.f)); \n"\
"        rw = y + 1 < height ? image_buffer[(y + 1) * width + x] : image_buffer[y * width + x]; rw /= rw.w; \n"\
"        value -= luminance(clamp(rw.xyz, 0.f, 1.f)); \n"\
"        rw = y - 1 >= 0 ? image_buffer[(y - 1) * width + x] : image_buffer[y * width + x]; rw /= rw.w; \n"\
"        value -= luminance(clamp(rw.xyz, 0.f, 1.f)); \n"\
"        rw = x + 1 < width ? image_buffer[y * width + x + 1] : image_buffer[y * width + x]; rw /= rw.w; \n"\
"        value -= luminance(clamp(rw.xyz, 0.f, 1.f)); \n"\
"        rw = x - 1 >= 0 ? image_buffer[y * width + x - 1] : image_buffer[y * width + x]; rw /= rw.w; \n"\
"        value -= luminance(clamp(rw.xyz, 0.f, 1.f)); \n"\
"        //rw = y + 1 < height && x + 1 < width ? image_buffer[(y + 1) * width + x + 1] : image_buffer[y * width + x]; rw /= rw.w; \n"\
"        //value -= luminance(clamp(rw.xyz, 0.f, 1.f)); \n"\
"        //rw = y - 1 >= 0 && x - 1 >= 0 ? image_buffer[(y - 1) * width + x - 1] : image_buffer[y * width + x]; rw /= rw.w; \n"\
"        //value -= luminance(clamp(rw.xyz, 0.f, 1.f)); \n"\
"        //rw = y + 1 < height && x - 1 >= 0 ? image_buffer[(y + 1) * width + x - 1] : image_buffer[y * width + x]; rw /= rw.w; \n"\
"        //value -= luminance(clamp(rw.xyz, 0.f, 1.f)); \n"\
"        //rw = y - 1 >= 0 && x + 1 < width ? image_buffer[(y - 1) * width + x + 1] : image_buffer[y * width + x]; rw /= rw.w; \n"\
"        //value -= luminance(clamp(rw.xyz, 0.f, 1.f)); \n"\
"    } \n"\
" \n"\
"    value = fabs(value); \n"\
"    lds[lid] = value; \n"\
"    barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"    group_reduce_add(lds, 256, lid); \n"\
" \n"\
"    float mean = lds[0] / (wx * wy); \n"\
"    barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"    /*lds[lid] = (mean - value) * (mean - value); \n"\
"    barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"    group_reduce_add(lds, 256, lid);*/ \n"\
" \n"\
"    if (x < width && y < height) \n"\
"    { \n"\
"        if (lx == 0 && ly == 0) \n"\
"        { \n"\
"            //float dev = lds[0] / (wx * wy - 1); \n"\
"            variance_buffer[gy * num_tiles + gx] = mean; \n"\
"        } \n"\
"    } \n"\
"} \n"\
" \n"\
"KERNEL \n"\
"void  OrthographicCamera_GeneratePaths( \n"\
"                                     // Camera \n"\
"                                     GLOBAL Camera const* restrict camera, \n"\
"                                     // Image resolution \n"\
"                                     int output_width, \n"\
"                                     int output_height, \n"\
"                                     // Pixel domain buffer \n"\
"                                     GLOBAL int const* restrict pixel_idx, \n"\
"                                     // Size of pixel domain buffer \n"\
"                                     GLOBAL int const* restrict num_pixels, \n"\
"                                     // RNG seed value \n"\
"                                     uint rng_seed, \n"\
"                                     // Current frame \n"\
"                                     uint frame, \n"\
"                                     // Rays to generate \n"\
"                                     GLOBAL ray* restrict rays, \n"\
"                                     // RNG data \n"\
"                                     GLOBAL uint* restrict random, \n"\
"                                     GLOBAL uint const* restrict sobol_mat \n"\
"                                     ) \n"\
"{ \n"\
"    int global_id = get_global_id(0); \n"\
"     \n"\
"    // Check borders \n"\
"    if (global_id < *num_pixels) \n"\
"    { \n"\
"        int idx = pixel_idx[global_id]; \n"\
"        int y = idx / output_width; \n"\
"        int x = idx % output_width; \n"\
"         \n"\
"        // Get pointer to ray & path handles \n"\
"        GLOBAL ray* my_ray = rays + global_id; \n"\
"         \n"\
"        // Initialize sampler \n"\
"        Sampler sampler; \n"\
"#if SAMPLER == SOBOL \n"\
"        uint scramble = random[x + output_width * y] * 0x1fe3434f; \n"\
"         \n"\
"        if (frame & 0xF) \n"\
"        { \n"\
"            random[x + output_width * y] = WangHash(scramble); \n"\
"        } \n"\
"         \n"\
"        Sampler_Init(&sampler, frame, SAMPLE_DIM_CAMERA_OFFSET, scramble); \n"\
"#elif SAMPLER == RANDOM \n"\
"        uint scramble = x + output_width * y * rng_seed; \n"\
"        Sampler_Init(&sampler, scramble); \n"\
"#elif SAMPLER == CMJ \n"\
"        uint rnd = random[x + output_width * y]; \n"\
"        uint scramble = rnd * 0x1fe3434f * ((frame + 133 * rnd) / (CMJ_DIM * CMJ_DIM)); \n"\
"        Sampler_Init(&sampler, frame % (CMJ_DIM * CMJ_DIM), SAMPLE_DIM_CAMERA_OFFSET, scramble); \n"\
"#endif \n"\
"         \n"\
"        // Generate sample \n"\
"#ifndef BAIKAL_GENERATE_SAMPLE_AT_PIXEL_CENTER \n"\
"        float2 sample0 = Sampler_Sample2D(&sampler, SAMPLER_ARGS); \n"\
"#else \n"\
"        float2 sample0 = make_float2(0.5f, 0.5f); \n"\
"#endif \n"\
"         \n"\
"        // Calculate [0..1] image plane sample \n"\
"        float2 img_sample; \n"\
"        img_sample.x = (float)x / output_width + sample0.x / output_width; \n"\
"        img_sample.y = (float)y / output_height + sample0.y / output_height; \n"\
"         \n"\
"        // Transform into [-0.5, 0.5] \n"\
"        float2 h_sample = img_sample - make_float2(0.5f, 0.5f); \n"\
"        // Transform into [-dim/2, dim/2] \n"\
"        float2 c_sample = h_sample * camera->dim; \n"\
"         \n"\
"        // Calculate direction to image plane \n"\
"        my_ray->d.xyz = normalize(camera->forward); \n"\
"        // Origin == camera position + nearz * d \n"\
"        my_ray->o.xyz = camera->p + c_sample.x * camera->right + c_sample.y * camera->up; \n"\
"        // Max T value = zfar - znear since we moved origin to znear \n"\
"        my_ray->o.w = camera->zcap.y - camera->zcap.x; \n"\
"        // Generate random time from 0 to 1 \n"\
"        my_ray->d.w = sample0.x; \n"\
"        // Set ray max \n"\
"        my_ray->extra.x = 0xFFFFFFFF; \n"\
"        my_ray->extra.y = 0xFFFFFFFF; \n"\
"        Ray_SetExtra(my_ray, 1.f); \n"\
"        Ray_SetMask(my_ray, VISIBILITY_MASK_PRIMARY); \n"\
"    } \n"\
"} \n"\
" \n"\
" \n"\
"#endif // MONTE_CARLO_RENDERER_CL \n"\
;
static const char g_normalmap_opencl[]= \
"/********************************************************************** \n"\
"Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved. \n"\
" \n"\
"Permission is hereby granted, free of charge, to any person obtaining a copy \n"\
"of this software and associated documentation files (the \"Software\"), to deal \n"\
"in the Software without restriction, including without limitation the rights \n"\
"to use, copy, modify, merge, publish, distribute, sublicense, and/or sell \n"\
"copies of the Software, and to permit persons to whom the Software is \n"\
"furnished to do so, subject to the following conditions: \n"\
" \n"\
"The above copyright notice and this permission notice shall be included in \n"\
"all copies or substantial portions of the Software. \n"\
" \n"\
"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR \n"\
"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, \n"\
"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE \n"\
"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER \n"\
"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, \n"\
"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN \n"\
"THE SOFTWARE. \n"\
"********************************************************************/ \n"\
"#ifndef NORMALMAP_CL \n"\
"#define NORMALMAP_CL \n"\
" \n"\
" \n"\
"void DifferentialGeometry_ApplyNormalMap(DifferentialGeometry* diffgeo, TEXTURE_ARG_LIST) \n"\
"{ \n"\
"    int nmapidx = diffgeo->mat.nmapidx; \n"\
"    if (nmapidx != -1) \n"\
"    { \n"\
"        // Now n, dpdu, dpdv is orthonormal basis \n"\
"        float3 mappednormal = 2.f * Texture_Sample2D(diffgeo->uv, TEXTURE_ARGS_IDX(nmapidx)).xyz - make_float3(1.f, 1.f, 1.f); \n"\
" \n"\
"        // Return mapped version \n"\
"        diffgeo->n = normalize(mappednormal.z *  diffgeo->n + mappednormal.x * diffgeo->dpdu + mappednormal.y * diffgeo->dpdv); \n"\
"        diffgeo->dpdv = normalize(cross(diffgeo->n, diffgeo->dpdu)); \n"\
"        diffgeo->dpdu = normalize(cross(diffgeo->dpdv, diffgeo->n)); \n"\
"    } \n"\
"} \n"\
" \n"\
"void DifferentialGeometry_ApplyBumpMap(DifferentialGeometry* diffgeo, TEXTURE_ARG_LIST) \n"\
"{ \n"\
"    int nmapidx = diffgeo->mat.nmapidx; \n"\
"    if (nmapidx != -1) \n"\
"    { \n"\
"        // Now n, dpdu, dpdv is orthonormal basis \n"\
"        float3 mappednormal = 2.f * Texture_SampleBump(diffgeo->uv, TEXTURE_ARGS_IDX(nmapidx)) - make_float3(1.f, 1.f, 1.f); \n"\
" \n"\
"        // Return mapped version \n"\
"        diffgeo->n = normalize(mappednormal.z * diffgeo->n + mappednormal.x * diffgeo->dpdu + mappednormal.y * diffgeo->dpdv); \n"\
"        diffgeo->dpdv = normalize(cross(diffgeo->n, diffgeo->dpdu)); \n"\
"        diffgeo->dpdu = normalize(cross(diffgeo->dpdv, diffgeo->n)); \n"\
"    } \n"\
"} \n"\
" \n"\
"void DifferentialGeometry_ApplyBumpNormalMap(DifferentialGeometry* diffgeo, TEXTURE_ARG_LIST) \n"\
"{ \n"\
"    int bump_flag = diffgeo->mat.bump_flag; \n"\
"    if (bump_flag) \n"\
"    { \n"\
"        DifferentialGeometry_ApplyBumpMap(diffgeo, TEXTURE_ARGS); \n"\
"    } \n"\
"    else \n"\
"    { \n"\
"        DifferentialGeometry_ApplyNormalMap(diffgeo, TEXTURE_ARGS); \n"\
"    } \n"\
"} \n"\
" \n"\
" \n"\
" \n"\
"#endif // NORMALMAP_CL \n"\
;
static const char g_path_opencl[]= \
"/********************************************************************** \n"\
"Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved. \n"\
" \n"\
"Permission is hereby granted, free of charge, to any person obtaining a copy \n"\
"of this software and associated documentation files (the \"Software\"), to deal \n"\
"in the Software without restriction, including without limitation the rights \n"\
"to use, copy, modify, merge, publish, distribute, sublicense, and/or sell \n"\
"copies of the Software, and to permit persons to whom the Software is \n"\
"furnished to do so, subject to the following conditions: \n"\
" \n"\
"The above copyright notice and this permission notice shall be included in \n"\
"all copies or substantial portions of the Software. \n"\
" \n"\
"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR \n"\
"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, \n"\
"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE \n"\
"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER \n"\
"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, \n"\
"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN \n"\
"THE SOFTWARE. \n"\
"********************************************************************/ \n"\
"#ifndef PATH_CL \n"\
"#define PATH_CL \n"\
" \n"\
" \n"\
"typedef struct _Path \n"\
"{ \n"\
"    float3 throughput; \n"\
"    int volume; \n"\
"    int flags; \n"\
"    int active; \n"\
"    int extra1; \n"\
"} Path; \n"\
" \n"\
"typedef enum _PathFlags \n"\
"{ \n"\
"    kNone = 0x0, \n"\
"    kKilled = 0x1, \n"\
"    kScattered = 0x2, \n"\
"    kSpecularBounce = 0x4 \n"\
"} PathFlags; \n"\
" \n"\
"bool Path_IsScattered(__global Path const* path) \n"\
"{ \n"\
"    return path->flags & kScattered; \n"\
"} \n"\
" \n"\
"bool Path_IsSpecular(__global Path const* path) \n"\
"{ \n"\
"    return path->flags & kSpecularBounce; \n"\
"} \n"\
" \n"\
"bool Path_IsAlive(__global Path const* path) \n"\
"{ \n"\
"    return ((path->flags & kKilled) == 0); \n"\
"} \n"\
" \n"\
"void Path_ClearScatterFlag(__global Path* path) \n"\
"{ \n"\
"    path->flags &= ~kScattered; \n"\
"} \n"\
" \n"\
"void Path_SetScatterFlag(__global Path* path) \n"\
"{ \n"\
"    path->flags |= kScattered; \n"\
"} \n"\
" \n"\
" \n"\
"void Path_ClearSpecularFlag(__global Path* path) \n"\
"{ \n"\
"    path->flags &= ~kSpecularBounce; \n"\
"} \n"\
" \n"\
"void Path_SetSpecularFlag(__global Path* path) \n"\
"{ \n"\
"    path->flags |= kSpecularBounce; \n"\
"} \n"\
" \n"\
"void Path_Restart(__global Path* path) \n"\
"{ \n"\
"    path->flags = 0; \n"\
"} \n"\
" \n"\
"int Path_GetVolumeIdx(__global Path const* path) \n"\
"{ \n"\
"    return path->volume; \n"\
"} \n"\
" \n"\
"void Path_SetVolumeIdx(__global Path* path, int volume_idx) \n"\
"{ \n"\
"    path->volume = volume_idx; \n"\
"} \n"\
" \n"\
"float3 Path_GetThroughput(__global Path const* path) \n"\
"{ \n"\
"    float3 t = path->throughput; \n"\
"    return t; \n"\
"} \n"\
" \n"\
"void Path_MulThroughput(__global Path* path, float3 mul) \n"\
"{ \n"\
"    path->throughput *= mul; \n"\
"} \n"\
" \n"\
"void Path_Kill(__global Path* path) \n"\
"{ \n"\
"    path->flags |= kKilled; \n"\
"} \n"\
" \n"\
"void Path_AddContribution(__global Path* path, __global float3* output, int idx, float3 val) \n"\
"{ \n"\
"    output[idx] += Path_GetThroughput(path) * val; \n"\
"} \n"\
" \n"\
" \n"\
" \n"\
"#endif \n"\
;
static const char g_path_tracing_estimator_opencl[]= \
" \n"\
"/********************************************************************** \n"\
"Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved. \n"\
" \n"\
"Permission is hereby granted, free of charge, to any person obtaining a copy \n"\
"of this software and associated documentation files (the \"Software\"), to deal \n"\
"in the Software without restriction, including without limitation the rights \n"\
"to use, copy, modify, merge, publish, distribute, sublicense, and/or sell \n"\
"copies of the Software, and to permit persons to whom the Software is \n"\
"furnished to do so, subject to the following conditions: \n"\
" \n"\
"The above copyright notice and this permission notice shall be included in \n"\
"all copies or substantial portions of the Software. \n"\
" \n"\
"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR \n"\
"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, \n"\
"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE \n"\
"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER \n"\
"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, \n"\
"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN \n"\
"THE SOFTWARE. \n"\
"********************************************************************/ \n"\
" \n"\
" \n"\
"KERNEL \n"\
"void InitPathData( \n"\
"    GLOBAL int const* restrict src_index,  \n"\
"    GLOBAL int* restrict dst_index, \n"\
"    GLOBAL int const* restrict num_elements, \n"\
"    GLOBAL Path* restrict paths \n"\
") \n"\
"{ \n"\
"    int global_id = get_global_id(0); \n"\
" \n"\
"    // Check borders \n"\
"    if (global_id < *num_elements) \n"\
"    { \n"\
"        GLOBAL Path* my_path = paths + global_id; \n"\
"        dst_index[global_id] = src_index[global_id]; \n"\
" \n"\
"        // Initalize path data \n"\
"        my_path->throughput = make_float3(1.f, 1.f, 1.f); \n"\
"        my_path->volume = INVALID_IDX; \n"\
"        my_path->flags = 0; \n"\
"        my_path->active = 0xFF; \n"\
"    } \n"\
"} \n"\
" \n"\
"// This kernel only handles scattered paths. \n"\
"// It applies direct illumination and generates \n"\
"// path continuation if multiscattering is enabled.  \n"\
"KERNEL void ShadeVolume( \n"\
"    // Ray batch \n"\
"    GLOBAL ray const* restrict rays, \n"\
"    // Intersection data \n"\
"    GLOBAL Intersection const* restrict isects, \n"\
"    // Hit indices \n"\
"    GLOBAL int const* restrict hit_indices, \n"\
"    // Pixel indices \n"\
"    GLOBAL int const*  restrict pixel_indices, \n"\
"    // Output indices \n"\
"    GLOBAL int const*  restrict output_indices, \n"\
"    // Number of rays \n"\
"    GLOBAL int const*  restrict num_hits, \n"\
"    // Vertices \n"\
"    GLOBAL float3 const* restrict vertices, \n"\
"    // Normals \n"\
"    GLOBAL float3 const* restrict normals, \n"\
"    // UVs \n"\
"    GLOBAL float2 const* restrict uvs, \n"\
"    // Indices \n"\
"    GLOBAL int const* restrict indices, \n"\
"    // Shapes \n"\
"    GLOBAL Shape const* restrict shapes, \n"\
"    // Materials \n"\
"    GLOBAL Material const* restrict materials, \n"\
"    // Textures \n"\
"    TEXTURE_ARG_LIST, \n"\
"    // Environment texture index \n"\
"    int env_light_idx, \n"\
"    // Emissives \n"\
"    GLOBAL Light const* restrict lights, \n"\
"    // Light distribution \n"\
"    GLOBAL int const* restrict light_distribution, \n"\
"    // Number of emissive objects \n"\
"    int num_lights, \n"\
"    // RNG seed \n"\
"    uint rng_seed, \n"\
"    // Sampler state  \n"\
"    GLOBAL uint* restrict random, \n"\
"    // Sobol matrices \n"\
"    GLOBAL uint const* restrict sobol_mat, \n"\
"    // Current bounce \n"\
"    int bounce, \n"\
"    // Current frame \n"\
"    int frame, \n"\
"    // Volume data \n"\
"    GLOBAL Volume const* restrict volumes, \n"\
"    // Shadow rays \n"\
"    GLOBAL ray* restrict shadow_rays, \n"\
"    // Light samples \n"\
"    GLOBAL float3* restrict light_samples, \n"\
"    // Path throughput \n"\
"    GLOBAL Path* restrict paths, \n"\
"    // Indirect rays (next path segment) \n"\
"    GLOBAL ray* restrict indirect_rays, \n"\
"    // Radiance \n"\
"    GLOBAL float3* restrict output \n"\
") \n"\
"{ \n"\
"    int global_id = get_global_id(0); \n"\
" \n"\
"    Scene scene = \n"\
"    { \n"\
"        vertices, \n"\
"        normals, \n"\
"        uvs, \n"\
"        indices, \n"\
"        shapes, \n"\
"        materials, \n"\
"        lights, \n"\
"        env_light_idx, \n"\
"        num_lights, \n"\
"        light_distribution \n"\
"    }; \n"\
" \n"\
"    if (global_id < *num_hits) \n"\
"    { \n"\
"        // Fetch index \n"\
"        int hit_idx = hit_indices[global_id]; \n"\
"        int pixel_idx = pixel_indices[global_id]; \n"\
"        Intersection isect = isects[hit_idx]; \n"\
" \n"\
"        GLOBAL Path* path = paths + pixel_idx; \n"\
" \n"\
"        // Only apply to scattered paths \n"\
"        if (!Path_IsScattered(path)) \n"\
"        { \n"\
"            return; \n"\
"        } \n"\
" \n"\
"        // Fetch incoming ray \n"\
"        float3 o = rays[hit_idx].o.xyz; \n"\
"        float3 wi = rays[hit_idx].d.xyz; \n"\
" \n"\
"        Sampler sampler; \n"\
"#if SAMPLER == SOBOL \n"\
"        uint scramble = random[pixel_idx] * 0x1fe3434f; \n"\
"        Sampler_Init(&sampler, frame, SAMPLE_DIM_SURFACE_OFFSET + bounce * SAMPLE_DIMS_PER_BOUNCE + SAMPLE_DIM_VOLUME_EVALUATE_OFFSET, scramble); \n"\
"#elif SAMPLER == RANDOM \n"\
"        uint scramble = pixel_idx * rng_seed; \n"\
"        Sampler_Init(&sampler, scramble); \n"\
"#elif SAMPLER == CMJ \n"\
"        uint rnd = random[pixel_idx]; \n"\
"        uint scramble = rnd * 0x1fe3434f * ((frame + 13 * rnd) / (CMJ_DIM * CMJ_DIM)); \n"\
"        Sampler_Init(&sampler, frame % (CMJ_DIM * CMJ_DIM), SAMPLE_DIM_SURFACE_OFFSET + bounce * SAMPLE_DIMS_PER_BOUNCE + SAMPLE_DIM_VOLUME_EVALUATE_OFFSET, scramble); \n"\
"#endif \n"\
" \n"\
" \n"\
"        // Here we know that volume_idx != -1 since this is a precondition \n"\
"        // for scattering event \n"\
"        int volume_idx = Path_GetVolumeIdx(path); \n"\
" \n"\
"        // Sample light source \n"\
"        float pdf = 0.f; \n"\
"        float selection_pdf = 0.f; \n"\
"        float3 wo; \n"\
" \n"\
"        int light_idx = Scene_SampleLight(&scene, Sampler_Sample1D(&sampler, SAMPLER_ARGS), &selection_pdf); \n"\
" \n"\
"        // Here we need fake differential geometry for light sampling procedure \n"\
"        DifferentialGeometry dg; \n"\
"        // put scattering position in there (it is along the current ray at isect.distance \n"\
"        // since EvaluateVolume has put it there \n"\
"        dg.p = o + wi * Intersection_GetDistance(isects + hit_idx); \n"\
"        // Get light sample intencity \n"\
"        float3 le = Light_Sample(light_idx, &scene, &dg, TEXTURE_ARGS, Sampler_Sample2D(&sampler, SAMPLER_ARGS), &wo, &pdf); \n"\
" \n"\
"        // Generate shadow ray \n"\
"        float shadow_ray_length = length(wo); \n"\
"        Ray_Init(shadow_rays + global_id, dg.p, normalize(wo), shadow_ray_length, 0.f, 0xFFFFFFFF); \n"\
" \n"\
"        // Evaluate volume transmittion along the shadow ray (it is incorrect if the light source is outside of the \n"\
"        // current volume, but in this case it will be discarded anyway since the intersection at the outer bound \n"\
"        // of a current volume), so the result is fully correct. \n"\
"        float3 tr = Volume_Transmittance(&volumes[volume_idx], &shadow_rays[global_id], shadow_ray_length); \n"\
" \n"\
"        // Volume emission is applied only if the light source is in the current volume(this is incorrect since the light source might be \n"\
"        // outside of a volume and we have to compute fraction of ray in this case, but need to figure out how) \n"\
"        float3 r = Volume_Emission(&volumes[volume_idx], &shadow_rays[global_id], shadow_ray_length); \n"\
" \n"\
"        // This is the estimate coming from a light source \n"\
"        // TODO: remove hardcoded phase func and sigma \n"\
"        r += tr * le * volumes[volume_idx].sigma_s * PhaseFunction_Uniform(wi, normalize(wo)) / pdf / selection_pdf; \n"\
" \n"\
"        // Only if we have some radiance compute the visibility ray \n"\
"        if (NON_BLACK(tr) && NON_BLACK(r) && pdf > 0.f) \n"\
"        { \n"\
"            // Put lightsample result \n"\
"            light_samples[global_id] = REASONABLE_RADIANCE(r * Path_GetThroughput(path)); \n"\
"        } \n"\
"        else \n"\
"        { \n"\
"            // Nothing to compute \n"\
"            light_samples[global_id] = 0.f; \n"\
"            // Otherwise make it incative to save intersector cycles (hopefully) \n"\
"            Ray_SetInactive(shadow_rays + global_id); \n"\
"        } \n"\
" \n"\
"#ifdef MULTISCATTER \n"\
"        // This is highly brute-force \n"\
"        // TODO: investigate importance sampling techniques here \n"\
"        wo = Sample_MapToSphere(Sampler_Sample2D(&sampler, SAMPLER_ARGS)); \n"\
"        pdf = 1.f / (4.f * PI); \n"\
" \n"\
"        // Generate new path segment \n"\
"        Ray_Init(indirect_rays + global_id, dg.p, normalize(wo), CRAZY_HIGH_DISTANCE, 0.f, 0xFFFFFFFF); \n"\
" \n"\
"        // Update path throughput multiplying by phase function. \n"\
"        Path_MulThroughput(path, PhaseFunction_Uniform(wi, normalize(wo)) / pdf); \n"\
"#else \n"\
"        // Single-scattering mode only, \n"\
"        // kill the path and compact away on next iteration \n"\
"        Path_Kill(path); \n"\
"        Ray_SetInactive(indirect_rays + global_id); \n"\
"#endif \n"\
"    } \n"\
"} \n"\
" \n"\
"// Handle ray-surface interaction possibly generating path continuation.  \n"\
"// This is only applied to non-scattered paths. \n"\
"KERNEL void ShadeSurface( \n"\
"    // Ray batch \n"\
"    GLOBAL ray const* restrict rays, \n"\
"    // Intersection data \n"\
"    GLOBAL Intersection const* restrict isects, \n"\
"    // Hit indices \n"\
"    GLOBAL int const* restrict hit_indices, \n"\
"    // Pixel indices \n"\
"    GLOBAL int const* restrict pixel_indices, \n"\
"    // Output indices \n"\
"    GLOBAL int const*  restrict output_indices, \n"\
"    // Number of rays \n"\
"    GLOBAL int const* restrict num_hits, \n"\
"    // Vertices \n"\
"    GLOBAL float3 const* restrict vertices, \n"\
"    // Normals \n"\
"    GLOBAL float3 const* restrict normals, \n"\
"    // UVs \n"\
"    GLOBAL float2 const* restrict uvs, \n"\
"    // Indices \n"\
"    GLOBAL int const* restrict indices, \n"\
"    // Shapes \n"\
"    GLOBAL Shape const* restrict shapes, \n"\
"    // Materials \n"\
"    GLOBAL Material const* restrict materials, \n"\
"    // Textures \n"\
"    TEXTURE_ARG_LIST, \n"\
"    // Environment texture index \n"\
"    int env_light_idx, \n"\
"    // Emissives \n"\
"    GLOBAL Light const* restrict lights, \n"\
"    // Light distribution \n"\
"    GLOBAL int const* restrict light_distribution, \n"\
"    // Number of emissive objects \n"\
"    int num_lights, \n"\
"    // RNG seed \n"\
"    uint rng_seed, \n"\
"    // Sampler states \n"\
"    GLOBAL uint* restrict random, \n"\
"    // Sobol matrices \n"\
"    GLOBAL uint const* restrict sobol_mat, \n"\
"    // Current bounce \n"\
"    int bounce, \n"\
"    // Frame \n"\
"    int frame, \n"\
"    // Volume data \n"\
"    GLOBAL Volume const* restrict volumes, \n"\
"    // Shadow rays \n"\
"    GLOBAL ray* restrict shadow_rays, \n"\
"    // Light samples \n"\
"    GLOBAL float3* restrict light_samples, \n"\
"    // Path throughput \n"\
"    GLOBAL Path* restrict paths, \n"\
"    // Indirect rays \n"\
"    GLOBAL ray* restrict indirect_rays, \n"\
"    // Radiance \n"\
"    GLOBAL float3* restrict output \n"\
") \n"\
"{ \n"\
"    int global_id = get_global_id(0); \n"\
" \n"\
"    Scene scene = \n"\
"    { \n"\
"        vertices, \n"\
"        normals, \n"\
"        uvs, \n"\
"        indices, \n"\
"        shapes, \n"\
"        materials, \n"\
"        lights, \n"\
"        env_light_idx, \n"\
"        num_lights, \n"\
"        light_distribution \n"\
"    }; \n"\
" \n"\
"    // Only applied to active rays after compaction \n"\
"    if (global_id < *num_hits) \n"\
"    { \n"\
"        // Fetch index \n"\
"        int hit_idx = hit_indices[global_id]; \n"\
"        int pixel_idx = pixel_indices[global_id]; \n"\
"        Intersection isect = isects[hit_idx]; \n"\
" \n"\
"        GLOBAL Path* path = paths + pixel_idx; \n"\
" \n"\
"        // Early exit for scattered paths \n"\
"        if (Path_IsScattered(path)) \n"\
"        { \n"\
"            return; \n"\
"        } \n"\
" \n"\
"        // Fetch incoming ray direction \n"\
"        float3 wi = -normalize(rays[hit_idx].d.xyz); \n"\
" \n"\
"        Sampler sampler; \n"\
"#if SAMPLER == SOBOL  \n"\
"        uint scramble = random[pixel_idx] * 0x1fe3434f; \n"\
"        Sampler_Init(&sampler, frame, SAMPLE_DIM_SURFACE_OFFSET + bounce * SAMPLE_DIMS_PER_BOUNCE, scramble); \n"\
"#elif SAMPLER == RANDOM \n"\
"        uint scramble = pixel_idx * rng_seed; \n"\
"        Sampler_Init(&sampler, scramble); \n"\
"#elif SAMPLER == CMJ \n"\
"        uint rnd = random[pixel_idx]; \n"\
"        uint scramble = rnd * 0x1fe3434f * ((frame + 331 * rnd) / (CMJ_DIM * CMJ_DIM)); \n"\
"        Sampler_Init(&sampler, frame % (CMJ_DIM * CMJ_DIM), SAMPLE_DIM_SURFACE_OFFSET + bounce * SAMPLE_DIMS_PER_BOUNCE, scramble); \n"\
"#endif \n"\
" \n"\
"        // Fill surface data \n"\
"        DifferentialGeometry diffgeo; \n"\
"        Scene_FillDifferentialGeometry(&scene, &isect, &diffgeo);  \n"\
" \n"\
"        // Check if we are hitting from the inside \n"\
"        float ngdotwi = dot(diffgeo.ng, wi); \n"\
"        bool backfacing = ngdotwi < 0.f; \n"\
" \n"\
"        // Select BxDF  \n"\
"        Material_Select(&scene, wi, &sampler, TEXTURE_ARGS, SAMPLER_ARGS, &diffgeo);  \n"\
" \n"\
"        // Terminate if emissive \n"\
"        if (Bxdf_IsEmissive(&diffgeo)) \n"\
"        { \n"\
"            if (!backfacing) \n"\
"            { \n"\
"                float weight = 1.f; \n"\
" \n"\
"                if (bounce > 0 && !Path_IsSpecular(path)) \n"\
"                { \n"\
"                    float2 extra = Ray_GetExtra(&rays[hit_idx]); \n"\
"                    float ld = isect.uvwt.w; \n"\
"                    float denom = fabs(dot(diffgeo.n, wi)) * diffgeo.area; \n"\
"                    // TODO: num_lights should be num_emissies instead, presence of analytical lights breaks this code \n"\
"                    float bxdf_light_pdf = denom > 0.f ? (ld * ld / denom / num_lights) : 0.f; \n"\
"                    weight = extra.x > 0.f ? BalanceHeuristic(1, extra.x, 1, bxdf_light_pdf) : 1.f; \n"\
"                } \n"\
" \n"\
"                // In this case we hit after an application of MIS process at previous step. \n"\
"                // That means BRDF weight has been already applied. \n"\
"                float3 v = Path_GetThroughput(path) * Emissive_GetLe(&diffgeo, TEXTURE_ARGS) * weight; \n"\
"                int output_index = output_indices[pixel_idx]; \n"\
"                ADD_FLOAT3(&output[output_index], v); \n"\
"            } \n"\
" \n"\
"            Path_Kill(path); \n"\
"            Ray_SetInactive(shadow_rays + global_id); \n"\
"            Ray_SetInactive(indirect_rays + global_id); \n"\
" \n"\
"            light_samples[global_id] = 0.f; \n"\
"            return; \n"\
"        } \n"\
" \n"\
" \n"\
"        float s = Bxdf_IsBtdf(&diffgeo) ? (-sign(ngdotwi)) : 1.f; \n"\
"        if (backfacing && !Bxdf_IsBtdf(&diffgeo)) \n"\
"        { \n"\
"            //Reverse normal and tangents in this case \n"\
"            //but not for BTDFs, since BTDFs rely \n"\
"            //on normal direction in order to arrange    \n"\
"            //indices of refraction \n"\
"            diffgeo.n = -diffgeo.n; \n"\
"            diffgeo.dpdu = -diffgeo.dpdu; \n"\
"            diffgeo.dpdv = -diffgeo.dpdv; \n"\
"            s = -s; \n"\
"        } \n"\
" \n"\
"        if (Bxdf_IsBtdf(&diffgeo)) \n"\
"        { \n"\
"            if (backfacing) \n"\
"            { \n"\
"                Path_SetVolumeIdx(path, INVALID_IDX); \n"\
"            } \n"\
"            else \n"\
"            { \n"\
"                Path_SetVolumeIdx(path, Scene_GetVolumeIndex(&scene, isect.shapeid - 1)); \n"\
"            } \n"\
"        } \n"\
" \n"\
"        DifferentialGeometry_ApplyBumpNormalMap(&diffgeo, TEXTURE_ARGS); \n"\
"        DifferentialGeometry_CalculateTangentTransforms(&diffgeo); \n"\
" \n"\
"        float ndotwi = fabs(dot(diffgeo.n, wi)); \n"\
" \n"\
"        float light_pdf = 0.f; \n"\
"        float bxdf_light_pdf = 0.f; \n"\
"        float bxdf_pdf = 0.f; \n"\
"        float light_bxdf_pdf = 0.f; \n"\
"        float selection_pdf = 0.f; \n"\
"        float3 radiance = 0.f; \n"\
"        float3 lightwo; \n"\
"        float3 bxdfwo; \n"\
"        float3 wo; \n"\
"        float bxdf_weight = 1.f; \n"\
"        float light_weight = 1.f; \n"\
" \n"\
"        int light_idx = Scene_SampleLight(&scene, Sampler_Sample1D(&sampler, SAMPLER_ARGS), &selection_pdf); \n"\
" \n"\
"        float3 throughput = Path_GetThroughput(path); \n"\
" \n"\
"        // Sample bxdf \n"\
"        float3 bxdf = Bxdf_Sample(&diffgeo, wi, TEXTURE_ARGS, Sampler_Sample2D(&sampler, SAMPLER_ARGS), &bxdfwo, &bxdf_pdf); \n"\
" \n"\
"        // If we have light to sample we can hopefully do mis  \n"\
"        if (light_idx > -1)  \n"\
"        { \n"\
"            // Sample light \n"\
"            float3 le = Light_Sample(light_idx, &scene, &diffgeo, TEXTURE_ARGS, Sampler_Sample2D(&sampler, SAMPLER_ARGS), &lightwo, &light_pdf); \n"\
"            light_bxdf_pdf = Bxdf_GetPdf(&diffgeo, wi, normalize(lightwo), TEXTURE_ARGS); \n"\
"            light_weight = Light_IsSingular(&scene.lights[light_idx]) ? 1.f : BalanceHeuristic(1, light_pdf * selection_pdf, 1, light_bxdf_pdf);  \n"\
" \n"\
"            // Apply MIS to account for both \n"\
"            if (NON_BLACK(le) && light_pdf > 0.0f && !Bxdf_IsSingular(&diffgeo)) \n"\
"            { \n"\
"                wo = lightwo; \n"\
"                float ndotwo = fabs(dot(diffgeo.n, normalize(wo))); \n"\
"                radiance = le * ndotwo * Bxdf_Evaluate(&diffgeo, wi, normalize(wo), TEXTURE_ARGS) * throughput * light_weight / light_pdf / selection_pdf; \n"\
"            } \n"\
"        } \n"\
" \n"\
"        // If we have some light here generate a shadow ray \n"\
"        if (NON_BLACK(radiance)) \n"\
"        { \n"\
"            // Generate shadow ray \n"\
"            float3 shadow_ray_o = diffgeo.p + CRAZY_LOW_DISTANCE * s * diffgeo.ng; \n"\
"            float3 temp = diffgeo.p + wo - shadow_ray_o; \n"\
"            float3 shadow_ray_dir = normalize(temp); \n"\
"            float shadow_ray_length = length(temp); \n"\
"            int shadow_ray_mask = VISIBILITY_MASK_BOUNCE_SHADOW(bounce); \n"\
" \n"\
"            Ray_Init(shadow_rays + global_id, shadow_ray_o, shadow_ray_dir, shadow_ray_length, 0.f, shadow_ray_mask); \n"\
" \n"\
"            // Apply the volume to shadow ray if needed \n"\
"            int volume_idx = Path_GetVolumeIdx(path); \n"\
"            if (volume_idx != -1) \n"\
"            { \n"\
"                radiance *= Volume_Transmittance(&volumes[volume_idx], &shadow_rays[global_id], shadow_ray_length); \n"\
"                radiance += Volume_Emission(&volumes[volume_idx], &shadow_rays[global_id], shadow_ray_length) * throughput; \n"\
"            } \n"\
" \n"\
"            // And write the light sample  \n"\
"            light_samples[global_id] = REASONABLE_RADIANCE(radiance); \n"\
"        } \n"\
"        else \n"\
"        { \n"\
"            // Otherwise save some intersector cycles \n"\
"            Ray_SetInactive(shadow_rays + global_id); \n"\
"            light_samples[global_id] = 0; \n"\
"        } \n"\
" \n"\
"        // Apply Russian roulette \n"\
"        float q = max(min(0.5f, \n"\
"            // Luminance \n"\
"            0.2126f * throughput.x + 0.7152f * throughput.y + 0.0722f * throughput.z), 0.01f); \n"\
"        // Only if it is 3+ bounce \n"\
"        bool rr_apply = bounce > 3; \n"\
"        bool rr_stop = Sampler_Sample1D(&sampler, SAMPLER_ARGS) > q && rr_apply; \n"\
" \n"\
"        if (rr_apply) \n"\
"        { \n"\
"            Path_MulThroughput(path, 1.f / q); \n"\
"        } \n"\
" \n"\
"        if (Bxdf_IsSingular(&diffgeo)) \n"\
"        { \n"\
"            Path_SetSpecularFlag(path); \n"\
"        } \n"\
"        else \n"\
"        { \n"\
"            Path_ClearSpecularFlag(path); \n"\
"        } \n"\
" \n"\
"        bxdfwo = normalize(bxdfwo); \n"\
"        float3 t = bxdf * fabs(dot(diffgeo.n, bxdfwo)); \n"\
" \n"\
"        // Only continue if we have non-zero throughput & pdf \n"\
"        if (NON_BLACK(t) && bxdf_pdf > 0.f && !rr_stop) \n"\
"        { \n"\
"            // Update the throughput \n"\
"            Path_MulThroughput(path, t / bxdf_pdf); \n"\
" \n"\
"            // Generate ray \n"\
"            float3 indirect_ray_dir = bxdfwo; \n"\
"            float3 indirect_ray_o = diffgeo.p + CRAZY_LOW_DISTANCE * s * diffgeo.ng; \n"\
"            int indirect_ray_mask = VISIBILITY_MASK_BOUNCE(bounce + 1); \n"\
" \n"\
"            Ray_Init(indirect_rays + global_id, indirect_ray_o, indirect_ray_dir, CRAZY_HIGH_DISTANCE, 0.f, indirect_ray_mask); \n"\
"            Ray_SetExtra(indirect_rays + global_id, make_float2(Bxdf_IsSingular(&diffgeo) ? 0.f : bxdf_pdf, 0.f)); \n"\
"        } \n"\
"        else \n"\
"        { \n"\
"            // Otherwise kill the path \n"\
"            Path_Kill(path); \n"\
"            Ray_SetInactive(indirect_rays + global_id); \n"\
"        } \n"\
"    } \n"\
"} \n"\
" \n"\
"///< Illuminate missing rays \n"\
"KERNEL void ShadeBackgroundEnvMap( \n"\
"    // Ray batch \n"\
"    GLOBAL ray const* restrict rays, \n"\
"    // Intersection data \n"\
"    GLOBAL Intersection const* restrict isects, \n"\
"    // Pixel indices \n"\
"    GLOBAL int const* restrict pixel_indices, \n"\
"    // Output indices \n"\
"    GLOBAL int const*  restrict output_indices, \n"\
"    // Number of rays \n"\
"    int num_rays, \n"\
"    GLOBAL Light const* restrict lights, \n"\
"    int env_light_idx, \n"\
"    // Textures \n"\
"    TEXTURE_ARG_LIST, \n"\
"    // Environment texture index \n"\
"    GLOBAL Path const* restrict paths, \n"\
"    GLOBAL Volume const* restrict volumes, \n"\
"    // Output values \n"\
"    GLOBAL float4* restrict output \n"\
") \n"\
"{ \n"\
"    int global_id = get_global_id(0); \n"\
" \n"\
"    if (global_id < num_rays) \n"\
"    { \n"\
"        int pixel_idx = pixel_indices[global_id]; \n"\
"        int output_index = output_indices[pixel_idx]; \n"\
" \n"\
"        float4 v = make_float4(0.f, 0.f, 0.f, 1.f); \n"\
" \n"\
"        // In case of a miss \n"\
"        if (isects[global_id].shapeid < 0 && env_light_idx != -1) \n"\
"        { \n"\
"            // Multiply by throughput \n"\
"            int volume_idx = paths[pixel_idx].volume; \n"\
" \n"\
"            Light light = lights[env_light_idx]; \n"\
" \n"\
" \n"\
"            if (volume_idx == -1) \n"\
"                v.xyz = light.multiplier * Texture_SampleEnvMap(rays[global_id].d.xyz, TEXTURE_ARGS_IDX(light.tex)); \n"\
"            else \n"\
"            { \n"\
"                v.xyz = light.multiplier * Texture_SampleEnvMap(rays[global_id].d.xyz, TEXTURE_ARGS_IDX(light.tex)) * \n"\
"                    Volume_Transmittance(&volumes[volume_idx], &rays[global_id], rays[global_id].o.w); \n"\
" \n"\
"                v.xyz += Volume_Emission(&volumes[volume_idx], &rays[global_id], rays[global_id].o.w); \n"\
"            } \n"\
"        } \n"\
" \n"\
"        ADD_FLOAT4(&output[output_index], v);  \n"\
"    } \n"\
"} \n"\
" \n"\
"///< Handle light samples and visibility info and add contribution to final buffer \n"\
"KERNEL void GatherLightSamples(   \n"\
"    // Pixel indices \n"\
"    GLOBAL int const* restrict pixel_indices, \n"\
"    // Output indices \n"\
"    GLOBAL int const*  restrict output_indices, \n"\
"    // Number of rays \n"\
"    GLOBAL int* restrict num_rays, \n"\
"    // Shadow rays hits \n"\
"    GLOBAL int const* restrict shadow_hits, \n"\
"    // Light samples \n"\
"    GLOBAL float3 const* restrict light_samples, \n"\
"    // throughput \n"\
"    GLOBAL Path const* restrict paths, \n"\
"    // Radiance sample buffer \n"\
"    GLOBAL float4* restrict output \n"\
") \n"\
"{ \n"\
"    int global_id = get_global_id(0); \n"\
" \n"\
"    if (global_id < *num_rays) \n"\
"    { \n"\
"        // Get pixel id for this sample set \n"\
"        int pixel_idx = pixel_indices[global_id]; \n"\
"        int output_index = output_indices[pixel_idx]; \n"\
" \n"\
"        // Prepare accumulator variable \n"\
"        float4 radiance = 0.f; \n"\
" \n"\
"        // Start collecting samples \n"\
"        { \n"\
"            // If shadow ray didn't hit anything and reached skydome \n"\
"            if (shadow_hits[global_id] == -1) \n"\
"            { \n"\
"                // Add its contribution to radiance accumulator \n"\
"                radiance.xyz += light_samples[global_id];   \n"\
"            } \n"\
"        } \n"\
" \n"\
"        // Divide by number of light samples (samples already have built-in throughput) \n"\
"        ADD_FLOAT4(&output[output_index], radiance);  \n"\
"    } \n"\
"} \n"\
" \n"\
"///< Handle light samples and visibility info and add contribution to final buffer \n"\
"KERNEL void GatherVisibility( \n"\
"    // Pixel indices \n"\
"    GLOBAL int const* restrict pixel_indices, \n"\
"    // Output indices \n"\
"    GLOBAL int const*  restrict output_indices, \n"\
"    // Number of rays \n"\
"    GLOBAL int* restrict num_rays, \n"\
"    // Shadow rays hits \n"\
"    GLOBAL int const* restrict shadow_hits, \n"\
"    // Radiance sample buffer \n"\
"    GLOBAL float4* restrict output \n"\
") \n"\
"{ \n"\
"    int global_id = get_global_id(0); \n"\
" \n"\
"    if (global_id < *num_rays) \n"\
"    { \n"\
"        // Get pixel id for this sample set \n"\
"        int pixel_idx = pixel_indices[global_id]; \n"\
"        int output_index = output_indices[pixel_idx]; \n"\
" \n"\
"        // Prepare accumulator variable \n"\
"        float4 visibility = make_float4(0.f, 0.f, 0.f, 1.f); \n"\
" \n"\
"        // Start collecting samples \n"\
"        { \n"\
"            // If shadow ray didn't hit anything and reached skydome \n"\
"            if (shadow_hits[global_id] == -1) \n"\
"            { \n"\
"                // Add its contribution to radiance accumulator \n"\
"                visibility.xyz += 1.f; \n"\
"            } \n"\
"        } \n"\
" \n"\
"        // Divide by number of light samples (samples already have built-in throughput) \n"\
"        ADD_FLOAT4(&output[output_index], visibility); \n"\
"    } \n"\
"} \n"\
" \n"\
"///< Restore pixel indices after compaction \n"\
"KERNEL void RestorePixelIndices( \n"\
"    // Compacted indices \n"\
"    GLOBAL int const* restrict compacted_indices, \n"\
"    // Number of compacted indices \n"\
"    GLOBAL int* restrict num_elements, \n"\
"    // Previous pixel indices \n"\
"    GLOBAL int const* restrict prev_indices, \n"\
"    // New pixel indices \n"\
"    GLOBAL int* restrict new_indices \n"\
") \n"\
"{ \n"\
"    int global_id = get_global_id(0); \n"\
" \n"\
"    // Handle only working subset \n"\
"    if (global_id < *num_elements) \n"\
"    { \n"\
"        new_indices[global_id] = prev_indices[compacted_indices[global_id]]; \n"\
"    } \n"\
"} \n"\
" \n"\
"///< Restore pixel indices after compaction \n"\
"KERNEL void FilterPathStream( \n"\
"    // Intersections \n"\
"    GLOBAL Intersection const* restrict isects, \n"\
"    // Number of compacted indices \n"\
"    GLOBAL int const* restrict num_elements, \n"\
"    // Pixel indices \n"\
"    GLOBAL int const* restrict pixel_indices, \n"\
"    // Paths \n"\
"    GLOBAL Path* restrict paths, \n"\
"    // Predicate \n"\
"    GLOBAL int* restrict predicate \n"\
") \n"\
"{ \n"\
"    int global_id = get_global_id(0); \n"\
" \n"\
"    // Handle only working subset \n"\
"    if (global_id < *num_elements) \n"\
"    { \n"\
"        int pixel_idx = pixel_indices[global_id]; \n"\
" \n"\
"        GLOBAL Path* path = paths + pixel_idx; \n"\
" \n"\
"        if (Path_IsAlive(path)) \n"\
"        { \n"\
"            bool kill = (length(Path_GetThroughput(path)) < CRAZY_LOW_THROUGHPUT); \n"\
" \n"\
"            if (!kill) \n"\
"            { \n"\
"                predicate[global_id] = isects[global_id].shapeid >= 0 ? 1 : 0; \n"\
"            } \n"\
"            else \n"\
"            { \n"\
"                Path_Kill(path); \n"\
"                predicate[global_id] = 0; \n"\
"            } \n"\
"        } \n"\
"        else \n"\
"        { \n"\
"            predicate[global_id] = 0; \n"\
"        } \n"\
"    } \n"\
"} \n"\
" \n"\
"///< Illuminate missing rays \n"\
"KERNEL void ShadeMiss( \n"\
"    // Ray batch \n"\
"    GLOBAL ray const* restrict rays, \n"\
"    // Intersection data \n"\
"    GLOBAL Intersection const* restrict isects,  \n"\
"    // Pixel indices \n"\
"    GLOBAL int const* restrict pixel_indices, \n"\
"    // Output indices \n"\
"    GLOBAL int const*  restrict output_indices, \n"\
"    // Number of rays \n"\
"    GLOBAL int const* restrict num_rays, \n"\
"    GLOBAL Light const* restrict lights, \n"\
"    // Light distribution \n"\
"    GLOBAL int const* restrict light_distribution, \n"\
"    // Number of emissive objects \n"\
"    int num_lights, \n"\
"    int env_light_idx, \n"\
"    // Textures \n"\
"    TEXTURE_ARG_LIST, \n"\
"    GLOBAL Path const* restrict paths, \n"\
"    GLOBAL Volume const* restrict volumes, \n"\
"    // Output values \n"\
"    GLOBAL float4* restrict output \n"\
") \n"\
"{ \n"\
"    int global_id = get_global_id(0); \n"\
" \n"\
"    if (global_id < *num_rays) \n"\
"    { \n"\
"        int pixel_idx = pixel_indices[global_id]; \n"\
"        int output_index = output_indices[pixel_idx]; \n"\
" \n"\
"        GLOBAL Path const* path = paths + pixel_idx; \n"\
" \n"\
"        // In case of a miss \n"\
"        if (isects[global_id].shapeid < 0 && Path_IsAlive(path)) \n"\
"        { \n"\
"            Light light = lights[env_light_idx]; \n"\
" \n"\
"            // Apply MIS \n"\
"            float selection_pdf = Distribution1D_GetPdfDiscreet(env_light_idx, light_distribution); \n"\
"            float light_pdf = EnvironmentLight_GetPdf(&light, 0, 0, rays[global_id].d.xyz, TEXTURE_ARGS); \n"\
"            float2 extra = Ray_GetExtra(&rays[global_id]); \n"\
"            float weight = extra.x > 0.f ? BalanceHeuristic(1, extra.x, 1, light_pdf * selection_pdf) : 1.f; \n"\
" \n"\
"            float3 t = Path_GetThroughput(path); \n"\
"            float4 v = 0.f; \n"\
"            v.xyz = REASONABLE_RADIANCE(weight * light.multiplier * Texture_SampleEnvMap(rays[global_id].d.xyz, TEXTURE_ARGS_IDX(light.tex)) * t); \n"\
"            ADD_FLOAT4(&output[output_index], v); \n"\
"        } \n"\
"    } \n"\
"} \n"\
" \n"\
;
static const char g_payload_opencl[]= \
"/********************************************************************** \n"\
"Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved. \n"\
" \n"\
"Permission is hereby granted, free of charge, to any person obtaining a copy \n"\
"of this software and associated documentation files (the \"Software\"), to deal \n"\
"in the Software without restriction, including without limitation the rights \n"\
"to use, copy, modify, merge, publish, distribute, sublicense, and/or sell \n"\
"copies of the Software, and to permit persons to whom the Software is \n"\
"furnished to do so, subject to the following conditions: \n"\
" \n"\
"The above copyright notice and this permission notice shall be included in \n"\
"all copies or substantial portions of the Software. \n"\
" \n"\
"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR \n"\
"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, \n"\
"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE \n"\
"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER \n"\
"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, \n"\
"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN \n"\
"THE SOFTWARE. \n"\
"********************************************************************/ \n"\
"#ifndef PAYLOAD_CL \n"\
"#define PAYLOAD_CL \n"\
" \n"\
"// Matrix \n"\
"typedef struct \n"\
"{ \n"\
"    float4 m0; \n"\
"    float4 m1; \n"\
"    float4 m2; \n"\
"    float4 m3; \n"\
"} matrix4x4; \n"\
" \n"\
"// Camera \n"\
"typedef struct \n"\
"    { \n"\
"        // Coordinate frame \n"\
"        float3 forward; \n"\
"        float3 right; \n"\
"        float3 up; \n"\
"        // Position \n"\
"        float3 p; \n"\
" \n"\
"        // Image plane width & height in current units \n"\
"        float2 dim; \n"\
"        // Near and far Z \n"\
"        float2 zcap; \n"\
"        // Focal lenght \n"\
"        float focal_length; \n"\
"        // Camera aspect_ratio ratio \n"\
"        float aspect_ratio; \n"\
"        float focus_distance; \n"\
"        float aperture; \n"\
"    } Camera; \n"\
" \n"\
"// Shape description \n"\
"typedef struct \n"\
"{ \n"\
"    // Shape starting index \n"\
"    int startidx; \n"\
"    // Start vertex \n"\
"    int startvtx; \n"\
"    // Start material idx \n"\
"    int material_idx; \n"\
"    // Number of primitives in the shape \n"\
"    int volume_idx; \n"\
"    // Linear motion vector \n"\
"    float3 linearvelocity; \n"\
"    // Angular velocity \n"\
"    float4 angularvelocity; \n"\
"    // Transform in row major format \n"\
"    matrix4x4 transform; \n"\
"} Shape; \n"\
" \n"\
" \n"\
"enum Bxdf \n"\
"{ \n"\
"    kZero, \n"\
"    kLambert, \n"\
"    kIdealReflect, \n"\
"    kIdealRefract, \n"\
"    kMicrofacetBeckmann, \n"\
"    kMicrofacetGGX, \n"\
"    kLayered, \n"\
"    kFresnelBlend, \n"\
"    kMix, \n"\
"    kEmissive, \n"\
"    kPassthrough, \n"\
"    kTranslucent, \n"\
"    kMicrofacetRefractionGGX, \n"\
"    kMicrofacetRefractionBeckmann, \n"\
"    kDisney \n"\
"}; \n"\
" \n"\
"// Material description \n"\
"typedef struct _Material \n"\
"{ \n"\
"    union \n"\
"    { \n"\
"        struct \n"\
"        { \n"\
"            float4 kx; \n"\
"            float ni; \n"\
"            float ns; \n"\
"            int kxmapidx; \n"\
"            int nsmapidx; \n"\
"            float fresnel; \n"\
"            int padding0[3]; \n"\
"        } simple; \n"\
" \n"\
"        struct \n"\
"        { \n"\
"            float weight; \n"\
"            int weight_map_idx; \n"\
"            int top_brdf_idx; \n"\
"            int base_brdf_idx; \n"\
"            int padding1[8]; \n"\
"        } compound; \n"\
" \n"\
"        struct \n"\
"        { \n"\
"            float4 base_color; \n"\
"             \n"\
"            float metallic; \n"\
"            float subsurface; \n"\
"            float specular; \n"\
"            float roughness; \n"\
"             \n"\
"            float specular_tint; \n"\
"            float anisotropy; \n"\
"            float sheen; \n"\
"            float sheen_tint; \n"\
"             \n"\
"            float clearcoat; \n"\
"            float clearcoat_gloss; \n"\
"            int base_color_map_idx; \n"\
"            int metallic_map_idx; \n"\
"             \n"\
"            int specular_map_idx; \n"\
"            int roughness_map_idx; \n"\
"            int specular_tint_map_idx; \n"\
"            int anisotropy_map_idx; \n"\
"             \n"\
"            int sheen_map_idx; \n"\
"            int sheen_tint_map_idx; \n"\
"            int clearcoat_map_idx; \n"\
"            int clearcoat_gloss_map_idx; \n"\
"        } disney; \n"\
"    }; \n"\
" \n"\
"    int type; \n"\
"    int bump_flag; \n"\
"    int thin; \n"\
"    int nmapidx; \n"\
"} Material; \n"\
" \n"\
"enum LightType \n"\
"{ \n"\
"    kPoint = 0x1, \n"\
"    kDirectional, \n"\
"    kSpot, \n"\
"    kArea, \n"\
"    kIbl \n"\
"}; \n"\
" \n"\
"typedef struct \n"\
"{ \n"\
"    union \n"\
"    { \n"\
"        // Area light \n"\
"        struct \n"\
"        { \n"\
"            int shapeidx; \n"\
"            int primidx; \n"\
"            int matidx; \n"\
"            int padding0; \n"\
"        }; \n"\
" \n"\
"        // IBL \n"\
"        struct \n"\
"        { \n"\
"            int tex; \n"\
"            int texdiffuse; \n"\
"            float multiplier; \n"\
"            int padding1; \n"\
"        }; \n"\
" \n"\
"        // Spot \n"\
"        struct \n"\
"        { \n"\
"            float ia; \n"\
"            float oa; \n"\
"            float f; \n"\
"            int padding2; \n"\
"        }; \n"\
"    }; \n"\
" \n"\
"    float3 p; \n"\
"    float3 d; \n"\
"    float3 intensity; \n"\
"    int type; \n"\
"    int padding[3]; \n"\
"} Light; \n"\
" \n"\
"typedef enum \n"\
"    { \n"\
"        kEmpty, \n"\
"        kHomogeneous, \n"\
"        kHeterogeneous \n"\
"    } VolumeType; \n"\
" \n"\
"typedef enum \n"\
"    { \n"\
"        kUniform = 0, \n"\
"        kRayleigh, \n"\
"        kMieMurky, \n"\
"        kMieHazy, \n"\
"        kHG // this one requires one extra coeff \n"\
"    } PhaseFunction; \n"\
" \n"\
"typedef struct _Volume \n"\
"    { \n"\
"        VolumeType type; \n"\
"        PhaseFunction phase_func; \n"\
" \n"\
"        // Id of volume data if present \n"\
"        int data; \n"\
"        int extra; \n"\
" \n"\
"        // Absorbtion \n"\
"        float3 sigma_a; \n"\
"        // Scattering \n"\
"        float3 sigma_s; \n"\
"        // Emission \n"\
"        float3 sigma_e; \n"\
"    } Volume; \n"\
" \n"\
"/// Supported formats \n"\
"enum TextureFormat \n"\
"{ \n"\
"    UNKNOWN, \n"\
"    RGBA8, \n"\
"    RGBA16, \n"\
"    RGBA32 \n"\
"}; \n"\
" \n"\
"/// Texture description \n"\
"typedef \n"\
"    struct _Texture \n"\
"    { \n"\
"        // Width, height and depth \n"\
"        int w; \n"\
"        int h; \n"\
"        int d; \n"\
"        // Offset in texture data array \n"\
"        int dataoffset; \n"\
"        // Format \n"\
"        int fmt; \n"\
"        int extra; \n"\
"    } Texture; \n"\
" \n"\
" \n"\
"// Hit data \n"\
"typedef struct _DifferentialGeometry \n"\
"{ \n"\
"    // World space position \n"\
"    float3 p; \n"\
"    // Shading normal \n"\
"    float3 n; \n"\
"    // Geo normal \n"\
"    float3 ng; \n"\
"    // UVs \n"\
"    float2 uv; \n"\
"    // Derivatives \n"\
"    float3 dpdu; \n"\
"    float3 dpdv; \n"\
"    float  area; \n"\
" \n"\
"    matrix4x4 world_to_tangent; \n"\
"    matrix4x4 tangent_to_world; \n"\
" \n"\
"    // Material \n"\
"    Material mat; \n"\
"    int material_index; \n"\
"    int transfer_mode; \n"\
"} DifferentialGeometry; \n"\
" \n"\
" \n"\
" \n"\
" \n"\
" \n"\
" \n"\
" \n"\
" \n"\
" \n"\
" \n"\
"#endif // PAYLOAD_CL \n"\
;
static const char g_ray_opencl[]= \
"/********************************************************************** \n"\
" Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved. \n"\
"  \n"\
" Permission is hereby granted, free of charge, to any person obtaining a copy \n"\
" of this software and associated documentation files (the \"Software\"), to deal \n"\
" in the Software without restriction, including without limitation the rights \n"\
" to use, copy, modify, merge, publish, distribute, sublicense, and/or sell \n"\
" copies of the Software, and to permit persons to whom the Software is \n"\
" furnished to do so, subject to the following conditions: \n"\
"  \n"\
" The above copyright notice and this permission notice shall be included in \n"\
" all copies or substantial portions of the Software. \n"\
"  \n"\
" THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR \n"\
" IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, \n"\
" FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE \n"\
" AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER \n"\
" LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, \n"\
" OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN \n"\
" THE SOFTWARE. \n"\
" ********************************************************************/ \n"\
"#ifndef RAY_CL \n"\
"#define RAY_CL \n"\
" \n"\
" \n"\
"// Ray descriptor \n"\
"typedef struct \n"\
"{ \n"\
"    // xyz - origin, w - max range \n"\
"    float4 o; \n"\
"    // xyz - direction, w - time \n"\
"    float4 d; \n"\
"    // x - ray mask, y - activity flag \n"\
"    int2 extra; \n"\
"    // Padding \n"\
"    float2 padding; \n"\
"} ray; \n"\
" \n"\
"// Set ray activity flag \n"\
"INLINE void Ray_SetInactive(GLOBAL ray* r) \n"\
"{ \n"\
"    r->extra.y = 0; \n"\
"} \n"\
" \n"\
"// Set extra data for ray \n"\
"INLINE void Ray_SetExtra(GLOBAL ray* r, float2 extra) \n"\
"{ \n"\
"    r->padding = extra; \n"\
"} \n"\
" \n"\
"// Set mask \n"\
"INLINE void Ray_SetMask(GLOBAL ray* r, int mask) \n"\
"{ \n"\
"    r->extra.x = mask; \n"\
"} \n"\
" \n"\
"INLINE int Ray_GetMask(GLOBAL ray* r) \n"\
"{ \n"\
"    return r->extra.x; \n"\
"} \n"\
" \n"\
"// Get extra data for ray \n"\
"INLINE float2 Ray_GetExtra(GLOBAL ray const* r) \n"\
"{ \n"\
"    return r->padding; \n"\
"} \n"\
" \n"\
"// Initialize ray structure \n"\
"INLINE void Ray_Init(GLOBAL ray* r, float3 o, float3 d, float maxt, float time, int mask) \n"\
"{ \n"\
"    r->o.xyz = o; \n"\
"    r->d.xyz = d; \n"\
"    r->o.w = maxt; \n"\
"    r->d.w = time; \n"\
"    r->extra.x = mask; \n"\
"    r->extra.y = 0xFFFFFFFF; \n"\
"} \n"\
" \n"\
"#endif \n"\
;
static const char g_sampling_opencl[]= \
"/********************************************************************** \n"\
"Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved. \n"\
" \n"\
"Permission is hereby granted, free of charge, to any person obtaining a copy \n"\
"of this software and associated documentation files (the \"Software\"), to deal \n"\
"in the Software without restriction, including without limitation the rights \n"\
"to use, copy, modify, merge, publish, distribute, sublicense, and/or sell \n"\
"copies of the Software, and to permit persons to whom the Software is \n"\
"furnished to do so, subject to the following conditions: \n"\
" \n"\
"The above copyright notice and this permission notice shall be included in \n"\
"all copies or substantial portions of the Software. \n"\
" \n"\
"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR \n"\
"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, \n"\
"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE \n"\
"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER \n"\
"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, \n"\
"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN \n"\
"THE SOFTWARE. \n"\
"********************************************************************/ \n"\
"#ifndef SAMPLING_CL \n"\
"#define SAMPLING_CL \n"\
" \n"\
" \n"\
"#define SAMPLE_DIMS_PER_BOUNCE 300 \n"\
"#define SAMPLE_DIM_CAMERA_OFFSET 1 \n"\
"#define SAMPLE_DIM_SURFACE_OFFSET 5 \n"\
"#define SAMPLE_DIM_VOLUME_APPLY_OFFSET 101 \n"\
"#define SAMPLE_DIM_VOLUME_EVALUATE_OFFSET 201 \n"\
"#define SAMPLE_DIM_IMG_PLANE_EVALUATE_OFFSET 401 \n"\
" \n"\
"typedef struct \n"\
"{ \n"\
"    uint seq; \n"\
"    uint s0; \n"\
"    uint s1; \n"\
"    uint s2; \n"\
"} SobolSampler; \n"\
" \n"\
"typedef struct _Sampler \n"\
"{ \n"\
"    uint index; \n"\
"    uint dimension; \n"\
"    uint scramble; \n"\
"    uint padding; \n"\
"} Sampler; \n"\
" \n"\
"#if SAMPLER == SOBOL \n"\
"#define SAMPLER_ARG_LIST __global uint const* sobol_mat \n"\
"#define SAMPLER_ARGS sobol_mat \n"\
"#elif SAMPLER == RANDOM \n"\
"#define SAMPLER_ARG_LIST int unused \n"\
"#define SAMPLER_ARGS 0 \n"\
"#elif SAMPLER == CMJ \n"\
"#define SAMPLER_ARG_LIST int unused \n"\
"#define SAMPLER_ARGS 0 \n"\
"#endif \n"\
" \n"\
"/** \n"\
"    Sobol sampler \n"\
"**/ \n"\
"#define MATSIZE 52 \n"\
" \n"\
"// The code is taken from: http://gruenschloss.org/sobol/kuo-2d-proj-single-precision.zip \n"\
"//  \n"\
"float SobolSampler_Sample1D(Sampler* sampler, __global uint const* mat) \n"\
"{ \n"\
"    uint result = sampler->scramble; \n"\
"    uint index = sampler->index; \n"\
"    for (uint i = sampler->dimension * MATSIZE; index;  index >>= 1, ++i) \n"\
"    { \n"\
"        if (index & 1) \n"\
"            result ^= mat[i]; \n"\
"    } \n"\
" \n"\
"    return result * (1.f / (1UL << 32)); \n"\
"} \n"\
" \n"\
"/** \n"\
"    Random sampler \n"\
"**/ \n"\
" \n"\
"/// Hash function \n"\
"uint WangHash(uint seed) \n"\
"{ \n"\
"    seed = (seed ^ 61) ^ (seed >> 16); \n"\
"    seed *= 9; \n"\
"    seed = seed ^ (seed >> 4); \n"\
"    seed *= 0x27d4eb2d; \n"\
"    seed = seed ^ (seed >> 15); \n"\
"    return seed; \n"\
"} \n"\
" \n"\
"/// Return random unsigned \n"\
"uint UniformSampler_SampleUint(Sampler* sampler) \n"\
"{ \n"\
"    sampler->index = WangHash(1664525U * sampler->index + 1013904223U); \n"\
"    return sampler->index; \n"\
"} \n"\
" \n"\
"/// Return random float \n"\
"float UniformSampler_Sample1D(Sampler* sampler) \n"\
"{ \n"\
"    return ((float)UniformSampler_SampleUint(sampler)) / 0xffffffffU; \n"\
"} \n"\
" \n"\
" \n"\
"/** \n"\
"    Correllated multi-jittered  \n"\
"**/ \n"\
" \n"\
"uint permute(uint i, uint l, uint p) \n"\
"{ \n"\
"    unsigned w = l - 1; \n"\
"    w |= w >> 1; \n"\
"    w |= w >> 2; \n"\
"    w |= w >> 4; \n"\
"    w |= w >> 8; \n"\
"    w |= w >> 16; \n"\
" \n"\
"    do \n"\
"    { \n"\
"        i ^= p; \n"\
"        i *= 0xe170893d; \n"\
"        i ^= p >> 16; \n"\
"        i ^= (i & w) >> 4; \n"\
"        i ^= p >> 8; \n"\
"        i *= 0x0929eb3f; \n"\
"        i ^= p >> 23; \n"\
"        i ^= (i & w) >> 1; \n"\
"        i *= 1 | p >> 27; \n"\
"        i *= 0x6935fa69; \n"\
"        i ^= (i & w) >> 11; \n"\
"        i *= 0x74dcb303; \n"\
"        i ^= (i & w) >> 2; \n"\
"        i *= 0x9e501cc3; \n"\
"        i ^= (i & w) >> 2; \n"\
"        i *= 0xc860a3df; \n"\
"        i &= w; \n"\
"        i ^= i >> 5; \n"\
"    } while (i >= l); \n"\
"    return (i + p) % l; \n"\
"} \n"\
" \n"\
"float randfloat(uint i, uint p) \n"\
"{ \n"\
"    i ^= p; \n"\
"    i ^= i >> 17; \n"\
"    i ^= i >> 10; \n"\
"    i *= 0xb36534e5; \n"\
"    i ^= i >> 12; \n"\
"    i ^= i >> 21; \n"\
"    i *= 0x93fc4795; \n"\
"    i ^= 0xdf6e307f; \n"\
"    i ^= i >> 17; \n"\
"    i *= 1 | p >> 18; \n"\
"    return i * (1.0f / 4294967808.0f); \n"\
"} \n"\
" \n"\
"float2 cmj(int s, int n, int p) \n"\
"{ \n"\
"    int sx = permute(s % n, n, p * 0xa511e9b3); \n"\
"    int sy = permute(s / n, n, p * 0x63d83595); \n"\
"    float jx = randfloat(s, p * 0xa399d265); \n"\
"    float jy = randfloat(s, p * 0x711ad6a5); \n"\
" \n"\
"    return make_float2((s % n + (sy + jx) / n) / n, \n"\
"        (s / n + (sx + jy) / n) / n); \n"\
"} \n"\
" \n"\
"float2 CmjSampler_Sample2D(Sampler* sampler) \n"\
"{ \n"\
"    int idx = permute(sampler->index, CMJ_DIM * CMJ_DIM, 0xa399d265 * sampler->dimension * sampler->scramble); \n"\
"    return cmj(idx, CMJ_DIM, sampler->dimension * sampler->scramble); \n"\
"} \n"\
" \n"\
"#if SAMPLER == SOBOL \n"\
"void Sampler_Init(Sampler* sampler, uint index, uint start_dimension, uint scramble) \n"\
"{ \n"\
"    sampler->index = index; \n"\
"    sampler->scramble = scramble; \n"\
"    sampler->dimension = start_dimension; \n"\
"} \n"\
"#elif SAMPLER == RANDOM \n"\
"void Sampler_Init(Sampler* sampler, uint seed) \n"\
"{ \n"\
"    sampler->index = seed; \n"\
"    sampler->scramble = 0; \n"\
"    sampler->dimension = 0; \n"\
"} \n"\
"#elif SAMPLER == CMJ \n"\
"void Sampler_Init(Sampler* sampler, uint index, uint dimension, uint scramble) \n"\
"{ \n"\
"    sampler->index = index; \n"\
"    sampler->scramble = scramble; \n"\
"    sampler->dimension = dimension; \n"\
"} \n"\
"#endif \n"\
" \n"\
" \n"\
"float2 Sampler_Sample2D(Sampler* sampler, SAMPLER_ARG_LIST) \n"\
"{ \n"\
"#if SAMPLER == SOBOL \n"\
"    float2 sample; \n"\
"    sample.x = SobolSampler_Sample1D(sampler, SAMPLER_ARGS); \n"\
"    ++(sampler->dimension); \n"\
"    sample.y = SobolSampler_Sample1D(sampler, SAMPLER_ARGS); \n"\
"    ++(sampler->dimension); \n"\
"    return sample; \n"\
"#elif SAMPLER == RANDOM \n"\
"    float2 sample; \n"\
"    sample.x = UniformSampler_Sample1D(sampler); \n"\
"    sample.y = UniformSampler_Sample1D(sampler); \n"\
"    return sample; \n"\
"#elif SAMPLER == CMJ \n"\
"    float2 sample; \n"\
"    sample = CmjSampler_Sample2D(sampler); \n"\
"    ++(sampler->dimension); \n"\
"    return sample; \n"\
"#endif \n"\
"} \n"\
" \n"\
"float Sampler_Sample1D(Sampler* sampler, SAMPLER_ARG_LIST) \n"\
"{ \n"\
"#if SAMPLER == SOBOL \n"\
"    float sample = SobolSampler_Sample1D(sampler, SAMPLER_ARGS); \n"\
"    ++(sampler->dimension); \n"\
"    return sample; \n"\
"#elif SAMPLER == RANDOM \n"\
"    return UniformSampler_Sample1D(sampler); \n"\
"#elif SAMPLER == CMJ \n"\
"    float2 sample; \n"\
"    sample = CmjSampler_Sample2D(sampler); \n"\
"    ++(sampler->dimension); \n"\
"    return sample.x; \n"\
"#endif \n"\
"} \n"\
" \n"\
"/// Sample hemisphere with cos weight \n"\
"float3 Sample_MapToHemisphere( \n"\
"                        // Sample \n"\
"                        float2 sample, \n"\
"                        // Hemisphere normal \n"\
"                        float3 n, \n"\
"                        // Cos power \n"\
"                        float e \n"\
"                        ) \n"\
"{ \n"\
"    // Construct basis \n"\
"    float3 u = GetOrthoVector(n); \n"\
"    float3 v = cross(u, n); \n"\
"    u = cross(n, v); \n"\
"     \n"\
"    // Calculate 2D sample \n"\
"    float r1 = sample.x; \n"\
"    float r2 = sample.y; \n"\
"     \n"\
"    // Transform to spherical coordinates \n"\
"    float sinpsi = sin(2*PI*r1); \n"\
"    float cospsi = cos(2*PI*r1); \n"\
"    float costheta = pow(1.f - r2, 1.f/(e + 1.f)); \n"\
"    float sintheta = sqrt(1.f - costheta * costheta); \n"\
"     \n"\
"    // Return the result \n"\
"    return normalize(u * sintheta * cospsi + v * sintheta * sinpsi + n * costheta); \n"\
"} \n"\
" \n"\
"float2 Sample_MapToDisk( \n"\
"    // Sample \n"\
"    float2 sample \n"\
"    ) \n"\
"{ \n"\
"    float r = native_sqrt(sample.x);  \n"\
"    float theta = 2 * PI * sample.y; \n"\
"    return make_float2(r * native_cos(theta), r * native_sin(theta)); \n"\
"} \n"\
" \n"\
"float2 Sample_MapToDiskConcentric( \n"\
"    // Sample \n"\
"    float2 sample \n"\
"    ) \n"\
"{ \n"\
"    float2 offset = 2.f * sample - make_float2(1.f, 1.f); \n"\
" \n"\
"    if (offset.x == 0 && offset.y == 0) return 0.f; \n"\
" \n"\
"    float theta, r; \n"\
" \n"\
"    if (fabs(offset.x) > fabs(offset.y))  \n"\
"    { \n"\
"        r = offset.x; \n"\
"        theta = PI / 4.f * (offset.y / offset.x); \n"\
"    } \n"\
"    else  \n"\
"    { \n"\
"        r = offset.y; \n"\
"        theta = PI / 2.f * ( 1.f - 0.5f * (offset.x / offset.y)); \n"\
"    } \n"\
"     \n"\
"    return make_float2(r * native_cos(theta), r * native_sin(theta)); \n"\
"} \n"\
" \n"\
"/// Sample hemisphere with cos weight \n"\
"float3 Sample_MapToSphere( \n"\
"                        // Sample \n"\
"                        float2 sample \n"\
"                        ) \n"\
"{ \n"\
"    float z = 1.f - 2.f * sample.x; \n"\
"    float r = native_sqrt(max(0.f, 1.f - z*z)); \n"\
"    float phi = 2.f * PI * sample.y; \n"\
"    float x = cos(phi); \n"\
"    float y = sin(phi); \n"\
"     \n"\
"    // Return the result \n"\
"    return make_float3(x,y,z); \n"\
"} \n"\
" \n"\
"float2 Sample_MapToPolygon(int n, float2 sample, float sample1) \n"\
"{ \n"\
"    float theta = 2.f * PI / n; \n"\
"    int edge = clamp((int)(sample1 * n), 0, n - 1); \n"\
"    float t = native_sqrt(sample.x); \n"\
"    float u = 1.f - t; \n"\
"    float v = t * sample.y; \n"\
"    float2 v1 = make_float2(native_cos(theta * edge), native_sin(theta * edge)); \n"\
"    float2 v2 = make_float2(native_cos(theta * (edge + 1)), native_sin(theta * (edge + 1))); \n"\
"    return u*v1 + v*v2;; \n"\
"} \n"\
" \n"\
"/// Power heuristic for multiple importance sampling \n"\
"float PowerHeuristic(int nf, float fpdf, int ng, float gpdf) \n"\
"{ \n"\
"    float f = nf * fpdf; \n"\
"    float g = ng * gpdf; \n"\
"    return (f*f) / (f*f + g*g); \n"\
"} \n"\
" \n"\
"/// Balance heuristic for multiple importance sampling \n"\
"float BalanceHeuristic(int nf, float fpdf, int ng, float gpdf) \n"\
"{ \n"\
"    float f = nf * fpdf; \n"\
"    float g = ng * gpdf; \n"\
"    return (f) / (f + g); \n"\
"} \n"\
" \n"\
"int lower_bound(GLOBAL float const* values, int n, float value) \n"\
"{ \n"\
"    int count = n; \n"\
"    int b = 0; \n"\
"    int it = 0; \n"\
"    int step = 0; \n"\
" \n"\
"    while (count > 0) \n"\
"    { \n"\
"        it = b; \n"\
"        step = count / 2; \n"\
"        it += step; \n"\
"        if (values[it] < value) \n"\
"        { \n"\
"            b = ++it; \n"\
"            count -= step + 1; \n"\
"        } \n"\
"        else \n"\
"        { \n"\
"            count = step; \n"\
"        } \n"\
"    } \n"\
" \n"\
"    return b; \n"\
"} \n"\
"/// Sample 1D distribution \n"\
"float Distribution1D_Sample(float s, GLOBAL int const* data, float* pdf) \n"\
"{ \n"\
"    int num_segments = data[0]; \n"\
" \n"\
"    GLOBAL float const* cdf_data = (GLOBAL float const*)&data[1]; \n"\
"    GLOBAL float const* pdf_data = cdf_data + num_segments + 1; \n"\
" \n"\
"    int segment_idx = max(lower_bound(cdf_data, num_segments + 1, s), 1); \n"\
" \n"\
"    // Find lerp coefficient \n"\
"    float du = (s - cdf_data[segment_idx - 1]) / (cdf_data[segment_idx] - cdf_data[segment_idx - 1]); \n"\
" \n"\
"    // Calc pdf \n"\
"    *pdf = pdf_data[segment_idx - 1]; \n"\
" \n"\
"    return (segment_idx - 1 + du) / num_segments;; \n"\
"} \n"\
" \n"\
"/// Sample 1D distribution \n"\
"int Distribution1D_SampleDiscrete(float s, GLOBAL int const* data, float* pdf) \n"\
"{ \n"\
"    int num_segments = data[0]; \n"\
" \n"\
"    GLOBAL float const* cdf_data = (GLOBAL float const*)&data[1]; \n"\
"    GLOBAL float const* pdf_data = cdf_data + num_segments + 1; \n"\
" \n"\
"    int segment_idx = max(lower_bound(cdf_data, num_segments + 1, s), 1); \n"\
" \n"\
"    // Find lerp coefficient \n"\
"    float du = (s - cdf_data[segment_idx - 1]) / (cdf_data[segment_idx] - cdf_data[segment_idx - 1]); \n"\
" \n"\
"    // Calc pdf \n"\
"    *pdf = pdf_data[segment_idx - 1] / num_segments; \n"\
" \n"\
"    return segment_idx - 1; \n"\
"} \n"\
" \n"\
"/// PDF of  1D distribution \n"\
"float Distribution1D_GetPdf(float s, GLOBAL int const* data) \n"\
"{ \n"\
"    int num_segments = data[0]; \n"\
"    GLOBAL float const* cdf_data = (GLOBAL float const*)&data[1]; \n"\
"    GLOBAL float const* pdf_data = cdf_data + num_segments + 1; \n"\
" \n"\
"    int segment_idx = max(lower_bound(cdf_data, num_segments + 1, s), 1); \n"\
" \n"\
"    // Calc pdf \n"\
"    return pdf_data[segment_idx - 1]; \n"\
"} \n"\
" \n"\
"/// PDF of  1D distribution \n"\
"float Distribution1D_GetPdfDiscreet(int d, GLOBAL int const* data) \n"\
"{ \n"\
"    int num_segments = data[0]; \n"\
"    GLOBAL float const* cdf_data = (GLOBAL float const*)&data[1]; \n"\
"    GLOBAL float const* pdf_data = cdf_data + num_segments + 1; \n"\
" \n"\
"    // Calc pdf \n"\
"    return pdf_data[d] / num_segments; \n"\
"} \n"\
" \n"\
" \n"\
" \n"\
"#endif // SAMPLING_CL \n"\
;
static const char g_scene_opencl[]= \
"/********************************************************************** \n"\
"Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved. \n"\
" \n"\
"Permission is hereby granted, free of charge, to any person obtaining a copy \n"\
"of this software and associated documentation files (the \"Software\"), to deal \n"\
"in the Software without restriction, including without limitation the rights \n"\
"to use, copy, modify, merge, publish, distribute, sublicense, and/or sell \n"\
"copies of the Software, and to permit persons to whom the Software is \n"\
"furnished to do so, subject to the following conditions: \n"\
" \n"\
"The above copyright notice and this permission notice shall be included in \n"\
"all copies or substantial portions of the Software. \n"\
" \n"\
"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR \n"\
"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, \n"\
"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE \n"\
"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER \n"\
"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, \n"\
"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN \n"\
"THE SOFTWARE. \n"\
"********************************************************************/ \n"\
"#ifndef SCENE_CL \n"\
"#define SCENE_CL \n"\
" \n"\
" \n"\
"typedef struct \n"\
"{ \n"\
"    // Vertices \n"\
"    GLOBAL float3 const* restrict vertices; \n"\
"    // Normals \n"\
"    GLOBAL float3 const* restrict normals; \n"\
"    // UVs \n"\
"    GLOBAL float2 const* restrict uvs; \n"\
"    // Indices \n"\
"    GLOBAL int const* restrict indices; \n"\
"    // Shapes \n"\
"    GLOBAL Shape const* restrict shapes; \n"\
"    // Materials \n"\
"    GLOBAL Material const* restrict materials; \n"\
"    // Emissive objects \n"\
"    GLOBAL Light const* restrict lights; \n"\
"    // Envmap idx \n"\
"    int env_light_idx; \n"\
"    // Number of emissive objects \n"\
"    int num_lights; \n"\
"    // Light distribution  \n"\
"    GLOBAL int const* restrict light_distribution; \n"\
"} Scene; \n"\
" \n"\
"// Get triangle vertices given scene, shape index and prim index \n"\
"INLINE void Scene_GetTriangleVertices(Scene const* scene, int shape_idx, int prim_idx, float3* v0, float3* v1, float3* v2) \n"\
"{ \n"\
"    // Extract shape data \n"\
"    Shape shape = scene->shapes[shape_idx]; \n"\
" \n"\
"    // Fetch indices starting from startidx and offset by prim_idx \n"\
"    int i0 = scene->indices[shape.startidx + 3 * prim_idx]; \n"\
"    int i1 = scene->indices[shape.startidx + 3 * prim_idx + 1]; \n"\
"    int i2 = scene->indices[shape.startidx + 3 * prim_idx + 2]; \n"\
" \n"\
"    // Fetch positions and transform to world space \n"\
"    *v0 = matrix_mul_point3(shape.transform, scene->vertices[shape.startvtx + i0]); \n"\
"    *v1 = matrix_mul_point3(shape.transform, scene->vertices[shape.startvtx + i1]); \n"\
"    *v2 = matrix_mul_point3(shape.transform, scene->vertices[shape.startvtx + i2]); \n"\
"} \n"\
" \n"\
"// Get triangle uvs given scene, shape index and prim index \n"\
"INLINE void Scene_GetTriangleUVs(Scene const* scene, int shape_idx, int prim_idx, float2* uv0, float2* uv1, float2* uv2) \n"\
"{ \n"\
"    // Extract shape data \n"\
"    Shape shape = scene->shapes[shape_idx]; \n"\
" \n"\
"    // Fetch indices starting from startidx and offset by prim_idx \n"\
"    int i0 = scene->indices[shape.startidx + 3 * prim_idx]; \n"\
"    int i1 = scene->indices[shape.startidx + 3 * prim_idx + 1]; \n"\
"    int i2 = scene->indices[shape.startidx + 3 * prim_idx + 2]; \n"\
" \n"\
"    // Fetch positions and transform to world space \n"\
"    *uv0 = scene->uvs[shape.startvtx + i0]; \n"\
"    *uv1 = scene->uvs[shape.startvtx + i1]; \n"\
"    *uv2 = scene->uvs[shape.startvtx + i2]; \n"\
"} \n"\
" \n"\
" \n"\
"// Interpolate position, normal and uv \n"\
"INLINE void Scene_InterpolateAttributes(Scene const* scene, int shape_idx, int prim_idx, float2 barycentrics, float3* p, float3* n, float2* uv, float* area) \n"\
"{ \n"\
"    // Extract shape data \n"\
"    Shape shape = scene->shapes[shape_idx]; \n"\
" \n"\
"    // Fetch indices starting from startidx and offset by prim_idx \n"\
"    int i0 = scene->indices[shape.startidx + 3 * prim_idx]; \n"\
"    int i1 = scene->indices[shape.startidx + 3 * prim_idx + 1]; \n"\
"    int i2 = scene->indices[shape.startidx + 3 * prim_idx + 2]; \n"\
" \n"\
"    // Fetch normals \n"\
"    float3 n0 = scene->normals[shape.startvtx + i0]; \n"\
"    float3 n1 = scene->normals[shape.startvtx + i1]; \n"\
"    float3 n2 = scene->normals[shape.startvtx + i2]; \n"\
" \n"\
"    // Fetch positions and transform to world space \n"\
"    float3 v0 = matrix_mul_point3(shape.transform, scene->vertices[shape.startvtx + i0]); \n"\
"    float3 v1 = matrix_mul_point3(shape.transform, scene->vertices[shape.startvtx + i1]); \n"\
"    float3 v2 = matrix_mul_point3(shape.transform, scene->vertices[shape.startvtx + i2]); \n"\
" \n"\
"    // Fetch UVs \n"\
"    float2 uv0 = scene->uvs[shape.startvtx + i0]; \n"\
"    float2 uv1 = scene->uvs[shape.startvtx + i1]; \n"\
"    float2 uv2 = scene->uvs[shape.startvtx + i2]; \n"\
" \n"\
"    // Calculate barycentric position and normal \n"\
"    *p = (1.f - barycentrics.x - barycentrics.y) * v0 + barycentrics.x * v1 + barycentrics.y * v2; \n"\
"    *n = normalize(matrix_mul_vector3(shape.transform, (1.f - barycentrics.x - barycentrics.y) * n0 + barycentrics.x * n1 + barycentrics.y * n2)); \n"\
"    *uv = (1.f - barycentrics.x - barycentrics.y) * uv0 + barycentrics.x * uv1 + barycentrics.y * uv2; \n"\
"    *area = 0.5f * length(cross(v2 - v0, v1 - v0)); \n"\
"} \n"\
" \n"\
"// Get material index of a shape face \n"\
"INLINE int Scene_GetMaterialIndex(Scene const* scene, int shape_idx, int prim_idx) \n"\
"{ \n"\
"    Shape shape = scene->shapes[shape_idx]; \n"\
"    return shape.material_idx; \n"\
"} \n"\
" \n"\
"INLINE int Scene_GetVolumeIndex(Scene const* scene, int shape_idx) \n"\
"{ \n"\
"    Shape shape = scene->shapes[shape_idx]; \n"\
"    return shape.volume_idx; \n"\
"} \n"\
" \n"\
"/// Fill DifferentialGeometry structure based on intersection info from RadeonRays \n"\
"void Scene_FillDifferentialGeometry(// Scene \n"\
"                              Scene const* scene, \n"\
"                              // RadeonRays intersection \n"\
"                              Intersection const* isect, \n"\
"                              // Differential geometry \n"\
"                              DifferentialGeometry* diffgeo \n"\
"                              ) \n"\
"{ \n"\
"    // Determine shape and polygon \n"\
"    int shape_idx = isect->shapeid - 1; \n"\
"    int prim_idx = isect->primid; \n"\
" \n"\
"    // Get barycentrics \n"\
"    float2 barycentrics = isect->uvwt.xy; \n"\
" \n"\
"    // Extract shape data \n"\
"    Shape shape = scene->shapes[shape_idx]; \n"\
" \n"\
"    // Interpolate attributes \n"\
"    float3 p; \n"\
"    float3 n; \n"\
"    float2 uv; \n"\
"    float area; \n"\
"    Scene_InterpolateAttributes(scene, shape_idx, prim_idx, barycentrics, &p, &n, &uv, &area); \n"\
"    // Triangle area (for area lighting) \n"\
"    diffgeo->area = area; \n"\
" \n"\
"    // Calculate barycentric position and normal \n"\
"    diffgeo->n = n; \n"\
"    diffgeo->p = p; \n"\
"    diffgeo->uv = uv; \n"\
" \n"\
"    // Get vertices \n"\
"    float3 v0, v1, v2; \n"\
"    Scene_GetTriangleVertices(scene, shape_idx, prim_idx, &v0, &v1, &v2); \n"\
" \n"\
"    // Calculate true normal \n"\
"    diffgeo->ng = normalize(cross(v1 - v0, v2 - v0)); \n"\
" \n"\
"    // Get material at shading point \n"\
"    int material_idx = Scene_GetMaterialIndex(scene, shape_idx, prim_idx); \n"\
"    diffgeo->mat = scene->materials[material_idx]; \n"\
" \n"\
"    // Get UVs \n"\
"    float2 uv0, uv1, uv2; \n"\
"    Scene_GetTriangleUVs(scene, shape_idx, prim_idx, &uv0, &uv1, &uv2); \n"\
" \n"\
"    // Reverse geometric normal if shading normal points to different side \n"\
"    if (dot(diffgeo->ng, diffgeo->n) < 0.f) \n"\
"    { \n"\
"        diffgeo->ng = -diffgeo->ng; \n"\
"    } \n"\
" \n"\
"    /// Calculate tangent basis \n"\
"    /// From PBRT book \n"\
"    float du1 = uv0.x - uv2.x; \n"\
"    float du2 = uv1.x - uv2.x; \n"\
"    float dv1 = uv0.y - uv2.y; \n"\
"    float dv2 = uv1.y - uv2.y; \n"\
"    float3 dp1 = v0 - v2; \n"\
"    float3 dp2 = v1 - v2; \n"\
"    float det = du1 * dv2 - dv1 * du2; \n"\
" \n"\
"    if (0 && det != 0.f) \n"\
"    { \n"\
"        float invdet = 1.f / det; \n"\
"        diffgeo->dpdu = normalize( (dv2 * dp1 - dv1 * dp2) * invdet ); \n"\
"        diffgeo->dpdv = normalize( (-du2 * dp1 + du1 * dp2) * invdet ); \n"\
"        diffgeo->dpdu -= dot(diffgeo->n, diffgeo->dpdu) * diffgeo->n; \n"\
"        diffgeo->dpdu = normalize(diffgeo->dpdu); \n"\
"         \n"\
"        diffgeo->dpdv -= dot(diffgeo->n, diffgeo->dpdv) * diffgeo->n; \n"\
"        diffgeo->dpdv -= dot(diffgeo->dpdu, diffgeo->dpdv) * diffgeo->dpdu; \n"\
"        diffgeo->dpdv = normalize(diffgeo->dpdv); \n"\
"    } \n"\
"    else \n"\
"    { \n"\
"        diffgeo->dpdu = normalize(GetOrthoVector(diffgeo->n)); \n"\
"        diffgeo->dpdv = normalize(cross(diffgeo->n, diffgeo->dpdu)); \n"\
"    } \n"\
" \n"\
"    diffgeo->material_index = material_idx; \n"\
"} \n"\
" \n"\
" \n"\
"// Calculate tangent transform matrices inside differential geometry \n"\
"INLINE void DifferentialGeometry_CalculateTangentTransforms(DifferentialGeometry* diffgeo) \n"\
"{ \n"\
"    diffgeo->world_to_tangent = matrix_from_rows3(diffgeo->dpdu, diffgeo->n, diffgeo->dpdv); \n"\
" \n"\
"    diffgeo->world_to_tangent.m0.w = -dot(diffgeo->dpdu, diffgeo->p); \n"\
"    diffgeo->world_to_tangent.m1.w = -dot(diffgeo->n, diffgeo->p); \n"\
"    diffgeo->world_to_tangent.m2.w = -dot(diffgeo->dpdv, diffgeo->p); \n"\
" \n"\
"    diffgeo->tangent_to_world = matrix_from_cols3(diffgeo->world_to_tangent.m0.xyz,  \n"\
"        diffgeo->world_to_tangent.m1.xyz, diffgeo->world_to_tangent.m2.xyz); \n"\
" \n"\
"    diffgeo->tangent_to_world.m0.w = diffgeo->p.x; \n"\
"    diffgeo->tangent_to_world.m1.w = diffgeo->p.y; \n"\
"    diffgeo->tangent_to_world.m2.w = diffgeo->p.z; \n"\
"} \n"\
" \n"\
"#define POWER_SAMPLING \n"\
" \n"\
"// Sample light index \n"\
"INLINE int Scene_SampleLight(Scene const* scene, float sample, float* pdf) \n"\
"{ \n"\
"#ifndef POWER_SAMPLING \n"\
"    int num_lights = scene->num_lights; \n"\
"    int light_idx = clamp((int)(sample * num_lights), 0, num_lights - 1); \n"\
"    *pdf = 1.f / num_lights; \n"\
"    return light_idx; \n"\
"#else \n"\
"    int num_lights = scene->num_lights; \n"\
"    int light_idx = Distribution1D_SampleDiscrete(sample, scene->light_distribution, pdf); \n"\
"    return light_idx; \n"\
"#endif \n"\
"} \n"\
" \n"\
"#endif \n"\
;
static const char g_sh_opencl[]= \
"/********************************************************************** \n"\
"Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved. \n"\
" \n"\
"Permission is hereby granted, free of charge, to any person obtaining a copy \n"\
"of this software and associated documentation files (the \"Software\"), to deal \n"\
"in the Software without restriction, including without limitation the rights \n"\
"to use, copy, modify, merge, publish, distribute, sublicense, and/or sell \n"\
"copies of the Software, and to permit persons to whom the Software is \n"\
"furnished to do so, subject to the following conditions: \n"\
" \n"\
"The above copyright notice and this permission notice shall be included in \n"\
"all copies or substantial portions of the Software. \n"\
" \n"\
"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR \n"\
"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, \n"\
"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE \n"\
"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER \n"\
"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, \n"\
"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN \n"\
"THE SOFTWARE. \n"\
"********************************************************************/ \n"\
" \n"\
"#define MAX_BAND 2 \n"\
"#define PI 3.14159265358979323846f \n"\
" \n"\
"enum TextureFormat \n"\
"{ \n"\
"    UNKNOWN, \n"\
"    RGBA8, \n"\
"    RGBA16, \n"\
"    RGBA32 \n"\
"}; \n"\
" \n"\
"// Texture description \n"\
"typedef struct _Texture \n"\
"{ \n"\
"    // Texture width, height and depth \n"\
"    int w; \n"\
"    int h; \n"\
"    int d; \n"\
"    // Offset in texture data array \n"\
"    int dataoffset; \n"\
"    int fmt; \n"\
"    int extra; \n"\
"} Texture; \n"\
" \n"\
" \n"\
"float3 make_float3(float x, float y, float z) \n"\
"{ \n"\
"    float3 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    res.z = z; \n"\
"    return res; \n"\
"} \n"\
" \n"\
" \n"\
"void CartesianToSpherical ( float3 cart, float* r, float* phi, float* theta ) \n"\
"{ \n"\
"    float temp = atan2(cart.x, cart.z); \n"\
"    *r = sqrt(cart.x*cart.x + cart.y*cart.y + cart.z*cart.z); \n"\
"    *phi = (float)((temp >= 0)?temp:(temp + 2*PI)); \n"\
"    *theta = acos(cart.y/ *r); \n"\
"} \n"\
" \n"\
" \n"\
"/// Sample 2D texture described by texture in texturedata pool \n"\
"float3 Sample2D(Texture const* texture, __global char const* texturedata, float2 uv) \n"\
"{ \n"\
"    // Get width and height \n"\
"    int width = texture->w; \n"\
"    int height = texture->h; \n"\
" \n"\
"    // Find the origin of the data in the pool \n"\
"    __global char const* mydata = texturedata + texture->dataoffset; \n"\
" \n"\
"    // Handle wrap \n"\
"    uv -= floor(uv); \n"\
" \n"\
"    // Reverse Y \n"\
"    // it is needed as textures are loaded with Y axis going top to down \n"\
"    // and our axis goes from down to top \n"\
"    uv.y = 1.f - uv.y; \n"\
" \n"\
"    // Figure out integer coordinates \n"\
"    int x = floor(uv.x * width); \n"\
"    int y = floor(uv.y * height); \n"\
" \n"\
"    // Calculate samples for linear filtering \n"\
"    int x1 = min(x + 1, width - 1); \n"\
"    int y1 = min(y + 1, height - 1); \n"\
" \n"\
"    // Calculate weights for linear filtering \n"\
"    float wx = uv.x * width - floor(uv.x * width); \n"\
"    float wy = uv.y * height - floor(uv.y * height); \n"\
" \n"\
"    if (texture->fmt == RGBA32) \n"\
"    { \n"\
"        __global float3 const* mydataf = (__global float3 const*)mydata; \n"\
" \n"\
"        // Get 4 values \n"\
"        float3 valx = *(mydataf + width * y + x); \n"\
" \n"\
"        // Filter and return the result \n"\
"        return valx; \n"\
"    } \n"\
"    else if (texture->fmt == RGBA16) \n"\
"    { \n"\
"        __global half const* mydatah = (__global half const*)mydata; \n"\
" \n"\
"        float valx = vload_half(0, mydatah + 4*(width * y + x)); \n"\
"        float valy = vload_half(0, mydatah + 4*(width * y + x) + 1); \n"\
"        float valz = vload_half(0, mydatah + 4*(width * y + x) + 2); \n"\
" \n"\
"        return make_float3(valx, valy, valz); \n"\
"    } \n"\
"    else \n"\
"    { \n"\
"        __global uchar4 const* mydatac = (__global uchar4 const*)mydata; \n"\
" \n"\
"        // Get 4 values \n"\
"        uchar4 valx = *(mydatac + width * y + x); \n"\
" \n"\
"        float3 valxf = make_float3((float)valx.x / 255.f, (float)valx.y / 255.f, (float)valx.z / 255.f); \n"\
" \n"\
"        // Filter and return the result \n"\
"        return valxf; \n"\
"    } \n"\
"} \n"\
" \n"\
" \n"\
"void ShEvaluate(float3 p, float* coeffs)  \n"\
"{ \n"\
"                     float fC0, fC1, fS0, fS1, fTmpA, fTmpB, fTmpC; \n"\
"                     float pz2 = p.z * p.z; \n"\
"                     coeffs[0] = 0.2820947917738781f; \n"\
"                     coeffs[2] = 0.4886025119029199f * p.z; \n"\
"                     coeffs[6] = 0.9461746957575601f * pz2 + -0.3153915652525201f; \n"\
"                     fC0 = p.x; \n"\
"                     fS0 = p.y; \n"\
"                     fTmpA = -0.48860251190292f; \n"\
"                     coeffs[3] = fTmpA * fC0; \n"\
"                     coeffs[1] = fTmpA * fS0; \n"\
"                     fTmpB = -1.092548430592079f * p.z; \n"\
"                     coeffs[7] = fTmpB * fC0; \n"\
"                     coeffs[5] = fTmpB * fS0; \n"\
"                     fC1 = p.x*fC0 - p.y*fS0; \n"\
"                     fS1 = p.x*fS0 + p.y*fC0; \n"\
"                     fTmpC = 0.5462742152960395f; \n"\
"                     coeffs[8] = fTmpC * fC1; \n"\
"                     coeffs[4] = fTmpC * fS1; \n"\
"} \n"\
" \n"\
"__attribute__((reqd_work_group_size(8, 8, 1))) \n"\
"///< Project the function represented by lat-long map lmmap to Sh up to lmax band \n"\
"__kernel void ShProject( \n"\
"    __global Texture const* textures, \n"\
"    // Texture data \n"\
"    __global char const* texturedata, \n"\
"    // Environment texture index \n"\
"    int envmapidx, \n"\
"    // Harmonic coefficients flattened: NumShTerms(lmax) * num_groups \n"\
"    __global float3* coeffs \n"\
"    ) \n"\
"{ \n"\
"    // Temporary work area for trigonom results \n"\
"    __local float3 cx[64]; \n"\
" \n"\
"    int x = get_global_id(0); \n"\
"    int y = get_global_id(1); \n"\
" \n"\
"    int xl = get_local_id(0); \n"\
"    int yl = get_local_id(1); \n"\
"    int wl = get_local_size(0); \n"\
" \n"\
"    int ngx = get_num_groups(0); \n"\
"    int gx = get_group_id(0); \n"\
"    int gy = get_group_id(1); \n"\
"    int g = gy * ngx + gx; \n"\
" \n"\
"    int lid = yl * wl + xl; \n"\
" \n"\
"    Texture envmap = textures[envmapidx]; \n"\
"    int w = envmap.w; \n"\
"    int h = envmap.h; \n"\
" \n"\
"    if (x < w && y < h) \n"\
"    { \n"\
"        // Calculate spherical angles \n"\
"        float thetastep = PI / h; \n"\
"        float phistep = 2.f*PI / w; \n"\
"        float theta0 = 0;//PI / h / 2.f; \n"\
"        float phi0 = 0;//2.f*PI / w / 2.f; \n"\
" \n"\
"        float phi = phi0 + x * phistep; \n"\
"        float theta = theta0 + y * thetastep; \n"\
" \n"\
"        float2 uv; \n"\
"        uv.x = (float)x/w; \n"\
"        uv.y = 1.f - (float)y/h; \n"\
"        float3 le = 3.f * Sample2D(&envmap, texturedata, uv); \n"\
" \n"\
"        float sinphi = sin(phi); \n"\
"        float cosphi = cos(phi); \n"\
"        float costheta = cos(theta); \n"\
"        float sintheta = sin(theta); \n"\
" \n"\
"        // Construct point on unit sphere \n"\
"        float3 p = normalize(make_float3(sintheta * cosphi, costheta, sintheta * sinphi)); \n"\
" \n"\
"        // Evaluate SH functions at w up to lmax band \n"\
"        float ylm[9]; \n"\
"        ShEvaluate(p, ylm); \n"\
" \n"\
"        // Evaluate Riemann sum \n"\
"        for (int i = 0; i < 9; ++i) \n"\
"        { \n"\
"            // Calculate the coefficient into local memory \n"\
"             cx[lid] = le * ylm[i] * sintheta * (PI / h) * (2.f * PI / w); \n"\
" \n"\
"             barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"             // Reduce the coefficient to get the resulting one \n"\
"             for (int stride = 1; stride <= (64 >> 1); stride <<= 1) \n"\
"             { \n"\
"                 if (lid < 64/(2*stride)) \n"\
"                 { \n"\
"                     cx[2*(lid + 1)*stride-1] = cx[2*(lid + 1)*stride-1] + cx[(2*lid + 1)*stride-1]; \n"\
"                 } \n"\
" \n"\
"                 barrier(CLK_LOCAL_MEM_FENCE); \n"\
"             } \n"\
" \n"\
"             // Put the coefficient into global memory \n"\
"             if (lid == 0) \n"\
"             { \n"\
"                coeffs[g * 9 + i] = cx[63]; \n"\
"             } \n"\
" \n"\
"             barrier(CLK_LOCAL_MEM_FENCE); \n"\
"        } \n"\
"    } \n"\
"} \n"\
" \n"\
"__attribute__((reqd_work_group_size(8, 8, 1))) \n"\
"///< Project the function represented by lat-long map lmmap to Sh up to lmax band \n"\
"__kernel void ShProjectHemisphericalProbe( \n"\
"    __global Texture const* textures, \n"\
"    // Texture data \n"\
"    __global char const* texturedata, \n"\
"    // Environment texture index \n"\
"    int envmapidx, \n"\
"    // Harmonic coefficients flattened: NumShTerms(lmax) * num_groups \n"\
"    __global float3* coeffs \n"\
"    ) \n"\
"{ \n"\
"    // Temporary work area for trigonom results \n"\
"    __local float3 cx[64]; \n"\
" \n"\
"    int x = get_global_id(0); \n"\
"    int y = get_global_id(1); \n"\
" \n"\
"    int xl = get_local_id(0); \n"\
"    int yl = get_local_id(1); \n"\
"    int wl = get_local_size(0); \n"\
" \n"\
"    int ngx = get_num_groups(0); \n"\
"    int gx = get_group_id(0); \n"\
"    int gy = get_group_id(1); \n"\
"    int g = gy * ngx + gx; \n"\
" \n"\
"    int lid = yl * wl + xl; \n"\
" \n"\
"    Texture envmap = textures[envmapidx]; \n"\
"    int w = envmap.w; \n"\
"    int h = envmap.h; \n"\
" \n"\
"    if (x < w && y < h) \n"\
"    { \n"\
"        // Calculate spherical angles \n"\
"        float thetastep = PI / h; \n"\
"        float phistep = 2.f*PI / w; \n"\
"        float theta0 = 0;//PI / h / 2.f; \n"\
"        float phi0 = 0;//2.f*PI / w / 2.f; \n"\
" \n"\
"        float phi = phi0 + x * phistep; \n"\
"        float theta = theta0 + y * thetastep; \n"\
" \n"\
"        float sinphi = sin(phi); \n"\
"        float cosphi = cos(phi); \n"\
"        float costheta = cos(theta); \n"\
"        float sintheta = sin(theta); \n"\
" \n"\
"        // Construct point on unit sphere \n"\
"        float3 p = normalize(make_float3(sintheta * cosphi, costheta, sintheta * sinphi)); \n"\
" \n"\
"        float envmapaspect = (float)envmap.h / envmap.w; \n"\
"        float2 uv = p.xz; \n"\
"        uv.y = 0.5f*uv.y + 0.5f; \n"\
"        uv.x = 0.5f*uv.x + 0.5f; \n"\
"        uv.x = (1.f - envmapaspect) * 0.5f + uv.x * envmapaspect; \n"\
" \n"\
"        //uv.x = (float)x/w; \n"\
"        //uv.y = 1.f - (float)y/h; \n"\
"        float3 le = Sample2D(&envmap, texturedata, uv); \n"\
" \n"\
"        // Evaluate SH functions at w up to lmax band \n"\
"        float ylm[9]; \n"\
"        ShEvaluate(p, ylm); \n"\
" \n"\
"        // Evaluate Riemann sum \n"\
"        for (int i = 0; i < 9; ++i) \n"\
"        { \n"\
"            // Calculate the coefficient into local memory \n"\
"             cx[lid] = le * ylm[i] * sintheta * (PI / h) * (2.f * PI / w); \n"\
" \n"\
"             barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"             // Reduce the coefficient to get the resulting one \n"\
"             for (int stride = 1; stride <= (64 >> 1); stride <<= 1) \n"\
"             { \n"\
"                 if (lid < 64/(2*stride)) \n"\
"                 { \n"\
"                     cx[2*(lid + 1)*stride-1] = cx[2*(lid + 1)*stride-1] + cx[(2*lid + 1)*stride-1]; \n"\
"                 } \n"\
" \n"\
"                 barrier(CLK_LOCAL_MEM_FENCE); \n"\
"             } \n"\
" \n"\
"             // Put the coefficient into global memory \n"\
"             if (lid == 0) \n"\
"             { \n"\
"                coeffs[g * 9 + i] = cx[63]; \n"\
"             } \n"\
" \n"\
"             barrier(CLK_LOCAL_MEM_FENCE); \n"\
"        } \n"\
"    } \n"\
"} \n"\
" \n"\
" \n"\
"#define GROUP_SIZE 256 \n"\
"__attribute__((reqd_work_group_size(GROUP_SIZE, 1, 1))) \n"\
"__kernel void ShReduce( \n"\
"    // Harmonic coefficients flattened: NumShTerms(lmax) * num_groups \n"\
"    const __global float3* coeffs, \n"\
"    // Number of sets \n"\
"    int numsets, \n"\
"    // Resulting coeffs \n"\
"    __global float3* result \n"\
"    ) \n"\
"{ \n"\
"    __local float3 lds[GROUP_SIZE]; \n"\
" \n"\
"    int gid = get_global_id(0); \n"\
"    int lid = get_global_id(0); \n"\
" \n"\
"    // How many items to reduce for a single work item \n"\
"    int numprivate = numsets / GROUP_SIZE; \n"\
" \n"\
"    for (int i=0;i<9;++i) \n"\
"    { \n"\
"        float3 res = {0,0,0}; \n"\
" \n"\
"        // Private reduction \n"\
"        for (int j=0;j<numprivate;++j) \n"\
"        { \n"\
"            res += coeffs[gid * numprivate * 9 + j * 9 + i]; \n"\
"        } \n"\
" \n"\
"        // LDS reduction \n"\
"        lds[lid] = res; \n"\
" \n"\
"        barrier (CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"        // Work group reduction \n"\
"        for (int stride = 1; stride <= (256 >> 1); stride <<= 1) \n"\
"        { \n"\
"            if (lid < GROUP_SIZE/(2*stride)) \n"\
"            { \n"\
"                lds[2*(lid + 1)*stride-1] = lds[2*(lid + 1)*stride-1] + lds[(2*lid + 1)*stride-1]; \n"\
"            } \n"\
" \n"\
"            barrier (CLK_LOCAL_MEM_FENCE); \n"\
"        } \n"\
" \n"\
"        // Write final result \n"\
"        if (lid == 0) \n"\
"        { \n"\
"            result[i] = lds[GROUP_SIZE-1]; \n"\
"        } \n"\
" \n"\
"        barrier (CLK_LOCAL_MEM_FENCE); \n"\
"    } \n"\
"} \n"\
"#undef GROUP_SIZE \n"\
" \n"\
" \n"\
"__attribute__((reqd_work_group_size(8, 8, 1))) \n"\
"__kernel void ShReconstructLmmap( \n"\
"    // SH coefficients: NumShTerms(lmax) items \n"\
"    const __global float3* coeffs, \n"\
"    // Resulting image width \n"\
"    int w, \n"\
"    // Resulting image height \n"\
"    int h, \n"\
"    // Resulting image \n"\
"    __global float3* lmmap \n"\
"    ) \n"\
"{ \n"\
"    int x = get_global_id(0); \n"\
"    int y = get_global_id(1); \n"\
" \n"\
"    int xl = get_local_id(0); \n"\
"    int yl = get_local_id(1); \n"\
"    int wl = get_local_size(0); \n"\
" \n"\
"    int ngx = get_num_groups(0); \n"\
"    int gx = get_group_id(0); \n"\
"    int gy = get_group_id(1); \n"\
" \n"\
"    if (x < w && y < h) \n"\
"    { \n"\
"        // Calculate spherical angles \n"\
"        float thetastep = M_PI / h; \n"\
"        float phistep = 2.f*M_PI / w; \n"\
"        float theta0 = 0;//M_PI / h / 2.f; \n"\
"        float phi0 = 0;//2.f*M_PI / w / 2.f; \n"\
" \n"\
"        float phi = phi0 + x * phistep; \n"\
"        float theta = theta0 + y * thetastep; \n"\
" \n"\
"        float2 uv; \n"\
"        uv.x = (float)x/w; \n"\
"        uv.y = (float)y/h; \n"\
" \n"\
"        float sinphi = sin(phi); \n"\
"        float cosphi = cos(phi); \n"\
"        float costheta = cos(theta); \n"\
"        float sintheta = sin(theta); \n"\
" \n"\
"        // Construct point on unit sphere \n"\
"        float3 p = normalize(make_float3(sintheta * cosphi, costheta, sintheta * sinphi)); \n"\
" \n"\
"        // Evaluate SH functions at w up to lmax band \n"\
"        float ylm[9]; \n"\
"        ShEvaluate(p, ylm); \n"\
" \n"\
"        // Evaluate Riemann sum \n"\
"        float3 val = {0, 0, 0}; \n"\
"        for (int i = 0; i < 9; ++i) \n"\
"        { \n"\
"            // Calculate the coefficient into local memory \n"\
"             val += ylm[i] * coeffs[i]; \n"\
"        } \n"\
" \n"\
"        int x = floor(uv.x * w); \n"\
"        int y = floor(uv.y * h); \n"\
" \n"\
"        lmmap[w * y + x] = val; \n"\
"    } \n"\
"} \n"\
;
static const char g_texture_opencl[]= \
"/********************************************************************** \n"\
"Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved. \n"\
" \n"\
"Permission is hereby granted, free of charge, to any person obtaining a copy \n"\
"of this software and associated documentation files (the \"Software\"), to deal \n"\
"in the Software without restriction, including without limitation the rights \n"\
"to use, copy, modify, merge, publish, distribute, sublicense, and/or sell \n"\
"copies of the Software, and to permit persons to whom the Software is \n"\
"furnished to do so, subject to the following conditions: \n"\
" \n"\
"The above copyright notice and this permission notice shall be included in \n"\
"all copies or substantial portions of the Software. \n"\
" \n"\
"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR \n"\
"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, \n"\
"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE \n"\
"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER \n"\
"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, \n"\
"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN \n"\
"THE SOFTWARE. \n"\
"********************************************************************/ \n"\
"#ifndef TEXTURE_CL \n"\
"#define TEXTURE_CL \n"\
" \n"\
" \n"\
" \n"\
" \n"\
"/// To simplify a bit \n"\
"#define TEXTURE_ARG_LIST __global Texture const* textures, __global char const* texturedata \n"\
"#define TEXTURE_ARG_LIST_IDX(x) int x, __global Texture const* textures, __global char const* texturedata \n"\
"#define TEXTURE_ARGS textures, texturedata \n"\
"#define TEXTURE_ARGS_IDX(x) x, textures, texturedata \n"\
" \n"\
"/// Sample 2D texture \n"\
"inline \n"\
"float4 Texture_Sample2D(float2 uv, TEXTURE_ARG_LIST_IDX(texidx)) \n"\
"{ \n"\
"    // Get width and height \n"\
"    int width = textures[texidx].w; \n"\
"    int height = textures[texidx].h; \n"\
" \n"\
"    // Find the origin of the data in the pool \n"\
"    __global char const* mydata = texturedata + textures[texidx].dataoffset; \n"\
" \n"\
"    // Handle UV wrap \n"\
"    // TODO: need UV mode support \n"\
"    uv -= floor(uv); \n"\
" \n"\
"    // Reverse Y: \n"\
"    // it is needed as textures are loaded with Y axis going top to down \n"\
"    // and our axis goes from down to top \n"\
"    uv.y = 1.f - uv.y; \n"\
" \n"\
"    // Calculate integer coordinates \n"\
"    int x0 = clamp((int)floor(uv.x * width), 0, width - 1); \n"\
"    int y0 = clamp((int)floor(uv.y * height), 0, height - 1); \n"\
" \n"\
"    // Calculate samples for linear filtering \n"\
"    int x1 = clamp(x0 + 1, 0,  width - 1); \n"\
"    int y1 = clamp(y0 + 1, 0, height - 1); \n"\
" \n"\
"    // Calculate weights for linear filtering \n"\
"    float wx = uv.x * width - floor(uv.x * width); \n"\
"    float wy = uv.y * height - floor(uv.y * height); \n"\
" \n"\
"    switch (textures[texidx].fmt) \n"\
"    { \n"\
"        case RGBA32: \n"\
"        { \n"\
"            __global float4 const* mydataf = (__global float4 const*)mydata; \n"\
" \n"\
"            // Get 4 values for linear filtering \n"\
"            float4 val00 = *(mydataf + width * y0 + x0); \n"\
"            float4 val01 = *(mydataf + width * y0 + x1); \n"\
"            float4 val10 = *(mydataf + width * y1 + x0); \n"\
"            float4 val11 = *(mydataf + width * y1 + x1); \n"\
" \n"\
"            // Filter and return the result \n"\
"            return lerp(lerp(val00, val01, wx), lerp(val10, val11, wx), wy); \n"\
"        } \n"\
" \n"\
"        case RGBA16: \n"\
"        { \n"\
"            __global half const* mydatah = (__global half const*)mydata; \n"\
" \n"\
"            // Get 4 values \n"\
"            float4 val00 = vload_half4(width * y0 + x0, mydatah); \n"\
"            float4 val01 = vload_half4(width * y0 + x1, mydatah); \n"\
"            float4 val10 = vload_half4(width * y1 + x0, mydatah); \n"\
"            float4 val11 = vload_half4(width * y1 + x1, mydatah); \n"\
" \n"\
"            // Filter and return the result \n"\
"            return lerp(lerp(val00, val01, wx), lerp(val10, val11, wx), wy); \n"\
"        } \n"\
" \n"\
"        case RGBA8: \n"\
"        { \n"\
"            __global uchar4 const* mydatac = (__global uchar4 const*)mydata; \n"\
" \n"\
"            // Get 4 values and convert to float \n"\
"            uchar4 valu00 = *(mydatac + width * y0 + x0); \n"\
"            uchar4 valu01 = *(mydatac + width * y0 + x1); \n"\
"            uchar4 valu10 = *(mydatac + width * y1 + x0); \n"\
"            uchar4 valu11 = *(mydatac + width * y1 + x1); \n"\
" \n"\
"            float4 val00 = make_float4((float)valu00.x / 255.f, (float)valu00.y / 255.f, (float)valu00.z / 255.f, (float)valu00.w / 255.f); \n"\
"            float4 val01 = make_float4((float)valu01.x / 255.f, (float)valu01.y / 255.f, (float)valu01.z / 255.f, (float)valu01.w / 255.f); \n"\
"            float4 val10 = make_float4((float)valu10.x / 255.f, (float)valu10.y / 255.f, (float)valu10.z / 255.f, (float)valu10.w / 255.f); \n"\
"            float4 val11 = make_float4((float)valu11.x / 255.f, (float)valu11.y / 255.f, (float)valu11.z / 255.f, (float)valu11.w / 255.f); \n"\
" \n"\
"            // Filter and return the result \n"\
"            return lerp(lerp(val00, val01, wx), lerp(val10, val11, wx), wy); \n"\
"        } \n"\
" \n"\
"        default: \n"\
"        { \n"\
"            return make_float4(0.f, 0.f, 0.f, 0.f); \n"\
"        } \n"\
"    } \n"\
"} \n"\
" \n"\
"/// Sample lattitue-longitude environment map using 3d vector \n"\
"inline \n"\
"float3 Texture_SampleEnvMap(float3 d, TEXTURE_ARG_LIST_IDX(texidx)) \n"\
"{ \n"\
"    // Transform to spherical coords \n"\
"    float r, phi, theta; \n"\
"    CartesianToSpherical(d, &r, &phi, &theta); \n"\
" \n"\
"    // Map to [0,1]x[0,1] range and reverse Y axis \n"\
"    float2 uv; \n"\
"    uv.x = phi / (2*PI); \n"\
"    uv.y = 1.f - theta / PI; \n"\
" \n"\
"    // Sample the texture \n"\
"    return Texture_Sample2D(uv, TEXTURE_ARGS_IDX(texidx)).xyz; \n"\
"} \n"\
" \n"\
"/// Get data from parameter value or texture \n"\
"inline \n"\
"float3 Texture_GetValue3f( \n"\
"                // Value \n"\
"                float3 v, \n"\
"                // Texture coordinate \n"\
"                float2 uv, \n"\
"                // Texture args \n"\
"                TEXTURE_ARG_LIST_IDX(texidx) \n"\
"                ) \n"\
"{ \n"\
"    // If texture present sample from texture \n"\
"    if (texidx != -1) \n"\
"    { \n"\
"        // Sample texture \n"\
"        return native_powr(Texture_Sample2D(uv, TEXTURE_ARGS_IDX(texidx)).xyz, 2.2f); \n"\
"    } \n"\
" \n"\
"    // Return fixed color otherwise \n"\
"    return v; \n"\
"} \n"\
" \n"\
"/// Get data from parameter value or texture \n"\
"inline \n"\
"float4 Texture_GetValue4f( \n"\
"                // Value \n"\
"                float4 v, \n"\
"                // Texture coordinate \n"\
"                float2 uv, \n"\
"                // Texture args \n"\
"                TEXTURE_ARG_LIST_IDX(texidx) \n"\
"                ) \n"\
"{ \n"\
"    // If texture present sample from texture \n"\
"    if (texidx != -1) \n"\
"    { \n"\
"        // Sample texture \n"\
"        return native_powr(Texture_Sample2D(uv, TEXTURE_ARGS_IDX(texidx)), 2.2f); \n"\
"    } \n"\
" \n"\
"    // Return fixed color otherwise \n"\
"    return v; \n"\
"} \n"\
" \n"\
"/// Get data from parameter value or texture \n"\
"inline \n"\
"float Texture_GetValue1f( \n"\
"                        // Value \n"\
"                        float v, \n"\
"                        // Texture coordinate \n"\
"                        float2 uv, \n"\
"                        // Texture args \n"\
"                        TEXTURE_ARG_LIST_IDX(texidx) \n"\
"                        ) \n"\
"{ \n"\
"    // If texture present sample from texture \n"\
"    if (texidx != -1) \n"\
"    { \n"\
"        // Sample texture \n"\
"        return Texture_Sample2D(uv, TEXTURE_ARGS_IDX(texidx)).x; \n"\
"    } \n"\
" \n"\
"    // Return fixed color otherwise \n"\
"    return v; \n"\
"} \n"\
" \n"\
"inline float3 TextureData_SampleNormalFromBump_uchar4(__global uchar4 const* mydatac, int width, int height, int t0, int s0) \n"\
"{ \n"\
"	int t0minus = clamp(t0 - 1, 0, height - 1); \n"\
"	int t0plus = clamp(t0 + 1, 0, height - 1); \n"\
"	int s0minus = clamp(s0 - 1, 0, width - 1); \n"\
"	int s0plus = clamp(s0 + 1, 0, width - 1); \n"\
" \n"\
"	const uchar utex00 = (*(mydatac + width * t0minus + s0minus)).x; \n"\
"	const uchar utex10 = (*(mydatac + width * t0minus + (s0))).x; \n"\
"	const uchar utex20 = (*(mydatac + width * t0minus + s0plus)).x; \n"\
" \n"\
"	const uchar utex01 = (*(mydatac + width * (t0)+s0minus)).x; \n"\
"	const uchar utex21 = (*(mydatac + width * (t0)+(s0 + 1))).x; \n"\
" \n"\
"	const uchar utex02 = (*(mydatac + width * t0plus + s0minus)).x; \n"\
"	const uchar utex12 = (*(mydatac + width * t0plus + (s0))).x; \n"\
"	const uchar utex22 = (*(mydatac + width * t0plus + s0plus)).x; \n"\
" \n"\
"	const float tex00 = (float)utex00 / 255.f; \n"\
"	const float tex10 = (float)utex10 / 255.f; \n"\
"	const float tex20 = (float)utex20 / 255.f; \n"\
" \n"\
"	const float tex01 = (float)utex01 / 255.f; \n"\
"	const float tex21 = (float)utex21 / 255.f; \n"\
" \n"\
"	const float tex02 = (float)utex02 / 255.f; \n"\
"	const float tex12 = (float)utex12 / 255.f; \n"\
"	const float tex22 = (float)utex22 / 255.f; \n"\
" \n"\
"	const float Gx = tex00 - tex20 + 2.0f * tex01 - 2.0f * tex21 + tex02 - tex22; \n"\
"	const float Gy = tex00 + 2.0f * tex10 + tex20 - tex02 - 2.0f * tex12 - tex22; \n"\
"	const float3 n = make_float3(Gx, Gy, 1.f); \n"\
" \n"\
"	return n; \n"\
"} \n"\
" \n"\
"inline float3 TextureData_SampleNormalFromBump_half4(__global half const* mydatah, int width, int height, int t0, int s0) \n"\
"{ \n"\
"	int t0minus = clamp(t0 - 1, 0, height - 1); \n"\
"	int t0plus = clamp(t0 + 1, 0, height - 1); \n"\
"	int s0minus = clamp(s0 - 1, 0, width - 1); \n"\
"	int s0plus = clamp(s0 + 1, 0, width - 1); \n"\
" \n"\
"	const float tex00 = vload_half4(width * t0minus + s0minus, mydatah).x; \n"\
"	const float tex10 = vload_half4(width * t0minus + (s0), mydatah).x; \n"\
"	const float tex20 = vload_half4(width * t0minus + s0plus, mydatah).x; \n"\
" \n"\
"	const float tex01 = vload_half4(width * (t0)+s0minus, mydatah).x; \n"\
"	const float tex21 = vload_half4(width * (t0)+s0plus, mydatah).x; \n"\
" \n"\
"	const float tex02 = vload_half4(width * t0plus + s0minus, mydatah).x; \n"\
"	const float tex12 = vload_half4(width * t0plus + (s0), mydatah).x; \n"\
"	const float tex22 = vload_half4(width * t0plus + s0plus, mydatah).x; \n"\
" \n"\
"	const float Gx = tex00 - tex20 + 2.0f * tex01 - 2.0f * tex21 + tex02 - tex22; \n"\
"	const float Gy = tex00 + 2.0f * tex10 + tex20 - tex02 - 2.0f * tex12 - tex22; \n"\
"	const float3 n = make_float3(Gx, Gy, 1.f); \n"\
" \n"\
"	return n; \n"\
"} \n"\
" \n"\
"inline float3 TextureData_SampleNormalFromBump_float4(__global float4 const* mydataf, int width, int height, int t0, int s0) \n"\
"{ \n"\
"	int t0minus = clamp(t0 - 1, 0, height - 1); \n"\
"	int t0plus = clamp(t0 + 1, 0, height - 1); \n"\
"	int s0minus = clamp(s0 - 1, 0, width - 1); \n"\
"	int s0plus = clamp(s0 + 1, 0, width - 1); \n"\
" \n"\
"	const float tex00 = (*(mydataf + width * t0minus + s0minus)).x; \n"\
"	const float tex10 = (*(mydataf + width * t0minus + (s0))).x; \n"\
"	const float tex20 = (*(mydataf + width * t0minus + s0plus)).x; \n"\
" \n"\
"	const float tex01 = (*(mydataf + width * (t0)+s0minus)).x; \n"\
"	const float tex21 = (*(mydataf + width * (t0)+s0plus)).x; \n"\
" \n"\
"	const float tex02 = (*(mydataf + width * t0plus + s0minus)).x; \n"\
"	const float tex12 = (*(mydataf + width * t0plus + (s0))).x; \n"\
"	const float tex22 = (*(mydataf + width * t0plus + s0plus)).x; \n"\
" \n"\
"	const float Gx = tex00 - tex20 + 2.0f * tex01 - 2.0f * tex21 + tex02 - tex22; \n"\
"	const float Gy = tex00 + 2.0f * tex10 + tex20 - tex02 - 2.0f * tex12 - tex22; \n"\
"	const float3 n = make_float3(Gx, Gy, 1.f); \n"\
" \n"\
"	return n; \n"\
"} \n"\
" \n"\
"/// Sample 2D texture \n"\
"inline \n"\
"float3 Texture_SampleBump(float2 uv, TEXTURE_ARG_LIST_IDX(texidx)) \n"\
"{ \n"\
"    // Get width and height \n"\
"    int width = textures[texidx].w; \n"\
"    int height = textures[texidx].h; \n"\
" \n"\
"    // Find the origin of the data in the pool \n"\
"    __global char const* mydata = texturedata + textures[texidx].dataoffset; \n"\
" \n"\
"    // Handle UV wrap \n"\
"    // TODO: need UV mode support \n"\
"    uv -= floor(uv); \n"\
" \n"\
"    // Reverse Y: \n"\
"    // it is needed as textures are loaded with Y axis going top to down \n"\
"    // and our axis goes from down to top \n"\
"    uv.y = 1.f - uv.y; \n"\
" \n"\
"    // Calculate integer coordinates \n"\
"    int s0 = clamp((int)floor(uv.x * width), 0, width - 1); \n"\
"    int t0 = clamp((int)floor(uv.y * height), 0, height - 1); \n"\
" \n"\
"	int s1 = clamp(s0 + 1, 0, width - 1); \n"\
"	int t1 = clamp(t0 + 1, 0, height - 1); \n"\
" \n"\
"	// Calculate weights for linear filtering \n"\
"	float wx = uv.x * width - floor(uv.x * width); \n"\
"	float wy = uv.y * height - floor(uv.y * height); \n"\
" \n"\
"    switch (textures[texidx].fmt) \n"\
"    { \n"\
"    case RGBA32: \n"\
"    { \n"\
"        __global float3 const* mydataf = (__global float3 const*)mydata; \n"\
" \n"\
"		float3 n00 = TextureData_SampleNormalFromBump_float4(mydataf, width, height, t0, s0); \n"\
"		float3 n01 = TextureData_SampleNormalFromBump_float4(mydataf, width, height, t0, s1); \n"\
"		float3 n10 = TextureData_SampleNormalFromBump_float4(mydataf, width, height, t1, s0); \n"\
"		float3 n11 = TextureData_SampleNormalFromBump_float4(mydataf, width, height, t1, s1); \n"\
" \n"\
"		float3 n = lerp3(lerp3(n00, n01, wx), lerp3(n10, n11, wx), wy); \n"\
" \n"\
"		return 0.5f * normalize(n) + make_float3(0.5f, 0.5f, 0.5f); \n"\
"    } \n"\
" \n"\
"    case RGBA16: \n"\
"    { \n"\
"        __global half const* mydatah = (__global half const*)mydata; \n"\
" \n"\
"		float3 n00 = TextureData_SampleNormalFromBump_half4(mydatah, width, height, t0, s0); \n"\
"		float3 n01 = TextureData_SampleNormalFromBump_half4(mydatah, width, height, t0, s1); \n"\
"		float3 n10 = TextureData_SampleNormalFromBump_half4(mydatah, width, height, t1, s0); \n"\
"		float3 n11 = TextureData_SampleNormalFromBump_half4(mydatah, width, height, t1, s1); \n"\
" \n"\
"		float3 n = lerp3(lerp3(n00, n01, wx), lerp3(n10, n11, wx), wy); \n"\
" \n"\
"		return 0.5f * normalize(n) + make_float3(0.5f, 0.5f, 0.5f); \n"\
"    } \n"\
" \n"\
"    case RGBA8: \n"\
"    { \n"\
"        __global uchar4 const* mydatac = (__global uchar4 const*)mydata; \n"\
" \n"\
"		float3 n00 = TextureData_SampleNormalFromBump_uchar4(mydatac, width, height, t0, s0); \n"\
"		float3 n01 = TextureData_SampleNormalFromBump_uchar4(mydatac, width, height, t0, s1); \n"\
"		float3 n10 = TextureData_SampleNormalFromBump_uchar4(mydatac, width, height, t1, s0); \n"\
"		float3 n11 = TextureData_SampleNormalFromBump_uchar4(mydatac, width, height, t1, s1); \n"\
" \n"\
"		float3 n = lerp3(lerp3(n00, n01, wx), lerp3(n10, n11, wx), wy); \n"\
" \n"\
"		return 0.5f * normalize(n) + make_float3(0.5f, 0.5f, 0.5f); \n"\
"    } \n"\
" \n"\
"    default: \n"\
"    { \n"\
"        return make_float3(0.f, 0.f, 0.f); \n"\
"    } \n"\
"    } \n"\
"} \n"\
" \n"\
" \n"\
" \n"\
"#endif // TEXTURE_CL \n"\
;
static const char g_utils_opencl[]= \
"/********************************************************************** \n"\
"Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved. \n"\
" \n"\
"Permission is hereby granted, free of charge, to any person obtaining a copy \n"\
"of this software and associated documentation files (the \"Software\"), to deal \n"\
"in the Software without restriction, including without limitation the rights \n"\
"to use, copy, modify, merge, publish, distribute, sublicense, and/or sell \n"\
"copies of the Software, and to permit persons to whom the Software is \n"\
"furnished to do so, subject to the following conditions: \n"\
" \n"\
"The above copyright notice and this permission notice shall be included in \n"\
"all copies or substantial portions of the Software. \n"\
" \n"\
"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR \n"\
"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, \n"\
"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE \n"\
"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER \n"\
"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, \n"\
"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN \n"\
"THE SOFTWARE. \n"\
"********************************************************************/ \n"\
"#ifndef UTILS_CL \n"\
"#define UTILS_CL \n"\
" \n"\
"#define PI 3.14159265358979323846f \n"\
" \n"\
" \n"\
"#ifndef APPLE \n"\
"/// These functions are defined on OSX already \n"\
"float4 make_float4(float x, float y, float z, float w) \n"\
"{ \n"\
"    float4 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    res.z = z; \n"\
"    res.w = w; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"float3 make_float3(float x, float y, float z) \n"\
"{ \n"\
"    float3 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    res.z = z; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"float2 make_float2(float x, float y) \n"\
"{ \n"\
"    float2 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"int2 make_int2(int x, int y) \n"\
"{ \n"\
"    int2 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    return res; \n"\
"} \n"\
"#endif \n"\
" \n"\
"matrix4x4 matrix_from_cols(float4 c0, float4 c1, float4 c2, float4 c3) \n"\
"{ \n"\
"    matrix4x4 m; \n"\
"    m.m0 = make_float4(c0.x, c1.x, c2.x, c3.x); \n"\
"    m.m1 = make_float4(c0.y, c1.y, c2.y, c3.y); \n"\
"    m.m2 = make_float4(c0.z, c1.z, c2.z, c3.z); \n"\
"    m.m3 = make_float4(c0.w, c1.w, c2.w, c3.w); \n"\
"    return m; \n"\
"} \n"\
" \n"\
"matrix4x4 matrix_from_rows(float4 c0, float4 c1, float4 c2, float4 c3) \n"\
"{ \n"\
"    matrix4x4 m; \n"\
"    m.m0 = c0; \n"\
"    m.m1 = c1; \n"\
"    m.m2 = c2; \n"\
"    m.m3 = c3; \n"\
"    return m; \n"\
"} \n"\
" \n"\
"matrix4x4 matrix_from_rows3(float3 c0, float3 c1, float3 c2) \n"\
"{ \n"\
"    matrix4x4 m; \n"\
"    m.m0.xyz = c0; m.m0.w = 0; \n"\
"    m.m1.xyz = c1; m.m1.w = 0; \n"\
"    m.m2.xyz = c2; m.m2.w = 0; \n"\
"    m.m3 = make_float4(0.f, 0.f, 0.f, 1.f); \n"\
"    return m; \n"\
"} \n"\
" \n"\
"matrix4x4 matrix_from_cols3(float3 c0, float3 c1, float3 c2) \n"\
"{ \n"\
"    matrix4x4 m; \n"\
"    m.m0 = make_float4(c0.x, c1.x, c2.x, 0.f); \n"\
"    m.m1 = make_float4(c0.y, c1.y, c2.y, 0.f); \n"\
"    m.m2 = make_float4(c0.z, c1.z, c2.z, 0.f); \n"\
"    m.m3 = make_float4(0.f, 0.f, 0.f, 1.f); \n"\
"    return m; \n"\
"} \n"\
" \n"\
"matrix4x4 matrix_transpose(matrix4x4 m) \n"\
"{ \n"\
"    return matrix_from_cols(m.m0, m.m1, m.m2, m.m3); \n"\
"} \n"\
" \n"\
"float4 matrix_mul_vector4(matrix4x4 m, float4 v) \n"\
"{ \n"\
"    float4 res; \n"\
"    res.x = dot(m.m0, v); \n"\
"    res.y = dot(m.m1, v); \n"\
"    res.z = dot(m.m2, v); \n"\
"    res.w = dot(m.m3, v); \n"\
"    return res; \n"\
"} \n"\
" \n"\
"float3 matrix_mul_vector3(matrix4x4 m, float3 v) \n"\
"{ \n"\
"    float3 res; \n"\
"    res.x = dot(m.m0.xyz, v); \n"\
"    res.y = dot(m.m1.xyz, v); \n"\
"    res.z = dot(m.m2.xyz, v); \n"\
"    return res; \n"\
"} \n"\
" \n"\
"float3 matrix_mul_point3(matrix4x4 m, float3 v) \n"\
"{ \n"\
"    float3 res; \n"\
"    res.x = dot(m.m0.xyz, v) + m.m0.w; \n"\
"    res.y = dot(m.m1.xyz, v) + m.m1.w; \n"\
"    res.z = dot(m.m2.xyz, v) + m.m2.w; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"/// Linearly interpolate between two values \n"\
"float4 lerp(float4 a, float4 b, float w) \n"\
"{ \n"\
"    return a + w*(b-a); \n"\
"} \n"\
" \n"\
"/// Linearly interpolate between two values \n"\
"float3 lerp3(float3 a, float3 b, float w) \n"\
"{ \n"\
"	return a + w*(b - a); \n"\
"} \n"\
" \n"\
"/// Translate cartesian coordinates to spherical system \n"\
"void CartesianToSpherical ( float3 cart, float* r, float* phi, float* theta ) \n"\
"{ \n"\
"    float temp = atan2(cart.x, cart.z); \n"\
"    *r = sqrt(cart.x*cart.x + cart.y*cart.y + cart.z*cart.z); \n"\
"    // Account for discontinuity \n"\
"    *phi = (float)((temp >= 0)?temp:(temp + 2*PI)); \n"\
"    *theta = acos(cart.y/ *r); \n"\
"} \n"\
" \n"\
"/// Get vector orthogonal to a given one \n"\
"float3 GetOrthoVector(float3 n) \n"\
"{ \n"\
"    float3 p; \n"\
" \n"\
"    if (fabs(n.z) > 0.f) { \n"\
"        float k = sqrt(n.y*n.y + n.z*n.z); \n"\
"        p.x = 0; p.y = -n.z/k; p.z = n.y/k; \n"\
"    } \n"\
"    else { \n"\
"        float k = sqrt(n.x*n.x + n.y*n.y); \n"\
"        p.x = n.y/k; p.y = -n.x/k; p.z = 0; \n"\
"    } \n"\
" \n"\
"    return normalize(p); \n"\
"} \n"\
" \n"\
"float luminance(float3 v) \n"\
"{ \n"\
"    // Luminance \n"\
"    return 0.2126f * v.x + 0.7152f * v.y + 0.0722f * v.z; \n"\
"} \n"\
" \n"\
"uint upper_power_of_two(uint v) \n"\
"{ \n"\
"    v--; \n"\
"    v |= v >> 1; \n"\
"    v |= v >> 2; \n"\
"    v |= v >> 4; \n"\
"    v |= v >> 8; \n"\
"    v |= v >> 16; \n"\
"    v++; \n"\
"    return v; \n"\
"} \n"\
" \n"\
"INLINE \n"\
"void atomic_add_float(volatile __global float* addr, float value) \n"\
"{ \n"\
"    union { \n"\
"        unsigned int u32; \n"\
"        float        f32; \n"\
"    } next, expected, current; \n"\
"    current.f32 = *addr; \n"\
"    do { \n"\
"        expected.f32 = current.f32; \n"\
"        next.f32 = expected.f32 + value; \n"\
"        current.u32 = atomic_cmpxchg((volatile __global unsigned int *)addr, \n"\
"            expected.u32, next.u32); \n"\
"    } while (current.u32 != expected.u32); \n"\
"} \n"\
" \n"\
"void atomic_add_float3(volatile __global float3* ptr, float3 value) \n"\
"{ \n"\
"    volatile __global float* p = (volatile __global float*)ptr; \n"\
"    atomic_add_float(p, value.x); \n"\
"    atomic_add_float(p + 1, value.y); \n"\
"    atomic_add_float(p + 2, value.z); \n"\
"} \n"\
" \n"\
"void atomic_add_float4(volatile __global float4* ptr, float4 value) \n"\
"{ \n"\
"    volatile __global float* p = (volatile __global float*)ptr; \n"\
"    atomic_add_float(p, value.x); \n"\
"    atomic_add_float(p + 1, value.y); \n"\
"    atomic_add_float(p + 2, value.z); \n"\
"    atomic_add_float(p + 3, value.w); \n"\
"} \n"\
" \n"\
"void add_float3(__global float3* ptr, float3 value) \n"\
"{ \n"\
"    *ptr += value; \n"\
"} \n"\
" \n"\
"void add_float4(__global float4* ptr, float4 value) \n"\
"{ \n"\
"    *ptr += value; \n"\
"} \n"\
" \n"\
" \n"\
"#endif // UTILS_CL \n"\
;
static const char g_vertex_opencl[]= \
"/********************************************************************** \n"\
"Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved. \n"\
" \n"\
"Permission is hereby granted, free of charge, to any person obtaining a copy \n"\
"of this software and associated documentation files (the \"Software\"), to deal \n"\
"in the Software without restriction, including without limitation the rights \n"\
"to use, copy, modify, merge, publish, distribute, sublicense, and/or sell \n"\
"copies of the Software, and to permit persons to whom the Software is \n"\
"furnished to do so, subject to the following conditions: \n"\
" \n"\
"The above copyright notice and this permission notice shall be included in \n"\
"all copies or substantial portions of the Software. \n"\
" \n"\
"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR \n"\
"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, \n"\
"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE \n"\
"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER \n"\
"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, \n"\
"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN \n"\
"THE SOFTWARE. \n"\
"********************************************************************/ \n"\
"#ifndef VERTEX_CL \n"\
"#define VERTEX_CL \n"\
" \n"\
"// Path vertex type \n"\
"enum PathVertexType \n"\
"{ \n"\
"    kCamera, \n"\
"    kSurface, \n"\
"    kVolume, \n"\
"    kLight \n"\
"}; \n"\
" \n"\
"// Path vertex descriptor \n"\
"typedef struct _PathVertex \n"\
"{ \n"\
"    float3 position; \n"\
"    float3 shading_normal; \n"\
"    float3 geometric_normal; \n"\
"    float2 uv; \n"\
"    float pdf_forward; \n"\
"    float pdf_backward; \n"\
"    float3 flow; \n"\
"    float3 unused; \n"\
"    int type; \n"\
"    int material_index; \n"\
"    int flags; \n"\
"    int padding; \n"\
"} PathVertex; \n"\
" \n"\
"// Initialize path vertex \n"\
"INLINE \n"\
"void PathVertex_Init( \n"\
"    PathVertex* v,  \n"\
"    float3 p,  \n"\
"    float3 n, \n"\
"    float3 ng,  \n"\
"    float2 uv,  \n"\
"    float pdf_fwd, \n"\
"    float pdf_bwd,  \n"\
"    float3 flow,  \n"\
"    int type, \n"\
"    int matidx \n"\
") \n"\
"{ \n"\
"    v->position = p; \n"\
"    v->shading_normal = n; \n"\
"    v->geometric_normal = ng; \n"\
"    v->uv = uv; \n"\
"    v->pdf_forward = pdf_fwd; \n"\
"    v->pdf_backward = pdf_bwd; \n"\
"    v->flow = flow; \n"\
"    v->type = type; \n"\
"    v->material_index = matidx; \n"\
"    v->flags = 0; \n"\
"    v->unused = 0.f; \n"\
"} \n"\
" \n"\
"#endif \n"\
;
static const char g_volumetrics_opencl[]= \
"/********************************************************************** \n"\
"Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved. \n"\
" \n"\
"Permission is hereby granted, free of charge, to any person obtaining a copy \n"\
"of this software and associated documentation files (the \"Software\"), to deal \n"\
"in the Software without restriction, including without limitation the rights \n"\
"to use, copy, modify, merge, publish, distribute, sublicense, and/or sell \n"\
"copies of the Software, and to permit persons to whom the Software is \n"\
"furnished to do so, subject to the following conditions: \n"\
" \n"\
"The above copyright notice and this permission notice shall be included in \n"\
"all copies or substantial portions of the Software. \n"\
" \n"\
"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR \n"\
"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, \n"\
"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE \n"\
"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER \n"\
"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, \n"\
"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN \n"\
"THE SOFTWARE. \n"\
"********************************************************************/ \n"\
"#ifndef VOLUMETRICS_CL \n"\
"#define VOLUMETRICS_CL \n"\
" \n"\
" \n"\
"#define FAKE_SHAPE_SENTINEL 0xFFFFFF \n"\
" \n"\
"// The following functions are taken from PBRT \n"\
"float PhaseFunction_Uniform(float3 wi, float3 wo) \n"\
"{ \n"\
"    return 1.f / (4.f * PI); \n"\
"} \n"\
" \n"\
"float PhaseFunction_Rayleigh(float3 wi, float3 wo) \n"\
"{ \n"\
"    float costheta = dot(wi, wo); \n"\
"    return  3.f / (16.f*PI) * (1 + costheta * costheta); \n"\
"} \n"\
" \n"\
"float PhaseFunction_MieHazy(float3 wi, float3 wo) \n"\
"{ \n"\
"    float costheta = dot(wi, wo); \n"\
"    return (0.5f + 4.5f * native_powr(0.5f * (1.f + costheta), 8.f)) / (4.f*PI); \n"\
"} \n"\
" \n"\
"float PhaseFunction_MieMurky(float3 wi, float3 wo) \n"\
"{ \n"\
"    float costheta = dot(wi, wo); \n"\
"    return (0.5f + 16.5f * native_powr(0.5f * (1.f + costheta), 32.f)) / (4.f*PI); \n"\
"} \n"\
" \n"\
"float PhaseFunction_HG(float3 wi, float3 wo, float g) \n"\
"{ \n"\
"    float costheta = dot(wi, wo); \n"\
"    return 1.f / (4.f * PI) * \n"\
"        (1.f - g*g) / native_powr(1.f + g*g - 2.f * g * costheta, 1.5f); \n"\
"} \n"\
" \n"\
"// Evaluate volume transmittance along the ray [0, dist] segment \n"\
"float3 Volume_Transmittance(__global Volume const* volume, __global ray const* ray, float dist) \n"\
"{ \n"\
"    switch (volume->type) \n"\
"    { \n"\
"        case kHomogeneous: \n"\
"        { \n"\
"            // For homogeneous it is e(-sigma * dist) \n"\
"            float3 sigma_t = volume->sigma_a + volume->sigma_s; \n"\
"            return native_exp(-sigma_t * dist); \n"\
"        } \n"\
"    } \n"\
"     \n"\
"    return 1.f; \n"\
"} \n"\
" \n"\
"// Evaluate volume selfemission along the ray [0, dist] segment \n"\
"float3 Volume_Emission(__global Volume const* volume, __global ray const* ray, float dist) \n"\
"{ \n"\
"    switch (volume->type) \n"\
"    { \n"\
"        case kHomogeneous: \n"\
"        { \n"\
"            // For homogeneous it is simply Tr * Ev (since sigma_e is constant) \n"\
"            return Volume_Transmittance(volume, ray, dist) * volume->sigma_e; \n"\
"        } \n"\
"    } \n"\
"     \n"\
"    return 0.f; \n"\
"} \n"\
" \n"\
"// Sample volume in order to find next scattering event \n"\
"float Volume_SampleDistance(__global Volume const* volume, __global ray const* ray, float maxdist, float sample, float* pdf) \n"\
"{ \n"\
"    switch (volume->type) \n"\
"    { \n"\
"        case kHomogeneous: \n"\
"        { \n"\
"            // The PDF = sigma * e(-sigma * x), so the larger sigma the closer we scatter \n"\
"            float sigma = (volume->sigma_s.x + volume->sigma_s.y + volume->sigma_s.z) / 3; \n"\
"            float d = sigma > 0.f ? (-native_log(sample) / sigma) : -1.f; \n"\
"            *pdf = sigma > 0.f ? (sigma * native_exp(-sigma * d)) : 0.f; \n"\
"            return d; \n"\
"        } \n"\
"    } \n"\
"     \n"\
"    return -1.f; \n"\
"} \n"\
" \n"\
"// Apply volume effects (absorbtion and emission) and scatter if needed. \n"\
"// The rays we handling here might intersect something or miss,  \n"\
"// since scattering can happen even for missed rays. \n"\
"// That's why this function is called prior to ray compaction. \n"\
"// In case ray has missed geometry (has shapeid < 0) and has been scattered, \n"\
"// we put FAKE_SHAPE_SENTINEL into shapeid to prevent ray from being compacted away. \n"\
"// \n"\
"__kernel void EvaluateVolume( \n"\
"    // Ray batch \n"\
"    __global ray const* rays, \n"\
"    // Pixel indices \n"\
"    __global int const* pixelindices, \n"\
"    // Output indices \n"\
"    __global int const* output_indices, \n"\
"    // Number of rays \n"\
"    __global int const* numrays, \n"\
"    // Volumes \n"\
"    __global Volume const* volumes, \n"\
"    // Textures \n"\
"    TEXTURE_ARG_LIST, \n"\
"    // RNG seed \n"\
"    uint rngseed, \n"\
"    // Sampler state \n"\
"    __global uint* random, \n"\
"    // Sobol matrices \n"\
"    __global uint const* sobol_mat, \n"\
"    // Current bounce  \n"\
"    int bounce, \n"\
"    // Current frame \n"\
"    int frame, \n"\
"    // Intersection data \n"\
"    __global Intersection* isects, \n"\
"    // Current paths \n"\
"    __global Path* paths, \n"\
"    // Output \n"\
"    __global float3* output \n"\
"    ) \n"\
"{ \n"\
"    int globalid = get_global_id(0); \n"\
" \n"\
"    // Only handle active rays \n"\
"    if (globalid < *numrays) \n"\
"    { \n"\
"        int pixelidx = pixelindices[globalid]; \n"\
"         \n"\
"        __global Path* path = paths + pixelidx; \n"\
" \n"\
"        // Path can be dead here since compaction step has not  \n"\
"        // yet been applied \n"\
"        if (!Path_IsAlive(path)) \n"\
"            return; \n"\
" \n"\
"        int volidx = Path_GetVolumeIdx(path); \n"\
" \n"\
"        // Check if we are inside some volume \n"\
"        if (volidx != -1) \n"\
"        { \n"\
"            Sampler sampler; \n"\
"#if SAMPLER == SOBOL \n"\
"            uint scramble = random[pixelidx] * 0x1fe3434f; \n"\
"            Sampler_Init(&sampler, frame, SAMPLE_DIM_SURFACE_OFFSET + bounce * SAMPLE_DIMS_PER_BOUNCE + SAMPLE_DIM_VOLUME_APPLY_OFFSET, scramble); \n"\
"#elif SAMPLER == RANDOM \n"\
"            uint scramble = pixelidx * rngseed; \n"\
"            Sampler_Init(&sampler, scramble); \n"\
"#elif SAMPLER == CMJ \n"\
"            uint rnd = random[pixelidx]; \n"\
"            uint scramble = rnd * 0x1fe3434f * ((frame + 71 * rnd) / (CMJ_DIM * CMJ_DIM)); \n"\
"            Sampler_Init(&sampler, frame % (CMJ_DIM * CMJ_DIM), SAMPLE_DIM_SURFACE_OFFSET + bounce * SAMPLE_DIMS_PER_BOUNCE + SAMPLE_DIM_VOLUME_APPLY_OFFSET, scramble); \n"\
"#endif \n"\
" \n"\
"            // Try sampling volume for a next scattering event \n"\
"            float pdf = 0.f; \n"\
"            float maxdist = Intersection_GetDistance(isects + globalid); \n"\
"            float d = Volume_SampleDistance(&volumes[volidx], &rays[globalid], maxdist, Sampler_Sample1D(&sampler, SAMPLER_ARGS), &pdf); \n"\
"             \n"\
"            // Check if we shall skip the event (it is either outside of a volume or not happened at all) \n"\
"            bool skip = d < 0 || d > maxdist || pdf <= 0.f; \n"\
" \n"\
"            if (skip) \n"\
"            { \n"\
"                // In case we skip we just need to apply volume absorbtion and emission for the segment we went through \n"\
"                // and clear scatter flag \n"\
"                Path_ClearScatterFlag(path); \n"\
"                // Emission contribution accounting for a throughput we have so far \n"\
"                Path_AddContribution(path, output, output_indices[pixelidx], Volume_Emission(&volumes[volidx], &rays[globalid], maxdist)); \n"\
"                // And finally update the throughput \n"\
"                Path_MulThroughput(path, Volume_Transmittance(&volumes[volidx], &rays[globalid], maxdist)); \n"\
"            } \n"\
"            else \n"\
"            { \n"\
"                // Set scattering flag to notify ShadeVolume kernel to handle this path \n"\
"                Path_SetScatterFlag(path); \n"\
"                // Emission contribution accounting for a throughput we have so far \n"\
"                Path_AddContribution(path, output, output_indices[pixelidx], Volume_Emission(&volumes[volidx], &rays[globalid], d) / pdf); \n"\
"                // Update the throughput \n"\
"                Path_MulThroughput(path, (Volume_Transmittance(&volumes[volidx], &rays[globalid], d) / pdf)); \n"\
"                // Put fake shape to prevent from being compacted away \n"\
"                isects[globalid].shapeid = FAKE_SHAPE_SENTINEL; \n"\
"                // And keep scattering distance around as well \n"\
"                isects[globalid].uvwt.w = d; \n"\
"            } \n"\
"        } \n"\
"    } \n"\
"} \n"\
" \n"\
"#endif // VOLUMETRICS_CL \n"\
;
static const char g_wavelet_denoise_opencl[]= \
"/********************************************************************** \n"\
"Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved. \n"\
" \n"\
"Permission is hereby granted, free of charge, to any person obtaining a copy \n"\
"of this software and associated documentation files (the \"Software\"), to deal \n"\
"in the Software without restriction, including without limitation the rights \n"\
"to use, copy, modify, merge, publish, distribute, sublicense, and/or sell \n"\
"copies of the Software, and to permit persons to whom the Software is \n"\
"furnished to do so, subject to the following conditions: \n"\
" \n"\
"The above copyright notice and this permission notice shall be included in \n"\
"all copies or substantial portions of the Software. \n"\
" \n"\
"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR \n"\
"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, \n"\
"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE \n"\
"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER \n"\
"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, \n"\
"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN \n"\
"THE SOFTWARE. \n"\
"********************************************************************/ \n"\
"#ifndef WAVELETDENOISE_CL \n"\
"#define WAVELETDENOISE_CL \n"\
" \n"\
" \n"\
"#define WAVELET_KERNEL_SIZE 25 \n"\
"#define GAUSS_KERNEL_SIZE 9 \n"\
"#define DENOM_EPS 1e-8f \n"\
"#define FRAME_BLEND_ALPHA 0.2f \n"\
" \n"\
"// Gauss filter 3x3 for variance prefiltering on first wavelet pass \n"\
"float4 GaussFilter3x3( \n"\
"    GLOBAL float4 const* restrict buffer,  \n"\
"    int2 buffer_size, \n"\
"    int2 uv) \n"\
"{ \n"\
"    const int2 kernel_offsets[GAUSS_KERNEL_SIZE] = { \n"\
"        make_int2(-1, -1),  make_int2(0, -1),   make_int2(1, -1), \n"\
"        make_int2(-1, 0),   make_int2(0, 0),    make_int2(1, 0), \n"\
"        make_int2(-1, 1),   make_int2(0, 1),    make_int2(1, 1), \n"\
"    }; \n"\
" \n"\
"    const float kernel_weights[GAUSS_KERNEL_SIZE] = { \n"\
"        1.0 / 16.0, 1.0 / 8.0, 1.0 / 16.0, \n"\
"        1.0 / 8.0, 1.0 / 4.0, 1.0 / 8.0, \n"\
"        1.0 / 16.0, 1.0 / 8.0, 1.0 / 16.0 \n"\
"    }; \n"\
" \n"\
"    float4 sample_out = make_float4(0.f, 0.f, 0.f, 0.f); \n"\
"     \n"\
"    for (int i = 0; i < GAUSS_KERNEL_SIZE; i++) \n"\
"    { \n"\
"        const int cx = clamp(uv.x + kernel_offsets[i].x, 0, buffer_size.x - 1); \n"\
"        const int cy = clamp(uv.y + kernel_offsets[i].y, 0, buffer_size.y - 1); \n"\
"        const int ci = cy * buffer_size.x + cx; \n"\
" \n"\
"        sample_out += kernel_weights[i] * buffer[ci]; \n"\
"    } \n"\
"     \n"\
"    return sample_out; \n"\
"} \n"\
" \n"\
"// Convertor to linear address with out of bounds clamp \n"\
"int ConvertToLinearAddress(int address_x, int address_y, int2 buffer_size) \n"\
"{ \n"\
"    int max_buffer_size = buffer_size.x * buffer_size.y; \n"\
"    return clamp(address_y * buffer_size.x + address_x, 0, max_buffer_size - 1); \n"\
"} \n"\
" \n"\
"int ConvertToLinearAddressInt2(int2 address, int2 buffer_size) \n"\
"{ \n"\
"    int max_buffer_size = buffer_size.x * buffer_size.y; \n"\
"    return clamp(address.y * buffer_size.x + address.x, 0, max_buffer_size - 1); \n"\
"} \n"\
" \n"\
"// Bilinear sampler \n"\
"float4 Sampler2DBilinear(GLOBAL float4 const* restrict buffer, int2 buffer_size, float2 uv) \n"\
"{ \n"\
"    uv = uv * make_float2(buffer_size.x, buffer_size.y) - make_float2(0.5f, 0.5f); \n"\
" \n"\
"    int x = floor(uv.x); \n"\
"    int y = floor(uv.y); \n"\
" \n"\
"    float2 uv_ratio = uv - make_float2(x, y); \n"\
"    float2 uv_inv   = make_float2(1.f, 1.f) - uv_ratio; \n"\
" \n"\
"    int x1 = clamp(x + 1, 0, buffer_size.x - 1); \n"\
"    int y1 = clamp(y + 1, 0, buffer_size.y - 1); \n"\
" \n"\
"    float4 r =  (buffer[ConvertToLinearAddress(x, y, buffer_size)]   * uv_inv.x + buffer[ConvertToLinearAddress(x1, y, buffer_size)]   * uv_ratio.x) * uv_inv.y +  \n"\
"                (buffer[ConvertToLinearAddress(x, y1, buffer_size)]  * uv_inv.x + buffer[ConvertToLinearAddress(x1, y1, buffer_size)]  * uv_ratio.x) * uv_ratio.y; \n"\
" \n"\
"    return r; \n"\
"} \n"\
" \n"\
" \n"\
"// Derivatives with subpixel values \n"\
"float4 dFdx(GLOBAL float4 const* restrict buffer, float2 uv, int2 buffer_size) \n"\
"{ \n"\
"    const float2 uv_x = uv + make_float2(1.0f / (float)buffer_size.x, 0.0); \n"\
" \n"\
"    return Sampler2DBilinear(buffer, buffer_size, uv_x) - Sampler2DBilinear(buffer, buffer_size, uv); \n"\
"} \n"\
" \n"\
"float4 dFdy(GLOBAL float4 const* restrict buffer, float2 uv, int2 buffer_size) \n"\
"{ \n"\
"    const float2 uv_y = uv + make_float2(0.0, 1.0f / (float)buffer_size.y); \n"\
" \n"\
"    return Sampler2DBilinear(buffer, buffer_size, uv_y) - Sampler2DBilinear(buffer, buffer_size, uv); \n"\
"} \n"\
" \n"\
" \n"\
"KERNEL \n"\
"void WaveletFilter_main( \n"\
"    // Color data \n"\
"    GLOBAL float4 const* restrict colors, \n"\
"    // Normal data \n"\
"    GLOBAL float4 const* restrict normals, \n"\
"    // Positional data \n"\
"    GLOBAL float4 const* restrict positions, \n"\
"    // Variance \n"\
"    GLOBAL float4 const* restrict variances, \n"\
"    // Albedo \n"\
"    GLOBAL float4 const* restrict albedo, \n"\
"    // Image resolution \n"\
"    int width, \n"\
"    int height, \n"\
"    // Filter width \n"\
"    int step_width, \n"\
"    // Filter kernel parameters \n"\
"    float sigma_color, \n"\
"    float sigma_position, \n"\
"    // Resulting color \n"\
"    GLOBAL float4* restrict out_colors \n"\
") \n"\
"{ \n"\
"    int2 global_id; \n"\
"    global_id.x = get_global_id(0); \n"\
"    global_id.y = get_global_id(1); \n"\
" \n"\
"    const float kernel_weights[WAVELET_KERNEL_SIZE] = { \n"\
"        1.0 / 256.0, 1.0 / 64.0, 3.0 / 128.0, 1.0 / 64.0, 1.0 / 256.0, \n"\
"        1.0 / 64.0, 1.0 / 16.0, 3.0 / 32.0, 1.0 / 16.0, 1.0 / 64.0, \n"\
"        3.0 / 128.0, 3.0 / 32.0, 9.0 / 64.0, 3.0 / 32.0, 3.0 / 128.0, \n"\
"        1.0 / 64.0, 1.0 / 16.0, 3.0 / 32.0, 1.0 / 16.0, 1.0 / 64.0, \n"\
"        1.0 / 256.0, 1.0 / 64.0, 3.0 / 128.0, 1.0 / 64.0, 1.0 / 256.0 }; \n"\
" \n"\
"    const int2 kernel_offsets[WAVELET_KERNEL_SIZE] = { \n"\
"        make_int2(-2, -2), make_int2(-1, -2), make_int2(0, -2),  make_int2(1, -2),  make_int2(2, -2), \n"\
"        make_int2(-2, -1), make_int2(-1, -1), make_int2(0, -2),  make_int2(1, -1),  make_int2(2, -1), \n"\
"        make_int2(-2, 0),  make_int2(-1, 0),  make_int2(0, 0),   make_int2(1, 0),   make_int2(2, 0), \n"\
"        make_int2(-2, 1),  make_int2(-1, 1),  make_int2(0, 1),   make_int2(1, 1),   make_int2(2, 1), \n"\
"        make_int2(-2, 2),  make_int2(-1, 2),  make_int2(0, 2),   make_int2(1, 2),   make_int2(2, 2) }; \n"\
"     \n"\
"    const int2 buffer_size  = make_int2(width, height); \n"\
"    const float2 uv = make_float2(global_id.x + 0.5f, global_id.y + 0.5f) / make_float2(width, height); \n"\
" \n"\
"    // From SVGF paper \n"\
"    const float sigma_variance = 4.0f; \n"\
" \n"\
"    // Check borders \n"\
"    if (global_id.x < width && global_id.y < height) \n"\
"    { \n"\
"        const int idx = global_id.y * width + global_id.x; \n"\
" \n"\
"        const float3 color = colors[idx].xyz; \n"\
"        const float3 position = positions[idx].xyz; \n"\
"        const float3 normal = normals[idx].xyz; \n"\
"        const float3 calbedo = albedo[idx].xyz / max(albedo[idx].w, 1.f); \n"\
" \n"\
"        const float variance = step_width == 1 ? sqrt(GaussFilter3x3(variances, buffer_size, global_id).z) : sqrt(variances[idx].z); \n"\
"        const float step_width_2 = (float)(step_width * step_width); \n"\
"         \n"\
"        float3 color_sum = color; // make_float3(0.0f, 0.0f, 0.0f); \n"\
"        float weight_sum = 1.f; \n"\
" \n"\
"        const float3 luminance = make_float3(0.2126f, 0.7152f, 0.0722f); \n"\
"        const float lum_color = dot(color, luminance); \n"\
" \n"\
"        if (length(position) > 0.f && !any(isnan(color))) \n"\
"        { \n"\
"            for (int i = 0; i < WAVELET_KERNEL_SIZE; i++) \n"\
"            { \n"\
"                const int cx = clamp(global_id.x + step_width * kernel_offsets[i].x, 0, width - 1); \n"\
"                const int cy = clamp(global_id.y + step_width * kernel_offsets[i].y, 0, height - 1); \n"\
"                const int ci = cy * width + cx; \n"\
" \n"\
"                const float3 sample_color       = colors[ci].xyz; \n"\
"                const float3 sample_normal      = normals[ci].xyz; \n"\
"                const float3 sample_position    = positions[ci].xyz; \n"\
"                const float3 sample_albedo      = albedo[ci].xyz / max(albedo[ci].w, 1.f); \n"\
" \n"\
"                const float3 delta_position     = position - sample_position; \n"\
"                const float3 delta_color        = calbedo - sample_albedo; \n"\
" \n"\
"                const float position_dist2      = dot(delta_position, delta_position) ; \n"\
"                const float color_dist2         = dot(delta_color, delta_color); \n"\
" \n"\
"                const float position_value     = exp(-position_dist2 / (sigma_position * 20.f)); \n"\
"                const float normal_value       = exp(-dot(sample_normal, normal) / 5.0f); \n"\
"                const float color_value        = exp(-color_dist2 / sigma_color); \n"\
" \n"\
"                const float position_weight     = isnan(position_value) ? 1.f : position_value; \n"\
"                const float normal_weight       = isnan(normal_value) ? 1.f : normal_value; \n"\
"                const float color_weight        = isnan(color_value) ? 1.f : color_value; \n"\
" \n"\
"                // Gives more sharp image then exp, but produces dark sillhoutes if color buffer contains more than 1 spp \n"\
"                //const float normal_weight       = pow(max(0.f, dot(sample_normal, normal)), 128.f); \n"\
" \n"\
"                const float lum_value           = step_width * exp(-fabs(lum_color - dot(luminance, sample_color)) / (sigma_variance * variance + DENOM_EPS)); \n"\
"                const float luminance_weight    = isnan(lum_value) ? 1.f : lum_value; \n"\
" \n"\
"                const float final_weight = color_weight * luminance_weight * normal_weight * position_weight * kernel_weights[i]; \n"\
" \n"\
"                color_sum   += final_weight * sample_color; \n"\
"                weight_sum  += final_weight; \n"\
"            } \n"\
" \n"\
"            out_colors[idx].xyz = color_sum / max(weight_sum, DENOM_EPS); \n"\
"            out_colors[idx].w = 1.f; \n"\
"        } \n"\
"        else \n"\
"        { \n"\
"            out_colors[idx].xyz = color; \n"\
"            out_colors[idx].w = 1.f; \n"\
"        } \n"\
"    } \n"\
"} \n"\
" \n"\
"KERNEL \n"\
"void WaveletGenerateMotionBuffer_main( \n"\
"    GLOBAL float4 const* restrict positions, \n"\
"    // Image resolution \n"\
"    int width, \n"\
"    int height, \n"\
"    // View-projection matrix of current frame \n"\
"    GLOBAL matrix4x4* restrict view_projection, \n"\
"    // View-projection matrix of previous frame \n"\
"    GLOBAL matrix4x4* restrict prev_view_projection, \n"\
"    // Resulting motion and depth \n"\
"    GLOBAL float4* restrict out_motion, \n"\
"    GLOBAL float4* restrict out_depth \n"\
") \n"\
"{ \n"\
"    int2 global_id; \n"\
"    global_id.x = get_global_id(0); \n"\
"    global_id.y = get_global_id(1); \n"\
"     \n"\
"    // Check borders \n"\
"    if (global_id.x < width && global_id.y < height) \n"\
"    { \n"\
"        const int idx = global_id.y * width + global_id.x; \n"\
"         \n"\
"        const float3 position_xyz = positions[idx].xyz / max(positions[idx].w, 1.f); \n"\
" \n"\
"        if (length(position_xyz) > 0) \n"\
"        { \n"\
"            const float4 position = make_float4(position_xyz.x, position_xyz.y, position_xyz.z, 1.0f); \n"\
" \n"\
"            float4 position_ps = matrix_mul_vector4(*view_projection, position); \n"\
"            float3 position_cs = position_ps.xyz / position_ps.w; \n"\
"            float2 position_ss = position_cs.xy * make_float2(0.5f, -0.5f) + make_float2(0.5f, 0.5f); \n"\
" \n"\
"            float4 prev_position_ps = matrix_mul_vector4(*prev_view_projection, position); \n"\
"            float2 prev_position_cs = prev_position_ps.xy / prev_position_ps.w; \n"\
"            float2 prev_position_ss = prev_position_cs * make_float2(0.5f, -0.5f) + make_float2(0.5f, 0.5f); \n"\
" \n"\
"            out_motion[idx] = (float4)(prev_position_ss - position_ss, 0.0f, 1.0f) * 2.0f; \n"\
"            out_depth[idx] = make_float4(0.0f, 0.0f, position_ps.w, 1.0f); \n"\
"        } \n"\
"        else \n"\
"        { \n"\
"            out_motion[idx] = make_float4(0.f, 0.f, 0.f, 1.f); \n"\
"        } \n"\
"    } \n"\
"} \n"\
" \n"\
"KERNEL \n"\
"void CopyBuffers_main( \n"\
"    // Input buffers \n"\
"    GLOBAL float4 const* restrict colors, \n"\
"    GLOBAL float4 const* restrict positions, \n"\
"    GLOBAL float4 const* restrict normals, \n"\
"    // Image resolution \n"\
"    int width, \n"\
"    int height, \n"\
"    // Output buffers \n"\
"    GLOBAL float4* restrict out_colors, \n"\
"    GLOBAL float4* restrict out_positions, \n"\
"    GLOBAL float4* restrict out_normals \n"\
") \n"\
"{ \n"\
"    int2 global_id; \n"\
"    global_id.x = get_global_id(0); \n"\
"    global_id.y = get_global_id(1); \n"\
" \n"\
"    // Check borders \n"\
"    if (global_id.x < width && global_id.y < height) \n"\
"    { \n"\
"        const int idx = global_id.y * width + global_id.x; \n"\
" \n"\
"        out_colors[idx] = (float4)(colors[idx].xyz / max(colors[idx].w,  1.f), 1.f); \n"\
"        out_positions[idx] = (float4)(positions[idx].xyz / max(positions[idx].w,  1.f), 1.f); \n"\
"        out_normals[idx] = (float4)(normals[idx].xyz / max(normals[idx].w,  1.f), 1.f); \n"\
"    } \n"\
"} \n"\
" \n"\
"KERNEL \n"\
"void CopyBuffer_main( \n"\
"    GLOBAL float4 const* restrict in_buffer, \n"\
"    GLOBAL float4* restrict out_buffer, \n"\
"    // Image resolution \n"\
"    int width, \n"\
"    int height \n"\
") \n"\
"{ \n"\
"    int2 global_id; \n"\
"    global_id.x = get_global_id(0); \n"\
"    global_id.y = get_global_id(1); \n"\
" \n"\
"    // Check borders \n"\
"    if (global_id.x < width && global_id.y < height) \n"\
"    { \n"\
"        const int idx = global_id.y * width + global_id.x; \n"\
"        out_buffer[idx] = in_buffer[idx]; \n"\
"    } \n"\
"} \n"\
" \n"\
"// Geometry consistency term - normal alignment test \n"\
"bool IsNormalConsistent(float3 nq, float3 np) \n"\
"{ \n"\
"    const float cos_pi_div_4  = cos(PI / 4.f); \n"\
"    return dot(nq, np) > cos_pi_div_4; \n"\
"} \n"\
" \n"\
"// Geometry consistency term - position consistency \n"\
"bool IsDepthConsistentPos( \n"\
"    GLOBAL float4 const* restrict buffer,  \n"\
"    GLOBAL float4 const* restrict prev_buffer,  \n"\
"    float2 uv,  \n"\
"    float2 motion,  \n"\
"    int2 buffer_size) \n"\
"{ \n"\
"    float2 uv_prev = uv + motion; \n"\
"     \n"\
"    float3 ddx = dFdx(buffer, uv, buffer_size).xyz * 4.f; \n"\
"    float3 ddy = dFdy(buffer, uv, buffer_size).xyz * 4.f; \n"\
" \n"\
"    float3 p0 = Sampler2DBilinear(buffer, buffer_size, uv).xyz; \n"\
"    float3 p1 = Sampler2DBilinear(prev_buffer, buffer_size, uv_prev).xyz; \n"\
" \n"\
"    return length(p1 - p0) < length(ddx) + length(ddy); \n"\
"} \n"\
" \n"\
" \n"\
" \n"\
"// Bilinear filtering with geometry test on each tap (resampling of previous color buffer) \n"\
"// TODO: Add depth test consistency \n"\
"float4 SampleWithGeometryTest(GLOBAL float4 const* restrict buffer, \n"\
"    float4 current_color, \n"\
"    float3 current_positions, \n"\
"    float3 current_normal, \n"\
"    GLOBAL float4 const* restrict positions, \n"\
"    GLOBAL float4 const* restrict normals, \n"\
"    GLOBAL float4 const* restrict prev_positions, \n"\
"    GLOBAL float4 const* restrict prev_normals, \n"\
"    int2 buffer_size, \n"\
"    float2 uv, \n"\
"    float2 uv_prev) \n"\
"{ \n"\
"    uv_prev.x = clamp(uv_prev.x, 0.f, 1.f); \n"\
"    uv_prev.y = clamp(uv_prev.y, 0.f, 1.f); \n"\
" \n"\
"    float2 scaled_uv = uv_prev * make_float2(buffer_size.x, buffer_size.y) - make_float2(0.5f, 0.5f); \n"\
" \n"\
"    int x = floor(scaled_uv.x); \n"\
"    int y = floor(scaled_uv.y); \n"\
" \n"\
"    const int2 offsets[4] = { \n"\
"        make_int2(x + 0, y + 0), \n"\
"        make_int2(x + 1, y + 0), \n"\
"        make_int2(x + 1, y + 1), \n"\
"        make_int2(x + 0, y + 1) \n"\
"    }; \n"\
" \n"\
"    const float3 normal_samples[4] = { \n"\
"        prev_normals[ConvertToLinearAddressInt2(offsets[0], buffer_size)].xyz, \n"\
"        prev_normals[ConvertToLinearAddressInt2(offsets[1], buffer_size)].xyz, \n"\
"        prev_normals[ConvertToLinearAddressInt2(offsets[2], buffer_size)].xyz, \n"\
"        prev_normals[ConvertToLinearAddressInt2(offsets[3], buffer_size)].xyz \n"\
"    }; \n"\
" \n"\
"    const bool is_normal_consistent[4] = { \n"\
"        IsNormalConsistent(current_normal, normal_samples[0]), \n"\
"        IsNormalConsistent(current_normal, normal_samples[1]), \n"\
"        IsNormalConsistent(current_normal, normal_samples[2]), \n"\
"        IsNormalConsistent(current_normal, normal_samples[3]) \n"\
"    }; \n"\
" \n"\
"    int num_consistent_samples = 0; \n"\
"     \n"\
"    for (int i = 0; i < 4; i++) num_consistent_samples += is_normal_consistent[i]  ? 1 : 0; \n"\
" \n"\
"    // Bilinear resample if all samples are consistent \n"\
"    if (num_consistent_samples == 4) \n"\
"    { \n"\
"        return Sampler2DBilinear(buffer, buffer_size, uv_prev); \n"\
"    } \n"\
" \n"\
"    const float4 buffer_samples[4] = { \n"\
"        buffer[ConvertToLinearAddressInt2(offsets[0], buffer_size)], \n"\
"        buffer[ConvertToLinearAddressInt2(offsets[1], buffer_size)], \n"\
"        buffer[ConvertToLinearAddressInt2(offsets[2], buffer_size)], \n"\
"        buffer[ConvertToLinearAddressInt2(offsets[3], buffer_size)] \n"\
"    }; \n"\
" \n"\
"    // Box filter otherwise \n"\
"    float weight = 1; \n"\
"    float4 sample = current_color; \n"\
" \n"\
"    for (int i = 0; i < 4; i++) \n"\
"    { \n"\
"        if (is_normal_consistent[i]) \n"\
"        { \n"\
"            sample += buffer_samples[i]; \n"\
"            weight += 1.f; \n"\
"        } \n"\
"    } \n"\
" \n"\
"    return sample / weight; \n"\
"} \n"\
" \n"\
"// Similarity function \n"\
"inline float C(float3 x1, float3 x2, float sigma) \n"\
"{ \n"\
"    float a = length(x1 - x2) / sigma; \n"\
"    a *= a; \n"\
"    return native_exp(-0.5f * a); \n"\
"} \n"\
" \n"\
"// Spatial bilateral variance estimation \n"\
"float4 BilateralVariance( \n"\
"    GLOBAL float4 const* restrict colors, \n"\
"    GLOBAL float4 const* restrict positions, \n"\
"    GLOBAL float4 const* restrict normals, \n"\
"    int2 global_id, \n"\
"    int2 buffer_size, \n"\
"    int kernel_size \n"\
") \n"\
"{ \n"\
"    const float3 luminance_weight = make_float3(0.2126f, 0.7152f, 0.0722f); \n"\
"     \n"\
"    float local_mean = 0.f; \n"\
"    float local_mean_2 = 0.f; \n"\
"    float sum_weight = 0.0f; \n"\
"     \n"\
"    const int idx = global_id.y * buffer_size.x + global_id.x; \n"\
" \n"\
"    float3 normal = normals[idx].w > 1 ? (normals[idx].xyz / normals[idx].w) : normals[idx].xyz; \n"\
"    float3 position = positions[idx].w > 1 ? (positions[idx].xyz / positions[idx].w) : positions[idx].xyz; \n"\
" \n"\
"    const int borders = kernel_size >> 1; \n"\
" \n"\
"    for (int i = -borders; i <= borders; ++i) \n"\
"    { \n"\
"        for (int j = -borders; j <= borders; ++j) \n"\
"        { \n"\
"            int cx = clamp(global_id.x + i, 0, buffer_size.x - 1); \n"\
"            int cy = clamp(global_id.y + j, 0, buffer_size.y - 1); \n"\
"            int ci = cy * buffer_size.x + cx; \n"\
" \n"\
"            float3 c = colors[ci].xyz / colors[ci].w; \n"\
"            float3 n = normals[ci].xyz / normals[ci].w; \n"\
"            float3 p = positions[ci].xyz / positions[ci].w; \n"\
"             \n"\
"            float sigma_position = 0.1f; \n"\
"            float sigma_normal = 0.1f; \n"\
" \n"\
"            if (length(p) > 0.f && !any(isnan(c))) \n"\
"            { \n"\
"                const float weight = C(p, position, sigma_position) * C(n, normal, sigma_normal); \n"\
"                const float luminance = dot(c, luminance_weight); \n"\
" \n"\
"                local_mean      += luminance * weight; \n"\
"                local_mean_2    += luminance * luminance * weight; \n"\
"                sum_weight      += weight; \n"\
"            } \n"\
"        } \n"\
"    } \n"\
"     \n"\
"    local_mean      = local_mean / max(DENOM_EPS, sum_weight); \n"\
"    local_mean_2    = local_mean_2 / max(DENOM_EPS, sum_weight);; \n"\
" \n"\
"    const float variance = local_mean_2 - local_mean * local_mean; \n"\
"     \n"\
"    return make_float4( local_mean, local_mean_2, variance, 1.f); \n"\
"} \n"\
" \n"\
"// Temporal accumulation of path tracer results and moments, spatio-temporal variance estimation \n"\
"KERNEL \n"\
"void TemporalAccumulation_main( \n"\
"    GLOBAL float4 const* restrict prev_colors, \n"\
"    GLOBAL float4 const* restrict prev_positions, \n"\
"    GLOBAL float4 const* restrict prev_normals, \n"\
"    GLOBAL float4* restrict in_out_colors, \n"\
"    GLOBAL float4 const*  restrict positions, \n"\
"    GLOBAL float4 const*  restrict normals, \n"\
"    GLOBAL float4 const* restrict motions, \n"\
"    GLOBAL float4 const* restrict prev_moments_and_variance, \n"\
"    GLOBAL float4* restrict moments_and_variance, \n"\
"    // Image resolution \n"\
"    int width, \n"\
"    int height \n"\
") \n"\
"{ \n"\
"    int2 global_id; \n"\
"    global_id.x = get_global_id(0); \n"\
"    global_id.y = get_global_id(1); \n"\
" \n"\
"    const int bilateral_filter_kernel_size = 7; \n"\
" \n"\
"    // Check borders \n"\
"    if (global_id.x < width && global_id.y < height) \n"\
"    { \n"\
"        const int idx = global_id.y * width + global_id.x; \n"\
"         \n"\
"        moments_and_variance[idx] = make_float4(0.f, 0.f, 0.f, 0.f); \n"\
" \n"\
"        const float3 position_xyz = positions[idx].xyz; \n"\
"        const float3 normal = normals[idx].xyz; \n"\
"        const float3 color = in_out_colors[idx].xyz; \n"\
" \n"\
"        if (length(position_xyz) > 0 && !any(isnan(color))) \n"\
"        { \n"\
"            const float2 motion     = motions[idx].xy; \n"\
"            const int2 buffer_size  = make_int2(width, height); \n"\
"       \n"\
"            const float2 uv = make_float2(global_id.x + 0.5f, global_id.y + 0.5f) / make_float2(width, height); \n"\
"            const float2 prev_uv = clamp(uv + motion, make_float2(0.f, 0.f), make_float2(1.f, 1.f)); \n"\
"             \n"\
"            const float3 prev_position_xyz  = Sampler2DBilinear(prev_positions, buffer_size, prev_uv).xyz; \n"\
"            const float3 prev_normal        = Sampler2DBilinear(prev_normals, buffer_size, prev_uv).xyz; \n"\
" \n"\
"            // Test for geometry consistency \n"\
"            if (length(prev_position_xyz) > 0 && IsDepthConsistentPos(positions, prev_positions, uv, motion, buffer_size) && IsNormalConsistent(prev_normal, normal)) \n"\
"            { \n"\
"                // Temporal accumulation of moments \n"\
"                const int prev_idx = global_id.y * width + global_id.x; \n"\
" \n"\
"                float4 prev_moments_and_variance_sample  = Sampler2DBilinear(prev_moments_and_variance, buffer_size, prev_uv); \n"\
" \n"\
"                const bool prev_moments_is_nan = any(isnan(prev_moments_and_variance_sample)); \n"\
" \n"\
"                float4 current_moments_and_variance_sample; \n"\
" \n"\
"                if (prev_moments_and_variance_sample.w < 4 || prev_moments_is_nan) \n"\
"                { \n"\
"                    // Not enought accumulated samples - get bilateral estimate \n"\
"                    current_moments_and_variance_sample = BilateralVariance(in_out_colors, positions, normals, global_id, buffer_size, bilateral_filter_kernel_size); \n"\
"                    prev_moments_and_variance_sample = prev_moments_is_nan ? current_moments_and_variance_sample : prev_moments_and_variance_sample; \n"\
"                } \n"\
"                else \n"\
"                { \n"\
"                    // Otherwise calculate moments for current color  \n"\
"                    const float3 luminance_weight = make_float3(0.2126f, 0.7152f, 0.0722f); \n"\
"                     \n"\
"                    const float first_moment = dot(in_out_colors[idx].xyz, luminance_weight); \n"\
"                    const float second_moment = first_moment * first_moment; \n"\
"                     \n"\
"                    current_moments_and_variance_sample = make_float4(first_moment, second_moment, 0.f, 1.f); \n"\
"                } \n"\
"                 \n"\
"                // Nan avoidance \n"\
"                if (any(isnan(prev_moments_and_variance_sample))) \n"\
"                { \n"\
"                    prev_moments_and_variance_sample = make_float4(0,0,0,1); \n"\
"                } \n"\
" \n"\
"                // Accumulate current and previous moments \n"\
"                moments_and_variance[idx].xy    = mix(prev_moments_and_variance_sample.xy, current_moments_and_variance_sample.xy, FRAME_BLEND_ALPHA); \n"\
"                moments_and_variance[idx].w     = 1; \n"\
" \n"\
"                const float mean          = moments_and_variance[idx].x; \n"\
"                const float mean_2        = moments_and_variance[idx].y; \n"\
"                 \n"\
"                moments_and_variance[idx].z     = mean_2 - mean * mean; \n"\
" \n"\
"                // Temporal accumulation of color \n"\
"                float3 prev_color = SampleWithGeometryTest(prev_colors, (float4)(color, 1.f), position_xyz, normal, positions, normals, prev_positions, prev_normals, buffer_size, uv, prev_uv).xyz; \n"\
" \n"\
"                in_out_colors[idx].xyz = mix(prev_color, color, FRAME_BLEND_ALPHA); \n"\
"                in_out_colors[idx].w = 1.0f; \n"\
"            } \n"\
"            else \n"\
"            { \n"\
"                // In case of disoclussion - calclulate variance by bilateral filter \n"\
"                //in_out_colors[idx].xyz = make_float3(1,0,0); \n"\
"                moments_and_variance[idx] = BilateralVariance(in_out_colors, positions, normals, global_id, buffer_size, bilateral_filter_kernel_size); \n"\
"            } \n"\
"        } \n"\
"    } \n"\
"} \n"\
" \n"\
"KERNEL \n"\
"void UpdateVariance_main( \n"\
"    // Color data \n"\
"    GLOBAL float4 const* restrict colors, \n"\
"    GLOBAL float4 const* restrict positions, \n"\
"    GLOBAL float4 const* restrict normals, \n"\
"    // Image resolution \n"\
"    int width, \n"\
"    int height, \n"\
"    GLOBAL float4* restrict out_variance \n"\
") \n"\
"{ \n"\
"    int2 global_id; \n"\
"    global_id.x = get_global_id(0); \n"\
"    global_id.y = get_global_id(1); \n"\
" \n"\
"    const int bilateral_filter_kernel_size = 3; \n"\
"    const int2 buffer_size  = make_int2(width, height); \n"\
" \n"\
"    // Check borders \n"\
"    if (global_id.x < width && global_id.y < height) \n"\
"    { \n"\
"        const int idx = global_id.y * width + global_id.x; \n"\
"        out_variance[idx] = BilateralVariance(colors, positions, normals, global_id, buffer_size, bilateral_filter_kernel_size); \n"\
"    }  \n"\
"} \n"\
"#endif \n"\
;
static const char* g_bxdf_opencl_inc[]= {
    g_payload_opencl,
    g_utils_opencl,
    g_texture_opencl,
    g_disney_opencl,
};
static const char* g_denoise_opencl_inc[]= {
    g_common_opencl,
};
static const char* g_disney_opencl_inc[]= {
    g_payload_opencl,
    g_utils_opencl,
    g_texture_opencl,
};
static const char* g_integrator_bdpt_opencl_inc[]= {
    g_common_opencl,
    g_ray_opencl,
    g_isect_opencl,
    g_payload_opencl,
    g_utils_opencl,
    g_texture_opencl,
    g_sampling_opencl,
    g_normalmap_opencl,
    g_disney_opencl,
    g_bxdf_opencl,
    g_scene_opencl,
    g_light_opencl,
    g_material_opencl,
    g_path_opencl,
    g_volumetrics_opencl,
    g_vertex_opencl,
};
static const char* g_light_opencl_inc[]= {
    g_payload_opencl,
    g_utils_opencl,
    g_texture_opencl,
    g_common_opencl,
    g_scene_opencl,
};
static const char* g_material_opencl_inc[]= {
    g_common_opencl,
    g_payload_opencl,
    g_utils_opencl,
    g_texture_opencl,
    g_disney_opencl,
    g_bxdf_opencl,
};
static const char* g_monte_carlo_renderer_opencl_inc[]= {
    g_common_opencl,
    g_ray_opencl,
    g_isect_opencl,
    g_payload_opencl,
    g_utils_opencl,
    g_texture_opencl,
    g_sampling_opencl,
    g_normalmap_opencl,
    g_disney_opencl,
    g_bxdf_opencl,
    g_scene_opencl,
    g_light_opencl,
    g_material_opencl,
    g_path_opencl,
    g_volumetrics_opencl,
    g_vertex_opencl,
};
static const char* g_normalmap_opencl_inc[]= {
    g_payload_opencl,
    g_utils_opencl,
    g_texture_opencl,
};
static const char* g_path_opencl_inc[]= {
    g_payload_opencl,
};
static const char* g_path_tracing_estimator_opencl_inc[]= {
    g_common_opencl,
    g_ray_opencl,
    g_isect_opencl,
    g_payload_opencl,
    g_utils_opencl,
    g_texture_opencl,
    g_sampling_opencl,
    g_normalmap_opencl,
    g_disney_opencl,
    g_bxdf_opencl,
    g_scene_opencl,
    g_light_opencl,
    g_material_opencl,
    g_path_opencl,
    g_volumetrics_opencl,
};
static const char* g_ray_opencl_inc[]= {
    g_common_opencl,
};
static const char* g_sampling_opencl_inc[]= {
    g_payload_opencl,
    g_utils_opencl,
};
static const char* g_scene_opencl_inc[]= {
    g_common_opencl,
    g_payload_opencl,
    g_utils_opencl,
};
static const char* g_texture_opencl_inc[]= {
    g_payload_opencl,
    g_utils_opencl,
};
static const char* g_utils_opencl_inc[]= {
    g_payload_opencl,
};
static const char* g_volumetrics_opencl_inc[]= {
    g_common_opencl,
    g_payload_opencl,
    g_path_opencl,
};
static const char* g_wavelet_denoise_opencl_inc[]= {
    g_common_opencl,
    g_payload_opencl,
    g_utils_opencl,
};
