#include <../Baikal/Kernels/CL/bxdf_uberv2_bricks.cl>

float CalculateFresnel(
    float ndotwi,
    // Incoming direction
    float3 wi,
    // Geometry
    DifferentialGeometry const* dg,
    float top_ior,
    float bottom_ior
)
{
/*    if (ndotwi < 0.f && dg->mat.thin)
    {
        ndotwi = -ndotwi;
    }*/

    float etai =  top_ior;
    float etat =  bottom_ior;
    float cosi = ndotwi;

    // Revert normal and eta if needed
    if (cosi < 0.f)
    {
        float tmp = etai;
        etai = etat;
        etat = tmp;
        cosi = -cosi;
    }

    float eta = etai / etat;
    float sini2 = 1.f - cosi * cosi;
    float sint2 = eta * eta * sini2;
    float fresnel = 1.f;

    if (sint2 < 1.f)
    {
        float cost = native_sqrt(max(0.f, 1.f - sint2));
        fresnel = FresnelDielectric(etai, etat, cosi, cost);
    }

    return fresnel;
}

void GetMaterialBxDFType(
    // Incoming direction
    float3 wi,
    // Sampler
    Sampler* sampler,
    // Sampler args
    SAMPLER_ARG_LIST,
    // Geometry
    DifferentialGeometry* dg
)
{
    dg->mat.bxdf_flags = 0;
    if ((dg->mat.uberv2.layers & kEmissionLayer) == kEmissionLayer) dg->mat.bxdf_flags = kBxdfEmissive;

    if ((dg->mat.uberv2.layers & kTransparencyLayer) == kTransparencyLayer)
    {
        float sample = Sampler_Sample1D(sampler, SAMPLER_ARGS);
        if (sample < dg->mat.uberv2.transparency)
        {
            dg->mat.bxdf_flags |= (kBxdfTransparency | kBxdfSingular);
            return;
        }
    }
    
    const bxdf_type = (dg->mat.uberv2.layers & (kCoatingLayer | kReflectionLayer | kRefractionLayer | kDiffuseLayer));
    const float ndotwi = dot(dg->n, wi);

    //No refraction
    if ((bxdf_type & kRefractionLayer) != kRefractionLayer)
    {
        dg->mat.bxdf_flags |= kBxdfBrdf;
        float top_ior = 1.0f;
        if ((bxdf_type & kCoatingLayer) == kCoatingLayer)
        {
            float sample = Sampler_Sample1D(sampler, SAMPLER_ARGS);
            const float fresnel = CalculateFresnel(ndotwi, wi, dg, top_ior, dg->mat.uberv2.coating_ior);
            if (sample < fresnel) //Will sample coating layer 
            {
                dg->mat.bxdf_flags |= kBxdfSingular;
                dg->mat.bxdf_flags |= kBxdfSampleCoating;
                return;
            }
            top_ior = dg->mat.uberv2.coating_ior;
        }

        if ((bxdf_type & kReflectionLayer) == kReflectionLayer)
        {
            const float fresnel = CalculateFresnel(ndotwi, wi, dg, top_ior, dg->mat.uberv2.reflection_ior);
            float sample = Sampler_Sample1D(sampler, SAMPLER_ARGS);
            if (sample < fresnel) //Will sample reflection layer 
            {
                if ((dg->mat.uberv2.reflection_roughness_idx == -1) && (dg->mat.uberv2.reflection_roughness < ROUGHNESS_EPS))
                    dg->mat.bxdf_flags |= kBxdfSingular;
                dg->mat.bxdf_flags |= kBxdfSampleReflection;
                return;
            }
        }
        dg->mat.bxdf_flags |= kBxdfSampleDiffuse;
        return;
    }
    else
    {
        float top_ior = 1.0f;
        if ((bxdf_type & kCoatingLayer) == kCoatingLayer)
        {
            const float fresnel = CalculateFresnel(ndotwi, wi, dg, top_ior, dg->mat.uberv2.coating_ior);
            float sample = Sampler_Sample1D(sampler, SAMPLER_ARGS);
            if (sample < fresnel) //Will sample coating layer 
            {
                dg->mat.bxdf_flags |= kBxdfBrdf;
                dg->mat.bxdf_flags |= kBxdfSingular;
                dg->mat.bxdf_flags |= kBxdfSampleCoating;
                return;
            }
            top_ior = dg->mat.uberv2.coating_ior;
        }

        if ((bxdf_type & kReflectionLayer) == kReflectionLayer)
        {
            const float fresnel = CalculateFresnel(ndotwi, wi, dg, top_ior, dg->mat.uberv2.reflection_ior);
            float sample = Sampler_Sample1D(sampler, SAMPLER_ARGS);
            if (sample < fresnel) //Will sample reflection layer 
            {
                dg->mat.bxdf_flags |= kBxdfBrdf;
                dg->mat.bxdf_flags |= kBxdfSampleReflection;
                if ((dg->mat.uberv2.reflection_roughness_idx == -1) && (dg->mat.uberv2.reflection_roughness < ROUGHNESS_EPS))
                    dg->mat.bxdf_flags |= kBxdfSingular;
                return;
            }
            top_ior = dg->mat.uberv2.reflection_ior;
        }

        //If we come here - we need to check if we sample lambert or refraction
        float refraction_fresnel = CalculateFresnel(ndotwi, wi, dg, top_ior, dg->mat.uberv2.refraction_ior);
        float sample = Sampler_Sample1D(sampler, SAMPLER_ARGS);
        if (sample < refraction_fresnel)
        {
            dg->mat.bxdf_flags |= kBxdfBrdf;
            dg->mat.bxdf_flags |= kBxdfSampleDiffuse;
        }
        else
        {
            dg->mat.bxdf_flags |= kBxdfSampleRefraction;
            if ((dg->mat.uberv2.refraction_roughness_idx == -1) && (dg->mat.uberv2.refraction_roughness < ROUGHNESS_EPS))
                dg->mat.bxdf_flags |= kBxdfSingular;
        }
    }
}

