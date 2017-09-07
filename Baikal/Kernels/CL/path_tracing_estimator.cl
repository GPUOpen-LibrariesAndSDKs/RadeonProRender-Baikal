
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
#include <../Baikal/Kernels/CL/common.cl>
#include <../Baikal/Kernels/CL/ray.cl>
#include <../Baikal/Kernels/CL/isect.cl>
#include <../Baikal/Kernels/CL/utils.cl>
#include <../Baikal/Kernels/CL/payload.cl>
#include <../Baikal/Kernels/CL/texture.cl>
#include <../Baikal/Kernels/CL/sampling.cl>
#include <../Baikal/Kernels/CL/normalmap.cl>
#include <../Baikal/Kernels/CL/bxdf.cl>
#include <../Baikal/Kernels/CL/light.cl>
#include <../Baikal/Kernels/CL/scene.cl>
#include <../Baikal/Kernels/CL/material.cl>
#include <../Baikal/Kernels/CL/volumetrics.cl>
#include <../Baikal/Kernels/CL/path.cl>


KERNEL
void InitPathData(
    GLOBAL int const* restrict src_index, 
    GLOBAL int* restrict dst_index,
    GLOBAL int const* restrict num_elements,
    GLOBAL Path* restrict paths
)
{
    int global_id = get_global_id(0);

    // Check borders
    if (global_id < *num_elements)
    {
        GLOBAL Path* my_path = paths + global_id;
        dst_index[global_id] = src_index[global_id];

        // Initalize path data
        my_path->throughput = make_float3(1.f, 1.f, 1.f);
        my_path->volume = INVALID_IDX;
        my_path->flags = 0;
        my_path->active = 0xFF;
    }
}

