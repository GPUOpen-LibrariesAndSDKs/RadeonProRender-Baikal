
#include "Baikal/SceneGraph/IO/scene_io.h"

#ifdef ENABLE_GLTF
#define _USE_MATH_DEFINES
#include <cmath>

#include "FreeImage.h"

#include "Baikal/SceneGraph/IO/image_io.h"
#include "Baikal/SceneGraph/scene1.h"
#include "Baikal/SceneGraph/shape.h"
#include "Baikal/SceneGraph/material.h"
#include "Baikal/SceneGraph/light.h"
#include "Baikal/SceneGraph/texture.h"
#include "Baikal/SceneGraph/camera.h"

#include "gltf/base64.h"
#include "SceneGraph/IO/gltf/gltf2.h"
#include "SceneGraph/IO/gltf/Extensions/Extensions.h"

#include <cassert>
#include <algorithm>
#include <iterator>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <unordered_set>

using namespace std::experimental::filesystem::v1;
namespace fs = std::experimental::filesystem;
using namespace Baikal;

namespace Baikal
{

    // glTF scene loader
    class SceneGltfIo : public SceneIo
    {
    public:

        // Load scene from file
        virtual Scene1::Ptr LoadScene(std::string const& filename, std::string const& basepath) const override;
    private:
        struct InterleavedAttributesResult
        {
            std::vector<char*> bufferData;
            std::unordered_map<std::string, char*> attributeMemoryAddresses;
        };


        RadeonRays::matrix GetNodeTransformMatrix(const gltf::Node& node, const RadeonRays::matrix& worldTransform) const;

        void ImportPostEffects() const;
        void ImportSceneParameters(Scene1::Ptr scene, int scene_index) const;
        void ImportSceneLights(Scene1::Ptr scene, int scene_index) const;
        void ImportNode(Scene1::Ptr scene, int scene_index, int node_index, RadeonRays::matrix& world_transform) const;
        void ImportCamera(Scene1::Ptr scene, gltf::Node const& node, RadeonRays::matrix const& world_transform) const;
        Shape::Ptr ImportMesh(Scene1::Ptr scene, int node_index, RadeonRays::matrix const& world_transform) const;
        void ImportShapeParameters(nlohmann::json const& extras, Shape::Ptr shape) const;
        std::istream* ImportBuffer(int bufferIndex) const;
        void ImportMaterial(Shape::Ptr shape, int materialIndex) const;
        Material::Ptr ImportMaterialNode(std::vector<amd::Node>& nodes, int nodeIndex) const;

        Texture::Ptr ImportImage(int imageIndex) const;

        InterleavedAttributesResult GetInterleavedAttributeData(const std::unordered_map<std::string, int>& attributes) const;
        void DestroyInterleavedAttributesResult(InterleavedAttributesResult& iar) const;
        void ApplyNormalMapAndEmissive(SingleBxdf::Ptr uber_material, gltf::Material& material) const;

        mutable std::string m_root_dir;
        mutable gltf::glTF m_gltf;
        mutable std::unordered_set<int> m_visited_nodes;
        mutable std::unordered_map<int, Shape::Ptr> m_shape_cache;
        mutable std::unordered_map<int, Texture::Ptr> m_image_cache;
        mutable std::unordered_map<int, std::istream*> m_buffer_cache;
        mutable std::unordered_map<int, Material::Ptr> m_material_cache;

    };


    // Create glTF scene loader
    inline size_t GetAccessorTypeSize(const gltf::Accessor& accessor)
    {
        size_t size = 0;

        switch (accessor.componentType)
        {
        case gltf::Accessor::ComponentType::BYTE:
        case gltf::Accessor::ComponentType::UNSIGNED_BYTE:
            size = sizeof(uint8_t);
            break;

        case gltf::Accessor::ComponentType::SHORT:
        case gltf::Accessor::ComponentType::UNSIGNED_SHORT:
            size = sizeof(uint16_t);
            break;

        case gltf::Accessor::ComponentType::UNSIGNED_INT:
        case gltf::Accessor::ComponentType::FLOAT:
            size = sizeof(uint32_t);
            break;
        }

        switch (accessor.type)
        {
        case gltf::Accessor::Type::SCALAR:
            break;

        case gltf::Accessor::Type::VEC2:
            size *= 2;
            break;

        case gltf::Accessor::Type::VEC3:
            size *= 3;
            break;

        case gltf::Accessor::Type::VEC4:
            size *= 4;
            break;

        case gltf::Accessor::Type::MAT2:
            size *= 4;
            break;

        case gltf::Accessor::Type::MAT3:
            size *= 9;
            break;

        case gltf::Accessor::Type::MAT4:
            size *= 16;
            break;
        }

        return size;
    }
    void SceneGltfIo::DestroyInterleavedAttributesResult(SceneGltfIo::InterleavedAttributesResult& iar) const
    {
        for (auto data : iar.bufferData)
            delete[] data;
    }

    std::unique_ptr<Baikal::SceneIo> SceneIo::CreateSceneIoGltf()
    {
        return std::unique_ptr<Baikal::SceneIo>(new SceneGltfIo());
    }

    Scene1::Ptr SceneGltfIo::LoadScene(std::string const& filename, std::string const& basepath) const
    {
        auto image_io(ImageIo::CreateImageIo());
                
        m_root_dir = basepath;
        // Allocate scene
        auto scene = Scene1::Create();

        SingleBxdf::Ptr default_material = SingleBxdf::Create(SingleBxdf::BxdfType::kLambert);
        default_material->SetInputValue("albedo", RadeonRays::float4(1.f, 1.f, 1.f));

        // Read gltf file from disk.
        if (!gltf::Import(filename, m_gltf))
        {
            std::cout << "Failed to import " << filename << std::endl;
            return nullptr;
        }

        //currently do nothing
        ImportPostEffects();

        if (m_gltf.scenes.size() != 1)
        {
            //multiple scene load not supported
            //using scene index from gltf
            std::cout << "Warning: several scenes present in glTF file, load only scene with index :" << m_gltf.scene << std::endl;
        }

        int scene_index = m_gltf.scene;

        // Import scene parameters.
        ImportSceneParameters(scene, scene_index);

        // Import scene lights (i.e. AMD_RPR_Lights extension).
        ImportSceneLights(scene, scene_index);

        RadeonRays::matrix ident;
        for (auto i : m_gltf.scenes[scene_index].nodes)
        {
            ImportNode(scene, scene_index, i, ident);
        }

        if (scene->GetNumLights() == 0)
        {
            // TODO: temporary code, add IBL
            auto ibl_texture = image_io->LoadImage("../Resources/Textures/studio015.hdr");

            auto ibl = ImageBasedLight::Create();
            ibl->SetTexture(ibl_texture);
            ibl->SetMultiplier(1.f);

            // TODO: temporary code to add directional light
            auto light = DirectionalLight::Create();
            light->SetDirection(RadeonRays::normalize(RadeonRays::float3(-1.1f, -0.6f, -0.2f)));
            light->SetEmittedRadiance(30.f * RadeonRays::float3(1.f, 0.95f, 0.92f));

            auto light1 = DirectionalLight::Create();
            light1->SetDirection(RadeonRays::float3(0.3f, -1.f, -0.5f));
            light1->SetEmittedRadiance(RadeonRays::float3(1.f, 0.8f, 0.65f));
            
            scene->AttachLight(ibl);
        }
        return scene;
    }

