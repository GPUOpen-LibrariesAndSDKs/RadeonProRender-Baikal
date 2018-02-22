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
        kPerspective,
        kPhysicalPerspective,
        kSpherical,
        kFisheye,
        kOrthographic
    };

    struct ClwScene
    {
        #include "Kernels/CL/payload.cl"

        CLWBuffer<RadeonRays::float3> vertices;
        CLWBuffer<RadeonRays::float3> normals;
        CLWBuffer<RadeonRays::float2> uvs;
        CLWBuffer<int> indices;
        CLWBuffer<int> shape_id_map;

        CLWBuffer<Shape> shapes;

        CLWBuffer<Material> materials;
        CLWBuffer<Light> lights;
        CLWBuffer<Volume> volumes;
        CLWBuffer<Texture> textures;
        CLWBuffer<char> texturedata;

        CLWBuffer<Camera> camera;
        CLWBuffer<int> light_distributions;

        std::unique_ptr<Bundle> material_bundle;
        std::unique_ptr<Bundle> volume_bundle;
        std::unique_ptr<Bundle> texture_bundle;

        int num_lights;
        int envmapidx;
        int background_idx;
        CameraType camera_type;

        std::vector<RadeonRays::Shape*> isect_shapes;
        std::vector<RadeonRays::Shape*> visible_shapes;
    };
}