// This kernel only handles scattered paths.
// It applies direct illumination and generates
// path continuation if multiscattering is enabled. 
KERNEL void ShadeVolume(
    // Ray batch
    GLOBAL ray const* restrict rays,
    // Intersection data
    GLOBAL Intersection const* restrict isects,
    // Hit indices
    GLOBAL int const* restrict hit_indices,
    // Pixel indices
    GLOBAL int const*  restrict pixel_indices,
    // Output indices
    GLOBAL int const*  restrict output_indices,
    // Number of rays
    GLOBAL int const*  restrict num_hits,
    // Vertices
    GLOBAL float3 const* restrict vertices,
    // Normals
    GLOBAL float3 const* restrict normals,
    // UVs
    GLOBAL float2 const* restrict uvs,
    // Indices
    GLOBAL int const* restrict indices,
    // Shapes
    GLOBAL Shape const* restrict shapes,
    // Material IDs
    GLOBAL int const* restrict material_ids,
    // Materials
    GLOBAL Material const* restrict materials,
    // Textures
    TEXTURE_ARG_LIST,
    // Environment texture index
    int env_light_idx,
    // Emissives
    GLOBAL Light const* restrict lights,
    // Light distribution
    GLOBAL int const* restrict light_distribution,
    // Number of emissive objects
    int num_lights,
    // RNG seed
    uint rng_seed,
    // Sampler state 
    GLOBAL uint* restrict random,
    // Sobol matrices
    GLOBAL uint const* restrict sobol_mat,
    // Current bounce
    int bounce,
    // Current frame
    int frame,
    // Volume data
    GLOBAL Volume const* restrict volumes,
    // Shadow rays
    GLOBAL ray* restrict shadow_rays,
    // Light samples
    GLOBAL float3* restrict light_samples,
    // Path throughput
    GLOBAL Path* restrict paths,
    // Indirect rays (next path segment)
    GLOBAL ray* restrict indirect_rays,
    // Radiance
    GLOBAL float3* restrict output
)
{
    int global_id = get_global_id(0);

    Scene scene =
    {
        vertices,
        normals,
        uvs,
        indices,
        shapes,
        material_ids,
        materials,
        lights,
        env_light_idx,
        num_lights,
        light_distribution
    };

    if (global_id < *num_hits)
    {
        // Fetch index
        int hit_idx = hit_indices[global_id];
        int pixel_idx = pixel_indices[global_id];
        Intersection isect = isects[hit_idx];

        GLOBAL Path* path = paths + pixel_idx;

        // Only apply to scattered paths
        if (!Path_IsScattered(path))
        {
            return;
        }

        // Fetch incoming ray
        float3 o = rays[hit_idx].o.xyz;
        float3 wi = rays[hit_idx].d.xyz;

        Sampler sampler;
#if SAMPLER == SOBOL
        uint scramble = random[pixel_idx] * 0x1fe3434f;
        Sampler_Init(&sampler, frame, SAMPLE_DIM_SURFACE_OFFSET + bounce * SAMPLE_DIMS_PER_BOUNCE + SAMPLE_DIM_VOLUME_EVALUATE_OFFSET, scramble);
#elif SAMPLER == RANDOM
        uint scramble = pixel_idx * rng_seed;
        Sampler_Init(&sampler, scramble);
#elif SAMPLER == CMJ
        uint rnd = random[pixel_idx];
        uint scramble = rnd * 0x1fe3434f * ((frame + 13 * rnd) / (CMJ_DIM * CMJ_DIM));
        Sampler_Init(&sampler, frame % (CMJ_DIM * CMJ_DIM), SAMPLE_DIM_SURFACE_OFFSET + bounce * SAMPLE_DIMS_PER_BOUNCE + SAMPLE_DIM_VOLUME_EVALUATE_OFFSET, scramble);
#endif


        // Here we know that volume_idx != -1 since this is a precondition
        // for scattering event
        int volume_idx = Path_GetVolumeIdx(path);

        // Sample light source
        float pdf = 0.f;
        float selection_pdf = 0.f;
        float3 wo;

        int light_idx = Scene_SampleLight(&scene, Sampler_Sample1D(&sampler, SAMPLER_ARGS), &selection_pdf);

        // Here we need fake differential geometry for light sampling procedure
        DifferentialGeometry dg;
        // put scattering position in there (it is along the current ray at isect.distance
        // since EvaluateVolume has put it there
        dg.p = o + wi * Intersection_GetDistance(isects + hit_idx);
        // Get light sample intencity
        float3 le = Light_Sample(light_idx, &scene, &dg, TEXTURE_ARGS, Sampler_Sample2D(&sampler, SAMPLER_ARGS), &wo, &pdf);

        // Generate shadow ray
        float shadow_ray_length = length(wo);
        Ray_Init(shadow_rays + global_id, dg.p, normalize(wo), shadow_ray_length, 0.f, 0xFFFFFFFF);

        // Evaluate volume transmittion along the shadow ray (it is incorrect if the light source is outside of the
        // current volume, but in this case it will be discarded anyway since the intersection at the outer bound
        // of a current volume), so the result is fully correct.
        float3 tr = Volume_Transmittance(&volumes[volume_idx], &shadow_rays[global_id], shadow_ray_length);

        // Volume emission is applied only if the light source is in the current volume(this is incorrect since the light source might be
        // outside of a volume and we have to compute fraction of ray in this case, but need to figure out how)
        float3 r = Volume_Emission(&volumes[volume_idx], &shadow_rays[global_id], shadow_ray_length);

        // This is the estimate coming from a light source
        // TODO: remove hardcoded phase func and sigma
        r += tr * le * volumes[volume_idx].sigma_s * PhaseFunction_Uniform(wi, normalize(wo)) / pdf / selection_pdf;

        // Only if we have some radiance compute the visibility ray
        if (NON_BLACK(tr) && NON_BLACK(r) && pdf > 0.f)
        {
            // Put lightsample result
            light_samples[global_id] = REASONABLE_RADIANCE(r * Path_GetThroughput(path));
        }
        else
        {
            // Nothing to compute
            light_samples[global_id] = 0.f;
            // Otherwise make it incative to save intersector cycles (hopefully)
            Ray_SetInactive(shadow_rays + global_id);
        }

#ifdef MULTISCATTER
        // This is highly brute-force
        // TODO: investigate importance sampling techniques here
        wo = Sample_MapToSphere(Sampler_Sample2D(&sampler, SAMPLER_ARGS));
        pdf = 1.f / (4.f * PI);

        // Generate new path segment
        Ray_Init(indirect_rays + global_id, dg.p, normalize(wo), CRAZY_HIGH_DISTANCE, 0.f, 0xFFFFFFFF);

        // Update path throughput multiplying by phase function.
        Path_MulThroughput(path, volumes[volume_idx].sigma_s * PhaseFunction_Uniform(wi, normalize(wo)) / pdf);
#else
        // Single-scattering mode only,
        // kill the path and compact away on next iteration
        Path_Kill(path);
        Ray_SetInactive(indirect_rays + global_id);
#endif
    }
}

