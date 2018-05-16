#ifndef BXDF_BASIC_CL
#define BXDF_BASIC_CL
/*
Microfacet Beckmann
*/

// Distribution fucntion
float MicrofacetDistribution_Beckmann_D(float roughness, float3 m)
{
    float ndotm = fabs(m.y);
    float ndotm2 = ndotm * ndotm;
    float sinmn = native_sqrt(1.f - clamp(ndotm * ndotm, 0.f, 1.f));
    float tanmn = ndotm > DENOM_EPS ? sinmn / ndotm : 0.f;
    float a2 = roughness * roughness;

    return ndotm > DENOM_EPS ? (1.f / (PI * a2 * ndotm2 * ndotm2)) * native_exp(-tanmn * tanmn / a2) : 0.f;
}

// PDF of the given direction
float MicrofacetDistribution_Beckmann_GetPdf(
    // Rougness
    float roughness,
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
    // We need to convert pdf(wh)->pdf(wo)
    float3 m = normalize(wi + wo);
    float wodotm = fabs(dot(wo, m));

    float mpdf = MicrofacetDistribution_Beckmann_D(roughness, m) * fabs(m.y);
    // See Humphreys and Pharr for derivation
    return mpdf / (4.f * wodotm);
}

// Sample the distribution
void MicrofacetDistribution_Beckmann_SampleNormal(// Roughness
    float roughness,
    // Geometry
    DifferentialGeometry const* dg,
    // Texture args
    TEXTURE_ARG_LIST,
    // Sample
    float2 sample,
    // Outgoing  direction
    float3* wh
)
{
    float r1 = sample.x;
    float r2 = sample.y;

    // Sample halfway vector first, then reflect wi around that
    float theta = atan(native_sqrt(-roughness*roughness*native_log(max(DENOM_EPS, 1.f - r1))));
    float costheta = native_cos(theta);
    float sintheta = native_sin(theta);

    // phi = 2*PI*ksi2
    float phi = 2.f*PI*r2;
    float cosphi = native_cos(phi);
    float sinphi = native_sin(phi);

    // Reflect wi around wh
    *wh = make_float3(sintheta * cosphi, costheta, sintheta * sinphi);
}

float MicrofacetDistribution_Beckmann_G1(float roughness, float3 v, float3 m)
{
    float ndotv = fabs(v.y);
    float mdotv = fabs(dot(m, v));
    float sinnv = native_sqrt(1.f - clamp(ndotv * ndotv, 0.f, 1.f));
    float tannv = ndotv > DENOM_EPS ? sinnv / ndotv : 0.f;
    float a = tannv > DENOM_EPS ? 1.f / (roughness * tannv) : 0.f;
    float a2 = a * a;

    if (a > 1.6f)
        return 1.f;

    return (3.535f * a + 2.181f * a2) / (1.f + 2.276f * a + 2.577f * a2);
}

// Shadowing function also depends on microfacet distribution
float MicrofacetDistribution_Beckmann_G(float roughness, float3 wi, float3 wo, float3 wh)
{
    return MicrofacetDistribution_Beckmann_G1(roughness, wi, wh) * MicrofacetDistribution_Beckmann_G1(roughness, wo, wh);
}


float3 MicrofacetBeckmann_Evaluate(
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
    const float3 ks = Texture_GetValue3f(dg->mat.simple.kx.xyz, dg, TEXTURE_ARGS_IDX(dg->mat.simple.kxmapidx));
    const float roughness = Texture_GetValue1f(dg->mat.simple.ns, dg, TEXTURE_ARGS_IDX(dg->mat.simple.nsmapidx));
    const float eta = dg->mat.simple.ni;

    // Incident and reflected zenith angles
    float costhetao = fabs(wo.y);
    float costhetai = fabs(wi.y);

    // Calc halfway vector
    float3 wh = normalize(wi + wo);

    float F = dg->mat.simple.fresnel;

    float denom = 4.f * costhetao * costhetai;

    // F(eta) * D * G * ks / (4 * cosa * cosi)
    return denom > DENOM_EPS ? F * ks * MicrofacetDistribution_Beckmann_G(roughness, wi, wo, wh) * MicrofacetDistribution_Beckmann_D(roughness, wh) / denom : 0.f;
}


