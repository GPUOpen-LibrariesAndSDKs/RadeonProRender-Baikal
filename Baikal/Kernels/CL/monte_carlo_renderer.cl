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
    GLOBAL uint const* restrict sobol_mat
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
#ifndef BAIKAL_GENERATE_SAMPLE_AT_PIXEL_CENTER
        float2 sample0 = Sampler_Sample2D(&sampler, SAMPLER_ARGS);
#else
        float2 sample0 = make_float2(0.5f, 0.5f);
#endif

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
        Ray_SetMask(my_ray, VISIBILITY_MASK_PRIMARY);
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
    GLOBAL uint const* restrict sobol_mat
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
#ifndef BAIKAL_GENERATE_SAMPLE_AT_PIXEL_CENTER
        float2 sample0 = Sampler_Sample2D(&sampler, SAMPLER_ARGS);
#else
        float2 sample0 = make_float2(0.5f, 0.5f);
#endif
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
        Ray_SetMask(my_ray, VISIBILITY_MASK_PRIMARY);
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
    GLOBAL uint const* restrict sobol_mat,
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
    GLOBAL uint const* restrict sobol_mat,
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

uint Part1By1(uint x)
{
    x &= 0x0000ffff;                  // x = ---- ---- ---- ---- fedc ba98 7654 3210
    x = (x ^ (x << 8)) & 0x00ff00ff; // x = ---- ---- fedc ba98 ---- ---- 7654 3210
    x = (x ^ (x << 4)) & 0x0f0f0f0f; // x = ---- fedc ---- ba98 ---- 7654 ---- 3210
    x = (x ^ (x << 2)) & 0x33333333; // x = --fe --dc --ba --98 --76 --54 --32 --10
    x = (x ^ (x << 1)) & 0x55555555; // x = -f-e -d-c -b-a -9-8 -7-6 -5-4 -3-2 -1-0
    return x;
}

uint Morton2D(uint x, uint y)
{
    return (Part1By1(y) << 1) + Part1By1(x);
}

KERNEL void GenerateTileDomain(
    int output_width,
    int output_height,
    int offset_x,
    int offset_y,
    int width,
    int height,
    uint rng_seed,
    uint frame,
    GLOBAL uint* restrict random,
    GLOBAL uint const* restrict sobol_mat,
    GLOBAL int* restrict indices,
    GLOBAL int* restrict count
)
{
    int2 global_id;
    global_id.x = get_global_id(0);
    global_id.y = get_global_id(1);

    int2 local_id;
    local_id.x = get_local_id(0);
    local_id.y = get_local_id(1);

    int2 group_id;
    group_id.x = get_group_id(0);
    group_id.y = get_group_id(1);

    int2 tile_size;
    tile_size.x = get_local_size(0);
    tile_size.y = get_local_size(1);

    int num_tiles_x = output_width / tile_size.x;
    int num_tiles_y = output_height / tile_size.y;

    int start_idx = output_width * offset_y + offset_x;

    if (global_id.x < width && global_id.y < height)
    {
        int idx = start_idx +
            (group_id.y * tile_size.y + local_id.y) * output_width +
            (group_id.x * tile_size.x + local_id.x);

        indices[global_id.y * width + global_id.x] = idx;
    }

    if (global_id.x == 0 && global_id.y == 0)
    {
        *count = width * height;
    }
}

