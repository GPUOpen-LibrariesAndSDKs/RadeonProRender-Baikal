#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <experimental/filesystem>
#include <FreeImage.h>
#include "gltf2.h"
#include "Extensions/Extensions.h"
#include "ProRenderGLTF.h"
namespace fs = std::experimental::filesystem;

namespace rpr
{
    class Exporter
    {
    public:
        Exporter(rpr_context context, rpr_material_system materialSystem, rprx_context uberMatContext)
            : m_context(context)
            , m_materialSystem(materialSystem)
            , m_uberMatContext(uberMatContext)
        {
        }

        bool Save(const std::vector<rpr_scene>& scenes, gltf::glTF& gltf, const std::string& path)
        {
            // Get the parent directory of the gltf file.
            m_rootDirectory = fs::path(path).parent_path().generic_string();
            if (m_rootDirectory == "")
                m_rootDirectory = ".";

            // Export everything to a glTF object.
            m_gltf = &gltf;
            ExportContextParameters();
            ExportPostEffects();
            ExportScenes(scenes);

            // Set scene index to the first one.
            if (m_gltf->scenes.size() > 0)
                m_gltf->scene = 0;

            // Add AMD info to asset tag.
            m_gltf->asset.copyright = "Advanced Micro Devices Copyright 2017";
            m_gltf->asset.generator = "Radeon ProRender GLTF Library";
            m_gltf->asset.version = "2.0";
            m_gltf->asset.minVersion = "2.0";

            //  Add AMD extension strings.
            m_gltf->extensionsUsed = { "AMD_RPR_post_effects", "AMD_RPR_camera", "AMD_RPR_lights", "AMD_RPR_material" };

            // Return success.
            return true;
        }

        bool Save(const std::vector<rpr_material_node>& materialNodes, const std::vector<rprx_material>& uberMaterials, gltf::glTF& gltf, const std::string& path)
        {
            // Save pointer to glTF object.
            m_gltf = &gltf;

            // Get the parent directory of the gltf file.
            m_rootDirectory = fs::path(path).parent_path().generic_string();
            if (m_rootDirectory == "")
                m_rootDirectory = ".";

            // Export all of the material nodes.
            for (auto& node : materialNodes)
                ExportMaterial(node);

            // Export all of the uber materials.
            for (auto& material : uberMaterials)
                ExportMaterial(material);

            // Return success.
            return true;
        }

    private:
        int ExportBuffer(std::string& filename, const char* data, size_t numBytes)
        {
            // Write the data to disk.
            std::ofstream file(m_rootDirectory + "/" + filename + ".bin", std::ios::binary);
            file.write(data, numBytes);
            file.close();

            // Create a GLTF buffer to reference the bin file.
            gltf::Buffer buffer;
            buffer.uri = filename + ".bin";
            buffer.byteLength = static_cast<int>(numBytes);
            m_gltf->buffers.push_back(std::move(buffer));
            return static_cast<int>(m_gltf->buffers.size() - 1);
        }

        int ExportImage(const std::string& filename, rpr_image image)
        {
            // Handle case where img is null.
            if (!image)
                return -1;

            // Check to see if image has already been exported.
            auto itr = m_exportedImages.find(image);
            if (itr != m_exportedImages.end())
                return itr->second;

            // Get the image properties.
            rpr_image_desc desc;
            rpr_image_format format;
            size_t numBytes;
            rprImageGetInfo(image, RPR_IMAGE_DESC, sizeof(desc), &desc, nullptr);
            rprImageGetInfo(image, RPR_IMAGE_FORMAT, sizeof(format), &format, nullptr);
            rprImageGetInfo(image, RPR_IMAGE_DATA_SIZEBYTE, sizeof(numBytes), &numBytes, nullptr);

            // Get the image data.
            std::vector<char> data(numBytes);
            rprImageGetInfo(image, RPR_IMAGE_DATA, numBytes, data.data(), nullptr);

            // Determine exported image extension and format.
            std::string filenameWithExt;
            FREE_IMAGE_FORMAT fif;
            FIBITMAP* bitmap = nullptr;
            if (format.type == RPR_COMPONENT_TYPE_UINT8)
            {
                // Swap red and blue channels for png images.
                for (auto i = 0U; i < data.size(); i += format.num_components)
                    std::swap(data[i + 0], data[i + 2]);

                // Save the image to disk.
                filenameWithExt = filename + ".png";
                fif = FIF_PNG;
                bitmap = FreeImage_Allocate(desc.image_width, desc.image_height, format.num_components * 8);
                memcpy(FreeImage_GetBits(bitmap), data.data(), desc.image_width * desc.image_height * format.num_components);
            }
            else
            {
                filenameWithExt = filename + ".hdr";
                fif = FIF_HDR;
                FREE_IMAGE_TYPE type;
                int bpp;
                if (format.type == RPR_COMPONENT_TYPE_FLOAT16)
                {
                    // Only 16-bit half precision RGB and RGBA images are supported.
                    if (format.num_components == 3)
                    {
                        type = FIT_RGB16;
                        bpp = 16 * 3;
                    }
                    else if (format.num_components == 4)
                    {
                        type = FIT_RGBA16;
                        bpp = 16 * 4;
                    }
                    else return -1;

                }
                else if (format.type == RPR_COMPONENT_TYPE_FLOAT32)
                {
                    if (format.num_components == 1)
                    {
                        type = FIT_FLOAT;
                        bpp = 32;
                    }
                    else if (format.num_components == 3)
                    {
                        type = FIT_RGBF;
                        bpp = 32 * 3;
                    }
                    else if (format.num_components == 4)
                    {
                        type = FIT_RGBAF;
                        bpp = 32 * 4;
                    }
                    else return -1;
                }

                bitmap = FreeImage_AllocateT(type, desc.image_width, desc.image_height, bpp);
                memcpy(FreeImage_GetBits(bitmap), data.data(), desc.image_width * desc.image_height * (bpp / 8));
            }

            // Flip image vertically since ProRender seems to return an incorrect orientation regardless of the original image format.
            FreeImage_FlipVertical(bitmap);

            // Export the image data.
            if (!FreeImage_Save(fif, bitmap, (m_rootDirectory + "/" + filenameWithExt).c_str()))
                return -1;

            // Add a new image to the GLTF object.
            gltf::Image img;
            img.uri = filenameWithExt;
            img.mimeType = gltf::Image::MimeType::IMAGE_PNG;
            m_gltf->images.push_back(std::move(img));

            // Add the image to the cache.
            int id = static_cast<int>(m_gltf->images.size() - 1);
            m_exportedImages.emplace(image, id);

            // Return new image index.
            return id;
        }

