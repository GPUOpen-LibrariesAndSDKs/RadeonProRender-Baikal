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
#include <../Baikal/Kernels/CL/ray.h>


typedef struct
{
    // Vertices
    GLOBAL float3 const* restrict vertices;
    // Normals
    GLOBAL float3 const* restrict normals;
    // UVs
    GLOBAL float2 const* restrict uvs;
    // Indices
    GLOBAL int const* restrict indices;
    // Shapes
    GLOBAL Shape const* restrict shapes;
    // Material attributes
    GLOBAL int const* restrict material_attributes;
    // Input map values
    GLOBAL InputMapData const* restrict input_map_values;
    // Emissive objects
    GLOBAL Light const* restrict lights;
    // Envmap idx
    int env_light_idx;
    // Number of emissive objects
    int num_lights;
    // Light distribution
    GLOBAL int const* restrict light_distribution;
} Scene;

// Get triangle vertices given scene, shape index and prim index
INLINE void Scene_GetTriangleVertices(Scene const* scene, int shape_idx, int prim_idx, float3* v0, float3* v1, float3* v2)
{
    // Extract shape data
    Shape shape = scene->shapes[shape_idx];

    // Fetch indices starting from startidx and offset by prim_idx
    int i0 = scene->indices[shape.startidx + 3 * prim_idx];
    int i1 = scene->indices[shape.startidx + 3 * prim_idx + 1];
    int i2 = scene->indices[shape.startidx + 3 * prim_idx + 2];

    // Fetch positions and transform to world space
    *v0 = matrix_mul_point3(shape.transform, scene->vertices[shape.startvtx + i0]);
    *v1 = matrix_mul_point3(shape.transform, scene->vertices[shape.startvtx + i1]);
    *v2 = matrix_mul_point3(shape.transform, scene->vertices[shape.startvtx + i2]);
}

// Get triangle uvs given scene, shape index and prim index
INLINE void Scene_GetTriangleUVs(Scene const* scene, int shape_idx, int prim_idx, float2* uv0, float2* uv1, float2* uv2)
{
    // Extract shape data
    Shape shape = scene->shapes[shape_idx];

    // Fetch indices starting from startidx and offset by prim_idx
    int i0 = scene->indices[shape.startidx + 3 * prim_idx];
    int i1 = scene->indices[shape.startidx + 3 * prim_idx + 1];
    int i2 = scene->indices[shape.startidx + 3 * prim_idx + 2];

    // Fetch positions and transform to world space
    *uv0 = scene->uvs[shape.startvtx + i0];
    *uv1 = scene->uvs[shape.startvtx + i1];
    *uv2 = scene->uvs[shape.startvtx + i2];
}

INLINE void Scene_GetTriangleNormals(Scene const* scene, int shape_idx, int prim_idx, float3* n0, float3* n1, float3* n2)
{
    // Extract shape data
    Shape shape = scene->shapes[shape_idx];

    // Fetch indices starting from startidx and offset by prim_idx
    int i0 = scene->indices[shape.startidx + 3 * prim_idx];
    int i1 = scene->indices[shape.startidx + 3 * prim_idx + 1];
    int i2 = scene->indices[shape.startidx + 3 * prim_idx + 2];

    // Fetch positions and transform to world space
    *n0 = matrix_mul_vector3(shape.transform, scene->normals[shape.startvtx + i0]);
    *n1 = matrix_mul_vector3(shape.transform, scene->normals[shape.startvtx + i1]);
    *n2 = matrix_mul_vector3(shape.transform, scene->normals[shape.startvtx + i2]);
}

// Interpolate position, normal and uv
INLINE void Scene_InterpolateAttributes(Scene const* scene, int shape_idx, int prim_idx, float2 barycentrics, float3* p, float3* n, float2* uv, float* area)
{
    // Extract shape data
    Shape shape = scene->shapes[shape_idx];

    // Fetch indices starting from startidx and offset by prim_idx
    int i0 = scene->indices[shape.startidx + 3 * prim_idx];
    int i1 = scene->indices[shape.startidx + 3 * prim_idx + 1];
    int i2 = scene->indices[shape.startidx + 3 * prim_idx + 2];

    // Fetch normals
    float3 n0 = scene->normals[shape.startvtx + i0];
    float3 n1 = scene->normals[shape.startvtx + i1];
    float3 n2 = scene->normals[shape.startvtx + i2];

    // Fetch positions and transform to world space
    float3 v0 = matrix_mul_point3(shape.transform, scene->vertices[shape.startvtx + i0]);
    float3 v1 = matrix_mul_point3(shape.transform, scene->vertices[shape.startvtx + i1]);
    float3 v2 = matrix_mul_point3(shape.transform, scene->vertices[shape.startvtx + i2]);

    // Fetch UVs
    float2 uv0 = scene->uvs[shape.startvtx + i0];
    float2 uv1 = scene->uvs[shape.startvtx + i1];
    float2 uv2 = scene->uvs[shape.startvtx + i2];

    // Calculate barycentric position and normal
    *p = (1.f - barycentrics.x - barycentrics.y) * v0 + barycentrics.x * v1 + barycentrics.y * v2;
    *n = normalize(matrix_mul_vector3(shape.transform, (1.f - barycentrics.x - barycentrics.y) * n0 + barycentrics.x * n1 + barycentrics.y * n2));
    *uv = (1.f - barycentrics.x - barycentrics.y) * uv0 + barycentrics.x * uv1 + barycentrics.y * uv2;
    *area = 0.5f * length(cross(v2 - v0, v1 - v0));
}