// Handle ray-surface interaction possibly generating path continuation. 
// This is only applied to non-scattered paths.
KERNEL void ShadeSurface(
    // Ray batch
    GLOBAL ray const* restrict rays,
    // Intersection data
    GLOBAL Intersection const* restrict isects,
    // Hit indices
    GLOBAL int const* restrict hit_indices,
    // Pixel indices
    GLOBAL int const* restrict pixel_indices,
    // Output indices
    GLOBAL int const*  restrict output_indices,
    // Number of rays
    GLOBAL int const* restrict num_hits,
    // Vertices
    GLOBAL float3 const* restrict vertices,
    // Normals
    GLOBAL float3 const* restrict normals,
    // UVs
    GLOBAL float2 const* restrict uvs,
    // Indices
    GLOBAL int const* restrict indices,
    // Shapes
    GLOBAL Shape const* restrict shapes,
    // Material IDs
    GLOBAL int const* restrict material_ids,
    // Materials
    GLOBAL Material const* restrict materials,
    // Textures
    TEXTURE_ARG_LIST,
    // Environment texture index
    int env_light_idx,
    // Emissives
    GLOBAL Light const* restrict lights,
    // Light distribution
    GLOBAL int const* restrict light_distribution,
    // Number of emissive objects
    int num_lights,
    // RNG seed
    uint rng_seed,
    // Sampler states
    GLOBAL uint* restrict random,
    // Sobol matrices
    GLOBAL uint const* restrict sobol_mat,
    // Current bounce
    int bounce,
    // Frame
    int frame,
    // Volume data
    GLOBAL Volume const* restrict volumes,
    // Shadow rays
    GLOBAL ray* restrict shadow_rays,
    // Light samples
    GLOBAL float3* restrict light_samples,
    // Path throughput
    GLOBAL Path* restrict paths,
    // Indirect rays
    GLOBAL ray* restrict indirect_rays,
    // Radiance
    GLOBAL float3* restrict output
)
{
    int global_id = get_global_id(0);

    Scene scene =
    {
        vertices,
        normals,
        uvs,
        indices,
        shapes,
        material_ids,
        materials,
        lights,
        env_light_idx,
        num_lights,
        light_distribution
    };

    // Only applied to active rays after compaction
    if (global_id < *num_hits)
    {
        // Fetch index
        int hit_idx = hit_indices[global_id];
        int pixel_idx = pixel_indices[global_id];
        Intersection isect = isects[hit_idx];

        GLOBAL Path* path = paths + pixel_idx;

        // Early exit for scattered paths
        if (Path_IsScattered(path))
        {
            return;
        }

        // Fetch incoming ray direction
        float3 wi = -normalize(rays[hit_idx].d.xyz);

        Sampler sampler;
#if SAMPLER == SOBOL 
        uint scramble = random[pixel_idx] * 0x1fe3434f;
        Sampler_Init(&sampler, frame, SAMPLE_DIM_SURFACE_OFFSET + bounce * SAMPLE_DIMS_PER_BOUNCE, scramble);
#elif SAMPLER == RANDOM
        uint scramble = pixel_idx * rng_seed;
        Sampler_Init(&sampler, scramble);
#elif SAMPLER == CMJ
        uint rnd = random[pixel_idx];
        uint scramble = rnd * 0x1fe3434f * ((frame + 331 * rnd) / (CMJ_DIM * CMJ_DIM));
        Sampler_Init(&sampler, frame % (CMJ_DIM * CMJ_DIM), SAMPLE_DIM_SURFACE_OFFSET + bounce * SAMPLE_DIMS_PER_BOUNCE, scramble);
#endif

        // Fill surface data
        DifferentialGeometry diffgeo;
        Scene_FillDifferentialGeometry(&scene, &isect, &diffgeo); 

        // Check if we are hitting from the inside
        float ngdotwi = dot(diffgeo.ng, wi);
        bool backfacing = ngdotwi < 0.f;

        // Select BxDF 
        Material_Select(&scene, wi, &sampler, TEXTURE_ARGS, SAMPLER_ARGS, &diffgeo);

        // Terminate if emissive
        if (Bxdf_IsEmissive(&diffgeo))
        {
            if (!backfacing)
            {
                float weight = 1.f;

                if (bounce > 0 && !Path_IsSpecular(path))
                {
                    float2 extra = Ray_GetExtra(&rays[hit_idx]);
                    float ld = isect.uvwt.w;
                    float denom = fabs(dot(diffgeo.n, wi)) * diffgeo.area;
                    // TODO: num_lights should be num_emissies instead, presence of analytical lights breaks this code
                    float bxdf_light_pdf = denom > 0.f ? (ld * ld / denom / num_lights) : 0.f;
                    weight = BalanceHeuristic(1, extra.x, 1, bxdf_light_pdf);
                }

                // In this case we hit after an application of MIS process at previous step.
                // That means BRDF weight has been already applied.
                float3 v = Path_GetThroughput(path) * Emissive_GetLe(&diffgeo, TEXTURE_ARGS) * weight;
                int output_index = output_indices[pixel_idx];
                ADD_FLOAT3(&output[output_index], v);
            }

            Path_Kill(path);
            Ray_SetInactive(shadow_rays + global_id);
            Ray_SetInactive(indirect_rays + global_id);

            light_samples[global_id] = 0.f;
            return;
        }


        float s = Bxdf_IsBtdf(&diffgeo) ? (-sign(ngdotwi)) : 1.f;
        if (backfacing && !Bxdf_IsBtdf(&diffgeo))
        {
            //Reverse normal and tangents in this case
            //but not for BTDFs, since BTDFs rely
            //on normal direction in order to arrange   
            //indices of refraction
            diffgeo.n = -diffgeo.n;
            diffgeo.dpdu = -diffgeo.dpdu;
            diffgeo.dpdv = -diffgeo.dpdv;
            s = -s;
        }


        DifferentialGeometry_ApplyBumpNormalMap(&diffgeo, TEXTURE_ARGS);
        DifferentialGeometry_CalculateTangentTransforms(&diffgeo);

        float ndotwi = fabs(dot(diffgeo.n, wi));

        float light_pdf = 0.f;
        float bxdf_light_pdf = 0.f;
        float bxdf_pdf = 0.f;
        float light_bxdf_pdf = 0.f;
        float selection_pdf = 0.f;
        float3 radiance = 0.f;
        float3 lightwo;
        float3 bxdfwo;
        float3 wo;
        float bxdf_weight = 1.f;
        float light_weight = 1.f;

        int light_idx = Scene_SampleLight(&scene, Sampler_Sample1D(&sampler, SAMPLER_ARGS), &selection_pdf);

        float3 throughput = Path_GetThroughput(path);

        // Sample bxdf
        float3 bxdf = Bxdf_Sample(&diffgeo, wi, TEXTURE_ARGS, Sampler_Sample2D(&sampler, SAMPLER_ARGS), &bxdfwo, &bxdf_pdf);

        // If we have light to sample we can hopefully do mis 
        if (light_idx > -1) 
        {
            // Sample light
            float3 le = Light_Sample(light_idx, &scene, &diffgeo, TEXTURE_ARGS, Sampler_Sample2D(&sampler, SAMPLER_ARGS), &lightwo, &light_pdf);
            light_bxdf_pdf = Bxdf_GetPdf(&diffgeo, wi, normalize(lightwo), TEXTURE_ARGS);
            light_weight = Light_IsSingular(&scene.lights[light_idx]) ? 1.f : BalanceHeuristic(1, light_pdf * selection_pdf, 1, light_bxdf_pdf); 

            // Apply MIS to account for both
            if (NON_BLACK(le) && light_pdf > 0.0f && !Bxdf_IsSingular(&diffgeo))
            {
                wo = lightwo;
                float ndotwo = fabs(dot(diffgeo.n, normalize(wo)));
                radiance = le * ndotwo * Bxdf_Evaluate(&diffgeo, wi, normalize(wo), TEXTURE_ARGS) * throughput * light_weight / light_pdf / selection_pdf;
            }
        }

        // If we have some light here generate a shadow ray
        if (NON_BLACK(radiance))
        {
            // Generate shadow ray
            float3 shadow_ray_o = diffgeo.p + CRAZY_LOW_DISTANCE * s * diffgeo.ng;
            float3 temp = diffgeo.p + wo - shadow_ray_o;
            float3 shadow_ray_dir = normalize(temp);
            float shadow_ray_length = length(temp);
            int shadow_ray_mask = 0x0000FFFF;

            Ray_Init(shadow_rays + global_id, shadow_ray_o, shadow_ray_dir, shadow_ray_length, 0.f, shadow_ray_mask);

            // Apply the volume to shadow ray if needed
            int volume_idx = Path_GetVolumeIdx(path);
            if (volume_idx != -1)
            {
                radiance *= Volume_Transmittance(&volumes[volume_idx], &shadow_rays[global_id], shadow_ray_length);
                radiance += Volume_Emission(&volumes[volume_idx], &shadow_rays[global_id], shadow_ray_length) * throughput;
            }

            // And write the light sample 
            light_samples[global_id] = REASONABLE_RADIANCE(radiance);
        }
        else
        {
            // Otherwise save some intersector cycles
            Ray_SetInactive(shadow_rays + global_id);
            light_samples[global_id] = 0;
        }

        // Apply Russian roulette
        float q = max(min(0.5f,
            // Luminance
            0.2126f * throughput.x + 0.7152f * throughput.y + 0.0722f * throughput.z), 0.01f);
        // Only if it is 3+ bounce
        bool rr_apply = bounce > 3;
        bool rr_stop = Sampler_Sample1D(&sampler, SAMPLER_ARGS) > q && rr_apply;

        if (rr_apply)
        {
            Path_MulThroughput(path, 1.f / q);
        }

        if (Bxdf_IsSingular(&diffgeo))
        {
            Path_SetSpecularFlag(path);
        }
        else
        {
            Path_ClearSpecularFlag(path);
        }

        bxdfwo = normalize(bxdfwo);
        float3 t = bxdf * fabs(dot(diffgeo.n, bxdfwo));

        // Only continue if we have non-zero throughput & pdf
        if (NON_BLACK(t) && bxdf_pdf > 0.f && !rr_stop)
        {
            // Update the throughput
            Path_MulThroughput(path, t / bxdf_pdf);

            // Generate ray
            float3 indirect_ray_dir = bxdfwo;
            float3 indirect_ray_o = diffgeo.p + CRAZY_LOW_DISTANCE * s * diffgeo.ng;

            Ray_Init(indirect_rays + global_id, indirect_ray_o, indirect_ray_dir, CRAZY_HIGH_DISTANCE, 0.f, 0xFFFFFFFF);
            Ray_SetExtra(indirect_rays + global_id, make_float2(bxdf_pdf, 0.f));
        }
        else
        {
            // Otherwise kill the path
            Path_Kill(path);
            Ray_SetInactive(indirect_rays + global_id);
        }
    }
}

