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
#ifndef MONTE_CARLO_RENDERER_CL
#define MONTE_CARLO_RENDERER_CL

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
#include <../Baikal/Kernels/CL/vertex.cl>

// Pinhole camera implementation.
// This kernel is being used if aperture value = 0.
KERNEL
void PerspectiveCamera_GeneratePaths(
    // Camera
    GLOBAL Camera const* restrict camera,
    // Image resolution
    int output_width,
    int output_height,
    // Pixel domain buffer
    GLOBAL int const* restrict pixel_idx,
    // Size of pixel domain buffer
    GLOBAL int const* restrict num_pixels,
    // RNG seed value
    uint rng_seed,
    // Current frame
    uint frame,
    // Rays to generate
    GLOBAL ray* restrict rays,
    // RNG data
    GLOBAL uint* restrict random,
    GLOBAL uint const* restrict sobolmat
)
{
    int global_id = get_global_id(0);

    // Check borders
    if (global_id < *num_pixels)
    {
        int idx = pixel_idx[global_id];
        int y = idx / output_width;
        int x = idx % output_width;

        // Get pointer to ray & path handles
        GLOBAL ray* my_ray = rays + global_id;

        // Initialize sampler
        Sampler sampler;
#if SAMPLER == SOBOL
        uint scramble = random[x + output_width * y] * 0x1fe3434f;

        if (frame & 0xF)
        {
            random[x + output_width * y] = WangHash(scramble);
        }

        Sampler_Init(&sampler, frame, SAMPLE_DIM_CAMERA_OFFSET, scramble);
#elif SAMPLER == RANDOM
        uint scramble = x + output_width * y * rng_seed;
        Sampler_Init(&sampler, scramble);
#elif SAMPLER == CMJ
        uint rnd = random[x + output_width * y];
        uint scramble = rnd * 0x1fe3434f * ((frame + 133 * rnd) / (CMJ_DIM * CMJ_DIM));
        Sampler_Init(&sampler, frame % (CMJ_DIM * CMJ_DIM), SAMPLE_DIM_CAMERA_OFFSET, scramble);
#endif

        // Generate sample
        float2 sample0 = Sampler_Sample2D(&sampler, SAMPLER_ARGS);

        // Calculate [0..1] image plane sample
        float2 img_sample;
        img_sample.x = (float)x / output_width + sample0.x / output_width;
        img_sample.y = (float)y / output_height + sample0.y / output_height;

        // Transform into [-0.5, 0.5]
        float2 h_sample = img_sample - make_float2(0.5f, 0.5f);
        // Transform into [-dim/2, dim/2]
        float2 c_sample = h_sample * camera->dim;

        // Calculate direction to image plane
        my_ray->d.xyz = normalize(camera->focal_length * camera->forward + c_sample.x * camera->right + c_sample.y * camera->up);
        // Origin == camera position + nearz * d
        my_ray->o.xyz = camera->p + camera->zcap.x * my_ray->d.xyz;
        // Max T value = zfar - znear since we moved origin to znear
        my_ray->o.w = camera->zcap.y - camera->zcap.x;
        // Generate random time from 0 to 1
        my_ray->d.w = sample0.x;
        // Set ray max
        my_ray->extra.x = 0xFFFFFFFF;
        my_ray->extra.y = 0xFFFFFFFF;
        Ray_SetExtra(my_ray, 1.f);
    }
}