        int ExportMaterialNode(std::vector<amd::Node>& nodes, rpr_material_node materialNode)
        {
            // Handle case where material node is null.
            if (!materialNode)
                return -1;

            // Get the material's type.
            rpr_material_node_type type;
            rprMaterialNodeGetInfo(materialNode, RPR_MATERIAL_NODE_TYPE, sizeof(type), &type, nullptr);
            amd::Node node = { "", static_cast<amd::Node::Type>(type) };

            // Add the node to the list to reserve the index.
            int nodeIndex = static_cast<int>(nodes.size());
            nodes.push_back(node);

            // Export each input parameter.
            size_t inputCount = 0;
            rprMaterialNodeGetInfo(materialNode, RPR_MATERIAL_NODE_INPUT_COUNT, sizeof(inputCount), &inputCount, nullptr);
            for (auto i = 0U; i < inputCount; ++i)
            {
                // Get then input name string.
                size_t nameLength = 0;
                rprMaterialNodeGetInputInfo(materialNode, i, RPR_MATERIAL_NODE_INPUT_NAME_STRING, 0, nullptr, &nameLength);
                std::vector<char> name(nameLength);
                rprMaterialNodeGetInputInfo(materialNode, i, RPR_MATERIAL_NODE_INPUT_NAME_STRING, nameLength, name.data(), nullptr);

                // Get the input tpe.
                rpr_uint inputType = 0;
                rprMaterialNodeGetInputInfo(materialNode, i, RPR_MATERIAL_NODE_INPUT_TYPE, sizeof(inputType), &inputType, nullptr);

                // Create a new Node Input object.
                amd::Input input;
                input.name = name.data();

                // Retrieve parameter value and store as Input object in the Node.
                switch (inputType)
                {
                case RPR_MATERIAL_NODE_INPUT_TYPE_FLOAT4:
                    input.type = amd::Input::Type::FLOAT4;
                    rprMaterialNodeGetInputInfo(materialNode, i, RPR_MATERIAL_NODE_INPUT_VALUE, sizeof(float) * 4, input.value.array.data(), nullptr);
                    break;

                case RPR_MATERIAL_NODE_INPUT_TYPE_UINT:
                    input.type = amd::Input::Type::UINT;
                    rprMaterialNodeGetInputInfo(materialNode, i, RPR_MATERIAL_NODE_INPUT_VALUE, sizeof(rpr_uint), &input.value.integer, nullptr);
                    break;

                case RPR_MATERIAL_NODE_INPUT_TYPE_NODE:
                {
                    // Set input type.
                    input.type = amd::Input::Type::NODE;

                    // Get the rpr_material_node input value.
                    rpr_material_node childNode = nullptr;
                    rprMaterialNodeGetInputInfo(materialNode, i, RPR_MATERIAL_NODE_INPUT_VALUE, sizeof(childNode), &childNode, nullptr);

                    // Export the rpr_material_node input value and set the current Input's integer value to it's index in the nodes array.
                    input.value.integer = ExportMaterialNode(nodes, childNode);
                    break;
                }

                case RPR_MATERIAL_NODE_INPUT_TYPE_IMAGE:
                {
                    // Set input type.
                    input.type = amd::Input::Type::IMAGE;

                    // Retrieve the image handle.
                    rpr_image img = nullptr;
                    rprMaterialNodeGetInputInfo(materialNode, i, RPR_MATERIAL_NODE_INPUT_VALUE, sizeof(img), &img, nullptr);
                    std::string filename = node.name + "_" + input.name + "_image" + std::to_string(m_gltf->images.size());

                    // Export the image and store the index in the input's integer value.
                    input.value.integer = ExportImage(filename, img);
                    break;
                }
                }

                // Add the Input to the Node.
                node.inputs.push_back(std::move(input));
            }

            // Update the node in the list.
            nodes[nodeIndex] = std::move(node);

            // Return the node index in the list.
            return nodeIndex;
        }

        int ExportMaterial(rpr_material_node rprMaterial)
        {
            // Handle case where handle is null.
            if (!rprMaterial)
                return -1;

            // Export the material node to the extension object.
            amd::AMD_RPR_Material ext;
            ExportMaterialNode(ext.nodes, rprMaterial);

            // Export extension to a new GLTF Material.
            gltf::Material material = {};
            material.name = "Material" + std::to_string(m_gltf->materials.size());
            amd::ExportExtension(ext, material.extensions);

            // Add GLTF Material to m_gltf object.
            m_gltf->materials.push_back(std::move(material));

            // Return the material index.
            return static_cast<int>(m_gltf->materials.size() - 1);
        }