float MicrofacetBeckmann_GetPdf(
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
    const float roughness = Texture_GetValue1f(dg->mat.simple.ns, dg, TEXTURE_ARGS_IDX(dg->mat.simple.nsmapidx));
    return MicrofacetDistribution_Beckmann_GetPdf(roughness, dg, wi, wo, TEXTURE_ARGS);
}

float3 MicrofacetBeckmann_Sample(
    // Geometry
    DifferentialGeometry const* dg,
    // Incoming direction
    float3 wi,
    // Texture args
    TEXTURE_ARG_LIST,
    // Sample
    float2 sample,
    // Outgoing  direction
    float3* wo,
    // PDF at wo
    float* pdf
)
{
    const float roughness = Texture_GetValue1f(dg->mat.simple.ns, dg, TEXTURE_ARGS_IDX(dg->mat.simple.nsmapidx));

    float3 wh;
    MicrofacetDistribution_Beckmann_SampleNormal(roughness, dg, TEXTURE_ARGS, sample, &wh);

    *wo = -wi + 2.f*fabs(dot(wi, wh)) * wh;

    *pdf = MicrofacetDistribution_Beckmann_GetPdf(roughness, dg, wi, *wo, TEXTURE_ARGS);

    return MicrofacetBeckmann_Evaluate(dg, wi, *wo, TEXTURE_ARGS);
}

/*
Microfacet GGX
*/
// Distribution fucntion
float MicrofacetDistribution_GGX_D(float roughness, float3 m)
{
    float ndotm = fabs(m.y);
    float ndotm2 = ndotm * ndotm;
    float sinmn = native_sqrt(1.f - clamp(ndotm * ndotm, 0.f, 1.f));
    float tanmn = ndotm > DENOM_EPS ? sinmn / ndotm : 0.f;
    float a2 = roughness * roughness;
    float denom = (PI * ndotm2 * ndotm2 * (a2 + tanmn * tanmn) * (a2 + tanmn * tanmn));
    return denom > DENOM_EPS ? (a2 / denom) : 1.f;
}

// PDF of the given direction
float MicrofacetDistribution_GGX_GetPdf(
    // Halfway vector
    float3 m,
    // Rougness
    float roughness,
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
    float mpdf = MicrofacetDistribution_GGX_D(roughness, m) * fabs(m.y);
    // See Humphreys and Pharr for derivation
    float denom = (4.f * fabs(dot(wo, m)));

    return denom > DENOM_EPS ? mpdf / denom : 0.f;
}

// Sample the distribution
void MicrofacetDistribution_GGX_SampleNormal(
    // Roughness
    float roughness,
    // Differential geometry
    DifferentialGeometry const* dg,
    // Texture args
    TEXTURE_ARG_LIST,
    // Sample
    float2 sample,
    // Outgoing  direction
    float3* wh
)
{
    float r1 = sample.x;
    float r2 = sample.y;

    // Sample halfway vector first, then reflect wi around that
    float theta = atan2(roughness * native_sqrt(r1), native_sqrt(1.f - r1));
    float costheta = native_cos(theta);
    float sintheta = native_sin(theta);

    // phi = 2*PI*ksi2
    float phi = 2.f * PI * r2;
    float cosphi = native_cos(phi);
    float sinphi = native_sin(phi);

    // Calculate wh
    *wh = make_float3(sintheta * cosphi, costheta, sintheta * sinphi);
}

//
float MicrofacetDistribution_GGX_G1(float roughness, float3 v, float3 m)
{
    float ndotv = fabs(v.y);
    float mdotv = fabs(dot(m, v));

    float sinnv = native_sqrt(1.f - clamp(ndotv * ndotv, 0.f, 1.f));
    float tannv = ndotv > DENOM_EPS ? sinnv / ndotv : 0.f;
    float a2 = roughness * roughness;
    return 2.f / (1.f + native_sqrt(1.f + a2 * tannv * tannv));
}

// Shadowing function also depends on microfacet distribution
float MicrofacetDistribution_GGX_G(float roughness, float3 wi, float3 wo, float3 wh)
{
    return MicrofacetDistribution_GGX_G1(roughness, wi, wh) * MicrofacetDistribution_GGX_G1(roughness, wo, wh);
}