// Physical camera implemenation.
// This kernel is being used if aperture > 0.
KERNEL void PerspectiveCameraDof_GeneratePaths(
    // Camera
    GLOBAL Camera const* restrict camera,
    // Image resolution
    int output_width,
    int output_height,
    // Pixel domain buffer
    GLOBAL int const* restrict pixel_idx,
    // Size of pixel domain buffer
    GLOBAL int const* restrict num_pixels,
    // RNG seed value
    uint rng_seed,
    // Current frame
    uint frame,
    // Rays to generate
    GLOBAL ray* restrict rays,
    // RNG data
    GLOBAL uint* restrict random,
    GLOBAL uint const* restrict sobolmat
)
{
    int global_id = get_global_id(0);

    // Check borders
    if (global_id < *num_pixels)
    {
        int idx = pixel_idx[global_id];
        int y = idx / output_width;
        int x = idx % output_width;

        // Get pointer to ray & path handles
        GLOBAL ray* my_ray = rays + global_id;

        // Initialize sampler
        Sampler sampler;
#if SAMPLER == SOBOL
        uint scramble = random[x + output_width * y] * 0x1fe3434f;

        if (frame & 0xF)
        {
            random[x + output_width * y] = WangHash(scramble);
        }

        Sampler_Init(&sampler, frame, SAMPLE_DIM_CAMERA_OFFSET, scramble);
#elif SAMPLER == RANDOM
        uint scramble = x + output_width * y * rng_seed;
        Sampler_Init(&sampler, scramble);
#elif SAMPLER == CMJ
        uint rnd = random[x + output_width * y];
        uint scramble = rnd * 0x1fe3434f * ((frame + 133 * rnd) / (CMJ_DIM * CMJ_DIM));
        Sampler_Init(&sampler, frame % (CMJ_DIM * CMJ_DIM), SAMPLE_DIM_CAMERA_OFFSET, scramble);
#endif

        // Generate pixel and lens samples
        float2 sample0 = Sampler_Sample2D(&sampler, SAMPLER_ARGS);
        float2 sample1 = Sampler_Sample2D(&sampler, SAMPLER_ARGS);

        // Calculate [0..1] image plane sample
        float2 img_sample;
        img_sample.x = (float)x / output_width + sample0.x / output_width;
        img_sample.y = (float)y / output_height + sample0.y / output_height;

        // Transform into [-0.5, 0.5]
        float2 h_sample = img_sample - make_float2(0.5f, 0.5f);
        // Transform into [-dim/2, dim/2]
        float2 c_sample = h_sample * camera->dim;

        // Generate sample on the lens
        float2 lens_sample = camera->aperture * Sample_MapToDiskConcentric(sample1);
        // Calculate position on focal plane
        float2 focal_plane_sample = c_sample * camera->focus_distance / camera->focal_length;
        // Calculate ray direction
        float2 camera_dir = focal_plane_sample - lens_sample;

        // Calculate direction to image plane
        my_ray->d.xyz = normalize(camera->forward * camera->focus_distance + camera->right * camera_dir.x + camera->up * camera_dir.y);
        // Origin == camera position + nearz * d
        my_ray->o.xyz = camera->p + lens_sample.x * camera->right + lens_sample.y * camera->up;
        // Max T value = zfar - znear since we moved origin to znear
        my_ray->o.w = camera->zcap.y - camera->zcap.x;
        // Generate random time from 0 to 1
        my_ray->d.w = sample0.x;
        // Set ray max
        my_ray->extra.x = 0xFFFFFFFF;
        my_ray->extra.y = 0xFFFFFFFF;
        Ray_SetExtra(my_ray, 1.f);
    }
}


KERNEL
void PerspectiveCamera_GenerateVertices(
    // Camera
    GLOBAL Camera const* restrict camera,
    // Image resolution
    int output_width,
    int output_height,
    // Pixel domain buffer
    GLOBAL int const* restrict pixel_idx,
    // Size of pixel domain buffer
    GLOBAL int const* restrict num_pixels,
    // RNG seed value
    uint rng_seed,
    // Current frame
    uint frame,
    // RNG data
    GLOBAL uint* restrict random,
    GLOBAL uint const* restrict sobolmat,
    // Rays to generate
    GLOBAL ray* restrict rays,
    // Eye subpath vertices
    GLOBAL PathVertex* restrict eye_subpath,
    // Eye subpath length
    GLOBAL int* restrict eye_subpath_length,
    // Path buffer
    GLOBAL Path* restrict paths
)