    inline RadeonRays::matrix SceneGltfIo::GetNodeTransformMatrix(const gltf::Node& node, const RadeonRays::matrix& worldTransform) const
    {
        RadeonRays::matrix result;

        // If node matrix isn't the identity then use that.
        if (node.matrix != decltype(node.matrix) { 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0 })
        {
            RadeonRays::matrix localTransform;
            memcpy(&localTransform.m[0][0], node.matrix.data(), sizeof(float) * 16);
            result = localTransform * worldTransform;
        }
        // Else use the translation, scale and rotation vectors.
        else
        {
            result = (translation(RadeonRays::float3(node.translation[0], node.translation[1], node.translation[2]))
                * quaternion_to_matrix(RadeonRays::quaternion(node.rotation[0], node.rotation[1], node.rotation[2], node.rotation[3]))
                * scale(RadeonRays::float3(node.scale[0], node.scale[1], node.scale[2]))) * worldTransform;
        }

        return result;
    }
    void SceneGltfIo::ImportNode(Scene1::Ptr scene, int scene_index, int node_index, RadeonRays::matrix& world_transform) const
    {
        // Validate node index.
        if (node_index < 0 || node_index >= m_gltf.nodes.size())
            return;

        // If node has already been visited then exit.
        if (m_visited_nodes.find(node_index) != m_visited_nodes.end())
        {
            std::cout << "ImportNode " << node_index << " already visited\n";
            return;
        }

        // Get the referenced node.
        auto const& node = m_gltf.nodes[node_index];

        // Mark the node as visited.
        m_visited_nodes.insert(node_index);

        // Check to see if node is a camera.
        if (node.camera != -1)
        {
            ImportCamera(scene, node, world_transform);
        }
        // Check to see if node is a mesh.
        else if (node.mesh >= 0 && node.mesh < m_gltf.meshes.size())
        {
            // Determine if node is a Radeon ProRender instance.
            bool is_instance = false;
            if (node.extras.find("rpr.shape.parameters") != node.extras.end())
            {
                auto& shapeParams = node.extras["rpr.shape.parameters"];
                if (shapeParams.find("parentShapeID") != shapeParams.end())
                {
                    // Since this is a Radeon ProRender instance, only a single rpr_shape was created by the parent Node.
                    RadeonRays::matrix identity;
                    auto base_shape = ImportMesh(scene, shapeParams.at("parentShapeID").get<int>(), identity);
                    if (base_shape)
                    {
                        // Mark that the node was a Radeon ProRender instance.
                        is_instance = true;

                        // Create a new Radeon ProRender instance.
                        Baikal::Instance::Ptr inst = Baikal::Instance::Create(base_shape);
                        // Set all of the shape parameters.
                        ImportShapeParameters(node.extras, inst);

                        // Calculate the node's transform.
                        RadeonRays::matrix node_transform = GetNodeTransformMatrix(node, world_transform);

                        // Set the shape transform.
                        inst->SetTransform(node_transform);

                        // Set the shape material.
                        if (node.mesh > 0 && node.mesh < m_gltf.meshes.size())
                            if (m_gltf.meshes[node.mesh].primitives.size() == 1)
                                ImportMaterial(inst, m_gltf.meshes[node.mesh].primitives[0].material);

                        // Attach the shape to the scene.
                        scene->AttachShape(inst);
                        
                        // Cache the shape by node index.
                        m_shape_cache.emplace(node_index, inst);
                    }
                }
            }

            // Else load it as a regular mesh.
            if (!is_instance)
            {
                ImportMesh(scene, node_index, world_transform);
            }
        }

        // Calculate the node's transform to pass to children as new world transform.
        RadeonRays::matrix nodeTransform = GetNodeTransformMatrix(node, world_transform);

        // Import the node's children.
        for (auto& childNodeIndex : node.children)
        {
            ImportNode(scene, scene_index, childNodeIndex, nodeTransform);
        }
    }


    void SceneGltfIo::ImportCamera(Scene1::Ptr scene, gltf::Node const& node, RadeonRays::matrix const& worldTransform) const
    {
        // Check the camera index.
        if (node.camera < 0 || node.camera >= m_gltf.cameras.size())
            return;

        // Lookup the gltf camera.
        auto& camera = m_gltf.cameras[node.camera];

        // Create a new camera.
        Baikal::PerspectiveCamera::Ptr new_cam;

        //// Check for AMD_RPR_camera extension.
        //amd::AMD_RPR_Camera ext;
        //if (ImportExtension(camera.extensions, ext))
        //{
        //    // Import camera parameters.
        //    rprCameraSetMode(rprCamera, static_cast<rpr_camera_mode>(ext.cameraMode));
        //    rprCameraSetApertureBlades(rprCamera, ext.apertureBlades);
        //    rprCameraSetExposure(rprCamera, ext.exposure);
        //    rprCameraSetFocusDistance(rprCamera, ext.focusDistance);
        //    rprCameraSetFocalLength(rprCamera, ext.focalLength);
        //    rprCameraSetFocalTilt(rprCamera, ext.focalTilt);
        //    rprCameraSetFStop(rprCamera, ext.fstop);
        //    rprCameraSetIPD(rprCamera, ext.ipd);
        //    rprCameraSetLensShift(rprCamera, ext.lensShift[0], ext.lensShift[1]);
        //    rprCameraSetOrthoWidth(rprCamera, ext.orthoWidth);
        //    rprCameraSetOrthoHeight(rprCamera, ext.orthoHeight);
        //    rprCameraSetSensorSize(rprCamera, ext.sensorSize[0], ext.sensorSize[1]);
        //    rprCameraSetTiltCorrection(rprCamera, ext.tiltCorrection[0], ext.tiltCorrection[1]);
        //    rprCameraLookAt(rprCamera, ext.position[0], ext.position[1], ext.position[2], ext.lookAt[0], ext.lookAt[1], ext.lookAt[2], ext.up[0], ext.up[1], ext.up[2]);
        //}
        //else if (camera.type == gltf::Camera::Type::PERSPECTIVE)

        RadeonRays::matrix cam_m;
        memcpy(&cam_m.m00, node.matrix.data(), 16 * sizeof(float));
        RadeonRays::float3 eye(0.f, 0.f, 0.f);
        RadeonRays::float3 at(0.f, 0.f, -1.f);
        RadeonRays::float3 up(0.f, 1.f, 0.f);
        eye = cam_m * eye;
        at = cam_m * at;
        up = cam_m * up;

        if (camera.type == gltf::Camera::Type::PERSPECTIVE)
        {
            new_cam = Baikal::PerspectiveCamera::Create(eye, at, up);

            // Get the Radeon ProRender camera's default sensor size.
            RadeonRays::float2 sensor_size = new_cam->GetSensorSize();

            // Convert gltf camera's vertical field of view to an appropriate focal length.
            float focalLength = 0.5f * sensor_size[1] / tan(camera.perspective.yfov * 0.5f) / 1000.f;

        }
        else if (camera.type == gltf::Camera::Type::ORTHOGRAPHIC)
        {
            std::cout << "Warning: orthographic camera is unsupported, use perspective instead." << std::endl;
            new_cam = Baikal::PerspectiveCamera::Create(eye, at, up);

            // TODO: handle ortho camera properly
            // Get the Radeon ProRender camera's default sensor size.
            RadeonRays::float2 sensor_size = new_cam->GetSensorSize();
            // Convert gltf camera's vertical field of view to an appropriate focal length.
            float focalLength = 0.5f * sensor_size[1] / tan(camera.perspective.yfov * 0.5f) / 1000.f;
        }

        // Attach the camera to the scene.
        scene->SetCamera(new_cam);
    }