float3 Fresnel_Blend(
    // IORs
    float top_ior,
    float bottom_ior,
    // Values to blend
    float3 top_value,
    float3 bottom_value,
    // Incoming direction
    float3 wi,
    // Geometry
    DifferentialGeometry const* dg)
{
    float fresnel = CalculateFresnel(wi.y, wi, dg, top_ior, bottom_ior);
    return fresnel * top_value + (1.f - fresnel) * bottom_value;
}

float Fresnel_Blend_F(
    // IORs
    float top_ior,
    float bottom_ior,
    // Values to blend
    float top_value,
    float bottom_value,
    // Incoming direction
    float3 wi,
    // Geometry
    DifferentialGeometry const* dg)
{
    float fresnel = CalculateFresnel(wi.y, wi, dg, top_ior, bottom_ior);
    return fresnel * top_value + (1.f - fresnel) * bottom_value;
}

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
    int layers = dg->mat.uberv2.layers;
    
    int fresnel_blend_layers = popcount(layers & (kCoatingLayer | kReflectionLayer | kDiffuseLayer | kRefractionLayer));
    
    if (fresnel_blend_layers == 0) return 0.0f;
    float3 result;
    //Check if we have single layer material or not
    if (fresnel_blend_layers == 1)
    {
        if ((layers & kCoatingLayer) == kCoatingLayer)
            result = UberV2_IdealReflect_Evaluate(dg, wi, wo, TEXTURE_ARGS);
        else if ((layers & kReflectionLayer) == kReflectionLayer)
            result = UberV2_Reflection_Evaluate(dg, wi, wo, TEXTURE_ARGS);
        else if ((layers & kRefractionLayer) == kRefractionLayer)
            result = UberV2_Refraction_Evaluate(dg, wi, wo, TEXTURE_ARGS);
        else if ((layers & kDiffuseLayer) == kDiffuseLayer)
            result = UberV2_Lambert_Evaluate(dg, wi, wo, TEXTURE_ARGS);

        if ((layers & kTransparencyLayer) == kTransparencyLayer)
        {
            result = mix(result, 0.f, dg->mat.uberv2.transparency);
        }

        return result;
    }

    float iors[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    float3 values[4];
    int id = 0;

    iors[id] = 1.0f;

    if ((layers & kCoatingLayer) == kCoatingLayer)
    {
        iors[id + 1] = dg->mat.uberv2.coating_ior;
        values[id] = UberV2_IdealReflect_Evaluate(dg, wi, wo, TEXTURE_ARGS);
        ++id;
    }

    if ((layers & kReflectionLayer) == kCoatingLayer)
    {
        iors[id + 1] = dg->mat.uberv2.reflection_ior;
        values[id] = UberV2_Reflection_Evaluate(dg, wi, wo, TEXTURE_ARGS);
        ++id;
    }

    if ((layers & kDiffuseLayer) == kDiffuseLayer)
    {
        values[id] = UberV2_Lambert_Evaluate(dg, wi, wo, TEXTURE_ARGS);
        ++id;
    }

    if ((layers & kRefractionLayer) == kRefractionLayer)
    {
        iors[id] = dg->mat.uberv2.refraction_ior;
        values[id] = UberV2_Refraction_Evaluate(dg, wi, wo, TEXTURE_ARGS);
    }

    result = values[fresnel_blend_layers - 1];
    for (int a = fresnel_blend_layers - 1; a > 0; --a)
    {
        result = Fresnel_Blend(iors[a - 1], iors[a], values[a - 1], result, wi, dg);
    }

    if ((layers & kTransparencyLayer) == kTransparencyLayer)
    {
        result = mix(result, 0.f, dg->mat.uberv2.transparency);
        return 0.f;
    }


    return result;
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
    const int layers = dg->mat.uberv2.layers;

    const int fresnel_blend_layers = popcount(layers & (kCoatingLayer | kReflectionLayer | kDiffuseLayer | kRefractionLayer));

    if (fresnel_blend_layers == 0) return 0.0f;

    float result = 0.0f;

    //Check if we have single layer material or not
    if (fresnel_blend_layers == 1)
    {
        if ((layers & kCoatingLayer) == kCoatingLayer)
            result = UberV2_IdealReflect_GetPdf(dg, wi, wo, TEXTURE_ARGS);
        else if ((layers & kReflectionLayer) == kReflectionLayer)
            result = UberV2_Reflection_GetPdf(dg, wi, wo, TEXTURE_ARGS);
        else if ((layers & kRefractionLayer) == kRefractionLayer)
            result = UberV2_Refraction_GetPdf(dg, wi, wo, TEXTURE_ARGS);
        else if ((layers & kDiffuseLayer) == kDiffuseLayer)
            result = UberV2_Lambert_GetPdf(dg, wi, wo, TEXTURE_ARGS);

        if ((layers & kTransparencyLayer) == kTransparencyLayer)
        {
            result = mix(result, 0.f, dg->mat.uberv2.transparency);
        }

        return result;
    }

    float iors[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    float values[4];
    int id = 0;

    iors[id] = 1.0f;

    if ((layers & kCoatingLayer) == kCoatingLayer)
    {
        iors[id + 1] = dg->mat.uberv2.coating_ior;
        values[id] = UberV2_IdealReflect_GetPdf(dg, wi, wo, TEXTURE_ARGS);
        ++id;
    }

    if ((layers & kReflectionLayer) == kCoatingLayer)
    {
        iors[id + 1] = dg->mat.uberv2.reflection_ior;
        values[id] = UberV2_Reflection_GetPdf(dg, wi, wo, TEXTURE_ARGS);
        ++id;
    }

    if ((layers & kDiffuseLayer) == kDiffuseLayer)
    {
        values[id] = UberV2_Lambert_GetPdf(dg, wi, wo, TEXTURE_ARGS);
        ++id;
    }

    if ((layers & kRefractionLayer) == kRefractionLayer)
    {
        iors[id] = dg->mat.uberv2.refraction_ior;
        values[id] = UberV2_Refraction_GetPdf(dg, wi, wo, TEXTURE_ARGS);
    }

    result = values[fresnel_blend_layers - 1];
    for (int a = fresnel_blend_layers - 1; a > 0; --a)
    {
        result = Fresnel_Blend_F(iors[a - 1], iors[a], values[a - 1], result, wi, dg);
    }

    if ((layers & kTransparencyLayer) == kTransparencyLayer)
    {
        return 0.f;
        result = mix(result, 0.f, dg->mat.uberv2.transparency);
    }

    return result;
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
    const int layers = dg->mat.uberv2.layers;
    if ((layers & kTransparencyLayer) == kTransparencyLayer)
    {
        if (sample.x < dg->mat.uberv2.transparency)
        {
            return UberV2_Passthrough_Sample(dg, wi, TEXTURE_ARGS, sample, wo, pdf);
        }
    }

    if ((dg->mat.bxdf_flags & kBxdfSampleCoating) == kBxdfSampleCoating)
        return UberV2_Coating_Sample(dg, wi, TEXTURE_ARGS, wo, pdf);
    else if ((dg->mat.bxdf_flags & kBxdfSampleReflection) == kBxdfSampleReflection)
        return UberV2_Reflection_Sample(dg, wi, TEXTURE_ARGS, sample, wo, pdf);
    else if ((dg->mat.bxdf_flags & kBxdfSampleDiffuse) == kBxdfSampleDiffuse)
        return UberV2_Lambert_Sample(dg, wi, TEXTURE_ARGS, sample, wo, pdf);
    else if ((dg->mat.bxdf_flags & kBxdfSampleRefraction) == kBxdfSampleRefraction)
        return UberV2_Refraction_Sample(dg, wi, TEXTURE_ARGS, sample, wo, pdf);

    return 0.f;
}