{
    int global_id = get_global_id(0);

    // Check borders
    if (global_id < *num_pixels)
    {
        int idx = pixel_idx[global_id];
        int y = idx / output_width;
        int x = idx % output_width;

        // Get pointer to ray & path handles
        GLOBAL ray* my_ray = rays + global_id;

        GLOBAL PathVertex* my_vertex = eye_subpath + BDPT_MAX_SUBPATH_LEN * idx;
        GLOBAL int* my_count = eye_subpath_length + idx;
        GLOBAL Path* my_path = paths + idx;

        // Initialize sampler
        Sampler sampler;
#if SAMPLER == SOBOL
        uint scramble = random[x + output_width * y] * 0x1fe3434f;

        if (frame & 0xF)
        {
            random[x + output_width * y] = WangHash(scramble);
        }

        Sampler_Init(&sampler, frame, SAMPLE_DIM_CAMERA_OFFSET, scramble);
#elif SAMPLER == RANDOM
        uint scramble = x + output_width * y * rng_seed;
        Sampler_Init(&sampler, scramble);
#elif SAMPLER == CMJ
        uint rnd = random[x + output_width * y];
        uint scramble = rnd * 0x1fe3434f * ((frame + 133 * rnd) / (CMJ_DIM * CMJ_DIM));
        Sampler_Init(&sampler, frame % (CMJ_DIM * CMJ_DIM), SAMPLE_DIM_CAMERA_OFFSET, scramble);
#endif

        // Generate sample
        float2 sample0 = Sampler_Sample2D(&sampler, SAMPLER_ARGS);

        // Calculate [0..1] image plane sample
        float2 img_sample;
        img_sample.x = (float)x / output_width + sample0.x / output_width;
        img_sample.y = (float)y / output_height + sample0.y / output_height;

        // Transform into [-0.5, 0.5]
        float2 h_sample = img_sample - make_float2(0.5f, 0.5f);
        // Transform into [-dim/2, dim/2]
        float2 c_sample = h_sample * camera->dim;

        // Calculate direction to image plane
        my_ray->d.xyz = normalize(camera->focal_length * camera->forward + c_sample.x * camera->right + c_sample.y * camera->up);
        // Origin == camera position + nearz * d
        my_ray->o.xyz = camera->p + camera->zcap.x * my_ray->d.xyz;
        // Max T value = zfar - znear since we moved origin to znear
        my_ray->o.w = camera->zcap.y - camera->zcap.x;
        // Generate random time from 0 to 1
        my_ray->d.w = sample0.x;
        // Set ray max
        my_ray->extra.x = 0xFFFFFFFF;
        my_ray->extra.y = 0xFFFFFFFF;
        Ray_SetExtra(my_ray, 1.f);

        PathVertex v;
        PathVertex_Init(&v,
            camera->p,
            camera->forward,
            camera->forward,
            0.f,
            1.f,
            1.f,
            1.f,
            kCamera,
            -1);

        *my_count = 1;
        *my_vertex = v;

        // Initlize path data
        my_path->throughput = make_float3(1.f, 1.f, 1.f);
        my_path->volume = -1;
        my_path->flags = 0;
        my_path->active = 0xFF;
    }
}

KERNEL
void PerspectiveCameraDof_GenerateVertices(
    // Camera
    GLOBAL Camera const* restrict camera,
    // Image resolution
    int output_width,
    int output_height,
    // Pixel domain buffer
    GLOBAL int const* restrict pixel_idx,
    // Size of pixel domain buffer
    GLOBAL int const* restrict num_pixels,
    // RNG seed value
    uint rng_seed,
    // Current frame
    uint frame,
    // RNG data
    GLOBAL uint* restrict random,
    GLOBAL uint const* restrict sobolmat,
    // Rays to generate
    GLOBAL ray* restrict rays,
    // Eye subpath vertices
    GLOBAL PathVertex* restrict eye_subpath,
    // Eye subpath length
    GLOBAL int* restrict eye_subpath_length,
    // Path buffer
    GLOBAL Path* restrict paths
)