float3 MicrofacetGGX_Evaluate(
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
    const float3 ks = Texture_GetValue3f(dg->mat.simple.kx.xyz, dg, TEXTURE_ARGS_IDX(dg->mat.simple.kxmapidx));
    const float roughness = Texture_GetValue1f(dg->mat.simple.ns, dg, TEXTURE_ARGS_IDX(dg->mat.simple.nsmapidx));

    // Incident and reflected zenith angles
    float costhetao = fabs(wo.y);
    float costhetai = fabs(wi.y);

    // Calc halfway vector
    float3 wh = normalize(wi + wo);

    float F = dg->mat.simple.fresnel;

    float denom = (4.f * costhetao * costhetai);

    return denom > DENOM_EPS ? F * ks * MicrofacetDistribution_GGX_G(roughness, wi, wo, wh) * MicrofacetDistribution_GGX_D(roughness, wh) / denom : 0.f;
}


float MicrofacetGGX_GetPdf(
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
    const float roughness = Texture_GetValue1f(dg->mat.simple.ns, dg, TEXTURE_ARGS_IDX(dg->mat.simple.nsmapidx));

    float3 wh = normalize(wo + wi);

    return MicrofacetDistribution_GGX_GetPdf(wh, roughness, dg, wi, wo, TEXTURE_ARGS);
}

float3 MicrofacetGGX_Sample(
    // Geometry
    DifferentialGeometry const* dg,
    // Incoming direction
    float3 wi,
    // Texture args
    TEXTURE_ARG_LIST,
    // Sample
    float2 sample,
    // Outgoing  direction
    float3* wo,
    // PDF at wo
    float* pdf
)
{
    const float roughness = Texture_GetValue1f(dg->mat.simple.ns, dg, TEXTURE_ARGS_IDX(dg->mat.simple.nsmapidx));

    float3 wh;
    MicrofacetDistribution_GGX_SampleNormal(roughness, dg, TEXTURE_ARGS, sample, &wh);

    *wo = -wi + 2.f*fabs(dot(wi, wh)) * wh;

    *pdf = MicrofacetDistribution_GGX_GetPdf(wh, roughness, dg, wi, *wo, TEXTURE_ARGS);

    return MicrofacetGGX_Evaluate(dg, wi, *wo, TEXTURE_ARGS);
}


/*
Lambert BRDF
*/
/// Lambert BRDF evaluation
float3 Lambert_Evaluate(
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
    const float3 kd = Texture_GetValue3f(dg->mat.simple.kx.xyz, dg, TEXTURE_ARGS_IDX(dg->mat.simple.kxmapidx));

    float F = dg->mat.simple.fresnel;

    return F * kd / PI;
}

/// Lambert BRDF PDF
float Lambert_GetPdf(
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
    return fabs(wo.y) / PI;
}

/// Lambert BRDF sampling
float3 Lambert_Sample(
    // Geometry
    DifferentialGeometry const* dg,
    // Incoming direction
    float3 wi,
    // Texture args
    TEXTURE_ARG_LIST,
    // Sample
    float2 sample,
    // Outgoing  direction
    float3* wo,
    // PDF at wo
    float* pdf
)
{
    const float3 kd = Texture_GetValue3f(dg->mat.simple.kx.xyz, dg, TEXTURE_ARGS_IDX(dg->mat.simple.kxmapidx));

    *wo = Sample_MapToHemisphere(sample, make_float3(0.f, 1.f, 0.f), 1.f);

    float F = dg->mat.simple.fresnel;

    *pdf = fabs((*wo).y) / PI;

    return F * kd / PI;
}

/*
Ideal reflection BRDF
*/
float3 IdealReflect_Evaluate(
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
    return 0.f;
}

/// Lambert BRDF sampling
float3 Translucent_Sample(
    // Geometry
    DifferentialGeometry const* dg,
    // Incoming direction
    float3 wi,
    // Texture args
    TEXTURE_ARG_LIST,
    // Sample
    float2 sample,
    // Outgoing  direction
    float3* wo,
    // PDF at wo
    float* pdf
)
{
    const float3 kd = Texture_GetValue3f(dg->mat.simple.kx.xyz, dg, TEXTURE_ARGS_IDX(dg->mat.simple.kxmapidx));

    float ndotwi = wi.y;

    float3 n = ndotwi > DENOM_EPS ? make_float3(0.f, -1.f, 0.f) : make_float3(0.f, 1.f, 0.f);

    *wo = normalize(Sample_MapToHemisphere(sample, n, 1.f));

    *pdf = fabs((*wo).y) / PI;

    return kd / PI;
}