// Interpolate position, normal and uv
INLINE void Scene_InterpolateVertices(Scene const* scene, int shape_idx, int prim_idx, float2 barycentrics, float3* p)
{
    // Extract shape data
    Shape shape = scene->shapes[shape_idx];

    // Fetch indices starting from startidx and offset by prim_idx
    int i0 = scene->indices[shape.startidx + 3 * prim_idx];
    int i1 = scene->indices[shape.startidx + 3 * prim_idx + 1];
    int i2 = scene->indices[shape.startidx + 3 * prim_idx + 2];

    // Fetch positions and transform to world space
    float3 v0 = matrix_mul_point3(shape.transform, scene->vertices[shape.startvtx + i0]);
    float3 v1 = matrix_mul_point3(shape.transform, scene->vertices[shape.startvtx + i1]);
    float3 v2 = matrix_mul_point3(shape.transform, scene->vertices[shape.startvtx + i2]);

    // Calculate barycentric position and normal
    *p = (1.f - barycentrics.x - barycentrics.y) * v0 + barycentrics.x * v1 + barycentrics.y * v2;
}

// Interpolate position, normal and uv
INLINE void Scene_InterpolateVerticesFromIntersection(Scene const* scene, Intersection const* isect, float3* p)
{
    // Extract shape data
    int shape_idx = isect->shapeid - 1;
    int prim_idx = isect->primid;
    float2 barycentrics = isect->uvwt.xy;

    Shape shape = scene->shapes[shape_idx];

    // Fetch indices starting from startidx and offset by prim_idx
    int i0 = scene->indices[shape.startidx + 3 * prim_idx];
    int i1 = scene->indices[shape.startidx + 3 * prim_idx + 1];
    int i2 = scene->indices[shape.startidx + 3 * prim_idx + 2];

    // Fetch positions and transform to world space
    float3 v0 = matrix_mul_point3(shape.transform, scene->vertices[shape.startvtx + i0]);
    float3 v1 = matrix_mul_point3(shape.transform, scene->vertices[shape.startvtx + i1]);
    float3 v2 = matrix_mul_point3(shape.transform, scene->vertices[shape.startvtx + i2]);

    // Calculate barycentric position and normal
    *p = (1.f - barycentrics.x - barycentrics.y) * v0 + barycentrics.x * v1 + barycentrics.y * v2;
}

// Interpolate position, normal and uv
INLINE void Scene_InterpolateNormalsFromIntersection(Scene const* scene, Intersection const* isect, float3* n)
{
    // Extract shape data
    int shape_idx = isect->shapeid - 1;
    int prim_idx = isect->primid;
    float2 barycentrics = isect->uvwt.xy;

    Shape shape = scene->shapes[shape_idx];

    // Fetch indices starting from startidx and offset by prim_idx
    int i0 = scene->indices[shape.startidx + 3 * prim_idx];
    int i1 = scene->indices[shape.startidx + 3 * prim_idx + 1];
    int i2 = scene->indices[shape.startidx + 3 * prim_idx + 2];

    // Fetch normals
    float3 n0 = scene->normals[shape.startvtx + i0];
    float3 n1 = scene->normals[shape.startvtx + i1];
    float3 n2 = scene->normals[shape.startvtx + i2];

    // Calculate barycentric position and normal
    *n = normalize(matrix_mul_vector3(shape.transform, (1.f - barycentrics.x - barycentrics.y) * n0 + barycentrics.x * n1 + barycentrics.y * n2));
}


INLINE int Scene_GetVolumeIndex(Scene const* scene, int shape_idx)
{
    Shape shape = scene->shapes[shape_idx];
    return shape.volume_idx;
}

