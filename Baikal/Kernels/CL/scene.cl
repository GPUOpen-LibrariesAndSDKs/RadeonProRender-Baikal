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
#ifndef SCENE_CL
#define SCENE_CL

#include <../Baikal/Kernels/CL/common.cl>
#include <../Baikal/Kernels/CL/utils.cl>
#include <../Baikal/Kernels/CL/payload.cl>

typedef struct
{
    /// Mesh data:
    GLOBAL float3 const* restrict mesh_vertices;
    GLOBAL float3 const* restrict mesh_normals;
    GLOBAL float2 const* restrict mesh_uvs;
    GLOBAL int const* restrict mesh_indices;

    // Curves data:
    GLOBAL float4 const* restrict curve_vertices;
    GLOBAL int const* restrict curve_indices;

    // Shapes
    GLOBAL Shape const* restrict shapes;

    // Material IDs
    GLOBAL int const* restrict mesh_materialids;  // per face
    GLOBAL int const* restrict curve_materialids; // per curve

    // Materials
    GLOBAL Material const* restrict materials;

    // Emissive objects
    GLOBAL Light const* restrict lights;

    // Envmap idx
    int env_light_idx;

    // Number of emissive objects
    int num_lights;

} Scene;

// Get triangle vertices given scene, shape index and prim index
INLINE void Scene_GetTriangleVertices(Scene const* scene, int shape_idx, int prim_idx, float3* v0, float3* v1, float3* v2)
{
    // Extract shape data
    Shape shape = scene->shapes[shape_idx];

    // Fetch indices starting from startidx and offset by prim_idx
    int i0 = scene->mesh_indices[shape.startidx + 3 * prim_idx];
    int i1 = scene->mesh_indices[shape.startidx + 3 * prim_idx + 1];
    int i2 = scene->mesh_indices[shape.startidx + 3 * prim_idx + 2];

    // Fetch positions and transform to world space
    *v0 = matrix_mul_point3(shape.transform, scene->mesh_vertices[shape.startvtx + i0]);
    *v1 = matrix_mul_point3(shape.transform, scene->mesh_vertices[shape.startvtx + i1]);
    *v2 = matrix_mul_point3(shape.transform, scene->mesh_vertices[shape.startvtx + i2]);
}


// Get triangle uvs given scene, shape index and prim index
INLINE void Scene_GetTriangleUVs(Scene const* scene, int shape_idx, int prim_idx, float2* uv0, float2* uv1, float2* uv2)
{
    // Extract shape data
    Shape shape = scene->shapes[shape_idx];

    // Fetch indices starting from startidx and offset by prim_idx
    int i0 = scene->mesh_indices[shape.startidx + 3 * prim_idx];
    int i1 = scene->mesh_indices[shape.startidx + 3 * prim_idx + 1];
    int i2 = scene->mesh_indices[shape.startidx + 3 * prim_idx + 2];

    // Fetch positions and transform to world space
    *uv0 = scene->mesh_uvs[shape.startvtx + i0];
    *uv1 = scene->mesh_uvs[shape.startvtx + i1];
    *uv2 = scene->mesh_uvs[shape.startvtx + i2];
}


// Interpolate position, normal and uv
INLINE void Scene_InterpolateTriangleAttributes(Scene const* scene, int shape_idx, int prim_idx, float2 barycentrics, float3* p, float3* n, float2* uv, float* area)
{
    // Extract shape data
    Shape shape = scene->shapes[shape_idx];

    // Fetch indices starting from startidx and offset by prim_idx
    int i0 = scene->mesh_indices[shape.startidx + 3*prim_idx];
    int i1 = scene->mesh_indices[shape.startidx + 3*prim_idx + 1];
    int i2 = scene->mesh_indices[shape.startidx + 3*prim_idx + 2];

    // Fetch normals
    float3 n0 = scene->mesh_normals[shape.startvtx + i0];
    float3 n1 = scene->mesh_normals[shape.startvtx + i1];
    float3 n2 = scene->mesh_normals[shape.startvtx + i2];

    // Fetch positions and transform to world space
    float3 v0 = matrix_mul_point3(shape.transform, scene->mesh_vertices[shape.startvtx + i0]);
    float3 v1 = matrix_mul_point3(shape.transform, scene->mesh_vertices[shape.startvtx + i1]);
    float3 v2 = matrix_mul_point3(shape.transform, scene->mesh_vertices[shape.startvtx + i2]);

    // Fetch UVs
    float2 uv0 = scene->mesh_uvs[shape.startvtx + i0];
    float2 uv1 = scene->mesh_uvs[shape.startvtx + i1];
    float2 uv2 = scene->mesh_uvs[shape.startvtx + i2];

    // Calculate barycentric position and normal
    *p = (1.f - barycentrics.x - barycentrics.y) * v0 + barycentrics.x * v1 + barycentrics.y * v2;
    *n = normalize(matrix_mul_vector3(shape.transform, (1.f - barycentrics.x - barycentrics.y) * n0 + barycentrics.x * n1 + barycentrics.y * n2));
    *uv = (1.f - barycentrics.x - barycentrics.y) * uv0 + barycentrics.x * uv1 + barycentrics.y * uv2;
    *area = 0.5f * length(cross(v2 - v0, v1 - v0));
}