        int ExportMaterial(rprx_material rprxMaterial)
        {
            // Handle case where handle is null.
            if (!rprxMaterial)
                return -1;

            // List of all uber material parameters to export.
            const std::vector<std::pair<std::string, rprx_parameter>> uberMatParams = {
                { "diffuse.color", RPRX_UBER_MATERIAL_DIFFUSE_COLOR },
                { "diffuse.weight", RPRX_UBER_MATERIAL_DIFFUSE_WEIGHT },
                { "diffuse.roughness", RPRX_UBER_MATERIAL_DIFFUSE_ROUGHNESS },
                { "reflection.color", RPRX_UBER_MATERIAL_REFLECTION_COLOR },
                { "reflection.weight", RPRX_UBER_MATERIAL_REFLECTION_WEIGHT },
                { "reflection.roughness", RPRX_UBER_MATERIAL_REFLECTION_ROUGHNESS },
                { "reflection.anisotropy", RPRX_UBER_MATERIAL_REFLECTION_ANISOTROPY },
                { "reflection.anisotropyRotation", RPRX_UBER_MATERIAL_REFLECTION_ANISOTROPY_ROTATION },
                { "reflection.mode", RPRX_UBER_MATERIAL_REFLECTION_MODE },
                { "reflection.ior", RPRX_UBER_MATERIAL_REFLECTION_IOR },
                { "reflection.metalness", RPRX_UBER_MATERIAL_REFLECTION_METALNESS },
                { "refraction.color", RPRX_UBER_MATERIAL_REFRACTION_COLOR },
                { "refraction.weight", RPRX_UBER_MATERIAL_REFRACTION_WEIGHT },
                { "refraction.roughness", RPRX_UBER_MATERIAL_REFRACTION_ROUGHNESS },
                { "refraction.ior", RPRX_UBER_MATERIAL_REFRACTION_IOR },
                { "refraction.iorMode", RPRX_UBER_MATERIAL_REFRACTION_IOR_MODE },
                { "refraction.thinSurface", RPRX_UBER_MATERIAL_REFRACTION_THIN_SURFACE },
                { "coating.color", RPRX_UBER_MATERIAL_COATING_COLOR },
                { "coating.weight", RPRX_UBER_MATERIAL_COATING_WEIGHT },
                { "coating.roughness", RPRX_UBER_MATERIAL_COATING_ROUGHNESS },
                { "coating.mode", RPRX_UBER_MATERIAL_COATING_MODE },
                { "coating.ior", RPRX_UBER_MATERIAL_COATING_IOR },
                { "coating.metalness", RPRX_UBER_MATERIAL_COATING_METALNESS },
                { "emission.color", RPRX_UBER_MATERIAL_EMISSION_COLOR },
                { "emission.weight", RPRX_UBER_MATERIAL_EMISSION_WEIGHT },
                { "emission.mode", RPRX_UBER_MATERIAL_EMISSION_MODE },
                { "transparency", RPRX_UBER_MATERIAL_TRANSPARENCY },
                { "normal", RPRX_UBER_MATERIAL_NORMAL },
                { "bump", RPRX_UBER_MATERIAL_BUMP },
                { "displacement", RPRX_UBER_MATERIAL_DISPLACEMENT },
            };

            // Create a new AMD_RPR_materials extension object.
            amd::AMD_RPR_Material ext;

            // Add a new material node to extension for the uber material.
            std::string name = "Material" + std::to_string(m_gltf->materials.size());
            amd::Node node = { name, amd::Node::Type::UBER };

            // Export each parameter.
            for (auto& param : uberMatParams)
            {
                // Get the Radeon ProRender parameter type.
                rpr_parameter_type type;
                rprxMaterialGetParameterType(m_uberMatContext, rprxMaterial, param.second, &type);

                // Create a new Node Input object.
                amd::Input input;
                input.name = param.first;

                // Retrieve parameter value and store as an Input object in the Node.
                switch (type)
                {
                case RPR_MATERIAL_NODE_INPUT_TYPE_FLOAT4:
                    input.type = amd::Input::Type::FLOAT4;
                    rprxMaterialGetParameterValue(m_uberMatContext, rprxMaterial, param.second, input.value.array.data());
                    break;

                case RPR_MATERIAL_NODE_INPUT_TYPE_UINT:
                    input.type = amd::Input::Type::UINT;
                    rprxMaterialGetParameterValue(m_uberMatContext, rprxMaterial, param.second, &input.value.integer);
                    break;

                case RPR_MATERIAL_NODE_INPUT_TYPE_NODE:
                {
                    // Set input type.
                    input.type = amd::Input::Type::IMAGE;

                    // Get the rpr_material_node input value.
                    rpr_material_node childNode = nullptr;
                    rprxMaterialGetParameterValue(m_uberMatContext, rprxMaterial, param.second, &childNode);

                    // Export the rpr_material_node input value and set the current Input's integer value to it's index in the glTF::materials array.
                    input.value.integer = ExportMaterialNode(ext.nodes, childNode);
                    break;
                }

                case RPR_MATERIAL_NODE_INPUT_TYPE_IMAGE:
                {
                    // Set input type.
                    input.type = amd::Input::Type::NODE;

                    // Retrieve the Radeon ProRender TEXTURE_NODE input value.
                    rpr_material_node textureNode = nullptr;
                    rprxMaterialGetParameterValue(m_uberMatContext, rprxMaterial, param.second, &textureNode);

                    // Assume the TEXTURE_NODE input value has an image set.
                    rpr_image img = nullptr;
                    rprMaterialNodeGetInfo(textureNode, RPR_MATERIAL_NODE_INPUT_VALUE, sizeof(img), &img, nullptr);
                    std::string filename = node.name + "_" + input.name + "_image" + std::to_string(m_gltf->images.size());

                    // Export the image and store the index in the input's integer value.
                    input.value.integer = ExportImage(filename, img);
                    break;
                }
                }

                // Add the Input to the Node.
                node.inputs.push_back(std::move(input));
            }

            // Add uber material node to extension object.
            ext.nodes.push_back(std::move(node));

            // Export extension to a new GLTF Material.
            gltf::Material material = {};
            material.name = node.name;
            amd::ExportExtension(ext, material.extensions);

            // Add GLTF Material to m_gltf object.
            m_gltf->materials.push_back(std::move(material));

            // Return the material index.
            return static_cast<int>(m_gltf->materials.size() - 1);
        }

        void ExportShapeParameters(nlohmann::json& extras, rpr_shape shape)
        {
            auto& shapeParams = extras["rpr.shape.parameters"];

            rpr_bool visibility = RPR_FALSE;
            rprMeshGetInfo(shape, RPR_SHAPE_VISIBILITY_FLAG, sizeof(visibility), &visibility, nullptr);
            shapeParams.emplace("visibility", visibility);

            rpr_bool shadow = RPR_FALSE;
            rprMeshGetInfo(shape, RPR_SHAPE_SHADOW_FLAG, sizeof(shadow), &shadow, nullptr);
            shapeParams.emplace("shadow", shadow);

            rpr_bool visibilityPrimaryOnly = RPR_FALSE;
            rprMeshGetInfo(shape, RPR_SHAPE_VISIBILITY_PRIMARY_ONLY_FLAG, sizeof(visibilityPrimaryOnly), &visibilityPrimaryOnly, nullptr);
            shapeParams.emplace("visibilityPrimaryOnly", visibilityPrimaryOnly);

            rpr_bool visibilityInSpecular = RPR_FALSE;
            rprMeshGetInfo(shape, RPR_SHAPE_VISIBILITY_IN_SPECULAR_FLAG, sizeof(visibilityInSpecular), &visibilityInSpecular, nullptr);
            shapeParams.emplace("visibilityInSpecular", visibilityInSpecular);

            rpr_bool shadowCatcher = RPR_FALSE;
            rprMeshGetInfo(shape, RPR_SHAPE_SHADOW_CATCHER_FLAG, sizeof(shadowCatcher), &shadowCatcher, nullptr);
            shapeParams.emplace("shadowCatcher", shadowCatcher);

            std::array<float, 4> angularMotion = { 0.0f, 0.0f, 0.0f, 0.0f };
            rprMeshGetInfo(shape, RPR_SHAPE_ANGULAR_MOTION, sizeof(float) * 4, angularMotion.data(), nullptr);
            shapeParams.emplace("angularMotion", angularMotion);

            std::array<float, 4> linearMotion = { 0.0f, 0.0f, 0.0f, 0.0f };
            rprMeshGetInfo(shape, RPR_SHAPE_LINEAR_MOTION, sizeof(float) * 4, linearMotion.data(), nullptr);
            shapeParams.emplace("linearMotion", linearMotion);

            rpr_uint subdivisionFactor = 0;
            rprMeshGetInfo(shape, RPR_SHAPE_SUBDIVISION_FACTOR, sizeof(subdivisionFactor), &subdivisionFactor, nullptr);
            shapeParams.emplace("subdivisionFactor", subdivisionFactor);

            float subdivisionCreaseWeight = 0;
            rprMeshGetInfo(shape, RPR_SHAPE_SUBDIVISION_CREASEWEIGHT, sizeof(subdivisionCreaseWeight), &subdivisionCreaseWeight, nullptr);
            shapeParams.emplace("subdivisionCreaseWeight", subdivisionCreaseWeight);

            rpr_uint subdivisionBoundaryInterop = 0;
            rprMeshGetInfo(shape, RPR_SHAPE_SUBDIVISION_BOUNDARYINTEROP, sizeof(subdivisionBoundaryInterop), &subdivisionBoundaryInterop, nullptr);
            shapeParams.emplace("subdivisionBoundaryInterop", subdivisionBoundaryInterop);

            std::array<float, 2> displacementScale = { 0.0f, 0.0f };
            rprMeshGetInfo(shape, RPR_SHAPE_DISPLACEMENT_SCALE, sizeof(float) * 2, displacementScale.data(), nullptr);
            shapeParams.emplace("displacementScale", displacementScale);

            rpr_uint objectGroupID = 0;
            rprMeshGetInfo(shape, RPR_SHAPE_DISPLACEMENT_SCALE, sizeof(objectGroupID), &objectGroupID, nullptr);
            shapeParams.emplace("objectGroupID", objectGroupID);
        }