KERNEL void GenerateTileDomain_Adaptive(
    int output_width,
    int output_height,
    int offset_x,
    int offset_y,
    int width,
    int height,
    uint rng_seed,
    uint frame,
    GLOBAL uint* restrict random,
    GLOBAL uint const* restrict sobol_mat,
    GLOBAL int const* restrict tile_distribution,
    GLOBAL int* restrict indices,
    GLOBAL int* restrict count
)
{
    int2 global_id;
    global_id.x = get_global_id(0);
    global_id.y = get_global_id(1);

    int2 local_id;
    local_id.x = get_local_id(0);
    local_id.y = get_local_id(1);

    int2 group_id;
    group_id.x = get_group_id(0);
    group_id.y = get_group_id(1);

    int2 tile_size;
    tile_size.x = get_local_size(0);
    tile_size.y = get_local_size(1);


    // Initialize sampler  
    Sampler sampler;
    int x = global_id.x;
    int y = global_id.y;
#if SAMPLER == SOBOL
    uint scramble = random[x + output_width * y] * 0x1fe3434f;

    if (frame & 0xF)
    {
        random[x + output_width * y] = WangHash(scramble);
    }

    Sampler_Init(&sampler, frame, SAMPLE_DIM_IMG_PLANE_EVALUATE_OFFSET, scramble);
#elif SAMPLER == RANDOM
    uint scramble = x + output_width * y * rng_seed;
    Sampler_Init(&sampler, scramble);
#elif SAMPLER == CMJ
    uint rnd = random[group_id.x + output_width *group_id.y];
    uint scramble = rnd * 0x1fe3434f * ((frame + 133 * rnd) / (CMJ_DIM * CMJ_DIM));
    Sampler_Init(&sampler, frame % (CMJ_DIM * CMJ_DIM), SAMPLE_DIM_IMG_PLANE_EVALUATE_OFFSET, scramble);
#endif

    float2 sample = Sampler_Sample2D(&sampler, SAMPLER_ARGS);

    float pdf;
    int tile = Distribution1D_SampleDiscrete(sample.x, tile_distribution, &pdf);

    int num_tiles_x = output_width / tile_size.x;
    int num_tiles_y = output_height / tile_size.y;

    int tile_y = clamp(tile / num_tiles_x , 0, num_tiles_y - 1);
    int tile_x = clamp(tile % num_tiles_x, 0, num_tiles_x - 1);

    int start_idx = output_width * offset_y + offset_x;

    if (global_id.x < width && global_id.y < height)
    {
        int idx = start_idx +
            (tile_y * tile_size.y + local_id.y) * output_width +
            (tile_x * tile_size.x + local_id.x);

        indices[global_id.y * width + global_id.x] = idx;
    }

    if (global_id.x == 0 && global_id.y == 0)
    {
        *count = width * height;
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
    GLOBAL uint const* restrict sobol_mat, 
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
    GLOBAL float4* restrict aov_gloss,
	// Mesh_id enabled flag
    int mesh_id_enabled,
	// Mesh_id AOV
    GLOBAL float4* restrict mesh_id,
    // Depth enabled flag
    int depth_enabled,
    // Depth map
    GLOBAL float4* restrict aov_depth,
    // NOTE: following are fake parameters, handled outside
    int visibility_enabled,
    GLOBAL float4* restrict aov_visibility
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
            
            if (mesh_id_enabled)
            {
                mesh_id[idx] = make_float4(isect.shapeid, isect.shapeid, isect.shapeid, 1.f);
            }
            
            if (depth_enabled)
            {
                float w = aov_depth[idx].w;
                if (w == 0.f)
                {
                    aov_depth[idx].xyz = isect.uvwt.w;
                    aov_depth[idx].w = 1.f;
                }
                else
                {
                    aov_depth[idx].xyz += isect.uvwt.w;
                    aov_depth[idx].w += 1.f;
                }
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

//#define ADAPTIVITY_DEBUG
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
#ifdef ADAPTIVITY_DEBUG
        float a = v.w < 1024 ? min(1.f, v.w / 1024.f) : 0.f;
        float4 mul_color = make_float4(1.f, 1.f - a, 1.f - a, 1.f);
        v *= mul_color;
#endif

        float4 val = clamp(native_powr(v / v.w, 1.f / gamma), 0.f, 1.f);
        write_imagef(img, make_int2(global_idx, global_idy), val);
    }
} 

KERNEL void AccumulateSingleSample(
    GLOBAL float4 const* restrict src_sample_data,
    GLOBAL float4* restrict dst_accumulation_data,
    GLOBAL int* restrict scatter_indices,
    int num_elements
)
{
    int global_id = get_global_id(0);

    if (global_id < num_elements)
    {
        int idx = scatter_indices[global_id];
        float4 sample = src_sample_data[global_id];
        dst_accumulation_data[idx].xyz += sample.xyz;
        dst_accumulation_data[idx].w += 1.f;
    }
}

INLINE void group_reduce_add(__local float* lds, int size, int lid)
{
    for (int offset = (size >> 1); offset > 0; offset >>= 1)
    {
        if (lid < offset)
        {
            lds[lid] += lds[lid + offset];
        }
        barrier(CLK_LOCAL_MEM_FENCE);
    }
}

INLINE void group_reduce_min(__local float* lds, int size, int lid)
{
    for (int offset = (size >> 1); offset > 0; offset >>= 1)
    {
        if (lid < offset)
        {
            lds[lid] = min(lds[lid], lds[lid + offset]);
        }
        barrier(CLK_LOCAL_MEM_FENCE);
    }
}


INLINE void group_reduce_max(__local float* lds, int size, int lid)
{
    for (int offset = (size >> 1); offset > 0; offset >>= 1)
    {
        if (lid < offset)
        {
            lds[lid] = max(lds[lid], lds[lid + offset]);
        }
        barrier(CLK_LOCAL_MEM_FENCE);
    }
}


KERNEL void EstimateVariance(
    GLOBAL float4 const* restrict image_buffer,
    GLOBAL float* restrict variance_buffer,
    int width,
    int height
)
{
    __local float lds[256];

    int x = get_global_id(0);
    int y = get_global_id(1);
    int lx = get_local_id(0);
    int ly = get_local_id(1);
    int gx = get_group_id(0);
    int gy = get_group_id(1);
    int wx = get_local_size(0);
    int wy = get_local_size(1);
    int num_tiles = (width + wx - 1) / wx;
    int lid = ly * wx + lx;

    float value = 0.f;
    if (x < width && y < height)
    {
        float4 rw = image_buffer[y * width + x]; rw /= rw.w;
        value = 4*luminance(clamp(rw.xyz, 0.f, 1.f));
        rw = y + 1 < height ? image_buffer[(y + 1) * width + x] : image_buffer[y * width + x]; rw /= rw.w;
        value -= luminance(clamp(rw.xyz, 0.f, 1.f));
        rw = y - 1 >= 0 ? image_buffer[(y - 1) * width + x] : image_buffer[y * width + x]; rw /= rw.w;
        value -= luminance(clamp(rw.xyz, 0.f, 1.f));
        rw = x + 1 < width ? image_buffer[y * width + x + 1] : image_buffer[y * width + x]; rw /= rw.w;
        value -= luminance(clamp(rw.xyz, 0.f, 1.f));
        rw = x - 1 >= 0 ? image_buffer[y * width + x - 1] : image_buffer[y * width + x]; rw /= rw.w;
        value -= luminance(clamp(rw.xyz, 0.f, 1.f));
        //rw = y + 1 < height && x + 1 < width ? image_buffer[(y + 1) * width + x + 1] : image_buffer[y * width + x]; rw /= rw.w;
        //value -= luminance(clamp(rw.xyz, 0.f, 1.f));
        //rw = y - 1 >= 0 && x - 1 >= 0 ? image_buffer[(y - 1) * width + x - 1] : image_buffer[y * width + x]; rw /= rw.w;
        //value -= luminance(clamp(rw.xyz, 0.f, 1.f));
        //rw = y + 1 < height && x - 1 >= 0 ? image_buffer[(y + 1) * width + x - 1] : image_buffer[y * width + x]; rw /= rw.w;
        //value -= luminance(clamp(rw.xyz, 0.f, 1.f));
        //rw = y - 1 >= 0 && x + 1 < width ? image_buffer[(y - 1) * width + x + 1] : image_buffer[y * width + x]; rw /= rw.w;
        //value -= luminance(clamp(rw.xyz, 0.f, 1.f));
    }

    value = fabs(value);
    lds[lid] = value;
    barrier(CLK_LOCAL_MEM_FENCE);

    group_reduce_add(lds, 256, lid);

    float mean = lds[0] / (wx * wy);
    barrier(CLK_LOCAL_MEM_FENCE);

    /*lds[lid] = (mean - value) * (mean - value);
    barrier(CLK_LOCAL_MEM_FENCE);

    group_reduce_add(lds, 256, lid);*/

    if (x < width && y < height)
    {
        if (lx == 0 && ly == 0)
        {
            //float dev = lds[0] / (wx * wy - 1);
            variance_buffer[gy * num_tiles + gx] = mean;
        }
    }
}

KERNEL
void  OrthographicCamera_GeneratePaths(
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
                                     GLOBAL uint const* restrict sobol_mat
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
#ifndef BAIKAL_GENERATE_SAMPLE_AT_PIXEL_CENTER
        float2 sample0 = Sampler_Sample2D(&sampler, SAMPLER_ARGS);
#else
        float2 sample0 = make_float2(0.5f, 0.5f);
#endif
        
        // Calculate [0..1] image plane sample
        float2 img_sample;
        img_sample.x = (float)x / output_width + sample0.x / output_width;
        img_sample.y = (float)y / output_height + sample0.y / output_height;
        
        // Transform into [-0.5, 0.5]
        float2 h_sample = img_sample - make_float2(0.5f, 0.5f);
        // Transform into [-dim/2, dim/2]
        float2 c_sample = h_sample * camera->dim;
        
        // Calculate direction to image plane
        my_ray->d.xyz = normalize(camera->forward);
        // Origin == camera position + nearz * d
        my_ray->o.xyz = camera->p + c_sample.x * camera->right + c_sample.y * camera->up;
        // Max T value = zfar - znear since we moved origin to znear
        my_ray->o.w = camera->zcap.y - camera->zcap.x;
        // Generate random time from 0 to 1
        my_ray->d.w = sample0.x;
        // Set ray max
        my_ray->extra.x = 0xFFFFFFFF;
        my_ray->extra.y = 0xFFFFFFFF;
        Ray_SetExtra(my_ray, 1.f);
        Ray_SetMask(my_ray, VISIBILITY_MASK_PRIMARY);
    }
}

///< Illuminate missing rays
KERNEL void ShadeBackgroundImage(
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
    int background_idx,
    // Output size
    int width,
    int height,
    // Textures
    TEXTURE_ARG_LIST,
    // Output values
    GLOBAL float4* restrict output
)
{
    int global_id = get_global_id(0);

    if (global_id < num_rays)
    {
        int pixel_idx = pixel_indices[global_id];
        int output_index = output_indices[pixel_idx];

        float x = (float)(output_index % width) / (float)width;
        float y = (float)(output_index / width) / (float)height;

        float4 v = make_float4(0.f, 0.f, 0.f, 1.f);

        // In case of a miss
        if (isects[global_id].shapeid < 0)
        {
            float2 uv = make_float2(x, y);
            v.xyz = Texture_Sample2D(uv, TEXTURE_ARGS_IDX(background_idx)).xyz;
        }
        
        ADD_FLOAT4(&output[output_index], v);
    }
}


#endif // MONTE_CARLO_RENDERER_CL