    Baikal::Shape::Ptr SceneGltfIo::ImportMesh(Scene1::Ptr scene, int node_index, RadeonRays::matrix const& world_transform) const
    {
        // Validate the index.
        if (node_index < 0 || node_index >= m_gltf.nodes.size())
            return nullptr;

        // See if the node index is in the cache.
        if (m_shape_cache.find(node_index) != m_shape_cache.end())
            return m_shape_cache.at(node_index);

        // Get the GLTF node at the specified index.
        auto& node = m_gltf.nodes[node_index];

        // Get the node's GLTF mesh.
        auto& mesh = m_gltf.meshes[node.mesh];

        // Iterate over all of the mesh's primitives.
        std::vector<Mesh::Ptr> rprShapesList;
        for (auto& primitive : mesh.primitives)
        {
            // If accessors for index and vertex attributes are interleaved, leave them interleaved.
            // In order to accomplish this, all accessors must be iterated over to determine which share common buffers.
            std::unordered_map<std::string, int> allMeshPrimitiveAttributes = { { "", primitive.indices } };
            for (auto& entry : primitive.attributes)
                allMeshPrimitiveAttributes.emplace(entry.first, entry.second);

            auto interleavedAttributes = GetInterleavedAttributeData(allMeshPrimitiveAttributes);

            // Get the accessor referencing the indices data.
            auto& indice_accessor = m_gltf.accessors[primitive.indices];

            // Get the bufferView referenced by the accessor.
            auto& indexBufferView = m_gltf.bufferViews[indice_accessor.bufferView];

            // Get the memory address of the indices from the interleaved attributes result.
            char* indices = interleavedAttributes.attributeMemoryAddresses.at("");

            // indices stride will always be 4 bytes for Radeon ProRender.
            int indicesStride = sizeof(int);

            // Indices must be in 32-bit integer format, convert if necessary.
            if (indice_accessor.componentType != gltf::Accessor::ComponentType::UNSIGNED_INT)
            {
                // Check byteStride for index bufferView.
                int byteStride = indexBufferView.byteStride;
                if (byteStride == 0)
                {
                    switch (indice_accessor.componentType)
                    {
                    case gltf::Accessor::ComponentType::BYTE:
                    case gltf::Accessor::ComponentType::UNSIGNED_BYTE:
                        byteStride = sizeof(uint8_t);
                        break;

                    case gltf::Accessor::ComponentType::SHORT:
                    case gltf::Accessor::ComponentType::UNSIGNED_SHORT:
                        byteStride = sizeof(uint16_t);
                        break;

                    case gltf::Accessor::ComponentType::UNSIGNED_INT:
                        byteStride = sizeof(uint32_t);
                        break;
                    }
                }

                // Allocate a new 32-bit index array for conversion.
                size_t typeSize = GetAccessorTypeSize(indice_accessor);
                std::uint32_t* intIndices = new std::uint32_t[indice_accessor.count];
                for (auto i = 0; i < indice_accessor.count; ++i)
                {
                    switch (indice_accessor.componentType)
                    {
                    case gltf::Accessor::ComponentType::BYTE:
                        intIndices[i] = *(indices + (i * indexBufferView.byteStride));
                        break;

                    case gltf::Accessor::ComponentType::UNSIGNED_BYTE:
                        intIndices[i] = *reinterpret_cast<uint8_t*>(indices + (i * byteStride));
                        break;

                    case gltf::Accessor::ComponentType::SHORT:
                        intIndices[i] = *reinterpret_cast<int16_t*>(indices + (i * byteStride));
                        break;

                    case gltf::Accessor::ComponentType::UNSIGNED_SHORT:
                        intIndices[i] = *reinterpret_cast<uint16_t*>(indices + (i * byteStride));
                        break;
                    }
                }

                // Assign old pointer to new one.
                indices = reinterpret_cast<char*>(intIndices);
            }

            // Extract all vertex attribute arrays.
            float* vertices = nullptr, *normals = nullptr, *texcoords0 = nullptr;
            int num_vertices = 0, num_normals = 0, num_texcoords0 = 0;
            int vertexStride = 0, normalStride = 0, texcoords0Stride = 0;

            // Define a convenience lambda function for extracting the data for each attribute.
            auto extractAttributeData = [&](const std::string& attributeName, float*& data, int& count, int& stride) -> void
            {
                // Look up the attribute name in the primitive's list of attributes.
                auto itr = primitive.attributes.find(attributeName);
                if (itr != primitive.attributes.end())
                {
                    // Get the referenced gltf Accessor.
                    auto& accessor = m_gltf.accessors[itr->second];

                    // Get the bufferView referenced by the accessor.
                    auto& bufferView = m_gltf.bufferViews[accessor.bufferView];

                    // Copy the element count and stride.
                    count = accessor.count;
                    stride = bufferView.byteStride;

                    // If stride is 0, data is tightly packed.  So manually determine stride.                        
                    if (stride == 0)
                        stride = static_cast<int>(GetAccessorTypeSize(accessor));

                    // Get a pointer to the starting address of the data.
                    data = reinterpret_cast<float*>(interleavedAttributes.attributeMemoryAddresses.at(attributeName));
#if 0
                    auto stream = ImportBuffer(bufferView.buffer);
                    char* temp = new char[bufferView.byteLength];
                    stream->seekg(bufferView.byteOffset + accessor.byteOffset);
                    stream->read(temp, bufferView.byteLength);
                    data = reinterpret_cast<float*>(temp);
#endif
                }
            };

            // Extract attribute data for positions, normals and texcoords.
            extractAttributeData(gltf::AttributeNames::POSITION, vertices, num_vertices, vertexStride);
            extractAttributeData(gltf::AttributeNames::NORMAL, normals, num_normals, normalStride);
            extractAttributeData(gltf::AttributeNames::TEXCOORD_0, texcoords0, num_texcoords0, texcoords0Stride);

            // Create a faces array that's required by Radeon ProRender.
            std::vector<int> numFaces(indice_accessor.count / 3, 3);

            //TODO: handle strides properly
            assert(vertexStride);
            assert(normalStride);
            assert(texcoords0Stride);

            // Create the Radeon ProRender shape.
            Mesh::Ptr mesh = Mesh::Create();
            mesh->SetVertices(vertices, num_vertices);
            mesh->SetNormals(normals, num_normals);
            mesh->SetUVs(texcoords0, num_texcoords0);
            mesh->SetIndices(reinterpret_cast<std::uint32_t*>(indices), indice_accessor.count);
            // TODO: set lightmap uvs!
 
            // Set the shape transform.
            RadeonRays::matrix node_transform = GetNodeTransformMatrix(node, world_transform);
            mesh->SetTransform(node_transform);

            // Import and set the shape's material.
            ImportMaterial(mesh, primitive.material);

            // Attach the shape to the scene.
            scene->AttachShape(mesh);

            // Cache the shape by node index.
            m_shape_cache.emplace(node_index, mesh);

            // Destroy the interleaved attributes result.
            DestroyInterleavedAttributesResult(interleavedAttributes);

            // Add the shape to the return result.
            if (mesh)
                rprShapesList.push_back(mesh);
        }

        // Return the first shape in the list.  For Radeon ProRender exports there will only ever be a single shape per Node and Mesh.
        if (rprShapesList.size() > 0) return rprShapesList[0];
        else return nullptr;
    }