INLINE void Scene_InterpolateSegmentAttributes(Scene const* scene, int shape_idx, int prim_idx, float u, 
                                               float3* axisHit, float* radiusHit, float3* segmentAxis)
{
    // Extract shape data
    Shape shape = scene->shapes[shape_idx];

    // Fetch indices starting from startidx and offset by prim_idx
    int i0 = scene->curve_indices[shape.startidx + 2*prim_idx];
    int i1 = scene->curve_indices[shape.startidx + 2*prim_idx + 1];

    // Fetch positions and transform to world space
    float4 v0L = scene->curve_vertices[shape.startvtx + i0];
    float4 v1L = scene->curve_vertices[shape.startvtx + i1];

    // @todo: for some reason, shape.transform doesn't get copied correctly
    float3 v0W = v0L.xyz; //matrix_mul_point3(shape.transform, v0L.xyz);
    float3 v1W = v1L.xyz; //matrix_mul_point3(shape.transform, v1L.xyz);

    *axisHit   = (1.f-u)*v0W.xyz + u*v1W.xyz;
    *radiusHit = (1.f-u)*v0L.w   + u*v1L.w;
    *segmentAxis = normalize(v1W.xyz - v0W.xyz);
}

// Get material index of a shape face
INLINE int Scene_GetFaceMaterialIndex(Scene const* scene, int shape_idx, int prim_idx)
{
    Shape shape = scene->shapes[shape_idx];
    return scene->mesh_materialids[shape.start_material_idx + prim_idx];
}