{
    int global_id = get_global_id(0);

    // Check borders
    if (global_id < *num_pixels)
    {
        int idx = pixel_idx[global_id];
        int y = idx / output_width;
        int x = idx % output_width;

        // Get pointer to ray & path handles
        GLOBAL ray* my_ray = rays + global_id;
        GLOBAL PathVertex* my_vertex = eye_subpath + BDPT_MAX_SUBPATH_LEN * (y * output_width + x);
        GLOBAL int* my_count = eye_subpath_length + y * output_width + x;
        GLOBAL Path* my_path = paths + y * output_width + x;

        // Initialize sampler
        Sampler sampler;
#if SAMPLER == SOBOL
        uint scramble = random[x + output_width * y] * 0x1fe3434f;

        if (frame & 0xF)
        {
            random[x + output_width * y] = WangHash(scramble);
        }

        Sampler_Init(&sampler, frame, SAMPLE_DIM_CAMERA_OFFSET, scramble);
#elif SAMPLER == RANDOM
        uint scramble = x + output_width * y * rng_seed;
        Sampler_Init(&sampler, scramble);
#elif SAMPLER == CMJ
        uint rnd = random[x + output_width * y];
        uint scramble = rnd * 0x1fe3434f * ((frame + 133 * rnd) / (CMJ_DIM * CMJ_DIM));
        Sampler_Init(&sampler, frame % (CMJ_DIM * CMJ_DIM), SAMPLE_DIM_CAMERA_OFFSET, scramble);
#endif

        // Generate pixel and lens samples
        float2 sample0 = Sampler_Sample2D(&sampler, SAMPLER_ARGS);
        float2 sample1 = Sampler_Sample2D(&sampler, SAMPLER_ARGS);

        // Calculate [0..1] image plane sample
        float2 img_sample;
        img_sample.x = (float)x / output_width + sample0.x / output_width;
        img_sample.y = (float)y / output_height + sample0.y / output_height;

        // Transform into [-0.5, 0.5]
        float2 h_sample = img_sample - make_float2(0.5f, 0.5f);
        // Transform into [-dim/2, dim/2]
        float2 c_sample = h_sample * camera->dim;

        // Generate sample on the lens
        float2 lens_sample = camera->aperture * Sample_MapToDiskConcentric(sample1);
        // Calculate position on focal plane
        float2 focal_plane_sample = c_sample * camera->focus_distance / camera->focal_length;
        // Calculate ray direction
        float2 camera_dir = focal_plane_sample - lens_sample;

        // Calculate direction to image plane
        my_ray->d.xyz = normalize(camera->forward * camera->focus_distance + camera->right * camera_dir.x + camera->up * camera_dir.y);
        // Origin == camera position + nearz * d
        my_ray->o.xyz = camera->p + lens_sample.x * camera->right + lens_sample.y * camera->up;
        // Max T value = zfar - znear since we moved origin to znear
        my_ray->o.w = camera->zcap.y - camera->zcap.x;
        // Generate random time from 0 to 1
        my_ray->d.w = sample0.x;
        // Set ray max
        my_ray->extra.x = 0xFFFFFFFF;
        my_ray->extra.y = 0xFFFFFFFF;
        Ray_SetExtra(my_ray, 1.f);

        PathVertex v;
        PathVertex_Init(&v,
            camera->p,
            camera->forward,
            camera->forward,
            0.f,
            1.f,
            1.f,
            1.f,
            kCamera,
            -1);

        *my_count = 1;
        *my_vertex = v;

        // Initlize path data
        my_path->throughput = make_float3(1.f, 1.f, 1.f);
        my_path->volume = -1;
        my_path->flags = 0;
        my_path->active = 0xFF;
    }
}


KERNEL void GenerateTileDomain(
    int output_width,
    int output_height,
    int tile_offset_x,
    int tile_offset_y,
    int tile_width,
    int tile_height,
    int subtile_width,
    int subtile_height,
    GLOBAL int* restrict pixelidx0,
    GLOBAL int* restrict count
)
{
    int tile_x = get_global_id(0);
    int tile_y = get_global_id(1);
    int tile_start_idx = output_width * tile_offset_y + tile_offset_x;

    if (tile_x < tile_width && tile_y < tile_height)
    {
        // TODO: implement subtile support
        int idx = tile_start_idx + tile_y * output_width + tile_x;
        pixelidx0[tile_y * tile_width + tile_x] = idx;
    }

    if (tile_x == 0 && tile_y == 0)
    {
        *count = tile_width * tile_height;
    }
}