    SceneGltfIo::InterleavedAttributesResult SceneGltfIo::GetInterleavedAttributeData(const std::unordered_map<std::string, int>& attributes) const
    {
        // Structure declaration for grouping attributes by buffer index and if their byte ranges overlap.
        struct InterleavedAttributes
        {
            int bufferIndex;
            int byteOffset;
            int byteLength;
            std::unordered_map<std::string, int> attributes;
        };

        // Iterate over all the attributes and group ones that are interleaved.
        std::vector<InterleavedAttributes> interleavedAttributes;
        for (auto& itr : attributes)
        {
            // For the given attribute, determine it's bufferIndex and memory range.
            // Get the buffer referenced by the accessor.
            auto& accessor = m_gltf.accessors[itr.second];
            auto& bufferView = m_gltf.bufferViews[accessor.bufferView];

            // Get accessor element type size.
            auto elementSize = std::max((size_t)bufferView.byteStride, GetAccessorTypeSize(accessor));

            // Search for an existing entry in the interleavedAttributes array.
            bool matchFound = false;
            for (auto& entry : interleavedAttributes)
            {
                // Check to see if the buffer indices match.
                if (entry.bufferIndex == bufferView.buffer)
                {
                    // Check to see if there's an overlap with the byte range.
                    int startOffset = bufferView.byteOffset + accessor.byteOffset;
                    int endOffset = static_cast<int>(startOffset + (elementSize * accessor.count));
                    if ((startOffset >= entry.byteOffset && startOffset < entry.byteOffset + entry.byteLength) || (endOffset > entry.byteOffset && endOffset < entry.byteOffset + entry.byteLength))
                    {
                        // Re-calculate the byte offset and length of the interleaved attribute buffer.
                        int entryEndOffset = entry.byteOffset + entry.byteLength;
                        entry.byteOffset = std::min(entry.byteOffset, startOffset);
                        entry.byteLength = std::max(entryEndOffset, endOffset) - entry.byteOffset;

                        // Add the attribute to the list.
                        entry.attributes.emplace(itr.first, itr.second);

                        // Flag that a match was found and exit loop.
                        matchFound = true;
                        break;
                    }
                }
            }

            // If no match was found, create a new entry in the interleavedAttributes array.
            if (!matchFound)
            {
                InterleavedAttributes ia;
                ia.bufferIndex = bufferView.buffer;
                ia.byteOffset = bufferView.byteOffset + accessor.byteOffset;
                ia.byteLength = static_cast<int>(elementSize * accessor.count);
                ia.attributes.emplace(itr.first, itr.second);
                interleavedAttributes.push_back(std::move(ia));
            }
        }

        // Allocate and read memory for each entry in the interleavedAttributes array.
        InterleavedAttributesResult result;
        for (auto& entry : interleavedAttributes)
        {
            // Import and read the buffer.
            auto stream = ImportBuffer(entry.bufferIndex);
            char* data = new char[entry.byteLength];
            stream->seekg(entry.byteOffset);
            stream->read(data, entry.byteLength);
            result.bufferData.push_back(data);
        }

        // Return a map containing memory addresses paired to each attribute.
        for (auto& itr : attributes)
        {
            // Get the accessor and bufferView for the given attribute.
            auto& accessor = m_gltf.accessors[itr.second];
            auto& bufferView = m_gltf.bufferViews[accessor.bufferView];

            // Iterate over all of the entries in interleavedAttributes to find the current attribute name.
            for (auto i = 0U; i < interleavedAttributes.size(); ++i)
            {
                // Check to see if the current attribute name exists in the InterleavedAttributes entry.
                auto& entry = interleavedAttributes[i];
                if (entry.attributes.find(itr.first) != entry.attributes.end())
                {
                    // Calculate correct pointer offset into buffer data for given attribute and insert into result map.
                    char* attributeStartAddress = result.bufferData[i] + ((accessor.byteOffset + bufferView.byteOffset) - entry.byteOffset);
                    result.attributeMemoryAddresses.emplace(itr.first, attributeStartAddress);
                }
            }
        }

        // Return the result.
        return result;
    }

    std::istream* SceneGltfIo::ImportBuffer(int bufferIndex) const
    {
        // Validate the index.
        if (bufferIndex < 0 || bufferIndex >= m_gltf.buffers.size())
            return nullptr;

        // See if the buffer is in the cache.
        if (m_buffer_cache.find(bufferIndex) != m_buffer_cache.end())
            return m_buffer_cache.at(bufferIndex);

        // Check to see if the buffer should be loaded from a base64 encode string.
        auto& buffer = m_gltf.buffers[bufferIndex];
        if (buffer.uri.find("data:application/octet-stream;base64,") != std::string::npos)
        {
            // Decode the data.
            std::string data = buffer.uri.substr(buffer.uri.find(";base64,") + strlen(";base64,"));
            data = base64_decode(data);

            // Create a std::istream object to reference the buffer data in memory.
            std::istream* stream = new std::istringstream(data, std::ios::binary | std::ios_base::in);

            // Store the buffer stream in the cache and return the pointer.
            m_buffer_cache.emplace(bufferIndex, stream);
            return stream;
        }
        // Check to see if buffer data resides on disk.
        else if (fs::exists(m_root_dir + "/" + buffer.uri))
        {
            // Open the file on disk as an std::istream object.
            std::istream* stream = new std::ifstream(m_root_dir + "/" + buffer.uri, std::ios::binary | std::ios::in);

            // Store the buffer stream in the cache and return the pointer.
            m_buffer_cache.emplace(bufferIndex, stream);
            return stream;
        }

        // Unsupported uri format.
        return nullptr;
    }