/// Fill DifferentialGeometry structure based on intersection info from RadeonRays
void Scene_FillDifferentialGeometry( Scene const* scene,
                                     Intersection const* isect,
                                     DifferentialGeometry* diffgeo )
{
    // Extract shape data
    int shape_idx = isect->shapeid - 1;
    int prim_idx = isect->primid;
    Shape shape = scene->shapes[shape_idx];
    
    // Get barycentrics
    float2 barycentrics = isect->uvwt.xy;

    // Mesh hit data
    if (shape.typeidx == SHAPE_TYPE_MESH)
    {
        // Interpolate attributes
        float3 p;
        float3 n;
        float2 uv;
        float area;
        Scene_InterpolateTriangleAttributes(scene, shape_idx, prim_idx, barycentrics, &p, &n, &uv, &area);

        // Triangle area (for area lighting)
        diffgeo->area = area;

        // Calculate barycentric position and normal
        diffgeo->n = n;
        diffgeo->p = p;

        // Get vertices
        float3 v0, v1, v2;
        Scene_GetTriangleVertices(scene, shape_idx, prim_idx, &v0, &v1, &v2);

        // Calculate true normal
        diffgeo->ng = normalize(cross(v1 - v0, v2 - v0));

        // Reverse geometric normal if shading normal points to different side
        if (dot(diffgeo->ng, diffgeo->n) < 0.f)
        diffgeo->ng = -diffgeo->ng;

        // Get material at shading point
        int material_idx = Scene_GetFaceMaterialIndex(scene, shape_idx, prim_idx);
        diffgeo->mat = scene->materials[material_idx];

        // Get UVs
        float2 uv0, uv1, uv2;
        Scene_GetTriangleUVs(scene, shape_idx, prim_idx, &uv0, &uv1, &uv2);

        /// Calculate tangent basis
        /// From PBRT book
        float du1 = uv0.x - uv2.x;
        float du2 = uv1.x - uv2.x;
        float dv1 = uv0.y - uv2.y;
        float dv2 = uv1.y - uv2.y;
        float3 dp1 = v0 - v2;
        float3 dp2 = v1 - v2;
        float det = du1 * dv2 - dv1 * du2;

        if (0 && det != 0.f)
        {
            float invdet = 1.f / det;
            diffgeo->dpdu = normalize( (dv2 * dp1 - dv1 * dp2) * invdet );
            diffgeo->dpdv = normalize( (-du2 * dp1 + du1 * dp2) * invdet );
            diffgeo->dpdu -= dot(diffgeo->n, diffgeo->dpdu) * diffgeo->n;
            diffgeo->dpdv -= dot(diffgeo->n, diffgeo->dpdv) * diffgeo->n;
            diffgeo->dpdv -= dot(diffgeo->dpdu, diffgeo->dpdv) * diffgeo->dpdu;
            diffgeo->dpdu = normalize(diffgeo->dpdu);
            diffgeo->dpdv = normalize(diffgeo->dpdv);
        }
        else
        {
            diffgeo->dpdu = normalize(GetOrthoVector(diffgeo->n));
            diffgeo->dpdv = normalize(cross(diffgeo->n, diffgeo->dpdu));
        }

        diffgeo->material_index = material_idx;
    }

    // Curve hit data
    else if (shape.typeidx == SHAPE_TYPE_CURVES)
    {
        float3 axisHit;
        float3 segmentAxis;
        float radiusHit;
        float u = barycentrics.x;
        Scene_InterpolateSegmentAttributes(scene, shape_idx, prim_idx, u, &axisHit, &radiusHit, &segmentAxis);
        diffgeo->dpdu = segmentAxis;

        // make an arbitrary orthonormal basis n, dpdu, dpdv  (given dpdu = segmentAxis)
        if (fabs(diffgeo->dpdu.z) < fabs(diffgeo->dpdu.x))
        {
            diffgeo->n.x =  diffgeo->dpdu.z;
            diffgeo->n.y =  0.f;
            diffgeo->n.z = -diffgeo->dpdu.x;
        }
        else
        {
            diffgeo->n.x =  0.f;
            diffgeo->n.y =  diffgeo->dpdu.z;
            diffgeo->n.z = -diffgeo->dpdu.y;
        }
        diffgeo->n = normalize(diffgeo->n);
        diffgeo->p = axisHit;
        diffgeo->dpdv = cross(diffgeo->n, diffgeo->dpdu);
        diffgeo->ng = diffgeo->n;
        diffgeo->uv = (float2)(u, 0.f);

        // (diffgeo->material_index not required)
        int material_idx = scene->curve_materialids[shape_idx];
        diffgeo->mat = scene->materials[material_idx];
    }
}


// Calculate tangent transform matrices inside differential geometry
INLINE void DifferentialGeometry_CalculateTangentTransforms(DifferentialGeometry* diffgeo)
{
    diffgeo->world_to_tangent = matrix_from_rows3(diffgeo->dpdu, diffgeo->n, diffgeo->dpdv);

    diffgeo->world_to_tangent.m0.w = -dot(diffgeo->dpdu, diffgeo->p);
    diffgeo->world_to_tangent.m1.w = -dot(diffgeo->n, diffgeo->p);
    diffgeo->world_to_tangent.m2.w = -dot(diffgeo->dpdv, diffgeo->p);

    diffgeo->tangent_to_world = matrix_from_cols3(diffgeo->world_to_tangent.m0.xyz, 
    diffgeo->world_to_tangent.m1.xyz, diffgeo->world_to_tangent.m2.xyz);

    diffgeo->tangent_to_world.m0.w = diffgeo->p.x;
    diffgeo->tangent_to_world.m1.w = diffgeo->p.y;
    diffgeo->tangent_to_world.m2.w = diffgeo->p.z;
}

// Sample light index
INLINE int Scene_SampleLight(Scene const* scene, float sample, float* pdf)
{
    int num_lights = scene->num_lights;
    int light_idx = clamp((int)(sample * num_lights), 0, num_lights - 1);
    *pdf = 1.f / num_lights;
    return light_idx;
}

#endif
