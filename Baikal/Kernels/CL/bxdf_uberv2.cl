float3 UberV2_Evaluate(
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
    const float3 kd = Texture_GetValue3f(dg->mat.uberv2.diffuse_color.xyz, dg->uv, TEXTURE_ARGS_IDX(dg->mat.uberv2.diffuse_color_idx));

    float F = dg->mat.simple.fresnel;

    return F * kd / PI;
}

/// Lambert BRDF PDF
float UberV2_GetPdf(
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
float3 UberV2_Sample(
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
    const float3 kd = Texture_GetValue3f(dg->mat.uberv2.diffuse_color.xyz, dg->uv, TEXTURE_ARGS_IDX(dg->mat.uberv2.diffuse_color_idx));

    *wo = Sample_MapToHemisphere(sample, make_float3(0.f, 1.f, 0.f), 1.f);

    float F = dg->mat.simple.fresnel;

    *pdf = fabs((*wo).y) / PI;

    return F * kd / PI;
}