    void SceneGltfIo::ImportMaterial(Baikal::Shape::Ptr shape, int materialIndex) const
    {

        // Validate the index.
        if (materialIndex < 0 || materialIndex >= m_gltf.materials.size())
            return;

        // See if the material index is in the cache.
        if (m_material_cache.find(materialIndex) != m_material_cache.end())
        {
            // Assign the material to the shape.
            if (shape)
            {
                auto mat = m_material_cache.at(materialIndex);
                shape->SetMaterial(mat);
            }

            return;
        }

        // Get the GLTF material at the specified index.
        auto& material = m_gltf.materials[materialIndex];

        // Check for the AMD_RPR_uber_material extension.
        amd::AMD_RPR_Material ext;
        if (ImportExtension(material.extensions, ext))
        {
            // Check to see if any nodes are present.
            if (ext.nodes.size() > 0)
            {
                // The very first node is always the root node.
                auto& rootNode = ext.nodes[0];


                // Recursively import the material node and any child nodes.
                Material::Ptr material_node = ImportMaterialNode(ext.nodes, 0 /* Root node index. */);

                // Apply the material to the shape.
                shape->SetMaterial(material_node);

                // Add the material to the cache.
                m_material_cache.emplace(materialIndex, material_node);
            }
        }
        else
        {
            // If KHR_materials_pbrSpecularGlossiness is present, it overrides the base pbrMetallicRoughness properties.
            //khr::KHR_Materials_PbrSpecularGlossiness ext;
            //if (khr::ImportExtension(material.extensions, ext)) uberMaterial = ImportPbrSpecularGlossiness(shape, material, ext);
            //else uberMaterial = ImportPbrMetallicRoughness(shape, material);
            // TODO: handle khronos extenstion materials properly
            SingleBxdf::Ptr uber_material = SingleBxdf::Create(SingleBxdf::BxdfType::kLambert);

            // Apply base color and texture properties.
            auto const& base_color_factor = material.pbrMetallicRoughness.baseColorFactor;
            uber_material->SetInputValue("albedo", { base_color_factor[0], base_color_factor[1], base_color_factor[2], base_color_factor[3] });

            auto base_color_image = ImportImage(material.pbrMetallicRoughness.baseColorTexture.index);
            if (base_color_image)
            {
                uber_material->SetInputValue("albedo", base_color_image);
            }

            // Apply metallic roughness properties.
            float metallicFactor = material.pbrMetallicRoughness.metallicFactor;
            float invMetallicFactor = 1.0f - metallicFactor;
            float roughness_factor = material.pbrMetallicRoughness.roughnessFactor;
            uber_material->SetInputValue("roughness", { roughness_factor , roughness_factor , roughness_factor , roughness_factor });
            // TODO: metal factor?

            auto metallic_roughness_image = ImportImage(material.pbrMetallicRoughness.metallicRoughnessTexture.index);
            if (metallic_roughness_image)
            {
                uber_material->SetInputValue("roughness", metallic_roughness_image);
            }

            // Apply normal map and emissive properties.
            ApplyNormalMapAndEmissive(uber_material, material);

            // Check to see if material creation was successful.
            if (uber_material)
            {
                shape->SetMaterial(uber_material);
                // Add the material to the cache.
                m_material_cache.emplace(materialIndex, uber_material);
            }
        }
    }

    Material::Ptr CreateMaterial(amd::Node::Type in_type)
    {
        Material::Ptr result = nullptr;
        switch (in_type)
        {
        case amd::Node::Type::UBER:
        case amd::Node::Type::DIFFUSE:
        case amd::Node::Type::ORENNAYAR:
        {
            SingleBxdf::Ptr mat = SingleBxdf::Create(SingleBxdf::BxdfType::kLambert);
            result = mat;
            break;
        }
        case amd::Node::Type::WARD:
        case amd::Node::Type::MICROFACET:
        {
            SingleBxdf::Ptr mat = SingleBxdf::Create(SingleBxdf::BxdfType::kMicrofacetGGX);
            result = mat;
            break;
        }
        case amd::Node::Type::MICROFACET_REFRACTION:
        {
            SingleBxdf::Ptr mat = SingleBxdf::Create(SingleBxdf::BxdfType::kMicrofacetRefractionGGX);
            result = mat;
            break;
        }
        case amd::Node::Type::REFLECTION:
        {
            SingleBxdf::Ptr mat = SingleBxdf::Create(SingleBxdf::BxdfType::kIdealReflect);
            result = mat;
            break;
        }
        case amd::Node::Type::REFRACTION:
        {
            SingleBxdf::Ptr mat = SingleBxdf::Create(SingleBxdf::BxdfType::kIdealRefract);
            result = mat;
            break;
        }
        case amd::Node::Type::STANDARD:
        {
            SingleBxdf::Ptr mat = SingleBxdf::Create(SingleBxdf::BxdfType::kLambert);
            result = mat;
            break;
        }
        case amd::Node::Type::DIFFUSE_REFRACTION:
        {
            SingleBxdf::Ptr mat = SingleBxdf::Create(SingleBxdf::BxdfType::kTranslucent);
            result = mat;
            break;
        }
        case amd::Node::Type::FRESNEL_SCHLICK:
        case amd::Node::Type::FRESNEL:
        {
            SingleBxdf::Ptr mat = SingleBxdf::Create(SingleBxdf::BxdfType::kTranslucent);
            result = mat;
            break;
        }
        case amd::Node::Type::EMISSIVE:
        {
            SingleBxdf::Ptr mat = SingleBxdf::Create(SingleBxdf::BxdfType::kEmissive);
            result = mat;
            break;
        }
        case amd::Node::Type::BLEND:
        {
            MultiBxdf::Ptr mat = MultiBxdf::Create(MultiBxdf::Type::kMix);
            mat->SetInputValue("weight", 0.5f);
            result = mat;
            break;
        }
        case amd::Node::Type::TRANSPARENT:
        case amd::Node::Type::PASSTHROUGH:
        {
            SingleBxdf::Ptr mat = SingleBxdf::Create(SingleBxdf::BxdfType::kPassthrough);
            result = mat;
            break;
        }
        case amd::Node::Type::ADD:
        case amd::Node::Type::ARITHMETIC:
        case amd::Node::Type::BLEND_VALUE:
        case amd::Node::Type::VOLUME:
        case amd::Node::Type::INPUT_LOOKUP:
        {
            SingleBxdf::Ptr mat = SingleBxdf::Create(SingleBxdf::BxdfType::kLambert);
            mat->SetInputValue("albedo", RadeonRays::float3(1.f, 0.f, 0.f));
            result = mat;
            break;
        }
        case amd::Node::Type::NORMAL_MAP:
        case amd::Node::Type::IMAGE_TEXTURE:
        case amd::Node::Type::NOISE2D_TEXTURE:
        case amd::Node::Type::DOT_TEXTURE:
        case amd::Node::Type::GRADIENT_TEXTURE:
        case amd::Node::Type::CONSTANT_TEXTURE:
        case amd::Node::Type::BUMP_MAP:
            //TODO: use texture
        {
            std::cout << "Error: need to create Texture instead of Material. Replace by lambert material." << std::endl;
            SingleBxdf::Ptr mat = SingleBxdf::Create(SingleBxdf::BxdfType::kLambert);
            mat->SetInputValue("albedo", RadeonRays::float3(1.f, 0.f, 0.f));
            result = mat;
            break;
        }
            //result = new Texture();
        default:
           std::cout << "Error: MaterialObject: unrecognized material type" << std::endl;
        }

        return result;
    }

