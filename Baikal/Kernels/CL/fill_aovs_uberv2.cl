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
#include <../Baikal/Kernels/CL/bxdf.cl>
#include <../Baikal/Kernels/CL/light.cl>
#include <../Baikal/Kernels/CL/scene.cl>
#include <../Baikal/Kernels/CL/volumetrics.cl>
#include <../Baikal/Kernels/CL/path.cl>
#include <../Baikal/Kernels/CL/vertex.cl>

// Fill AOVs
KERNEL void FillAOVsUberV2(
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
    GLOBAL int const* restrict material_attributes,
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
    // Shape id map enabled flag
    int shape_ids_enabled,
    // Shape id map stores shape ud in every pixel
    // And negative number if there is no any shape in the pixel
    GLOBAL float4* restrict aov_shape_ids,
    // NOTE: following are fake parameters, handled outside
    int visibility_enabled,
    GLOBAL float4* restrict aov_visibility,
    GLOBAL InputMapData const* restrict input_map_values
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
        material_attributes,
        input_map_values,
        lights,
        env_light_idx,
        num_lights
    };

    // Only applied to active rays after compaction
    if (global_id < *num_items)
    {
        Intersection isect = isects[global_id];
        int idx = pixel_idx[global_id];

        if (shape_ids_enabled)
            aov_shape_ids[idx].x = -1;

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
                UberV2ShaderData uber_shader_data;
                UberV2PrepareInputs(&diffgeo, input_map_values, material_attributes, TEXTURE_ARGS, &uber_shader_data);
                GetMaterialBxDFType(wi, &sampler, SAMPLER_ARGS, &diffgeo, &uber_shader_data);

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
                UberV2_ApplyShadingNormal(&diffgeo, &uber_shader_data);
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
                UberV2ShaderData uber_shader_data;
                UberV2PrepareInputs(&diffgeo, input_map_values, material_attributes, TEXTURE_ARGS, &uber_shader_data);

                const float3 kd = ((diffgeo.mat.layers & kDiffuseLayer) == kDiffuseLayer) ?
                    uber_shader_data.diffuse_color.xyz : (float3)(0.0f);

                aov_albedo[idx].xyz += kd;
                aov_albedo[idx].w += 1.f;
            }

            if (world_tangent_enabled)
            {
                float ngdotwi = dot(diffgeo.ng, wi);
                bool backfacing = ngdotwi < 0.f;

                // Select BxDF
                UberV2ShaderData uber_shader_data;
                UberV2PrepareInputs(&diffgeo, input_map_values, material_attributes, TEXTURE_ARGS, &uber_shader_data);
                GetMaterialBxDFType(wi, &sampler, SAMPLER_ARGS, &diffgeo, &uber_shader_data);

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

                UberV2_ApplyShadingNormal(&diffgeo, &uber_shader_data);
                DifferentialGeometry_CalculateTangentTransforms(&diffgeo);

                aov_world_tangent[idx].xyz += diffgeo.dpdu;
                aov_world_tangent[idx].w += 1.f;
            }

            if (world_bitangent_enabled)
            {
                float ngdotwi = dot(diffgeo.ng, wi);
                bool backfacing = ngdotwi < 0.f;

                // Select BxDF
                UberV2ShaderData uber_shader_data;
                UberV2PrepareInputs(&diffgeo, input_map_values, material_attributes, TEXTURE_ARGS, &uber_shader_data);
                GetMaterialBxDFType(wi, &sampler, SAMPLER_ARGS, &diffgeo, &uber_shader_data);

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

                UberV2_ApplyShadingNormal(&diffgeo, &uber_shader_data);
                DifferentialGeometry_CalculateTangentTransforms(&diffgeo);

                aov_world_bitangent[idx].xyz += diffgeo.dpdv;
                aov_world_bitangent[idx].w += 1.f;
            }

            if (gloss_enabled)
            {
                float ngdotwi = dot(diffgeo.ng, wi);
                bool backfacing = ngdotwi < 0.f;

                // Select BxDF
                UberV2ShaderData uber_shader_data;
                UberV2PrepareInputs(&diffgeo, input_map_values, material_attributes, TEXTURE_ARGS, &uber_shader_data);
                GetMaterialBxDFType(wi, &sampler, SAMPLER_ARGS, &diffgeo, &uber_shader_data);

                float gloss = 0.f;
                if ((diffgeo.mat.layers & kCoatingLayer) == kCoatingLayer)
                {
                    gloss = 1.0f;
                }
                else if ((diffgeo.mat.layers & kReflectionLayer) == kReflectionLayer)
                {
                    gloss = 1.0f - uber_shader_data.reflection_roughness;
                    if ((diffgeo.mat.layers & kRefractionLayer) == kRefractionLayer)
                    {
                        gloss = max(gloss, 1.0f - uber_shader_data.refraction_roughness);
                    }
                }
                else if ((diffgeo.mat.layers & kRefractionLayer) == kRefractionLayer)
                {
                    gloss = 1.0f - uber_shader_data.refraction_roughness;
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

            if (shape_ids_enabled)
            {
                aov_shape_ids[idx].x = shapes[isect.shapeid - 1].id;
            }
        }
    }
}



#endif // MONTE_CARLO_RENDERER_CL