///< Illuminate missing rays
KERNEL void ShadeBackgroundEnvMap(
    // Ray batch
    GLOBAL ray const* restrict rays,
    // Intersection data
    GLOBAL Intersection const* restrict isects,
    // Pixel indices
    GLOBAL int const* restrict pixel_indices,
    // Output indices
    GLOBAL int const*  restrict output_indices,
    // Number of rays
    int num_rays,
    GLOBAL Light const* restrict lights,
    int env_light_idx,
    // Textures
    TEXTURE_ARG_LIST,
    // Environment texture index
    GLOBAL Path const* restrict paths,
    GLOBAL Volume const* restrict volumes,
    // Output values
    GLOBAL float4* restrict output
)
{
    int global_id = get_global_id(0);

    if (global_id < num_rays)
    {
        int pixel_idx = pixel_indices[global_id];
        int output_index = output_indices[pixel_idx];

        float4 v = make_float4(0.f, 0.f, 0.f, 1.f);

        // In case of a miss
        if (isects[global_id].shapeid < 0 && env_light_idx != -1)
        {
            // Multiply by throughput
            int volume_idx = paths[pixel_idx].volume;

            Light light = lights[env_light_idx];


            if (volume_idx == -1)
                v.xyz = light.multiplier * Texture_SampleEnvMap(rays[global_id].d.xyz, TEXTURE_ARGS_IDX(light.tex));
            else
            {
                v.xyz = light.multiplier * Texture_SampleEnvMap(rays[global_id].d.xyz, TEXTURE_ARGS_IDX(light.tex)) *
                    Volume_Transmittance(&volumes[volume_idx], &rays[global_id], rays[global_id].o.w);

                v.xyz += Volume_Emission(&volumes[volume_idx], &rays[global_id], rays[global_id].o.w);
            }
        }

        ADD_FLOAT4(&output[output_index], v); 
    }
}