    Baikal::Material::Ptr SceneGltfIo::ImportMaterialNode(std::vector<amd::Node>& nodes, int nodeIndex) const
    {
        // Check node index range.
        if (nodeIndex < 0 || nodeIndex >= nodes.size())
            return nullptr;

        // Get the Node at the specified index.
        auto& node = nodes[nodeIndex];

        // Create a new Radeon ProRender material node.
        Material::Ptr material_node = CreateMaterial(node.type);
        // Import all of the inputs.
        for (auto& input : node.inputs)
        {
            std::string input_name = input.name;

            switch (input.type)
            {
            case amd::Input::Type::FLOAT4:
            {
                RadeonRays::float4 val = {input.value.array[0],
                                          input.value.array[1],
                                          input.value.array[2],
                                          input.value.array[3]};


                material_node->SetInputValue(input_name, val);
                break;
            }
            case amd::Input::Type::UINT:
                material_node->SetInputValue(input_name, (float)input.value.integer);
                break;
            case amd::Input::Type::NODE:
            {
                // Check bounds of node reference.
                if (input.value.integer >= 0 && input.value.integer < nodes.size())
                {
                    // Import the material node and apply to uber material parameter.
                    auto child_node = ImportMaterialNode(nodes, input.value.integer);
                    if (child_node)
                        material_node->SetInputValue(input_name, child_node);
                }
                break;
            }

            case amd::Input::Type::IMAGE:
            {
                // Import the referenced image.
                auto image = ImportImage(input.value.integer);
                if (image)
                {
                    // Apply texture node to uber material input slot.
                    material_node->SetInputValue(input_name, image);
                }
                break;
            }
            }
        }

        return material_node;
    }