// Lambert BRDF evaluation
float3 Translucent_Evaluate(
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
    const float3 kd = Texture_GetValue3f(dg->mat.simple.kx.xyz, dg, TEXTURE_ARGS_IDX(dg->mat.simple.kxmapidx));

    float ndotwi = wi.y;
    float ndotwo = wo.y;

    if (ndotwi * ndotwo > 0.f)
        return 0.f;

    return kd / PI;
}

/// Lambert BRDF PDF
float Translucent_GetPdf(
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
    float ndotwi = wi.y;
    float ndotwo = wo.y;

    if (ndotwi * ndotwo > 0)
        return 0.f;

    return fabs(ndotwo) / PI;
}

float IdealReflect_GetPdf(
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
    return 0.f;
}

float3 IdealReflect_Sample(
    // Geometry
    DifferentialGeometry const* dg,
    // Incoming direction
    float3 wi,
    // Texture args
    TEXTURE_ARG_LIST,
    // Sample
    float2 sample,
    // Outgoing  direction
    float3* wo,
    // PDF at wo
    float* pdf
)
{
    const float3 ks = Texture_GetValue3f(dg->mat.simple.kx.xyz, dg, TEXTURE_ARGS_IDX(dg->mat.simple.kxmapidx));
    const float eta = dg->mat.simple.ni;

    // Mirror reflect wi
    *wo = normalize(make_float3(-wi.x, wi.y, -wi.z));

    // PDF is infinite at that point, but deltas are going to cancel out while evaluating
    // so set it to 1.f
    *pdf = 1.f;

    float F = dg->mat.simple.fresnel;

    float coswo = fabs((*wo).y);

    // Return reflectance value
    return coswo > DENOM_EPS ? (F * ks * (1.f / coswo)) : 0.f;
}

/*
Ideal refraction BTDF
*/

float3 IdealRefract_Evaluate(
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
    return 0.f;
}

float IdealRefract_GetPdf(
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
    return 0.f;
}

float3 IdealRefract_Sample(
    // Geometry
    DifferentialGeometry const* dg,
    // Incoming direction
    float3 wi,
    // Texture args
    TEXTURE_ARG_LIST,
    // Sample
    float2 sample,
    // Outgoing  direction
    float3* wo,
    // PDF at wo
    float* pdf
)
{
    const float3 ks = Texture_GetValue3f(dg->mat.simple.kx.xyz, dg, TEXTURE_ARGS_IDX(dg->mat.simple.kxmapidx));

    float etai = 1.f;
    float etat = dg->mat.simple.ni;
    float cosi = wi.y;

    bool entering = cosi > 0.f;

    // Revert normal and eta if needed
    if (!entering)
    {
        float tmp = etai;
        etai = etat;
        etat = tmp;
    }

    float eta = etai / etat;
    float sini2 = 1.f - cosi * cosi;
    float sint2 = eta * eta * sini2;

    if (sint2 >= 1.f)
    {
        *pdf = 0.f;
        return 0.f;
    }

    float cost = native_sqrt(max(0.f, 1.f - sint2));

    // Transmitted ray
    float F = dg->mat.simple.fresnel;

    *wo = normalize(make_float3(eta * -wi.x, entering ? -cost : cost, eta * -wi.z));

    *pdf = 1.f;

    return cost > DENOM_EPS ? (F * eta * eta * ks / cost) : 0.f;
}