///< Handle light samples and visibility info and add contribution to final buffer
KERNEL void GatherLightSamples(  
    // Pixel indices
    GLOBAL int const* restrict pixel_indices,
    // Output indices
    GLOBAL int const*  restrict output_indices,
    // Number of rays
    GLOBAL int* restrict num_rays,
    // Shadow rays hits
    GLOBAL int const* restrict shadow_hits,
    // Light samples
    GLOBAL float3 const* restrict light_samples,
    // throughput
    GLOBAL Path const* restrict paths,
    // Radiance sample buffer
    GLOBAL float4* restrict output
)
{
    int global_id = get_global_id(0);

    if (global_id < *num_rays)
    {
        // Get pixel id for this sample set
        int pixel_idx = pixel_indices[global_id];
        int output_index = output_indices[pixel_idx];

        // Prepare accumulator variable
        float4 radiance = 0.f;

        // Start collecting samples
        {
            // If shadow ray didn't hit anything and reached skydome
            if (shadow_hits[global_id] == -1)
            {
                // Add its contribution to radiance accumulator
                radiance.xyz += light_samples[global_id];
            }
        }

        // Divide by number of light samples (samples already have built-in throughput)
        ADD_FLOAT4(&output[output_index], radiance);
    }
}

///< Restore pixel indices after compaction
KERNEL void RestorePixelIndices(
    // Compacted indices
    GLOBAL int const* restrict compacted_indices,
    // Number of compacted indices
    GLOBAL int* restrict num_elements,
    // Previous pixel indices
    GLOBAL int const* restrict prev_indices,
    // New pixel indices
    GLOBAL int* restrict new_indices
)
{
    int global_id = get_global_id(0);

    // Handle only working subset
    if (global_id < *num_elements)
    {
        new_indices[global_id] = prev_indices[compacted_indices[global_id]];
    }
}