    Texture::Ptr SceneGltfIo::ImportImage(int imageIndex) const
    {
        // Validate the index against the image arrya.
        if (imageIndex < 0 || imageIndex >= m_gltf.images.size())
            return nullptr;

        // See if image is in the cache.
        if (m_image_cache.find(imageIndex) != m_image_cache.end())
            return m_image_cache.at(imageIndex);

        // Check to see if image should be loaded from a base64 encoded string.
        auto& image = m_gltf.images[imageIndex];
        if (image.uri.find("data:image/") != std::string::npos && image.uri.find(";base64,") != std::string::npos)
        {
            // Decode the data.
            std::string data = image.uri.substr(image.uri.find(";base64,") + strlen(";base64,"));
            data = base64_decode(data);

            // Get the image type.
            std::string type = image.uri.substr(image.uri.find("data:image/") + strlen("data:image/"), 3);


            // Decipher the format and descriptor from the type and byte length.
            if (type == "png" || type == "jpg")
            {
                // Load the bitmap from memory using FreeImage.                
                FIMEMORY* memory = FreeImage_OpenMemory(reinterpret_cast<BYTE*>(&data[0]), static_cast<DWORD>(data.size()));
                FIBITMAP* bitmap = FreeImage_LoadFromMemory((type == "png") ? FIF_PNG : FIF_JPEG, memory);
                if (!bitmap)
                    return nullptr;

                // Convert to 32-bits if image type isn't floating point (this also removes any byte padding on each row).
                FIBITMAP* temp = FreeImage_ConvertTo32Bits(bitmap);
                FreeImage_Unload(bitmap);
                bitmap = temp;

                // Pass the bitmap data to Radeon ProRender to create a new rpr_image handle.                   
                Texture::Format format = Texture::Format::kRgba8;
                RadeonRays::int2 img_size = { (int)FreeImage_GetWidth(bitmap), (int)FreeImage_GetHeight(bitmap)};

                // Swap blue and red channels of JPEG and PNG images.
                const int kNumComponents = 4;
                if ((type == "png" || type == "jpg"))
                {
                    BYTE* data = FreeImage_GetBits(bitmap);
                    for (size_t y = 0; y < img_size.y; ++y)
                    {
                        for (size_t x = 0; x < img_size.x; ++x)
                        {
                            std::swap(data[(y * img_size.x + x) * kNumComponents + 0], data[(y * img_size.x + x) * kNumComponents + 2]);
                        }
                    }
                }

                unsigned int mem_size = FreeImage_GetMemorySize(bitmap);
                BYTE* tex_data = new BYTE[mem_size];
                memcpy(tex_data, FreeImage_GetBits(bitmap), mem_size);
                Texture::Ptr img = Texture::Create(reinterpret_cast<char*>(tex_data), img_size, format);
                // Store the image in the cache.
                m_image_cache.emplace(imageIndex, img);

                // Free the bitmap memory.
                FreeImage_Unload(bitmap);

                // Return the image handle.
                return img;
            }
        }
        // Check to see if image should be loaded from disk.
        else if (fs::exists(m_root_dir + "/" + image.uri))
        {
            // Determine image type.
            FREE_IMAGE_FORMAT fif;
            std::string extension = fs::path(image.uri).extension().generic_string();
            if (extension == ".bmp") fif = FIF_BMP;
            else if (extension == ".jpg") fif = FIF_JPEG;
            else if (extension == ".png") fif = FIF_PNG;
            else if (extension == ".tga") fif = FIF_TARGA;
            else if (extension == ".tif" || extension == ".tiff") fif = FIF_TIFF;
            else if (extension == ".exr") fif = FIF_EXR;
            else if (extension == ".hdr") fif = FIF_HDR;

            // Load image from disk using FreeImage.
            FIBITMAP* bitmap = FreeImage_Load(fif, (m_root_dir + "/" + image.uri).c_str());
            if (bitmap)
            {
                // Convert to 32-bits if image type isn't floating point (this also removes any byte padding on each row).
                if (fif != FIF_HDR && fif != FIF_EXR)
                {
                    FIBITMAP* temp = FreeImage_ConvertTo32Bits(bitmap);
                    FreeImage_Unload(bitmap);
                    bitmap = temp;
                }

                // Pass the bitmap data to Radeon ProRender to create a new rpr_image handle.
                const int kNumComponents = FreeImage_GetBPP(bitmap) / 8;
                //TODO: fix
                assert(kNumComponents == 4);
                Texture::Format format = Texture::Format::kRgba8;
                RadeonRays::int2 img_size = { (int)FreeImage_GetWidth(bitmap), (int)FreeImage_GetHeight(bitmap)};

                // Handle EXR and HDR images as 32-bit channels.
                if (fif == FIF_HDR || fif == FIF_EXR)
                {
                    //TODO: fix
                    assert((FreeImage_GetBPP(bitmap) / 32) == 4);
                    format = Texture::Format::kRgba32;
                }

                // Swap blue and red channels of JPEG and PNG images.
                if ((fif == FIF_JPEG || fif == FIF_PNG) && kNumComponents)
                {
                    BYTE* data = FreeImage_GetBits(bitmap);
                    for (size_t y = 0; y < img_size.y; ++y)
                    {
                        for (size_t x = 0; x < img_size.x; ++x)
                        {
                            std::swap(data[(y * img_size.x + x) * kNumComponents + 0], data[(y * img_size.x + x) * kNumComponents + 2]);
                        }
                    }
                }
                unsigned int mem_size = FreeImage_GetMemorySize(bitmap);
                BYTE* tex_data = new BYTE[mem_size];
                memcpy(tex_data, FreeImage_GetBits(bitmap), mem_size);
                Texture::Ptr img = Texture::Create(reinterpret_cast<char*>(tex_data), img_size, format);

                // Store the image in the cache.
                m_image_cache.emplace(imageIndex, img);

                // Free the bitmap memory.
                FreeImage_Unload(bitmap);

                // Return the image handle.
                return img;
            }
        }
        // Check to see if the image data is referenced by a bufferView.
        else if (image.bufferView != -1)
        {
            // Get the bufferView referenced by the image.
            auto& bufferView = m_gltf.bufferViews[image.bufferView];

            // Get the buffer data stream.
            auto stream = ImportBuffer(bufferView.buffer);
            if (stream)
            {
                // Read the data from the stream.
                char* data = new char[bufferView.byteLength];
                stream->seekg(bufferView.byteOffset);
                stream->read(data, bufferView.byteLength);

                // Load the bitmap from memory using FreeImage.                
                FIMEMORY* memory = FreeImage_OpenMemory(reinterpret_cast<BYTE*>(data), static_cast<DWORD>(bufferView.byteLength));
                FIBITMAP* bitmap = FreeImage_LoadFromMemory((image.mimeType == gltf::Image::MimeType::IMAGE_PNG) ? FIF_PNG : FIF_JPEG, memory);
                if (!bitmap)
                    return nullptr;

                // Pass the bitmap data to Radeon ProRender to create a new rpr_image handle.
                const int kNumComponents = FreeImage_GetBPP(bitmap) / 8;
                //TODO: fix
                assert(kNumComponents == 4);
                Texture::Format format = Texture::Format::kRgba8;
                RadeonRays::int2 img_size = { (int)FreeImage_GetWidth(bitmap), (int)FreeImage_GetHeight(bitmap) };
        
                unsigned int mem_size = FreeImage_GetMemorySize(bitmap);
                BYTE* tex_data = new BYTE[mem_size];
                memcpy(tex_data, FreeImage_GetBits(bitmap), mem_size);
                Texture::Ptr img = Texture::Create(reinterpret_cast<char*>(tex_data), img_size, format);

                // Store the image in the cache.
                m_image_cache.emplace(imageIndex, img);

                // Free the bitmap memory.
                FreeImage_Unload(bitmap);

                // Free the data byte array.
                delete[] data;

                // Return the image handle.
                return img;
            }
        }

        // Unsupported uri format.
        return nullptr;
    }

    void SceneGltfIo::ApplyNormalMapAndEmissive(SingleBxdf::Ptr uber_material, gltf::Material& material) const
    {
        // Apply normal map texture.
        Texture::Ptr normal_texture = ImportImage(material.normalTexture.index);
        if (normal_texture)
        {
            uber_material->SetInputValue("normal", normal_texture);
        }

        //TODO: handle emissive material
        // Apply emissive factor and texture.
        //auto& emissiveFactor = material.emissiveFactor;
        //if (emissiveFactor[0] > 0 || emissiveFactor[1] > 0 || emissiveFactor[2] > 0)
        //{
        //    uber_material->SetBxdfType(SingleBxdf::BxdfType::kEmissive);
        //}

        Texture::Ptr emissive_texture = ImportImage(material.emissiveTexture.index);
        if (emissive_texture)
        {
            std::cout << "Warning: emissive texture not supported." << std::endl;
        }
    }

    void SceneGltfIo::ImportPostEffects() const
    {
        // Import post effects from GLTF object's extensions.
        amd::AMD_RPR_Post_Effects ext;
        if (amd::ImportExtension(m_gltf.extensions, ext))
        {
            // Iterate over all post effect entries.
            for (auto& postEffect : ext.postEffects)
            {
                // Create Radeon ProRender post effects from each entry.
                switch (postEffect.type)
                {
                case amd::PostEffect::Type::WHITE_BALANCE:
                    //rprPostEffectSetParameter1u(rprPostEffect, "colorspace", static_cast<rpr_uint>(postEffect.whiteBalance.colorSpace));
                    //rprPostEffectSetParameter1f(rprPostEffect, "colortemp", postEffect.whiteBalance.colorTemperature);
                    break;

                case amd::PostEffect::Type::SIMPLE_TONEMAP:
                    //rprPostEffectSetParameter1f(rprPostEffect, "exposure", postEffect.simpleTonemap.exposure);
                    //rprPostEffectSetParameter1f(rprPostEffect, "contrast", postEffect.simpleTonemap.contrast);
                    //rprPostEffectSetParameter1u(rprPostEffect, "tonemap", postEffect.simpleTonemap.enableTonemap);
                    break;
                }
            }
        }
    }