        struct ShapeVertexData
        {
            std::vector<rpr_int> indices;
            std::vector<float> vertices;
            std::vector<std::tuple<std::string, gltf::Accessor::Type, size_t, size_t, size_t>> attributes;
        };

        bool GetShapeVertexData(rpr_shape shape, ShapeVertexData& result)
        {
            // Combine all vertex attributes into a large block of memory that uses a single index buffer.
            std::array<std::tuple<rpr_mesh_info, rpr_mesh_info, rpr_mesh_info, std::string>, 4> vertexAttributes = {
                std::tuple<rpr_mesh_info, rpr_mesh_info, rpr_mesh_info, std::string>{ RPR_MESH_VERTEX_INDEX_ARRAY, RPR_MESH_VERTEX_COUNT, RPR_MESH_VERTEX_ARRAY, gltf::AttributeNames::POSITION },
                std::tuple<rpr_mesh_info, rpr_mesh_info, rpr_mesh_info, std::string>{ RPR_MESH_NORMAL_INDEX_ARRAY, RPR_MESH_NORMAL_COUNT, RPR_MESH_NORMAL_ARRAY, gltf::AttributeNames::NORMAL },
                std::tuple<rpr_mesh_info, rpr_mesh_info, rpr_mesh_info, std::string>{ RPR_MESH_UV_INDEX_ARRAY, RPR_MESH_UV_COUNT, RPR_MESH_UV_ARRAY, gltf::AttributeNames::TEXCOORD_0 },
                std::tuple<rpr_mesh_info, rpr_mesh_info, rpr_mesh_info, std::string>{ RPR_MESH_UV2_INDEX_ARRAY, RPR_MESH_UV2_COUNT, RPR_MESH_UV2_ARRAY, gltf::AttributeNames::TEXCOORD_1 }
            };

            // First iterate over all of the vertex attributes get determine total number of float elements.
            size_t totalAttributesNumElements = 0;
            for (auto& entry : vertexAttributes)
            {
                size_t attributeCount, attributeNumBytes;
                rprMeshGetInfo(shape, std::get<1>(entry), sizeof(attributeCount), &attributeCount, nullptr);
                rprMeshGetInfo(shape, std::get<2>(entry), 0, nullptr, &attributeNumBytes);

                if (attributeCount > 0)
                {
                    size_t numComponents = attributeNumBytes / (sizeof(float) * attributeCount);
                    totalAttributesNumElements += numComponents;
                }
            }

            // Get the total number of indices from the first attribute.
            size_t indexArrayNumBytes;
            rprMeshGetInfo(shape, std::get<0>(vertexAttributes[0]), 0, nullptr, &indexArrayNumBytes);
            size_t indiceCount = indexArrayNumBytes / sizeof(rpr_int);

            // Allocate memory for all vertex attributes.
            result.vertices.resize(indiceCount * totalAttributesNumElements);

            // Offset for next attribute write location.
            size_t attributeWriteOffset = 0;

            // Iterate over all vertex attributes again and write to memory.
            size_t indexCount = 0;
            for (auto& entry : vertexAttributes)
            {
                // Get the index and data array for the current vertex attribute.
                size_t indexArrayNumBytes, attributeCount, attributeNumBytes;
                rprMeshGetInfo(shape, std::get<0>(entry), 0, nullptr, &indexArrayNumBytes);
                rprMeshGetInfo(shape, std::get<1>(entry), sizeof(attributeCount), &attributeCount, nullptr);
                rprMeshGetInfo(shape, std::get<2>(entry), 0, nullptr, &attributeNumBytes);

                // Skip empty attributes.
                if (attributeCount == 0)
                    continue;

                std::vector<rpr_int> indexArray(indexArrayNumBytes / sizeof(rpr_int));
                rprMeshGetInfo(shape, std::get<0>(entry), indexArrayNumBytes, indexArray.data(), nullptr);

                std::vector<float> attributeArray(attributeNumBytes / sizeof(float));
                rpr_int error = rprMeshGetInfo(shape, std::get<2>(entry), attributeNumBytes, attributeArray.data(), nullptr);

                // Validate index array counts are the same across vertex attributes.
                if (indexCount > 0 && indexCount != indexArrayNumBytes / sizeof(rpr_int))
                    return false;
                // if index array count has never been set, use the first vertex attribute's index array.
                else if (indexCount == 0)
                    indexCount = indexArrayNumBytes / sizeof(rpr_int);

                // Write the attribute data to ShapeVertexData result.
                size_t numComponents = attributeNumBytes / (sizeof(float) * attributeCount);
                int j = 0;
                for (auto i : indexArray)
                {
                    for (auto c = 0; c < numComponents; ++c)
                        result.vertices[attributeWriteOffset + j * numComponents + c] = attributeArray[i * numComponents + c];
                    ++j;
                }

                // Add attribute information to ShapeVertexData result.
                gltf::Accessor::Type type = gltf::Accessor::Type::SCALAR;
                if (numComponents == 2) type = gltf::Accessor::Type::VEC2;
                else if (numComponents == 3) type = gltf::Accessor::Type::VEC3;
                else if (numComponents == 4) type = gltf::Accessor::Type::VEC4;

                size_t bytesWritten = indexArray.size() * numComponents * sizeof(float);
                result.attributes.push_back(std::make_tuple(std::get<3>(entry), type, attributeWriteOffset * sizeof(float), bytesWritten, indiceCount));

                // Increment attribute write offset.
                attributeWriteOffset += indexArray.size() * numComponents;
            }

            /* std::ofstream outfile("F:/vertices.txt");
            for (int i = 0; i < result.vertices.size(); i += 8)
            {
            outfile << result.vertices[i] << ", " << result.vertices[i + 1] << ", " << result.vertices[i + 2] << "\n";
            }
            outfile.close();*/

            // Generate a new index buffer for all vertex attributes.
            result.indices.resize(indexCount);
            for (auto i = 0U; i < indexCount; ++i)
                result.indices[i] = i;

            return true;
        }