// Fill AOVs
KERNEL void FillAOVs(
    // Ray batch
    GLOBAL ray const* restrict rays,
    // Intersection data
    GLOBAL Intersection const* restrict isects,
    // Pixel indices
    GLOBAL int const* restrict pixel_idx,
    // Number of pixels
    GLOBAL int const* restrict num_items,
    // Vertices
    GLOBAL float3 const*restrict  vertices,
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
    // Number of emissive objects
    int num_lights,
    // RNG seed
    uint rngseed,
    // Sampler states
    GLOBAL uint* restrict random,
    // Sobol matrices
    GLOBAL uint const* restrict sobolmat,
    // Frame
    int frame,
    // World position flag
    int world_position_enabled,
    // World position AOV
    GLOBAL float4* restrict aov_world_position,
    // World normal flag
    int world_shading_normal_enabled,
    // World normal AOV
    GLOBAL float4* restrict aov_world_shading_normal,
    // World true normal flag
    int world_geometric_normal_enabled,
    // World true normal AOV
    GLOBAL float4* restrict aov_world_geometric_normal,
    // UV flag
    int uv_enabled,
    // UV AOV
    GLOBAL float4* restrict aov_uv,
    // Wireframe flag
    int wireframe_enabled,
    // Wireframe AOV
    GLOBAL float4* restrict aov_wireframe,
    // Albedo flag
    int albedo_enabled,
    // Wireframe AOV
    GLOBAL float4* restrict aov_albedo,
    // World tangent flag
    int world_tangent_enabled,
    // World tangent AOV
    GLOBAL float4* restrict aov_world_tangent,
    // World bitangent flag
    int world_bitangent_enabled,
    // World bitangent AOV
    GLOBAL float4* restrict aov_world_bitangent,
    // Gloss enabled flag
    int gloss_enabled,
    // Specularity map
    GLOBAL float4* restrict aov_gloss
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
        num_lights
    };

    // Only applied to active rays after compaction
    if (global_id < *num_items)
    {
        Intersection isect = isects[global_id];
        int idx = pixel_idx[global_id];

        if (isect.shapeid > -1)
        {
            // Fetch incoming ray direction
            float3 wi = -normalize(rays[global_id].d.xyz);

            Sampler sampler;
#if SAMPLER == SOBOL 
            uint scramble = random[global_id] * 0x1fe3434f;
            Sampler_Init(&sampler, frame, SAMPLE_DIM_SURFACE_OFFSET, scramble);
#elif SAMPLER == RANDOM
            uint scramble = global_id * rngseed;
            Sampler_Init(&sampler, scramble);
#elif SAMPLER == CMJ
            uint rnd = random[global_id];
            uint scramble = rnd * 0x1fe3434f * ((frame + 331 * rnd) / (CMJ_DIM * CMJ_DIM));
            Sampler_Init(&sampler, frame % (CMJ_DIM * CMJ_DIM), SAMPLE_DIM_SURFACE_OFFSET, scramble);
#endif

            // Fill surface data
            DifferentialGeometry diffgeo;
            Scene_FillDifferentialGeometry(&scene, &isect, &diffgeo);

            if (world_position_enabled)
            {
                aov_world_position[idx].xyz += diffgeo.p;
                aov_world_position[idx].w += 1.f;
            }

            if (world_shading_normal_enabled)
            {
                float ngdotwi = dot(diffgeo.ng, wi);
                bool backfacing = ngdotwi < 0.f;

                // Select BxDF
                Material_Select(&scene, wi, &sampler, TEXTURE_ARGS, SAMPLER_ARGS, &diffgeo);

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
                }

                DifferentialGeometry_ApplyBumpNormalMap(&diffgeo, TEXTURE_ARGS);
                DifferentialGeometry_CalculateTangentTransforms(&diffgeo);

                aov_world_shading_normal[idx].xyz += diffgeo.n;
                aov_world_shading_normal[idx].w += 1.f;
            }

            if (world_geometric_normal_enabled)
            {
                aov_world_geometric_normal[idx].xyz += diffgeo.ng;
                aov_world_geometric_normal[idx].w += 1.f;
            }

            if (wireframe_enabled)
            {
                bool hit = (isect.uvwt.x < 1e-3) || (isect.uvwt.y < 1e-3) || (1.f - isect.uvwt.x - isect.uvwt.y < 1e-3);
                float3 value = hit ? make_float3(1.f, 1.f, 1.f) : make_float3(0.f, 0.f, 0.f);
                aov_wireframe[idx].xyz += value;
                aov_wireframe[idx].w += 1.f;
            }

            if (uv_enabled)
            {
                aov_uv[idx].xy += diffgeo.uv.xy;
                aov_uv[idx].w += 1.f;
            }

            if (albedo_enabled)
            {
                float ngdotwi = dot(diffgeo.ng, wi);
                bool backfacing = ngdotwi < 0.f;

                // Select BxDF
                Material_Select(&scene, wi, &sampler, TEXTURE_ARGS, SAMPLER_ARGS, &diffgeo);

                const float3 kd = Texture_GetValue3f(diffgeo.mat.simple.kx.xyz, diffgeo.uv, TEXTURE_ARGS_IDX(diffgeo.mat.simple.kxmapidx));

                aov_albedo[idx].xyz += kd;
                aov_albedo[idx].w += 1.f;
            }

            if (world_tangent_enabled)
            {
                float ngdotwi = dot(diffgeo.ng, wi);
                bool backfacing = ngdotwi < 0.f;

                // Select BxDF
                Material_Select(&scene, wi, &sampler, TEXTURE_ARGS, SAMPLER_ARGS, &diffgeo);

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
                }

                DifferentialGeometry_ApplyBumpNormalMap(&diffgeo, TEXTURE_ARGS);
                DifferentialGeometry_CalculateTangentTransforms(&diffgeo);

                aov_world_tangent[idx].xyz += diffgeo.dpdu;
                aov_world_tangent[idx].w += 1.f;
            }

            if (world_bitangent_enabled)
            {
                float ngdotwi = dot(diffgeo.ng, wi);
                bool backfacing = ngdotwi < 0.f;

                // Select BxDF
                Material_Select(&scene, wi, &sampler, TEXTURE_ARGS, SAMPLER_ARGS, &diffgeo);

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
                }

                DifferentialGeometry_ApplyBumpNormalMap(&diffgeo, TEXTURE_ARGS);
                DifferentialGeometry_CalculateTangentTransforms(&diffgeo);

                aov_world_bitangent[idx].xyz += diffgeo.dpdv;
                aov_world_bitangent[idx].w += 1.f;
            }

            if (gloss_enabled)
            {
                float ngdotwi = dot(diffgeo.ng, wi);
                bool backfacing = ngdotwi < 0.f;

                // Select BxDF
                Material_Select(&scene, wi, &sampler, TEXTURE_ARGS, SAMPLER_ARGS, &diffgeo);

                float gloss = 0.f;

                int type = diffgeo.mat.type;
                if (type == kIdealReflect || type == kIdealReflect || type == kPassthrough)
                {
                    gloss = 1.f;
                }
                else if (type == kMicrofacetGGX || type == kMicrofacetBeckmann ||
                    type == kMicrofacetRefractionGGX || type == kMicrofacetRefractionBeckmann)
                {
                    gloss = 1.f - Texture_GetValue1f(diffgeo.mat.simple.ns, diffgeo.uv, TEXTURE_ARGS_IDX(diffgeo.mat.simple.nsmapidx));
                }


                aov_gloss[idx].xyz += gloss;
                aov_gloss[idx].w += 1.f;
            }
        }
    }
}


// Copy data to interop texture if supported
KERNEL void AccumulateData(
    GLOBAL float4 const* src_data,
    int num_elements,
    GLOBAL float4* dst_data
)
{
    int global_id = get_global_id(0);

    if (global_id < num_elements)
    {
        float4 v = src_data[global_id];
        dst_data[global_id] += v;
    }
}

// Copy data to interop texture if supported
KERNEL void ApplyGammaAndCopyData(
    GLOBAL float4 const* data,
    int img_width,
    int img_height,
    float gamma,
    write_only image2d_t img
)
{
    int global_id = get_global_id(0);

    int global_idx = global_id % img_width;
    int global_idy = global_id / img_width;

    if (global_idx < img_width && global_idy < img_height)
    {
        float4 v = data[global_id];
        float4 val = clamp(native_powr(v / v.w, 1.f / gamma), 0.f, 1.f);
        write_imagef(img, make_int2(global_idx, global_idy), val);
    }
}


#endif // MONTE_CARLO_RENDERER_CL