/// Fill DifferentialGeometry structure based on intersection info from RadeonRays
void Scene_FillDifferentialGeometry(// Scene
                              Scene const* scene,
                              // RadeonRays intersection
                              Intersection const* isect,
                              // Differential geometry
                              DifferentialGeometry* diffgeo
                              )
{
    // Determine shape and polygon
    int shape_idx = isect->shapeid - 1;
    int prim_idx = isect->primid;

    // Get barycentrics
    float2 barycentrics = isect->uvwt.xy;

    // Extract shape data
    Shape shape = scene->shapes[shape_idx];

    // Interpolate attributes
    float3 p;
    float3 n;
    float2 uv;
    float area;
    Scene_InterpolateAttributes(scene, shape_idx, prim_idx, barycentrics, &p, &n, &uv, &area);
    // Triangle area (for area lighting)
    diffgeo->area = area;

    // Calculate barycentric position and normal
    diffgeo->n = n;
    diffgeo->p = p;
    diffgeo->uv = uv;

    // Get vertices
    float3 v0, v1, v2;
    Scene_GetTriangleVertices(scene, shape_idx, prim_idx, &v0, &v1, &v2);

    // Calculate true normal
    diffgeo->ng = normalize(cross(v1 - v0, v2 - v0));

    // Get material at shading point
    diffgeo->mat = shape.material;

    // Get UVs
    float2 uv0, uv1, uv2;
    Scene_GetTriangleUVs(scene, shape_idx, prim_idx, &uv0, &uv1, &uv2);

    // Reverse geometric normal if shading normal points to different side
    if (dot(diffgeo->ng, diffgeo->n) < 0.f)
    {
        diffgeo->ng = -diffgeo->ng;
    }

    // Calculate position and normal derivatives of the surface with respect to texcoord
    // From PBRT book
    float2 duv02 = uv0 - uv2;
    float2 duv12 = uv1 - uv2;
    float  det = duv02.x * duv12.y - duv02.y * duv12.x;
    bool   degenerate_uv = fabs(det) < 1e-08f;

    float3 n0, n1, n2;
    Scene_GetTriangleNormals(scene, shape_idx, prim_idx, &n0, &n1, &n2);

    if (!degenerate_uv)
    {
        float invdet = 1.f / det;

        float3 dp02  = v0 - v2;
        float3 dp12  = v1 - v2;
        diffgeo->dpdu = ( duv12.y * dp02 - duv02.y * dp12) * invdet;
        diffgeo->dpdv = (-duv12.x * dp02 + duv02.x * dp12) * invdet;

        float3 dn02 = n0 - n2;
        float3 dn12 = n1 - n2;
        diffgeo->dndu = ( duv12.y * dn02 - duv02.y * dn12) * invdet;
        diffgeo->dndv = (-duv12.x * dn02 + duv02.x * dn12) * invdet;
    }
    else
    {
        diffgeo->dpdu = diffgeo->dpdv = 0.0f;
        diffgeo->dndu = diffgeo->dndv = 0.0f;
    }

    // Initialize screen space uv derivatives
    diffgeo->duvdx = diffgeo->duvdy = 0.0f;

    // Compute tangent and bitangent vectors
    diffgeo->tangent = diffgeo->dpdu;
    // Gram–Schmidt orthogonalization
    diffgeo->tangent -= dot(diffgeo->n, diffgeo->tangent) * diffgeo->n;
    diffgeo->tangent = normalize(diffgeo->tangent);
    // Bitangent vector need to be orthogonal to tangent, since we use
    // the advantage of an orthogonal matrix that is easily invertible
    diffgeo->bitangent = normalize(cross(diffgeo->tangent, diffgeo->n));
/*
    // Check handedness when uv are mirrored
    if (dot(cross(diffgeo->n, diffgeo->tangent), diffgeo->bitangent) < 0.0f)
    {
        diffgeo->tangent *= -1.0f;
    }
*/


}

// Calculate tangent transform matrices inside differential geometry
INLINE void DifferentialGeometry_CalculateTangentTransforms(DifferentialGeometry* diffgeo)
{
    diffgeo->world_to_tangent = matrix_from_rows3(diffgeo->tangent, diffgeo->n, diffgeo->bitangent);

    diffgeo->world_to_tangent.m0.w = -dot(diffgeo->tangent, diffgeo->p);
    diffgeo->world_to_tangent.m1.w = -dot(diffgeo->n, diffgeo->p);
    diffgeo->world_to_tangent.m2.w = -dot(diffgeo->bitangent, diffgeo->p);

    diffgeo->tangent_to_world = matrix_from_cols3(diffgeo->world_to_tangent.m0.xyz,
        diffgeo->world_to_tangent.m1.xyz, diffgeo->world_to_tangent.m2.xyz);

    diffgeo->tangent_to_world.m0.w = diffgeo->p.x;
    diffgeo->tangent_to_world.m1.w = diffgeo->p.y;
    diffgeo->tangent_to_world.m2.w = diffgeo->p.z;
}