        void ExportShape(gltf::Scene& scene, rpr_shape rprShape, bool addNodeToScene = true)
        {
            // Get the shape type (mesh vs instance).
            rpr_shape_type type;
            rpr_int status = rprShapeGetInfo(rprShape, RPR_SHAPE_TYPE, sizeof(type), &type, nullptr);
            if (status == RPR_SUCCESS)
            {
                // A mesh must create the full node > mesh > accessor > bufferView > buffer hierarchy.
                if (type == RPR_SHAPE_TYPE_MESH)
                {
                    // Make sure the shape hasn't already been exported twice.  If it has, then just add the nodeID to the scene.
                    auto itr = m_exportedShapes.find(rprShape);
                    if (itr != m_exportedShapes.end())
                    {
                        // Add the node ID to the scene since this is the case where an RPR instance was found before it's parent shape.
                        // Which caused the parent shape's data to be exported but not added to the scene.
                        scene.nodes.push_back(itr->second);
                        return;
                    }

                    // Since Radeon ProRender supports different index buffers for each vertex attribute, the data must be extracted and reshuffled
                    // so only a single index buffer references the data.  This WILL induce additional overhead and latency unfortunately.
                    ShapeVertexData shapeVertexData;
                    if (!GetShapeVertexData(rprShape, shapeVertexData))
                        return;

                    // Create the GLTF Accessor for the indices.
                    gltf::Accessor accessor;
                    accessor.type = gltf::Accessor::Type::SCALAR;
                    accessor.bufferView = static_cast<int>(m_gltf->bufferViews.size());
                    accessor.byteOffset = 0;
                    accessor.componentType = gltf::Accessor::ComponentType::UNSIGNED_INT;
                    accessor.count = static_cast<int>(shapeVertexData.indices.size());
                    m_gltf->accessors.push_back(std::move(accessor));

                    // Create a GLTF BufferView for the indices and export to disk.
                    gltf::BufferView bufferView;
                    std::string filename = "mesh" + std::to_string(m_gltf->meshes.size()) + "_indices_buffer" + std::to_string(m_gltf->buffers.size());
                    bufferView.buffer = ExportBuffer(filename, reinterpret_cast<const char*>(shapeVertexData.indices.data()), shapeVertexData.indices.size() * sizeof(int));
                    bufferView.byteOffset = 0;
                    bufferView.byteLength = static_cast<int>(shapeVertexData.indices.size() * sizeof(int));
                    bufferView.byteStride = sizeof(int);
                    bufferView.target = gltf::BufferView::Target::ELEMENT_ARRAY_BUFFER;
                    m_gltf->bufferViews.push_back(std::move(bufferView));

                    // Create a new GLTF Mesh_Primitive object for shape, default to triangle primitives.
                    gltf::Mesh_Primitive meshPrimitive;
                    meshPrimitive.mode = gltf::Mesh_Primitive::Mode::TRIANGLES;
                    meshPrimitive.indices = static_cast<int>(m_gltf->accessors.size()) - 1;

                    // Add all of the vertex attributes to the Mesh_Primitive instance.
                    for (auto& entry : shapeVertexData.attributes)
                    {
                        // Add attribute to Mesh_Primitive.
                        meshPrimitive.attributes.emplace(std::get<0>(entry), static_cast<int>(m_gltf->accessors.size()));

                        // Create a GLTF Accessor to reference it.
                        gltf::Accessor accessor;
                        accessor.type = std::get<1>(entry);
                        accessor.bufferView = static_cast<int>(m_gltf->bufferViews.size());
                        accessor.byteOffset = 0;
                        accessor.componentType = gltf::Accessor::ComponentType::FLOAT;
                        accessor.count = static_cast<int>(std::get<4>(entry));

                        // Calculate attribute element byte stride.
                        int numComponents = 1;
                        switch (accessor.type)
                        {
                        case gltf::Accessor::Type::VEC2:
                            numComponents = 2;
                            break;
                        case gltf::Accessor::Type::VEC3:
                            numComponents = 3;
                            break;
                        case gltf::Accessor::Type::VEC4:
                            numComponents = 4;
                            break;
                        }

                        int byteStride = sizeof(float) * numComponents;

                        // Create a GLTF BufferView for this segment of the buffer.
                        gltf::BufferView bufferView;
                        bufferView.buffer = static_cast<int>(m_gltf->buffers.size());
                        bufferView.byteOffset = static_cast<int>(std::get<2>(entry));
                        bufferView.byteLength = static_cast<int>(std::get<3>(entry));
                        bufferView.byteStride = byteStride;
                        bufferView.target = gltf::BufferView::Target::ARRAY_BUFFER;
                        m_gltf->bufferViews.push_back(std::move(bufferView));

                        // Determine min and max range for vertex attribute.
                        accessor.min.resize(numComponents, FLT_MAX);
                        accessor.max.resize(numComponents, -FLT_MAX);
                        float* attributeData = reinterpret_cast<float*>(&shapeVertexData.vertices[bufferView.byteOffset / sizeof(float)]);
                        for (auto i = 0; i < accessor.count; ++i)
                        {
                            for (auto c = 0; c < numComponents; ++c)
                            {
                                float value = attributeData[i * numComponents + c];
                                accessor.min[c] = std::min(accessor.min[c], value);
                                accessor.max[c] = std::max(accessor.max[c], value);
                            }
                        }

                        // Now add the accessor to the glTF object after determining min and max ranges.
                        m_gltf->accessors.push_back(std::move(accessor));
                    }

                    // Export a GLTF Buffer for the vertex attributes.
                    filename = "mesh" + std::to_string(m_gltf->meshes.size()) + "_vertex_buffer" + std::to_string(m_gltf->buffers.size());
                    ExportBuffer(filename, reinterpret_cast<const char*>(shapeVertexData.vertices.data()), shapeVertexData.vertices.size() * sizeof(float));

                    // Create a new GLTF Node object for the shape and add shape's transform.
                    gltf::Node node;
                    node.mesh = static_cast<int>(m_gltf->meshes.size());
                    rprMeshGetInfo(rprShape, RPR_SHAPE_TRANSFORM, sizeof(float) * 16, node.matrix.data(), nullptr);

                    // Export all shape properties to a json object to be stored in the GLTF Node's "extras".
                    ExportShapeParameters(node.extras, rprShape);

                    // Add the node ID to the GLTF scene.
                    if (addNodeToScene)
                        scene.nodes.push_back(static_cast<int>(m_gltf->nodes.size()));

                    // Add node to GLTF object.
                    m_gltf->nodes.push_back(std::move(node));

                    // Export the material.
                    rpr_material_node rprMaterial = nullptr;
                    if (rprShapeGetInfo(rprShape, RPR_SHAPE_MATERIAL, sizeof(rprMaterial), &rprMaterial, nullptr) == RPR_SUCCESS && rprMaterial)
                        meshPrimitive.material = ExportMaterial(rprMaterial);
                    else
                    {
                        rprx_material rprxMaterial = nullptr;
                        if (rprxShapeGetMaterial(m_uberMatContext, rprShape, &rprxMaterial) == RPR_SUCCESS && rprxMaterial)
                            meshPrimitive.material = ExportMaterial(rprxMaterial);
                    }

                    // Create a new GLTF Mesh object for the shape.
                    gltf::Mesh mesh;
                    mesh.primitives.push_back(std::move(meshPrimitive));
                    m_gltf->meshes.push_back(std::move(mesh));
                }
                else if (type == RPR_SHAPE_TYPE_INSTANCE)
                {
                    // Get the parent shape of the instance.
                    rpr_shape parentShape = nullptr;
                    rprInstanceGetBaseShape(rprShape, &parentShape);

                    // Look up the GLTF Node created by the parent shape.
                    // If it doesn't exist yet, export it.
                    auto itr = m_exportedShapes.find(parentShape);
                    int parentNodeID = -1;
                    if (itr != m_exportedShapes.end())
                        parentNodeID = itr->second;
                    else
                    {
                        ExportShape(scene, parentShape, false);
                        parentNodeID = static_cast<int>(m_gltf->nodes.size() - 1);
                    }

                    // Get the parent m_gltf Node.
                    auto& parentNode = m_gltf->nodes[parentNodeID];

                    // Copy the parent's Mesh (which includes all Mesh_Primitive and Accessor instances).
                    gltf::Mesh meshCopy = m_gltf->meshes[parentNode.mesh];

                    // Export the instance material.
                    // Export the material.
                    rpr_material_node rprMaterial = nullptr;
                    if (rprShapeGetInfo(rprShape, RPR_SHAPE_MATERIAL, sizeof(rprMaterial), &rprMaterial, nullptr) == RPR_SUCCESS && rprMaterial)
                        meshCopy.primitives[0].material = ExportMaterial(rprMaterial);
                    else
                    {
                        rprx_material rprxMaterial = nullptr;
                        if (rprxShapeGetMaterial(m_uberMatContext, rprShape, &rprxMaterial) == RPR_SUCCESS && rprxMaterial)
                            meshCopy.primitives[0].material = ExportMaterial(rprxMaterial);
                    }

                    // Create a new GLTF Node for the instance and reference the mesh copy.
                    gltf::Node node;
                    node.mesh = static_cast<int>(m_gltf->meshes.size());

                    // Export the instance transform.
                    rprShapeGetInfo(rprShape, RPR_SHAPE_TRANSFORM, sizeof(float) * 16, node.matrix.data(), nullptr);

                    // Export the instance parameters.
                    ExportShapeParameters(node.extras, rprShape);

                    // Tag the node indicating it's a Radeon ProRender instance.
                    auto& shapeParams = node.extras["rpr.shape.parameters"];
                    shapeParams.emplace("parentShapeID", parentNodeID);

                    // Add the node ID to the GLTF scene.
                    scene.nodes.push_back(static_cast<int>(m_gltf->nodes.size()));

                    // Add the node to the GLTF object.
                    m_gltf->nodes.push_back(std::move(node));

                    // Add the Mesh to the GLTF object.
                    m_gltf->meshes.push_back(std::move(meshCopy));
                }

                // Keep track of which node IDs map to which rpr_shape handles.
                m_exportedShapes.emplace(rprShape, static_cast<int>(m_gltf->nodes.size() - 1));
            }
        }

