#include <float.h>
#include <algorithm>
#include <Math/mathutils.h>
#include "gltf2.h"
#include "ProRenderGLTF.h"

namespace rpr
{
    typedef RadeonProRender::matrix mat4;
    typedef RadeonProRender::float4 vec4;

    struct BBox
    {
        float min[3] = { FLT_MAX };
        float max[3] = { -FLT_MAX };
    };

    static mat4 GetNodeTransformMatrix(gltf::Node& node, const mat4& worldTransform)
    {
        mat4 result;

        // If node matrix isn't the identity then use that.
        if (node.matrix != decltype(node.matrix) { 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0 })
        {
            mat4 localTransform;
            memcpy(&localTransform.m[0][0], node.matrix.data(), sizeof(float) * 16);
            result = localTransform * worldTransform;
        }
        // Else use the translation, scale and rotation vectors.
        else
        {
            using namespace RadeonProRender;
            result = (translation(float3(node.translation[0], node.translation[1], node.translation[2]))
                * quaternion_to_matrix(quaternion(node.rotation[0], node.rotation[1], node.rotation[2], node.rotation[3]))
                * scale(float3(node.scale[0], node.scale[1], node.scale[2]))) * worldTransform;
        }

        return result;
    }

    void GetNodeBoundingBox(gltf::glTF& gltf, gltf::Scene& scene, gltf::Node& node, BBox& bbox, mat4& worldTransform)
    {
        // Calculate the node's final transform.
        mat4 transform = GetNodeTransformMatrix(node, worldTransform);

        // Iterate over the node's mesh primitives.
        if (node.mesh >= 0 && node.mesh < gltf.meshes.size())
        {
            auto& mesh = gltf.meshes[node.mesh];
            for (auto& primitive : mesh.primitives)
            {
                // Get the position attribute accessor.
                auto& accessor = gltf.accessors[primitive.attributes.at(gltf::AttributeNames::POSITION)];

                // Transform the min and max values to world space.
                vec4 p0 = { accessor.min[0], accessor.min[1], accessor.min[2], 1.0f };
                vec4 p1 = { accessor.max[0], accessor.max[1], accessor.max[2], 1.0f };

                // Declare own mat4 * vec4 function since the one in Math/matrix.h only handles float3.
                auto mul = [](const mat4& m, const vec4& v) -> vec4 {
                    vec4 result;

                    for (int i = 0; i < 4; ++i)
                    {
                        result[i] = 0.0f;
                        for (int j = 0; j < 4; ++j)
                        {
                            result[i] += m.m[j][i] * v[j];
                        }
                    }

                    return result;
                };

                // Transform the min and max points by the matrix.
                p0 = mul(transform, p0);
                p1 = mul(transform, p1);

                // Apply the min and max coordinates to the bounding box.
                bbox.min[0] = std::min(bbox.min[0], std::min(p0.x, p1.x));
                bbox.min[1] = std::min(bbox.min[1], std::min(p0.y, p1.y));
                bbox.min[2] = std::min(bbox.min[2], std::min(p0.z, p1.z));
                bbox.max[0] = std::max(bbox.max[0], std::max(p0.x, p1.x));
                bbox.max[1] = std::max(bbox.max[1], std::max(p0.y, p1.y));
                bbox.max[2] = std::max(bbox.max[2], std::max(p0.z, p1.z));
            }
        }

        // Iterate over the node's children.
        for (auto& childIndex : node.children)
        {
            // Validate the child node index.
            if (childIndex < 0 || childIndex >= gltf.nodes.size())
                continue;

            // Recursively process child nodes.
            GetNodeBoundingBox(gltf, scene, gltf.nodes[childIndex], bbox, transform);
        }
    }

    bool GetSceneBoundingBox(gltf::glTF& gltf, int sceneIndex, float& minX, float& minY, float& minZ, float& maxX, float& maxY, float& maxZ)
    {
        // Check scene index bounds.
        if (sceneIndex < 0 || sceneIndex >= gltf.scenes.size())
            return false;

        // Traverse nodes in specified scene.
        BBox bbox;
        auto& scene = gltf.scenes[sceneIndex];
        for (auto& nodeIndex : scene.nodes)
        {
            // Validate node index.
            if (nodeIndex < 0 || nodeIndex >= gltf.nodes.size())
                continue;

            // If node references a camera ignore it.
            auto& node = gltf.nodes[nodeIndex];
            if (node.camera != -1)
                continue;

            // Get the node's bounding box.
            mat4 identity;
            GetNodeBoundingBox(gltf, scene, node, bbox, identity);
        }

        // Copy the final bounding box values.
        minX = bbox.min[0];
        minY = bbox.min[1];
        minZ = bbox.min[2];
        maxX = bbox.max[0];
        maxY = bbox.max[1];
        maxZ = bbox.max[2];

        return true;
    }
}