// Compute screen-space derivative of uv coords
INLINE void DifferentialGeometry_CalculatePartialDerivatives(
                          // Auxiliary ray
                          GLOBAL aux_ray const* my_ray,
                          // Differential geometry
                          DifferentialGeometry* diffgeo,
                          float3* position_derivative,
                          float2* uv_derivative
                          )
{
    //float3 o = vload_half3(0, (GLOBAL half*)&my_ray->o);
    //float3 d = vload_half3(0, (GLOBAL half*)&my_ray->d);
    float3 o = my_ray->o;
    float3 d = my_ray->d;

    // Find intersection point of auxiliary ray with the dpdu plane
    float t = dot(diffgeo->n, diffgeo->p - o) / dot(diffgeo->n, d);

    float3 p = o + d * t;

    // Next we need to find uv offset from the position offset
    // This can be found by solving the system:
    // delta_p = dpdu * delta_u + dpdv * delta_v

    // This can be written in matrix form:
    // (delta_p.x)    (dpdu.x dpdv.x)   (delta_u)
    // (delta_p.y) == (dpdu.y dpdv.y) * (       )
    // (delta_p.z)    (dpdu.z dpdv.z)   (delta_v)

    // This is an overdetermined system (has 2 variables and 3 equations), so
    // one of the equations could be degenerate

    // Calculate cross product of the derivatives (actually, this is the surface normal)
    // to choose non-degenerate equations
    float3 derivative_det = fabs(diffgeo->n);

    // Linear system of equations created from partial derivatives
    float4 derivative_matrix;

    // Delta position between intersection points of main and auxiliary ray
    float2 delta_p;

    // Choose not degenerated matrix rows
    if (derivative_det.x > max(derivative_det.y, derivative_det.z))
    {
        derivative_matrix = make_float4(diffgeo->dpdu.y,
                                        diffgeo->dpdv.y,
                                        diffgeo->dpdu.z,
                                        diffgeo->dpdv.z);
        delta_p = p.yz - diffgeo->p.yz;

    }
    else if (derivative_det.y > max(derivative_det.x, derivative_det.z))
    {
        derivative_matrix = make_float4(diffgeo->dpdu.x,
                                        diffgeo->dpdv.x,
                                        diffgeo->dpdu.z,
                                        diffgeo->dpdv.z);
        delta_p = p.xz - diffgeo->p.xz;

    }
    else
    {
        derivative_matrix = make_float4(diffgeo->dpdu.x,
                                        diffgeo->dpdv.x,
                                        diffgeo->dpdu.y,
                                        diffgeo->dpdv.y);
        delta_p = p.xy - diffgeo->p.xy;

    }

    // Solve linear system using Cramer rule
    float matrix_det = derivative_matrix.x * derivative_matrix.w - derivative_matrix.y * derivative_matrix.z;

    float det1 = derivative_matrix.w * delta_p.x - derivative_matrix.y * delta_p.y;
    float det2 = derivative_matrix.x * delta_p.y - derivative_matrix.z * delta_p.x;

    *position_derivative = p - diffgeo->p;
    *uv_derivative = make_float2(det1, det2) / matrix_det;

}

INLINE void DifferentialGeometry_CalculateScreenSpaceUVDerivatives(
                                                // Differential geometry
                                                DifferentialGeometry* diffgeo,
                                                // auxiliary rays
                                                GLOBAL aux_ray const* aux_ray_x,
                                                GLOBAL aux_ray const* aux_ray_y
                                                )
{
    if (aux_ray_x != NULL && aux_ray_y != NULL)
    {
        // Calculate differentials
        DifferentialGeometry_CalculatePartialDerivatives(aux_ray_x, diffgeo, &diffgeo->dpdx, &diffgeo->duvdx);
        DifferentialGeometry_CalculatePartialDerivatives(aux_ray_y, diffgeo, &diffgeo->dpdy, &diffgeo->duvdy);
    }

}

#define POWER_SAMPLING

// Sample light index
INLINE int Scene_SampleLight(Scene const* scene, float sample, float* pdf)
{
#ifndef POWER_SAMPLING
    int num_lights = scene->num_lights;
    int light_idx = clamp((int)(sample * num_lights), 0, num_lights - 1);
    *pdf = 1.f / num_lights;
    return light_idx;
#else
    int num_lights = scene->num_lights;
    int light_idx = Distribution1D_SampleDiscrete(sample, scene->light_distribution, pdf);
    return light_idx;
#endif
}

#endif