float3 MicrofacetRefractionGGX_Evaluate(
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
    const float3 ks = Texture_GetValue3f(dg->mat.simple.kx.xyz, dg, TEXTURE_ARGS_IDX(dg->mat.simple.kxmapidx));
    const float roughness = max(Texture_GetValue1f(dg->mat.simple.ns, dg, TEXTURE_ARGS_IDX(dg->mat.simple.nsmapidx)), ROUGHNESS_EPS);

    float ndotwi = wi.y;
    float ndotwo = wo.y;

    if (ndotwi * ndotwo >= 0.f)
    {
        return 0.f;
    }

    float etai = 1.f;
    float etat = dg->mat.simple.ni;

    // Revert normal and eta if needed
    if (ndotwi < 0.f)
    {
        float tmp = etai;
        etai = etat;
        etat = tmp;
    }

    // Calc halfway vector
    float3 ht = -(etai * wi + etat * wo);
    float3 wh = normalize(ht);

    float widotwh = fabs(dot(wh, wi));
    float wodotwh = fabs(dot(wh, wo));

    float F = dg->mat.simple.fresnel;

    float denom = dot(ht, ht);
    denom *= (fabs(ndotwi) * fabs(ndotwo));

    return denom > DENOM_EPS ? (F * ks * (widotwh * wodotwh)  * (etat)* (etat)*
        MicrofacetDistribution_GGX_G(roughness, wi, wo, wh) * MicrofacetDistribution_GGX_D(roughness, wh) / denom) : 0.f;
}



float MicrofacetRefractionGGX_GetPdf(
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
    const float roughness = max(Texture_GetValue1f(dg->mat.simple.ns, dg, TEXTURE_ARGS_IDX(dg->mat.simple.nsmapidx)), ROUGHNESS_EPS);
    float ndotwi = wi.y;
    float ndotwo = wo.y;

    float etai = 1.f;
    float etat = dg->mat.simple.ni;

    if (ndotwi * ndotwo >= 0.f)
    {
        return 0.f;
    }

    // Revert normal and eta if needed
    if (ndotwi < 0.f)
    {
        float tmp = etai;
        etai = etat;
        etat = tmp;
    }

    // Calc halfway vector
    float3 ht = -(etai * wi + etat * wo);

    float3 wh = normalize(ht);

    float wodotwh = fabs(dot(wo, wh));

    float whpdf = MicrofacetDistribution_GGX_D(roughness, wh) * fabs(wh.y);

    float whwo = wodotwh * etat * etat;

    float denom = dot(ht, ht);

    return denom > DENOM_EPS ? whpdf * whwo / denom : 0.f;
}

float3 MicrofacetRefractionGGX_Sample(
    // Geometry
    DifferentialGeometry const* dg,
    // Incoming direction
    float3 wi,
    // Texture args
    TEXTURE_ARG_LIST,
    // Sample
    float2 sample,
    // Outgoing  direction
    float3* wo,
    // PDF at wo
    float* pdf
)
{
    const float3 ks = Texture_GetValue3f(dg->mat.simple.kx.xyz, dg, TEXTURE_ARGS_IDX(dg->mat.simple.kxmapidx));
    const float roughness = max(Texture_GetValue1f(dg->mat.simple.ns, dg, TEXTURE_ARGS_IDX(dg->mat.simple.nsmapidx)), ROUGHNESS_EPS);

    float ndotwi = wi.y;

    if (ndotwi == 0.f)
    {
        *pdf = 0.f;
        return 0.f;
    }

    float etai = 1.f;
    float etat = dg->mat.simple.ni;
    float s = 1.f;

    // Revert normal and eta if needed
    if (ndotwi < 0.f)
    {
        float tmp = etai;
        etai = etat;
        etat = tmp;
        s = -s;
    }

    float3 wh;
    MicrofacetDistribution_GGX_SampleNormal(roughness, dg, TEXTURE_ARGS, sample, &wh);

    float c = dot(wi, wh);
    float eta = etai / etat;

    float d = 1 + eta * (c * c - 1);

    if (d <= 0.f)
    {
        *pdf = 0.f;
        return 0.f;
    }

    *wo = normalize((eta * c - s * native_sqrt(d)) * wh - eta * wi);

    *pdf = MicrofacetRefractionGGX_GetPdf(dg, wi, *wo, TEXTURE_ARGS);

    return MicrofacetRefractionGGX_Evaluate(dg, wi, *wo, TEXTURE_ARGS);
}