    void SceneGltfIo::ImportSceneParameters(Baikal::Scene1::Ptr scene, int scene_index) const
    {
        auto const& gltf_scene = m_gltf.scenes[scene_index];
        if (gltf_scene.extras.find("rpr.scene.parameters") != gltf_scene.extras.end())
        {
            // Check for background image.
            auto& scene_params = gltf_scene.extras["rpr.scene.parameters"];
            if (scene_params.find("backgroundImage") != scene_params.end())
            {
                int imageIndex = scene_params["backgroundImage"].get<int>();
                if (imageIndex != -1)
                {
                    Texture::Ptr img = ImportImage(imageIndex);
                    if (img)
                    {
                        ImageBasedLight::Ptr ibl = ImageBasedLight::Create();
                        ibl->SetTexture(img);
                        scene->AttachLight(ibl);
                    }
                }
            }
        }
    }

    void SceneGltfIo::ImportShapeParameters(nlohmann::json const& extras, Baikal::Shape::Ptr shape) const
    {
        // See if Radeon ProRender specific shape parameters are present.
        if (extras.find("rpr.shape.parameters") != extras.end())
        {
            // Import and set all of the parameters.
            auto& shapeParams = extras["rpr.shape.parameters"];
            auto const kVisAll = Baikal::Shape::Visibility::VisibleForAll();
            auto const kInvisAll = Baikal::Shape::Visibility::InvisibleForAll();
            auto const kVisPrimaryOnly = Baikal::Shape::Visibility::VisibleForPrimary();
            for (auto itr = shapeParams.begin(); itr != shapeParams.end(); ++itr)
            {
                bool value = itr.value().get<bool>();
                if (itr.key() == "visibility")
                    shape->SetVisibilityMask(value ? kVisAll : kInvisAll);
                else if (itr.key() == "shadow")
                    // TODO: fix
                    //shape->SetShadow(value);
                    std::cout << "Warning: shape shadow not implemented in Baikal. Ignoring." << std::endl;
                else if (itr.key() == "visibilityPrimaryOnly" && value)
                    shape->SetVisibilityMask(kVisPrimaryOnly);
                else
                {
                    std::cout << "Warning: unhandled shape param - " << itr.key() << std::endl;
                }
                //else if (itr.key() == "visibilityInSpecular")
                //    rprShapeSetVisibilityInSpecular(shape, itr.value().get<rpr_bool>());
                //else if (itr.key() == "shadowCatcher")
                //    //rprShapeSetShadowCatcher(shape, itr.value().get<rpr_bool>());
                //else if (itr.key() == "angularMotion")
                //{
                //    std::array<float, 4> value = itr.value().get<decltype(value)>();
                //    rprShapeSetAngularMotion(shape, value[0], value[1], value[2], value[3]);
                //}
                //else if (itr.key() == "linearMotion")
                //{
                //    std::array<float, 3> value = itr.value().get<decltype(value)>();
                //    rprShapeSetLinearMotion(shape, value[0], value[1], value[2]);
                //}
                //else if (itr.key() == "subdivisionFactor")
                //    rprShapeSetSubdivisionFactor(shape, itr.value().get<rpr_uint>());
                //else if (itr.key() == "subdivisionCreaseWeight")
                //    rprShapeSetSubdivisionCreaseWeight(shape, itr.value().get<float>());
                //else if (itr.key() == "subdivisionBoundaryInterop")
                //    rprShapeSetSubdivisionBoundaryInterop(shape, itr.value().get<rpr_uint>());
                //else if (itr.key() == "displacementScale")
                //{
                //    std::array<float, 2> value = itr.value().get<decltype(value)>();
                //    rprShapeSetDisplacementScale(shape, value[0], value[1]);
                //}
                //else if (itr.key() == "objectGroupID")
                //    rprShapeSetObjectGroupID(shape, itr.value().get<rpr_uint>());
            }
        }
    }

    void SceneGltfIo::ImportSceneLights(Baikal::Scene1::Ptr scene, int scene_index) const
    {
        auto& gltf_scene = m_gltf.scenes[scene_index];

        // Check for the AMD_RPR_lights extension.
        amd::AMD_RPR_Lights ext;
        if (amd::ImportExtension(gltf_scene.extensions, ext))
        {
            // Iterate over all of the lights.
            for (auto& light : ext.lights)
            {
                // Handle all light types.
                Baikal::Light::Ptr new_light = nullptr;
                switch (light.type)
                {
                case amd::Light::Type::POINT:
                    new_light = Baikal::PointLight::Create();
                    new_light->SetEmittedRadiance({ light.point.radiantPower[0],
                                                    light.point.radiantPower[1],
                                                    light.point.radiantPower[2]});
                    break;
                case amd::Light::Type::DIRECTIONAL:
                    new_light = Baikal::DirectionalLight::Create();
                    new_light->SetEmittedRadiance({ light.point.radiantPower[0],
                                                    light.point.radiantPower[1],
                                                    light.point.radiantPower[2] });
                    // TODO: unhandled shadow softness
                    //rprDirectionalLightSetShadowSoftness(rprLight, light.directional.shadowSoftness);
                    break;

                case amd::Light::Type::SPOT:
                {
                    auto spot_light = Baikal::SpotLight::Ptr();
                    new_light = spot_light;

                    spot_light->SetEmittedRadiance({ light.point.radiantPower[0],
                                                    light.point.radiantPower[1],
                                                    light.point.radiantPower[2] });
                    spot_light->SetConeShape({ light.spot.innerAngle, light.spot.outerAngle });
                    break;
                }

                case amd::Light::Type::ENVIRONMENT:
                {
                    auto env_light = Baikal::ImageBasedLight::Create();
                    new_light = env_light;
                    env_light->SetMultiplier(light.environment.intensityScale);

                    if (light.environment.image != -1)
                    {
                        Texture::Ptr img = ImportImage(light.environment.image);
                        if (img)
                        {
                            env_light->SetTexture(img);
                        }
                    }
                    break;
                }
                case amd::Light::Type::SKY:
                    std::cout << "Warning: unhandled sky light. Use point light instead." << std::endl;
                    new_light = Baikal::PointLight::Create();
                    break;

                case amd::Light::Type::IES:
                    std::cout << "Warning: unhandled IES light. Use point light instead." << std::endl;
                    new_light = Baikal::PointLight::Create();
                    break;
                }

                RadeonRays::matrix m;
                memcpy(&m.m00, light.transform.data(), 16 * sizeof(float));
                m = m.transpose();
                new_light->SetPosition(RadeonRays::float3(m.m03, m.m13, m.m23, m.m33));
                //float3(0, 0, -1) is a default direction vector
                new_light->SetDirection(m * RadeonRays::float3(0, 0, -1));
                // Attach the light to the scene.
                scene->AttachLight(new_light);
            }
        }
    }

} //Baikal
#else

namespace Baikal
{
    std::unique_ptr<SceneIo> SceneIo::CreateSceneIoGltf()
    {
        throw std::runtime_error("GLTF support is disabled. Please build with --gltf premake option.\n");
        return nullptr;
    }
}

#endif //ENABLE_GLTF
