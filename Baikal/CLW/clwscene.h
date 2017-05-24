#pragma once

#include "CLW.h"
#include "math/float3.h"
#include "SceneGraph/scene1.h"
#include "radeon_rays.h"
#include "SceneGraph/Collector/collector.h"


namespace Baikal
{
    using namespace RadeonRays;

    enum class CameraType
    {
        kDefault,
        kPhysical,
        kSpherical,
        kFisheye
    };

    struct ClwScene
    {
        #include "CL/payload.cl"

		// mesh data
        CLWBuffer<RadeonRays::float3> mesh_vertices;
        CLWBuffer<RadeonRays::float3> mesh_normals;
        CLWBuffer<RadeonRays::float2> mesh_uvs;
        CLWBuffer<int> mesh_indices;

		// curve data
		CLWBuffer<RadeonRays::float4> curve_vertices; // "CVs"
		CLWBuffer<int> curve_indices;
		
		//CLWBuffer<RadeonRays::float2> curve_uvs;
		// For now, curve material data ignored.

		// per-shape info
        CLWBuffer<Shape> shapes;
        CLWBuffer<Material> materials;
        CLWBuffer<Light> lights;
        CLWBuffer<int> mesh_materialids;  // mesh material ids, one per face
		CLWBuffer<int> curve_materialids; // one per curves shape

        CLWBuffer<Volume> volumes;
        CLWBuffer<Texture> textures;
        CLWBuffer<char> texturedata;

        CLWBuffer<Camera> camera;

        std::unique_ptr<Bundle> material_bundle;
        std::unique_ptr<Bundle> texture_bundle;

        int num_lights;
        int envmapidx;
        CameraType camera_type;

        std::vector<RadeonRays::Shape*> isect_shapes;
        std::vector<RadeonRays::Shape*> visible_shapes;
    };
}