        void ExportLight(amd::AMD_RPR_Lights& lightsExt, rpr_light rprLight)
        {
            // Create a new GLTF AMD_RPR_Lights extension light object.
            amd::Light light;

            // Get the light transform.
            rprLightGetInfo(rprLight, RPR_LIGHT_TRANSFORM, sizeof(float) * 16, light.transform.data(), nullptr);

            // Query the light type of the Radeon ProRender light handle.
            rpr_light_info type;
            rprLightGetInfo(rprLight, RPR_LIGHT_TYPE, sizeof(type), &type, nullptr);

            // Fill in the light properties.
            switch (type)
            {
            case RPR_LIGHT_TYPE_POINT:
                light.type = amd::Light::Type::POINT;
                rprLightGetInfo(rprLight, RPR_POINT_LIGHT_RADIANT_POWER, sizeof(float) * 4, light.point.radiantPower.data(), nullptr);
                break;

            case RPR_LIGHT_TYPE_DIRECTIONAL:
                light.type = amd::Light::Type::DIRECTIONAL;
                rprLightGetInfo(rprLight, RPR_DIRECTIONAL_LIGHT_RADIANT_POWER, sizeof(float) * 4, light.directional.radiantPower.data(), nullptr);
                rprLightGetInfo(rprLight, RPR_DIRECTIONAL_LIGHT_SHADOW_SOFTNESS, sizeof(float), &light.directional.shadowSoftness, nullptr);
                break;

            case RPR_LIGHT_TYPE_SPOT:
                light.type = amd::Light::Type::SPOT;
                rprLightGetInfo(rprLight, RPR_SPOT_LIGHT_RADIANT_POWER, sizeof(float) * 4, light.spot.radiantPower.data(), nullptr);
                rprLightGetInfo(rprLight, RPR_SPOT_LIGHT_CONE_SHAPE, sizeof(float) * 2, &light.spot.innerAngle, nullptr);
                break;

            case RPR_LIGHT_TYPE_ENVIRONMENT:
            {
                light.type = amd::Light::Type::ENVIRONMENT;
                rpr_image image = nullptr;
                rprLightGetInfo(rprLight, RPR_ENVIRONMENT_LIGHT_IMAGE, sizeof(image), &image, nullptr);

                std::string filename = "light" + std::to_string(lightsExt.lights.size()) + "_environment_image" + std::to_string(m_gltf->images.size());
                light.environment.image = ExportImage(filename, image);
                rprLightGetInfo(rprLight, RPR_ENVIRONMENT_LIGHT_INTENSITY_SCALE, sizeof(float), &light.environment.intensityScale, nullptr);
                break;
            }

            case RPR_LIGHT_TYPE_SKY:
                light.type = amd::Light::Type::SKY;
                rprLightGetInfo(rprLight, RPR_SKY_LIGHT_TURBIDITY, sizeof(float), &light.sky.turbidity, nullptr);
                rprLightGetInfo(rprLight, RPR_SKY_LIGHT_ALBEDO, sizeof(float), &light.sky.albedo, nullptr);
                rprLightGetInfo(rprLight, RPR_SKY_LIGHT_SCALE, sizeof(float), &light.sky.scale, nullptr);
                break;

            case RPR_LIGHT_TYPE_IES:
            {
                rpr_ies_image_desc desc;
                rprLightGetInfo(rprLight, RPR_IES_LIGHT_IMAGE_DESC, sizeof(desc), &desc, nullptr);
                size_t numBytes = strlen(desc.data);

                light.type = amd::Light::Type::IES;
                light.ies.nx = desc.w;
                light.ies.ny = desc.h;
                rprLightGetInfo(rprLight, RPR_IES_LIGHT_RADIANT_POWER, sizeof(float) * 4, light.ies.radiantPower.data(), nullptr);

                // Save the IES data to the export directory as a bin file.
                std::string filename = "light" + std::to_string(lightsExt.lights.size()) + "_ies_data_buffer" + std::to_string(m_gltf->buffers.size());
                light.ies.buffer = ExportBuffer(filename, desc.data, numBytes);
                break;
            }
            }

            // Add the light to the extension object.
            lightsExt.lights.push_back(std::move(light));
        }