float3 MicrofacetRefractionBeckmann_Evaluate(
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
    const float3 ks = Texture_GetValue3f(dg->mat.simple.kx.xyz, dg, TEXTURE_ARGS_IDX(dg->mat.simple.kxmapidx));
    const float roughness = max(Texture_GetValue1f(dg->mat.simple.ns, dg, TEXTURE_ARGS_IDX(dg->mat.simple.nsmapidx)), ROUGHNESS_EPS);

    float ndotwi = wi.y;
    float ndotwo = wo.y;

    float etai = 1.f;
    float etat = dg->mat.simple.ni;

    // Revert normal and eta if needed
    if (ndotwi < 0.f)
    {
        float tmp = etai;
        etai = etat;
        etat = tmp;
    }

    // Calc halfway vector
    float3 ht = -(etai * wi + etat * wo);
    float3 wh = normalize(ht);

    float widotwh = fabs(dot(wh, wi));
    float wodotwh = fabs(dot(wh, wo));

    float F = dg->mat.simple.fresnel;

    float denom = dot(ht, ht);
    denom *= (fabs(ndotwi) * fabs(ndotwo));

    return denom > DENOM_EPS ? (F * ks * (widotwh * wodotwh)  * (etat)* (etat)*
        MicrofacetDistribution_Beckmann_G(roughness, wi, wo, wh) * MicrofacetDistribution_Beckmann_D(roughness, wh) / denom) : 0.f;
}



float MicrofacetRefractionBeckmann_GetPdf(
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
    const float roughness = Texture_GetValue1f(dg->mat.simple.ns, dg, TEXTURE_ARGS_IDX(dg->mat.simple.nsmapidx));
    float ndotwi = wi.y;
    float ndotwo = wo.y;

    float etai = 1.f;
    float etat = dg->mat.simple.ni;

    // Revert normal and eta if needed
    if (ndotwi < 0.f)
    {
        float tmp = etai;
        etai = etat;
        etat = tmp;
    }

    // Calc halfway vector
    float3 ht = -(etai * wi + etat * wo);

    float3 wh = normalize(ht);

    float wodotwh = fabs(dot(wo, wh));

    float whpdf = MicrofacetDistribution_Beckmann_D(roughness, wh) * fabs(wh.y);

    float whwo = wodotwh * etat * etat;

    float denom = dot(ht, ht);

    return denom > DENOM_EPS ? whpdf * whwo / denom : 0.f;
}

float3 MicrofacetRefractionBeckmann_Sample(
    // Geometry
    DifferentialGeometry const* dg,
    // Incoming direction
    float3 wi,
    // Texture args
    TEXTURE_ARG_LIST,
    // Sample
    float2 sample,
    // Outgoing  direction
    float3* wo,
    // PDF at wo
    float* pdf
)
{
    const float3 ks = Texture_GetValue3f(dg->mat.simple.kx.xyz, dg, TEXTURE_ARGS_IDX(dg->mat.simple.kxmapidx));
    const float roughness = Texture_GetValue1f(dg->mat.simple.ns, dg, TEXTURE_ARGS_IDX(dg->mat.simple.nsmapidx));

    float ndotwi = wi.y;

    float etai = 1.f;
    float etat = dg->mat.simple.ni;
    float s = 1.f;

    // Revert normal and eta if needed
    if (ndotwi < 0.f)
    {
        float tmp = etai;
        etai = etat;
        etat = tmp;
        s = -s;
    }

    float3 wh;
    MicrofacetDistribution_Beckmann_SampleNormal(roughness, dg, TEXTURE_ARGS, sample, &wh);

    float c = dot(wi, wh);
    float eta = etai / etat;

    float d = 1 + eta * (c * c - 1);

    if (d <= 0)
    {
        *pdf = 0.f;
        return 0.f;
    }

    *wo = normalize((eta * c - s * native_sqrt(d)) * wh - eta * wi);

    *pdf = MicrofacetRefractionBeckmann_GetPdf(dg, wi, *wo, TEXTURE_ARGS);

    return MicrofacetRefractionBeckmann_Evaluate(dg, wi, *wo, TEXTURE_ARGS);
}

float3 Passthrough_Sample(
    // Geometry
    DifferentialGeometry const* dg,
    // Incoming direction
    float3 wi,
    // Texture args
    TEXTURE_ARG_LIST,
    // Sample
    float2 sample,
    // Outgoing  direction
    float3* wo,
    // PDF at wo
    float* pdf
)
{

    *wo = -wi;
    float coswo = fabs((*wo).y);

    // PDF is infinite at that point, but deltas are going to cancel out while evaluating
    // so set it to 1.f
    *pdf = 1.f;

    //
    return coswo > 1e-5f ? (1.f / coswo) : 0.f;
}

#endif