///< Restore pixel indices after compaction
KERNEL void FilterPathStream(
    // Intersections
    GLOBAL Intersection const* restrict isects,
    // Number of compacted indices
    GLOBAL int const* restrict num_elements,
    // Pixel indices
    GLOBAL int const* restrict pixel_indices,
    // Paths
    GLOBAL Path* restrict paths,
    // Predicate
    GLOBAL int* restrict predicate
)
{
    int global_id = get_global_id(0);

    // Handle only working subset
    if (global_id < *num_elements)
    {
        int pixel_idx = pixel_indices[global_id];

        GLOBAL Path* path = paths + pixel_idx;

        if (Path_IsAlive(path))
        {
            bool kill = (length(Path_GetThroughput(path)) < CRAZY_LOW_THROUGHPUT);

            if (!kill)
            {
                predicate[global_id] = isects[global_id].shapeid >= 0 ? 1 : 0;
            }
            else
            {
                Path_Kill(path);
                predicate[global_id] = 0;
            }
        }
        else
        {
            predicate[global_id] = 0;
        }
    }
}

///< Illuminate missing rays
KERNEL void ShadeMiss(
    // Ray batch
    GLOBAL ray const* restrict rays,
    // Intersection data
    GLOBAL Intersection const* restrict isects, 
    // Pixel indices
    GLOBAL int const* restrict pixel_indices,
    // Output indices
    GLOBAL int const*  restrict output_indices,
    // Number of rays
    GLOBAL int const* restrict num_rays,
    GLOBAL Light const* restrict lights,
    // Light distribution
    GLOBAL int const* restrict light_distribution,
    // Number of emissive objects
    int num_lights,
    int env_light_idx,
    // Textures
    TEXTURE_ARG_LIST,
    GLOBAL Path const* restrict paths,
    GLOBAL Volume const* restrict volumes,
    // Output values
    GLOBAL float4* restrict output
)
{
    int global_id = get_global_id(0);

    if (global_id < *num_rays)
    {
        int pixel_idx = pixel_indices[global_id];
        int output_index = output_indices[pixel_idx];

        GLOBAL Path const* path = paths + pixel_idx;

        // In case of a miss
        if (isects[global_id].shapeid < 0 && Path_IsAlive(path))
        {
            Light light = lights[env_light_idx];

            // Apply MIS
            float selection_pdf = Distribution1D_GetPdfDiscreet(env_light_idx, light_distribution);
            float light_pdf = EnvironmentLight_GetPdf(&light, 0, 0, rays[global_id].d.xyz, TEXTURE_ARGS);
            float2 extra = Ray_GetExtra(&rays[global_id]);
            float weight = BalanceHeuristic(1, extra.x, 1, light_pdf * selection_pdf);

            float3 t = Path_GetThroughput(path);
            float4 v = 0.f;
            v.xyz = REASONABLE_RADIANCE(weight * light.multiplier * Texture_SampleEnvMap(rays[global_id].d.xyz, TEXTURE_ARGS_IDX(light.tex)) * t);
            ADD_FLOAT4(&output[output_index], v);
        }
    }
}