        void ExportCamera(gltf::Scene& scene, rpr_camera camera)
        {
            // Declare an instance of AMD_RPR_Camera.
            amd::AMD_RPR_Camera amd_rpr_camera;

            // Query properties of Radeon ProRender camera.
            rpr_camera_info mode;
            rprCameraGetInfo(camera, RPR_CAMERA_MODE, sizeof(mode), &mode, nullptr);
            amd_rpr_camera.cameraMode = static_cast<amd::AMD_RPR_Camera::CameraMode>(mode);
            rprCameraGetInfo(camera, RPR_CAMERA_APERTURE_BLADES, sizeof(int), &amd_rpr_camera.apertureBlades, nullptr);
            rprCameraGetInfo(camera, RPR_CAMERA_EXPOSURE, sizeof(float), &amd_rpr_camera.exposure, nullptr);
            rprCameraGetInfo(camera, RPR_CAMERA_FOCUS_DISTANCE, sizeof(float), &amd_rpr_camera.focusDistance, nullptr);
            rprCameraGetInfo(camera, RPR_CAMERA_FOCAL_LENGTH, sizeof(float), &amd_rpr_camera.focalLength, nullptr);
            rprCameraGetInfo(camera, RPR_CAMERA_FOCAL_TILT, sizeof(float), &amd_rpr_camera.focalTilt, nullptr);
            rprCameraGetInfo(camera, RPR_CAMERA_FSTOP, sizeof(float), &amd_rpr_camera.fstop, nullptr);
            rprCameraGetInfo(camera, RPR_CAMERA_IPD, sizeof(float), &amd_rpr_camera.ipd, nullptr);
            rprCameraGetInfo(camera, RPR_CAMERA_LENS_SHIFT, sizeof(float) * 2, amd_rpr_camera.lensShift.data(), nullptr);
            rprCameraGetInfo(camera, RPR_CAMERA_ORTHO_WIDTH, sizeof(float), &amd_rpr_camera.orthoWidth, nullptr);
            rprCameraGetInfo(camera, RPR_CAMERA_ORTHO_HEIGHT, sizeof(float), &amd_rpr_camera.orthoHeight, nullptr);
            rprCameraGetInfo(camera, RPR_CAMERA_SENSOR_SIZE, sizeof(float), &amd_rpr_camera.sensorSize, nullptr);
            rprCameraGetInfo(camera, RPR_CAMERA_TILT_CORRECTION, sizeof(float), &amd_rpr_camera.tiltCorrection, nullptr);
            rprCameraGetInfo(camera, RPR_CAMERA_POSITION, sizeof(float) * 4, amd_rpr_camera.position.data(), nullptr);
            rprCameraGetInfo(camera, RPR_CAMERA_LOOKAT, sizeof(float) * 4, amd_rpr_camera.lookAt.data(), nullptr);
            rprCameraGetInfo(camera, RPR_CAMERA_UP, sizeof(float) * 4, amd_rpr_camera.up.data(), nullptr);

            // Fix issue where the json library does not know how to process FLT_MAX.
            if (isinf(amd_rpr_camera.fstop))
                amd_rpr_camera.fstop = FLT_MAX * 0.75f;

            // Declare an instance of the m_gltf camera and fill in default values.
            gltf::Camera gltf_camera;
            switch (amd_rpr_camera.cameraMode)
            {
            case amd::AMD_RPR_Camera::CameraMode::ORTHOGRAPHIC:
                gltf_camera.type = gltf::Camera::Type::PERSPECTIVE;
                gltf_camera.orthographic.xmag = 1.0f;
                gltf_camera.orthographic.ymag = 1.0f;
                gltf_camera.orthographic.znear = 1e-3f;
                gltf_camera.orthographic.zfar = 1e3f;
                break;

            default:
                gltf_camera.type = gltf::Camera::Type::PERSPECTIVE;
                gltf_camera.perspective.aspectRatio = 1.0f;
                gltf_camera.perspective.yfov = 60.0f * 3.14159f / 180.0f;
                gltf_camera.perspective.znear = 1e-2f;
                gltf_camera.perspective.zfar = 1e5f;
                break;
            }

            // Add AMD_RPR_Camera as an extension.
            amd::ExportExtension(amd_rpr_camera, gltf_camera.extensions);

            // Declare a m_gltf node to reference the camera and store the transform.
            gltf::Node node;
            node.camera = static_cast<int>(m_gltf->cameras.size());
            rprCameraGetInfo(camera, RPR_CAMERA_TRANSFORM, sizeof(float) * 16, node.matrix.data(), nullptr);

            // Add the node ID to the scene.
            scene.nodes.push_back(static_cast<int>(m_gltf->nodes.size()));

            // Add the camera to the m_gltf object's cameras array.
            m_gltf->cameras.push_back(std::move(gltf_camera));

            // Add the node to the m_gltf object's nodes array.
            m_gltf->nodes.push_back(std::move(node));
        }

        void ExportScene(rpr_scene rprScene)
        {
            // Get the name of the Radeon ProRender scene.
            size_t nameLength = 0;
            rpr_int status = rprSceneGetInfo(rprScene, RPR_OBJECT_NAME, 0, nullptr, &nameLength);
            if (status != RPR_SUCCESS) return;

            std::string name(nameLength, '\0');
            status = rprSceneGetInfo(rprScene, RPR_OBJECT_NAME, nameLength, &name[0], nullptr);
            if (status != RPR_SUCCESS) return;

            // Create a new GLTF scene.
            gltf::Scene scene;
            scene.name = name.c_str();

            // Export scene properties.
            auto& sceneParams = scene.extras["rpr.scene.parameters"];

            // Export background image.
            rpr_image backgroundImage = nullptr;
            rprSceneGetBackgroundImage(rprScene, &backgroundImage);
            if (backgroundImage)
            {
                std::string filename = "scene" + std::to_string(m_gltf->scenes.size()) + "_backgroundImage";
                sceneParams.emplace("backgroundImage", ExportImage(filename, backgroundImage));
            }

            /* TODO
            RPR_SCENE_ENVIRONMENT_OVERRIDE_REFLECTION
            *_REFLECTION
            *_REFRACTION
            *_TRANSPARENCY
            *_BACKGROUND
            */

            // Export the scene's camera.
            rpr_camera camera = nullptr;
            status = rprSceneGetCamera(rprScene, &camera);
            if (status != RPR_SUCCESS) return;
            if (camera != nullptr)
                ExportCamera(scene, camera);

            // Export the scene's lights.
            size_t lightCount = 0;
            status = rprSceneGetInfo(rprScene, RPR_SCENE_LIGHT_COUNT, sizeof(lightCount), &lightCount, nullptr);
            if (status != RPR_SUCCESS) return;
            if (lightCount > 0)
            {
                // Declare an instance of the AMD_RPR_lights extension.
                amd::AMD_RPR_Lights lightsExt;

                // Get list of lights.
                std::vector<rpr_light> lights(lightCount);
                status = rprSceneGetInfo(rprScene, RPR_SCENE_LIGHT_LIST, sizeof(rpr_light) * lightCount, lights.data(), nullptr);
                if (status != RPR_SUCCESS) return;

                // Export all of the lights.
                for (auto i = 0U; i < lightCount; ++i)
                {
                    ExportLight(lightsExt, lights[i]);
                }

                // Add the lights extension to the m_gltf Scene object.
                amd::ExportExtension(lightsExt, scene.extensions);
            }

            // Export the scene's shapes.
            size_t shapeCount = 0;
            status = rprSceneGetInfo(rprScene, RPR_SCENE_SHAPE_LIST, 0, nullptr, &shapeCount);
            shapeCount /= sizeof(rpr_shape);
            if (status != RPR_SUCCESS) return;
            if (shapeCount > 0)
            {
                // Get list of shapes.
                std::vector<rpr_shape> shapes(shapeCount);
                status = rprSceneGetInfo(rprScene, RPR_SCENE_SHAPE_LIST, shapeCount * sizeof(rpr_shape), shapes.data(), nullptr);
                if (status != RPR_SUCCESS) return;

                // Export all of the shapes.
                for (auto i = 0U; i < shapeCount; ++i)
                {
                    ExportShape(scene, shapes[i]);
                }
            }

            // Add the scene to the GLTF object.
            m_gltf->scenes.push_back(std::move(scene));
        }

        void ExportScenes(const std::vector<rpr_scene>& scenes)
        {
            for (auto scene : scenes)
            {
                ExportScene(scene);
            }
        }

        void ExportPostEffects()
        {
            // Create new post effects extension object.
            amd::AMD_RPR_Post_Effects ext;

            rpr_uint postEffectCount = 0;
            if (rprContextGetAttachedPostEffectCount(m_context, &postEffectCount) == RPR_SUCCESS)
            {
                for (auto i = 0U; i < postEffectCount; ++i)
                {
                    rpr_post_effect rprPostEffect = nullptr;
                    rprContextGetAttachedPostEffect(m_context, i, &rprPostEffect);

                    rpr_post_effect_type type;
                    rprPostEffectGetInfo(rprPostEffect, RPR_POST_EFFECT_TYPE, sizeof(type), &type, nullptr);

                    amd::PostEffect postEffect;
                    postEffect.type = static_cast<amd::PostEffect::Type>(type);

                    switch (type)
                    {
                    case RPR_POST_EFFECT_TONE_MAP:
                        break;
                    case RPR_POST_EFFECT_WHITE_BALANCE:
                    {
                        rpr_uint colorSpace;
                        rprPostEffectGetInfo(rprPostEffect, RPR_POST_EFFECT_WHITE_BALANCE_COLOR_SPACE, sizeof(colorSpace), &colorSpace, nullptr);
                        postEffect.whiteBalance.colorSpace = static_cast<amd::WhiteBalance::ColorSpace>(colorSpace);
                        rprPostEffectGetInfo(rprPostEffect, RPR_POST_EFFECT_WHITE_BALANCE_COLOR_TEMPERATURE, sizeof(postEffect.whiteBalance.colorTemperature), &postEffect.whiteBalance.colorTemperature, nullptr);
                        break;
                    }
                    case RPR_POST_EFFECT_SIMPLE_TONEMAP:
                        rprPostEffectGetInfo(rprPostEffect, RPR_POST_EFFECT_SIMPLE_TONEMAP_EXPOSURE, sizeof(postEffect.simpleTonemap.exposure), &postEffect.simpleTonemap.exposure, nullptr);
                        rprPostEffectGetInfo(rprPostEffect, RPR_POST_EFFECT_SIMPLE_TONEMAP_CONTRAST, sizeof(postEffect.simpleTonemap.contrast), &postEffect.simpleTonemap.contrast, nullptr);
                        rprPostEffectGetInfo(rprPostEffect, RPR_POST_EFFECT_SIMPLE_TONEMAP_ENABLE_TONEMAP, sizeof(postEffect.simpleTonemap.enableTonemap), &postEffect.simpleTonemap.enableTonemap, nullptr);
                        break;
                    case RPR_POST_EFFECT_NORMALIZATION:
                        break;
                    case RPR_POST_EFFECT_GAMMA_CORRECTION:
                        break;
                    }

                    ext.postEffects.push_back(std::move(postEffect));
                }
            }

            // Add extension to GLTF object.
            amd::ExportExtension(ext, m_gltf->extensions);
        }

        void ExportContextParameters()
        {
            // Add entry to "extras" for Radeon ProRender m_context parameters.
            auto& contextParams = m_gltf->extras["rpr.context.parameters"];

            // Query the number of m_context parameters.
            size_t paramCount = 0;
            rpr_int status = rprContextGetInfo(m_context, RPR_CONTEXT_PARAMETER_COUNT, sizeof(paramCount), &paramCount, nullptr);
            if (status == RPR_SUCCESS)
            {
                // Export all the m_context parameters to the "extras" parameter of the glTF object.
                for (int i = 0; i < static_cast<int>(paramCount); ++i)
                {
                    // Get the parameter name string.
                    size_t nameLength;
                    rprContextGetParameterInfo(m_context, i, RPR_PARAMETER_NAME_STRING, 0, nullptr, &nameLength);
                    std::string name(nameLength, '\0');
                    rprContextGetParameterInfo(m_context, i, RPR_PARAMETER_NAME_STRING, nameLength, &name[0], nullptr);

                    // Get the parameter type.
                    rpr_parameter_type type;
                    rprContextGetParameterInfo(m_context, i, RPR_PARAMETER_TYPE, sizeof(type), &type, nullptr);

                    // Insert the parameter into the "extras" json object.
                    // Only the data types below are supported.
                    switch (type)
                    {
                    case RPR_PARAMETER_TYPE_UINT:
                    {
                        uint32_t value;
                        rprContextGetParameterInfo(m_context, i, RPR_PARAMETER_VALUE, sizeof(uint32_t), &value, nullptr);
                        contextParams.emplace(name.c_str(), value);
                        break;
                    }
                    case RPR_PARAMETER_TYPE_FLOAT:
                    {
                        float value;
                        rprContextGetParameterInfo(m_context, i, RPR_PARAMETER_VALUE, sizeof(float), &value, nullptr);
                        contextParams.emplace(name.c_str(), value);
                        break;
                    }
                    case RPR_PARAMETER_TYPE_FLOAT3:
                    case RPR_PARAMETER_TYPE_FLOAT4:
                    {
                        int numComponents = type - RPR_PARAMETER_TYPE_FLOAT + 1;
                        std::vector<float> values(numComponents);
                        rprContextGetParameterInfo(m_context, i, RPR_PARAMETER_VALUE, sizeof(float) * numComponents, values.data(), nullptr);
                        contextParams.emplace(name.c_str(), values);
                        break;
                    }
                    }
                }
            }
        }

        rpr_context m_context;
        rpr_material_system m_materialSystem;
        rprx_context m_uberMatContext;

        gltf::glTF* m_gltf;
        std::string m_rootDirectory;

        std::unordered_map<rpr_shape, int> m_exportedShapes;
        std::unordered_map<rpr_material_node, int> m_exportedMaterialNodes;
        std::unordered_map<rpr_image, int> m_exportedImages;
    };

    /*bool ExportToGLTF(gltf::glTF& gltf, const std::string& filename, rpr_context context, rpr_material_system materialSystem, rprx_context uberMatContext, const std::vector<rpr_scene>& scenes)
    {
        Exporter exporter(context, materialSystem, uberMatContext);
        return exporter.Save(scenes, gltf, filename);
    }*/    

    bool ExportMaterialsToGLTF(gltf::glTF& gltf, const std::string& filename, rpr_context context, rpr_material_system materialSystem, rprx_context uberMatContext, const std::vector<rpr_material_node>& materialNodes, const std::vector<rprx_material>& uberMaterials)
    {
        Exporter exporter(context, materialSystem, uberMatContext);
        return exporter.Save(materialNodes, uberMaterials, gltf, filename);
    }
}

bool rprExportToGLTF(const char* filename, rpr_context context, rpr_material_system materialSystem, rprx_context uberMatContext, const rpr_scene* scenes, size_t sceneCount)
{
    std::vector<rpr_scene> scenesList;
    for (auto i = 0U; i < sceneCount; ++i)
        scenesList.push_back(scenes[i]);

    rpr::Exporter exporter(context, materialSystem, uberMatContext);
    gltf::glTF gltf;
    if (!exporter.Save(scenesList, gltf, filename))
        return false;

    return gltf::Export(filename, gltf);
}