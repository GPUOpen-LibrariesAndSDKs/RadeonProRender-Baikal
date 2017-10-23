#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <experimental/filesystem>
#include <FreeImage.h>
#include <RadeonProRender.h>
#include <RprSupport.h>
#include <Math/mathutils.h>
#include "gltf2.h"
#include "base64.h"
#include "Extensions/Extensions.h"
#include "ProRenderGLTF.h"
namespace fs = std::experimental::filesystem;

namespace rpr
{
    typedef RadeonProRender::matrix mat4;

    class Importer
    {
    public:
        Importer(rpr_context context, rpr_material_system materialSystem, rprx_context uberMatContext)
            : m_context(context)
            , m_materialSystem(materialSystem)
            , m_uberMatContext(uberMatContext)
        {
        }

        bool Load(gltf::glTF& gltf, const std::string& path, std::vector<rpr_scene>& scenes)
        {
            // Save reference to the gltf object.
            m_gltf = &gltf;

            // Get the parent directory of the gltf file.
            m_rootDirectory = fs::path(path).parent_path().generic_string();
            if (m_rootDirectory == "")
                m_rootDirectory = ".";

            // Import RPR API objects.
            ImportContextParameters();
            ImportPostEffects();
            ImportScenes(scenes);

            // Return success.
            return true;
        }

        bool Load(gltf::glTF& gltf, const std::string& path, rpr_scene* scene)
        {
            // Save reference to the gltf object.
            m_gltf = &gltf;

            // Get the parent directory of the gltf file.
            m_rootDirectory = fs::path(path).parent_path().generic_string();
            if (m_rootDirectory == "")
                m_rootDirectory = ".";

            // Import RPR API objects.
            ImportContextParameters();
            ImportPostEffects();
            
            // Import the glTF file's scene index.
            rpr_scene importedScene = ImportScene(gltf.scenes[gltf.scene]);
            if (!importedScene)
                return false;

            // Set the context's scene and save the address.            
            rprContextSetScene(m_context, importedScene);
            *scene = importedScene;
            
            // Return success.
            return true;
        }

        bool Load(gltf::glTF& gltf, const std::string& path, std::vector<rpr_material_node>& materialNodes, std::vector<rprx_material>& uberMaterials)
        {
            // Save reference to the gltf object.
            m_gltf = &gltf;

            // Get the parent directory of the gltf file.
            m_rootDirectory = fs::path(path).parent_path().generic_string();
            if (m_rootDirectory == "")
                m_rootDirectory = ".";

            // Import all of the materials into the internal cache.
            for (auto i = 0; i < m_gltf->materials.size(); ++i)
                ImportMaterial(nullptr, i);

            // Separate out cached materials into rpr_material_ndoe and rprx_material arrays.
            for (auto itr : m_materialCache)
            {
                auto entry = itr.second;
                if (entry.first == amd::Node::Type::UBER) uberMaterials.push_back(reinterpret_cast<rprx_material>(entry.second));
                else materialNodes.push_back(reinterpret_cast<rpr_material_node>(entry.second));
            }

            // Return success.
            return true;
        }

    private:
        // Utility functions.
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

        inline mat4 GetNodeTransformMatrix(gltf::Node& node, const mat4& worldTransform)
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

        struct InterleavedAttributesResult
        {
            std::vector<char*> bufferData;
            std::unordered_map<std::string, char*> attributeMemoryAddresses;
        };

        InterleavedAttributesResult GetInterleavedAttributeData(const std::unordered_map<std::string, int>& attributes)
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
                auto& accessor = m_gltf->accessors[itr.second];
                auto& bufferView = m_gltf->bufferViews[accessor.bufferView];

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
                delete stream;
                /*if (entry.bufferIndex == 0)
                {
                    int* p = (int*)data;
                    for (int i = 0; i < entry.byteLength / sizeof(int); ++i)
                        std::cout << p[i] << " ";
                    std::cout << "\n";
                    system("pause");
                }*/
            }

            // Return a map containing memory addresses paired to each attribute.
            for (auto& itr : attributes)
            {
                // Get the accessor and bufferView for the given attribute.
                auto& accessor = m_gltf->accessors[itr.second];
                auto& bufferView = m_gltf->bufferViews[accessor.bufferView];

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

        void DestroyInterleavedAttributesResult(InterleavedAttributesResult& iar)
        {
            for (auto data : iar.bufferData)
                delete[] data;
        }

        void ResetInternal()
        {
            for (auto& itr : m_bufferCache)
                delete itr.second;
            m_bufferCache.clear();

            m_imageCache.clear();
        }

        std::istream* ImportBuffer(int bufferIndex)
        {
            // Validate the index.
            if (bufferIndex < 0 || bufferIndex >= m_gltf->buffers.size())
                return nullptr;

            // See if the buffer is in the cache.
            //if (m_bufferCache.find(bufferIndex) != m_bufferCache.end())
            //    return m_bufferCache.at(bufferIndex);

            // Check to see if the buffer should be loaded from a base64 encode string.
            auto& buffer = m_gltf->buffers[bufferIndex];
            if (buffer.uri.find("data:application/octet-stream;base64,") != std::string::npos)
            {
                // Decode the data.
                std::string data = buffer.uri.substr(buffer.uri.find(";base64,") + strlen(";base64,"));
                data = base64_decode(data);

                // Create a std::istream object to reference the buffer data in memory.
                std::istream* stream = new std::istringstream(data, std::ios::binary | std::ios_base::in);

                // Store the buffer stream in the cache and return the pointer.
                //m_bufferCache.emplace(bufferIndex, stream);
                return stream;
            }
            // Check to see if buffer data resides on disk.
            else if (fs::exists(m_rootDirectory + "/" + buffer.uri))
            {
                // Open the file on disk as an std::istream object.
                std::istream* stream = new std::ifstream(m_rootDirectory + "/" + buffer.uri, std::ios::binary | std::ios::in);

                // Store the buffer stream in the cache and return the pointer.
                //m_bufferCache.emplace(bufferIndex, stream);
                return stream;
            }

            // Unsupported uri format.
            return nullptr;
        }

        rpr_image ImportImage(int imageIndex)
        {
            // Validate the index against the image arrya.
            if (imageIndex < 0 || imageIndex >= m_gltf->images.size())
                return nullptr;

            // See if image is in the cache.
            if (m_imageCache.find(imageIndex) != m_imageCache.end())
                return m_imageCache.at(imageIndex);

            // Check to see if image should be loaded from a base64 encoded string.
            auto& image = m_gltf->images[imageIndex];
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
                    rpr_image_format format = { 4, RPR_COMPONENT_TYPE_UINT8 };
                    rpr_image_desc desc = { FreeImage_GetWidth(bitmap), FreeImage_GetHeight(bitmap), 1, 0, 0 };
       
                    // Swap blue and red channels of JPEG and PNG images.
                    if ((type == "png" || type == "jpg") && format.num_components >= 3)
                    {
                        BYTE* data = FreeImage_GetBits(bitmap);
                        for (size_t y = 0; y < desc.image_height; ++y)
                        {
                            for (size_t x = 0; x < desc.image_width; ++x)
                            {
                                std::swap(data[(y * desc.image_width + x) * format.num_components + 0], data[(y * desc.image_width + x) * format.num_components + 2]);
                            }
                        }
                    }

                    rpr_image img = nullptr;
                    rpr_int result = rprContextCreateImage(m_context, format, &desc, FreeImage_GetBits(bitmap), &img);
                    if (result != RPR_SUCCESS)
                        return nullptr;

                    // Store the image in the cache.
                    m_imageCache.emplace(imageIndex, img);

                    // Free the bitmap memory.
                    FreeImage_Unload(bitmap);

                    // Return the image handle.
                    return img;
                }
            }
            // Check to see if image should be loaded from disk.
            else if (fs::exists(m_rootDirectory + "/" + image.uri))
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
                FIBITMAP* bitmap = FreeImage_Load(fif, (m_rootDirectory + "/" + image.uri).c_str());
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
                    rpr_image_format format = { FreeImage_GetBPP(bitmap) / 8, RPR_COMPONENT_TYPE_UINT8 };
                    rpr_image_desc desc = { FreeImage_GetWidth(bitmap), FreeImage_GetHeight(bitmap), 1, 0, 0 };

                    // Handle EXR and HDR images as 32-bit channels.
                    if (fif == FIF_HDR || fif == FIF_EXR)
                    {
                        format.num_components = FreeImage_GetBPP(bitmap) / 32;
                        format.type = RPR_COMPONENT_TYPE_FLOAT32;
                    }

                    // Swap blue and red channels of JPEG and PNG images.
                    if ((fif == FIF_JPEG || fif == FIF_PNG) && format.num_components >= 3)
                    {
                        BYTE* data = FreeImage_GetBits(bitmap);
                        for (size_t y = 0; y < desc.image_height; ++y)
                        {
                            for (size_t x = 0; x < desc.image_width; ++x)
                            {
                                std::swap(data[(y * desc.image_width + x) * format.num_components + 0], data[(y * desc.image_width + x) * format.num_components + 2]);
                            }
                        }
                    }

                    rpr_image img = nullptr;
                    rpr_int result = rprContextCreateImage(m_context, format, &desc, FreeImage_GetBits(bitmap), &img);
                    if (result != RPR_SUCCESS)
                        return nullptr;

                    // Store the image in the cache.
                    m_imageCache.emplace(imageIndex, img);

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
                auto& bufferView = m_gltf->bufferViews[image.bufferView];

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
                    rpr_image_format format = { FreeImage_GetBPP(bitmap) / 8, RPR_COMPONENT_TYPE_UINT8 };
                    rpr_image_desc desc = { FreeImage_GetWidth(bitmap), FreeImage_GetHeight(bitmap), 1, FreeImage_GetPitch(bitmap), 0 };

                    rpr_image img = nullptr;
                    rpr_int result = rprContextCreateImage(m_context, format, &desc, FreeImage_GetBits(bitmap), &img);
                    if (result != RPR_SUCCESS)
                        return nullptr;

                    // Store the image in the cache.
                    m_imageCache.emplace(imageIndex, img);

                    // Free the bitmap memory.
                    FreeImage_Unload(bitmap);

                    // Free the data byte array.
                    delete[] data;

                    // Delete stream.
                    delete stream;

                    // Return the image handle.
                    return img;
                }
            }

            // Unsupported uri format.
            return nullptr;
        }

        rpr_material_node ImportMaterialNode(std::vector<amd::Node>& nodes, int nodeIndex)
        {
            // Check node index range.
            if (nodeIndex < 0 || nodeIndex >= nodes.size())
                return nullptr;

            // Get the Node at the specified index.
            auto& node = nodes[nodeIndex];

            // Create a new Radeon ProRender material node.
            rpr_material_node materialNode = nullptr;
            rpr_int result = rprMaterialSystemCreateNode(m_materialSystem, static_cast<rpr_material_node_type>(node.type), &materialNode);
            if (result != RPR_SUCCESS)
                return nullptr;

            // Import all of the inputs.
            for (auto& input : node.inputs)
            {
                switch (input.type)
                {
                case amd::Input::Type::FLOAT4:
                    rprMaterialNodeSetInputF(materialNode, input.name.c_str(), input.value.array[0], input.value.array[1], input.value.array[2], input.value.array[3]);
                    break;

                case amd::Input::Type::UINT:
                    rprMaterialNodeSetInputU(materialNode, input.name.c_str(), input.value.integer);
                    break;

                case amd::Input::Type::NODE:
                {
                    // Check bounds of node reference.
                    if (input.value.integer >= 0 && input.value.integer < nodes.size())
                    {
                        // Import the material node and apply to uber material parameter.
                        auto childNode = ImportMaterialNode(nodes, input.value.integer);
                        if (childNode)
                            rprMaterialNodeSetInputN(materialNode, input.name.c_str(), childNode);
                    }
                    break;
                }

                case amd::Input::Type::IMAGE:
                {
                    // Import the referenced image.
                    auto image = ImportImage(input.value.integer);
                    if (image)
                    {
#if 0
                        // Create a IMAGE_TEXTURE node first.
                        rpr_material_node textureNode = nullptr;
                        rprMaterialSystemCreateNode(m_materialSystem, RPR_MATERIAL_NODE_IMAGE_TEXTURE, &textureNode);

                        // Apply image to texture node.
                        rprMaterialNodeSetInputImageData(textureNode, "data", image);

                        // Apply texture node to uber material input slot.
                        rprMaterialNodeSetInputN(materialNode, input.name.c_str(), textureNode);
#else
                        // Apply image to material node.
                        rprMaterialNodeSetInputImageData(materialNode, input.name.c_str(), image);
#endif
                    }
                    break;
                }
                }
            }

            return materialNode;
        }

        void ApplyNormalMapAndEmissive(rprx_material uberMaterial, gltf::Material& material)
        {
            // Apply normal map texture.
            rpr_image normalTexture = ImportImage(material.normalTexture.index);
            if (normalTexture)
            {
                // Create an IMAGE_TEXTURE material node to reference the normal map texture.
                rpr_material_node imageTexture = nullptr;
                rprMaterialSystemCreateNode(m_materialSystem, RPR_MATERIAL_NODE_IMAGE_TEXTURE, &imageTexture);
                rprMaterialNodeSetInputImageData(imageTexture, "data", normalTexture);

                // Create a NORMAL_MAP material node.
                rpr_material_node normalMap = nullptr;
                rprMaterialSystemCreateNode(m_materialSystem, RPR_MATERIAL_NODE_NORMAL_MAP, &normalMap);
                rprMaterialNodeSetInputN(normalMap, "color", imageTexture);

                // Populate the uber material input slot.
                rprxMaterialSetParameterN(m_uberMatContext, uberMaterial, RPRX_UBER_MATERIAL_NORMAL, normalMap);
            }

            // Apply emissive factor and texture.
            auto& emissiveFactor = material.emissiveFactor;
            if (emissiveFactor[0] > 0 || emissiveFactor[1] > 0 || emissiveFactor[2] > 0)
            {
                rprxMaterialSetParameterF(m_uberMatContext, uberMaterial, RPRX_UBER_MATERIAL_EMISSION_COLOR, emissiveFactor[0], emissiveFactor[1], emissiveFactor[2], 1.0f);

                float weight = std::max(emissiveFactor[0], std::max(emissiveFactor[1], emissiveFactor[2]));
                rprxMaterialSetParameterF(m_uberMatContext, uberMaterial, RPRX_UBER_MATERIAL_EMISSION_WEIGHT, weight, weight, weight, weight);
            }

            rpr_image emissiveTexture = ImportImage(material.emissiveTexture.index);
            if (emissiveTexture)
            {
                // Emissive factor is multiplied by texture value.
                rpr_material_node imageTexture = nullptr;
                rprMaterialSystemCreateNode(m_materialSystem, RPR_MATERIAL_NODE_IMAGE_TEXTURE, &imageTexture);
                rprMaterialNodeSetInputImageData(imageTexture, "data", emissiveTexture);
                
                rpr_material_node finalEmissiveValue = nullptr;
                rprMaterialSystemCreateNode(m_materialSystem, RPR_MATERIAL_NODE_ARITHMETIC, &finalEmissiveValue);
                rprMaterialNodeSetInputN(finalEmissiveValue, "color0", imageTexture);
                rprMaterialNodeSetInputF(finalEmissiveValue, "color1", emissiveFactor[0], emissiveFactor[1], emissiveFactor[2], 1.0f);
                rprMaterialNodeSetInputU(finalEmissiveValue, "op", RPR_MATERIAL_NODE_OP_MUL);
                
                rprxMaterialSetParameterN(m_uberMatContext, uberMaterial, RPRX_UBER_MATERIAL_EMISSION_COLOR, finalEmissiveValue);

                // Need to set the emissive weight to the color however the weight must be a single value across the input vector.
                // So here we select the max of the 3 color components.
                rpr_material_node selectX = nullptr;
                rprMaterialSystemCreateNode(m_materialSystem, RPR_MATERIAL_NODE_ARITHMETIC, &selectX);
                rprMaterialNodeSetInputN(selectX, "color0", finalEmissiveValue);
                rprMaterialNodeSetInputU(selectX, "op", RPR_MATERIAL_NODE_OP_SELECT_X);

                rpr_material_node selectY = nullptr;
                rprMaterialSystemCreateNode(m_materialSystem, RPR_MATERIAL_NODE_ARITHMETIC, &selectY);
                rprMaterialNodeSetInputN(selectY, "color0", finalEmissiveValue);
                rprMaterialNodeSetInputU(selectY, "op", RPR_MATERIAL_NODE_OP_SELECT_Y);

                rpr_material_node selectZ = nullptr;
                rprMaterialSystemCreateNode(m_materialSystem, RPR_MATERIAL_NODE_ARITHMETIC, &selectZ);
                rprMaterialNodeSetInputN(selectZ, "color0", finalEmissiveValue);
                rprMaterialNodeSetInputU(selectZ, "op", RPR_MATERIAL_NODE_OP_SELECT_Z);

                rpr_material_node selectMax0 = nullptr;
                rprMaterialSystemCreateNode(m_materialSystem, RPR_MATERIAL_NODE_ARITHMETIC, &selectMax0);
                rprMaterialNodeSetInputN(selectMax0, "color0", selectX);
                rprMaterialNodeSetInputN(selectMax0, "color1", selectY);
                rprMaterialNodeSetInputU(selectMax0, "op", RPR_MATERIAL_NODE_OP_MAX);

                rpr_material_node selectMax1 = nullptr;
                rprMaterialSystemCreateNode(m_materialSystem, RPR_MATERIAL_NODE_ARITHMETIC, &selectMax1);
                rprMaterialNodeSetInputN(selectMax1, "color0", selectMax0);
                rprMaterialNodeSetInputN(selectMax1, "color1", selectZ);
                rprMaterialNodeSetInputU(selectMax1, "op", RPR_MATERIAL_NODE_OP_MAX);

                rprxMaterialSetParameterN(m_uberMatContext, uberMaterial, RPRX_UBER_MATERIAL_EMISSION_WEIGHT, selectMax1);
            }
        }

        void ApplyAlphaMode(rprx_material uberMaterial, rpr_material_node imageTexture, gltf::Material& material)
        {
            // Apply the specified alpha mode.
            switch (material.alphaMode)
            {
            case gltf::Material::AlphaMode::MASK:
            {
                // Create a series of arithmetic nodes to accomplish alpha mask.
                rpr_material_node alphaSelect = nullptr;
                rprMaterialSystemCreateNode(m_materialSystem, RPR_MATERIAL_NODE_ARITHMETIC, &alphaSelect);
                rprMaterialNodeSetInputN(alphaSelect, "color0", imageTexture);
                rprMaterialNodeSetInputU(alphaSelect, "op", RPR_MATERIAL_NODE_OP_SELECT_W);

                rpr_material_node add = nullptr;
                rprMaterialSystemCreateNode(m_materialSystem, RPR_MATERIAL_NODE_ARITHMETIC, &add);
                rprMaterialNodeSetInputN(alphaSelect, "color0", alphaSelect);
                rprMaterialNodeSetInputF(alphaSelect, "color1", 1.0f - material.alphaCutoff, 1.0f - material.alphaCutoff, 1.0f - material.alphaCutoff, 1.0f - material.alphaCutoff);
                rprMaterialNodeSetInputU(alphaSelect, "op", RPR_MATERIAL_NODE_OP_ADD);

                rpr_material_node cutoff = nullptr;
                rprMaterialSystemCreateNode(m_materialSystem, RPR_MATERIAL_NODE_ARITHMETIC, &cutoff);
                rprMaterialNodeSetInputN(alphaSelect, "color0", add);
                rprMaterialNodeSetInputU(alphaSelect, "op", RPR_MATERIAL_NODE_OP_FLOOR);

                // Plug cutoff node into transparency input slot.
                rprxMaterialSetParameterN(m_uberMatContext, uberMaterial, RPRX_UBER_MATERIAL_TRANSPARENCY, cutoff);
                break;
            }

            case gltf::Material::AlphaMode::BLEND:
            {
                // Create a series of arithmetic nodes to accomplish alpha blending.
                rpr_material_node alphaSelect = nullptr;
                rprMaterialSystemCreateNode(m_materialSystem, RPR_MATERIAL_NODE_ARITHMETIC, &alphaSelect);
                rprMaterialNodeSetInputN(alphaSelect, "color0", imageTexture);
                rprMaterialNodeSetInputU(alphaSelect, "op", RPR_MATERIAL_NODE_OP_SELECT_W);

                // Plug alpha select node into transparency input slot.
                rprxMaterialSetParameterN(m_uberMatContext, uberMaterial, RPRX_UBER_MATERIAL_TRANSPARENCY, alphaSelect);
                break;
            }
            }
        }

        rprx_material ImportPbrSpecularGlossiness(rpr_shape shape, gltf::Material& material, khr::KHR_Materials_PbrSpecularGlossiness& pbrSpecularGlossiness)
        {
            // Create a new Radeon ProRender uber material.
            rprx_material uberMaterial = nullptr;
            rpr_int result = rprxCreateMaterial(m_uberMatContext, RPRX_MATERIAL_UBER, &uberMaterial);
            if (result != RPR_SUCCESS)
                return nullptr;

            // Always use PBR reflection mode.
            rprxMaterialSetParameterU(m_uberMatContext, uberMaterial, RPRX_UBER_MATERIAL_REFLECTION_MODE, RPRX_UBER_MATERIAL_REFLECTION_MODE_PBR);

            // Apply base color and texture properties.
            auto& diffuseFactor = pbrSpecularGlossiness.diffuseFactor;
            rprxMaterialSetParameterF(m_uberMatContext, uberMaterial, RPRX_UBER_MATERIAL_DIFFUSE_COLOR, diffuseFactor[0], diffuseFactor[1], diffuseFactor[2], diffuseFactor[3]);
            auto diffuseImage = ImportImage(pbrSpecularGlossiness.diffuseTexture.index);
            if (diffuseImage)
            {
                // Create an IMAGE_TEXTURE material node.
                rpr_material_node imageTexture = nullptr;
                rprMaterialSystemCreateNode(m_materialSystem, RPR_MATERIAL_NODE_IMAGE_TEXTURE, &imageTexture);
                rprMaterialNodeSetInputImageData(imageTexture, "data", diffuseImage);
                rprxMaterialSetParameterN(m_uberMatContext, uberMaterial, RPRX_UBER_MATERIAL_DIFFUSE_COLOR, imageTexture);

                // Check alpha mode of material.
                ApplyAlphaMode(uberMaterial, imageTexture, material);
            }

            // Apply specular glossiness properties.
            auto& specularFactor = pbrSpecularGlossiness.specularFactor;
            float glossinessFactor = pbrSpecularGlossiness.glossinessFactor;                        
            rprxMaterialSetParameterF(m_uberMatContext, uberMaterial, RPRX_UBER_MATERIAL_REFLECTION_COLOR, specularFactor[0], specularFactor[1], specularFactor[2], 1.0f);
            rprxMaterialSetParameterF(m_uberMatContext, uberMaterial, RPRX_UBER_MATERIAL_REFLECTION_WEIGHT, 1.0f, 1.0f, 1.0f, 1.0f);
            rprxMaterialSetParameterF(m_uberMatContext, uberMaterial, RPRX_UBER_MATERIAL_REFLECTION_ROUGHNESS, 1.0f - glossinessFactor, 1.0f - glossinessFactor, 1.0f - glossinessFactor, 1.0f);

            rpr_image specularGlossinessImage = ImportImage(pbrSpecularGlossiness.specularGlossinessTexture.index);
            if (specularGlossinessImage)
            {
                // Create an IMAGE_TEXTURE material node.
                rpr_material_node imageTexture = nullptr;
                rprMaterialSystemCreateNode(m_materialSystem, RPR_MATERIAL_NODE_IMAGE_TEXTURE, &imageTexture);
                rprMaterialNodeSetInputImageData(imageTexture, "data", specularGlossinessImage);
                rprxMaterialSetParameterN(m_uberMatContext, uberMaterial, RPRX_UBER_MATERIAL_DIFFUSE_COLOR, imageTexture);

                // Use the RGB components of the specular glossiness texture as the specular color (NOTE: assuming alpha component is ignored here).
                rprxMaterialSetParameterN(m_uberMatContext, uberMaterial, RPRX_UBER_MATERIAL_REFLECTION_COLOR, imageTexture);

                // Select out the glossiness factor.
                rpr_material_node glossinessSelect = nullptr;
                rprMaterialSystemCreateNode(m_materialSystem, RPR_MATERIAL_NODE_ARITHMETIC, &glossinessSelect);
                rprMaterialNodeSetInputN(glossinessSelect, "color0", imageTexture);
                rprMaterialNodeSetInputU(glossinessSelect, "op", RPR_MATERIAL_NODE_OP_SELECT_W);

                // Invert the glossiness value (i.e. 1.0 - glossiness) to get reflection roughness.
                rpr_material_node invertdGlossiness = nullptr;
                rprMaterialSystemCreateNode(m_materialSystem, RPR_MATERIAL_NODE_ARITHMETIC, &invertdGlossiness);
                rprMaterialNodeSetInputF(invertdGlossiness, "color0", 1.0f, 1.0f, 1.0f, 1.0f);
                rprMaterialNodeSetInputN(invertdGlossiness, "color1", glossinessSelect);
                rprMaterialNodeSetInputU(invertdGlossiness, "op", RPR_MATERIAL_NODE_OP_SUB);

                // Set the final reflection roughness input value.
                rprxMaterialSetParameterN(m_uberMatContext, uberMaterial, RPRX_UBER_MATERIAL_REFLECTION_ROUGHNESS, invertdGlossiness);
            }

            // Apply normal map and emissive properties.
            ApplyNormalMapAndEmissive(uberMaterial, material);

            // Return the material handle.
            return uberMaterial;
        }

        rprx_material ImportPbrMetallicRoughness(rpr_shape shape, gltf::Material& material)
        {
            // Create a new Radeon ProRender uber material.
            rprx_material uberMaterial = nullptr;
            rpr_int result = rprxCreateMaterial(m_uberMatContext, RPRX_MATERIAL_UBER, &uberMaterial);
            if (result != RPR_SUCCESS)
                return nullptr;
            
            // Always use PBR reflection mode.
            rprxMaterialSetParameterU(m_uberMatContext, uberMaterial, RPRX_UBER_MATERIAL_REFLECTION_MODE, RPRX_UBER_MATERIAL_REFLECTION_MODE_METALNESS);

            // Apply base color and texture properties.
            auto& baseColorFactor = material.pbrMetallicRoughness.baseColorFactor;
            rprxMaterialSetParameterF(m_uberMatContext, uberMaterial, RPRX_UBER_MATERIAL_DIFFUSE_COLOR, baseColorFactor[0], baseColorFactor[1], baseColorFactor[2], baseColorFactor[3]);
            rprxMaterialSetParameterF(m_uberMatContext, uberMaterial, RPRX_UBER_MATERIAL_REFLECTION_COLOR, baseColorFactor[0], baseColorFactor[1], baseColorFactor[2], baseColorFactor[3]);
            rprxMaterialSetParameterF(m_uberMatContext, uberMaterial, RPRX_UBER_MATERIAL_REFLECTION_WEIGHT, 1.0f, 1.0f, 1.0f, 1.0f);

            auto baseColorImage = ImportImage(material.pbrMetallicRoughness.baseColorTexture.index);
            if (baseColorImage)
            {
                // Create an IMAGE_TEXTURE material node to reference the base texture.
                rpr_material_node imageTexture = nullptr;
                result = rprMaterialSystemCreateNode(m_materialSystem, RPR_MATERIAL_NODE_IMAGE_TEXTURE, &imageTexture);
                result = rprMaterialNodeSetInputImageData(imageTexture, "data", baseColorImage);

                rprxMaterialSetParameterN(m_uberMatContext, uberMaterial, RPRX_UBER_MATERIAL_DIFFUSE_COLOR, imageTexture);
                rprxMaterialSetParameterN(m_uberMatContext, uberMaterial, RPRX_UBER_MATERIAL_REFLECTION_COLOR, imageTexture);

                // Apply alpha mode of material.
                ApplyAlphaMode(uberMaterial, imageTexture, material);
            }

            // Apply metallic roughness properties.
            float metallicFactor = material.pbrMetallicRoughness.metallicFactor;
            float invMetallicFactor = 1.0f - metallicFactor;
            float roughnessFactor = material.pbrMetallicRoughness.roughnessFactor;            
            rprxMaterialSetParameterF(m_uberMatContext, uberMaterial, RPRX_UBER_MATERIAL_REFLECTION_METALNESS, metallicFactor, metallicFactor, metallicFactor, 1.0f);
            rprxMaterialSetParameterF(m_uberMatContext, uberMaterial, RPRX_UBER_MATERIAL_REFLECTION_ROUGHNESS, roughnessFactor, roughnessFactor, roughnessFactor, 1.0f);
            
            auto metallicRoughnessImage = ImportImage(material.pbrMetallicRoughness.metallicRoughnessTexture.index);
            if (metallicRoughnessImage)
            {
                // Create an IMAGE_TEXTURE material node.
                rpr_material_node imageTexture = nullptr;
                rprMaterialSystemCreateNode(m_materialSystem, RPR_MATERIAL_NODE_IMAGE_TEXTURE, &imageTexture);
                rprMaterialNodeSetInputImageData(imageTexture, "data", metallicRoughnessImage);

                // Select out the metallic and roughness components
                rpr_material_node metallicSelect = nullptr;
                rprMaterialSystemCreateNode(m_materialSystem, RPR_MATERIAL_NODE_ARITHMETIC, &metallicSelect);
                rprMaterialNodeSetInputN(metallicSelect, "color0", imageTexture);
                rprMaterialNodeSetInputU(metallicSelect, "op", RPR_MATERIAL_NODE_OP_SELECT_Z);

                rpr_material_node roughnessSelect = nullptr;
                rprMaterialSystemCreateNode(m_materialSystem, RPR_MATERIAL_NODE_ARITHMETIC, &roughnessSelect);
                rprMaterialNodeSetInputN(roughnessSelect, "color0", imageTexture);
                rprMaterialNodeSetInputU(roughnessSelect, "op", RPR_MATERIAL_NODE_OP_SELECT_Y);

                rprxMaterialSetParameterN(m_uberMatContext, uberMaterial, RPRX_UBER_MATERIAL_REFLECTION_METALNESS, metallicSelect);
                rprxMaterialSetParameterN(m_uberMatContext, uberMaterial, RPRX_UBER_MATERIAL_REFLECTION_ROUGHNESS, roughnessSelect);
            }
      
            // Apply normal map and emissive properties.
            ApplyNormalMapAndEmissive(uberMaterial, material);

            // Return the material handle.
            return uberMaterial;
        }

        void ImportMaterial(rpr_shape shape, int materialIndex)
        {
            // Validate the index.
            if (materialIndex < 0 || materialIndex >= m_gltf->materials.size())
                return;
            
            // See if the material index is in the cache.
            if (m_materialCache.find(materialIndex) != m_materialCache.end())
            {
                // Assign the material to the shape.
                if (shape)
                {
                    auto& pair = m_materialCache.at(materialIndex);
                    if (std::get<0>(pair) == amd::Node::Type::UBER)
                    {
                        rprx_material handle = reinterpret_cast<rprx_material>(std::get<1>(pair));
                        rprxShapeAttachMaterial(m_uberMatContext, shape, handle);
                        rprxMaterialCommit(m_uberMatContext, handle);
                    }
                    else
                    {
                        rpr_material_node handle = reinterpret_cast<rpr_material_node>(std::get<1>(pair));
                        rprShapeSetMaterial(shape, handle);
                    }
                }

                return;
            }

            // Get the GLTF material at the specified index.
            auto& material = m_gltf->materials[materialIndex];

            // Check for the AMD_RPR_uber_material extension.
            amd::AMD_RPR_Material ext;
            if (ImportExtension(material.extensions, ext))
            {
                // Check to see if any nodes are present.
                if (ext.nodes.size() > 0)
                {
                    // The very first node is always the root node.
                    auto& rootNode = ext.nodes[0];

                    // Handle uber materials and Radeon ProRender material nodes separately.
                    if (rootNode.type == amd::Node::Type::UBER)
                    {
                        // List of all uber material parameters to import.
                        const std::unordered_map<std::string, rprx_parameter> uberMatParams = {
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

                        // Create a new Radeon ProRender uber material.
                        rprx_material uberMaterial = nullptr;
                        rpr_int result = rprxCreateMaterial(m_uberMatContext, RPRX_MATERIAL_UBER, &uberMaterial);
                        if (result != RPR_SUCCESS)
                            return;

                        // Import all of the inputs.
                        for (auto& input : rootNode.inputs)
                        {
                            rprx_parameter param = uberMatParams.at(input.name);

                            switch (input.type)
                            {
                            case amd::Input::Type::FLOAT4:
                                rprxMaterialSetParameterF(m_uberMatContext, uberMaterial, param, input.value.array[0], input.value.array[1], input.value.array[2], input.value.array[3]);
                                break;

                            case amd::Input::Type::UINT:
                                rprxMaterialSetParameterU(m_uberMatContext, uberMaterial, param, input.value.integer);
                                break;

                            case amd::Input::Type::NODE:
                            {
                                // Check bounds of node reference.
                                if (input.value.integer >= 0 && input.value.integer < ext.nodes.size())
                                {
                                    // Import the material node and apply to uber material parameter.
                                    auto materialNode = ImportMaterialNode(ext.nodes, input.value.integer);
                                    if (materialNode)
                                        rprxMaterialSetParameterN(m_uberMatContext, uberMaterial, param, materialNode);
                                }
                                break;
                            }

                            case amd::Input::Type::IMAGE:
                            {
                                // Import the referenced image.
                                auto image = ImportImage(input.value.integer);
                                if (image)
                                {
                                    // Create a IMAGE_TEXTURE node first.
                                    rpr_material_node textureNode = nullptr;
                                    rprMaterialSystemCreateNode(m_materialSystem, RPR_MATERIAL_NODE_IMAGE_TEXTURE, &textureNode);

                                    // Apply image to texture node.
                                    rprMaterialNodeSetInputImageData(textureNode, "data", image);

                                    // Apply texture node to uber material input slot.
                                    rprxMaterialSetParameterN(m_uberMatContext, uberMaterial, param, textureNode);
                                }
                                break;
                            }
                            }
                        }

                        // Apply the uber material to the shape.
                        rprxShapeAttachMaterial(m_uberMatContext, shape, uberMaterial);

                        // Commit changes to uber material.
                        rprxMaterialCommit(m_uberMatContext, uberMaterial);

                        // Add the material to the cache.
                        m_materialCache.emplace(materialIndex, std::make_pair(amd::Node::Type::UBER, reinterpret_cast<void*>(uberMaterial)));
                    }
                    else
                    {
                        // Recursively import the material node and any child nodes.
                        rpr_material_node materialNode = ImportMaterialNode(ext.nodes, 0 /* Root node index. */);

                        // Apply the material to the shape.
                        rpr_int result = rprShapeSetMaterial(shape, materialNode);
                        if (result != RPR_SUCCESS)
                            return;

                        // Add the material to the cache.
                        m_materialCache.emplace(materialIndex, std::make_pair(rootNode.type, reinterpret_cast<void*>(materialNode)));
                    }
                }
            }
            // Use GLTF's material model.
            else
            {
                // If KHR_materials_pbrSpecularGlossiness is present, it overrides the base pbrMetallicRoughness properties.
                rprx_material uberMaterial = nullptr;
                khr::KHR_Materials_PbrSpecularGlossiness ext;
                if (khr::ImportExtension(material.extensions, ext)) uberMaterial = ImportPbrSpecularGlossiness(shape, material, ext);
                else uberMaterial = ImportPbrMetallicRoughness(shape, material);

                // Check to see if material creation was successful.
                if (uberMaterial)
                {
                    // Apply the uber material to the shape.
                    rprxShapeAttachMaterial(m_uberMatContext, shape, uberMaterial);

                    // Commit changes to uber material.
                    rprxMaterialCommit(m_uberMatContext, uberMaterial);

                    // Add the material to the cache.
                    m_materialCache.emplace(materialIndex, std::make_pair(amd::Node::Type::UBER, reinterpret_cast<void*>(uberMaterial)));
                }
            }
        }

        void ImportShapeParameters(nlohmann::json& extras, rpr_shape shape)
        {
            // See if Radeon ProRender specific shape parameters are present.
            if (extras.find("rpr.shape.parameters") != extras.end())
            {
                // Import and set all of the parameters.
                auto& shapeParams = extras["rpr.shape.parameters"];
                for (auto itr = shapeParams.begin(); itr != shapeParams.end(); ++itr)
                {
                    if (itr.key() == "visibility")
                        rprShapeSetVisibility(shape, itr.value().get<rpr_bool>());
                    else if (itr.key() == "shadow")
                        rprShapeSetShadow(shape, itr.value().get<rpr_bool>());
                    else if (itr.key() == "visibilityPrimaryOnly")
                        rprShapeSetVisibilityPrimaryOnly(shape, itr.value().get<rpr_bool>());
                    else if (itr.key() == "visibilityInSpecular")
                        rprShapeSetVisibilityInSpecular(shape, itr.value().get<rpr_bool>());
                    else if (itr.key() == "shadowCatcher")
                        rprShapeSetShadowCatcher(shape, itr.value().get<rpr_bool>());
                    else if (itr.key() == "angularMotion")
                    {
                        std::array<float, 4> value = itr.value().get<decltype(value)>();
                        rprShapeSetAngularMotion(shape, value[0], value[1], value[2], value[3]);
                    }
                    else if (itr.key() == "linearMotion")
                    {
                        std::array<float, 3> value = itr.value().get<decltype(value)>();
                        rprShapeSetLinearMotion(shape, value[0], value[1], value[2]);
                    }
                    else if (itr.key() == "subdivisionFactor")
                        rprShapeSetSubdivisionFactor(shape, itr.value().get<rpr_uint>());
                    else if (itr.key() == "subdivisionCreaseWeight")
                        rprShapeSetSubdivisionCreaseWeight(shape, itr.value().get<float>());
                    else if (itr.key() == "subdivisionBoundaryInterop")
                        rprShapeSetSubdivisionBoundaryInterop(shape, itr.value().get<rpr_uint>());
                    else if (itr.key() == "displacementScale")
                    {
                        std::array<float, 2> value = itr.value().get<decltype(value)>();
                        rprShapeSetDisplacementScale(shape, value[0], value[1]);
                    }
                    else if (itr.key() == "objectGroupID")
                        rprShapeSetObjectGroupID(shape, itr.value().get<rpr_uint>());
                }
            }
        }

        rpr_shape ImportMesh(rpr_scene rprScene, int nodeIndex, const mat4& worldTransform)
        {
            // Validate the index.
            if (nodeIndex < 0 || nodeIndex >= m_gltf->nodes.size())
                return nullptr;

            // See if the node index is in the cache.
            if (m_shapeCache.find(nodeIndex) != m_shapeCache.end())
                return m_shapeCache.at(nodeIndex);

            // Get the GLTF node at the specified index.
            auto& node = m_gltf->nodes[nodeIndex];

            // Get the node's GLTF mesh.
            auto& mesh = m_gltf->meshes[node.mesh];

            // Iterate over all of the mesh's primitives.
            std::vector<rpr_shape> rprShapesList;
            for (auto& primitive : mesh.primitives)
            {
                // If accessors for index and vertex attributes are interleaved, leave them interleaved.
                // In order to accomplish this, all accessors must be iterated over to determine which share common buffers.
                std::unordered_map<std::string, int> allMeshPrimitiveAttributes = { { "", primitive.indices } };
                for (auto& entry : primitive.attributes)
                    allMeshPrimitiveAttributes.emplace(entry.first, entry.second);

                auto interleavedAttributes = GetInterleavedAttributeData(allMeshPrimitiveAttributes);

                // Get the accessor referencing the indice data.
                auto& indiceAccessor = m_gltf->accessors[primitive.indices];

                // Get the bufferView referenced by the accessor.
                auto& indexBufferView = m_gltf->bufferViews[indiceAccessor.bufferView];
                
                // Get the memory address of the indices from the interleaved attributes result.
                char* indices = interleavedAttributes.attributeMemoryAddresses.at("");

                // Indice stride will always be 4 bytes for Radeon ProRender.
                int indicesStride = sizeof(int);

                // Indices must be in 32-bit integer format, convert if necessary.
                if (indiceAccessor.componentType != gltf::Accessor::ComponentType::UNSIGNED_INT)
                {
                    // Check byteStride for index bufferView.
                    int byteStride = indexBufferView.byteStride;
                    if (byteStride == 0)
                    {
                        switch (indiceAccessor.componentType)
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
                    size_t typeSize = GetAccessorTypeSize(indiceAccessor);
                    int* intIndices = new int[indiceAccessor.count];
                    for (auto i = 0; i < indiceAccessor.count; ++i)
                    {
                        switch (indiceAccessor.componentType)
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
                rpr_int numVertices = 0, numNormals = 0, numTexcoords0 = 0;
                rpr_int vertexStride = 0, normalStride = 0, texcoords0Stride = 0;

                // Define a convenience lambda function for extracting the data for each attribute.
                auto extractAttributeData = [&](const std::string& attributeName, float*& data, rpr_int& count, rpr_int& stride) -> void
                {
                    // Look up the attribute name in the primitive's list of attributes.
                    auto itr = primitive.attributes.find(attributeName);
                    if (itr != primitive.attributes.end())
                    {
                        // Get the referenced gltf Accessor.
                        auto& accessor = m_gltf->accessors[itr->second];

                        // Get the bufferView referenced by the accessor.
                        auto& bufferView = m_gltf->bufferViews[accessor.bufferView];

                        // Copy the element count and stride.
                        count = accessor.count;
                        stride = bufferView.byteStride;

                        // If stride is 0, data is tightly packed.  So manually determine stride.                        
                        if (stride == 0)
                            stride = static_cast<rpr_int>(GetAccessorTypeSize(accessor));
 
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
                extractAttributeData(gltf::AttributeNames::POSITION, vertices, numVertices, vertexStride);
                extractAttributeData(gltf::AttributeNames::NORMAL, normals, numNormals, normalStride);
                extractAttributeData(gltf::AttributeNames::TEXCOORD_0, texcoords0, numTexcoords0, texcoords0Stride);               

                // Create a faces array that's required by Radeon ProRender.
                std::vector<rpr_int> numFaces(indiceAccessor.count / 3, 3);

                // Create the Radeon ProRender shape.
                rpr_shape rprShape = nullptr;
                rpr_int status = rprContextCreateMesh(m_context, vertices, numVertices, vertexStride,
                    normals, numNormals, normalStride,
                    texcoords0, numTexcoords0, texcoords0Stride,
                    reinterpret_cast<rpr_int*>(indices), indicesStride,
                    reinterpret_cast<rpr_int*>(indices), indicesStride,
                    reinterpret_cast<rpr_int*>(indices), indicesStride,
                    numFaces.data(), static_cast<rpr_int>(numFaces.size()), &rprShape);
                if (status == RPR_SUCCESS)
                {
                    // Set all of the shape parameters.
                    ImportShapeParameters(node.extras, rprShape);

                    // Set the shape transform.
                    mat4 nodeTransform = GetNodeTransformMatrix(node, worldTransform);
                    rprShapeSetTransform(rprShape, RPR_FALSE, &nodeTransform.m[0][0]);

                    // Import and set the shape's material.
                    ImportMaterial(rprShape, primitive.material);

                    // Attach the shape to the scene.
                    rprSceneAttachShape(rprScene, rprShape);

                    // Cache the shape by node index.
                    m_shapeCache.emplace(nodeIndex, rprShape);
                }

                // Destroy the interleaved attributes result.
                DestroyInterleavedAttributesResult(interleavedAttributes);

                // Add the shape to the return result.
                if (rprShape)
                    rprShapesList.push_back(rprShape);
            }

            // Return the first shape in the list.  For Radeon ProRender exports there will only ever be a single shape per Node and Mesh.
            if (rprShapesList.size() > 0) return rprShapesList[0];
            else return nullptr;
        }

        void ImportCamera(rpr_scene rprScene, gltf::Node& node, const mat4& worldTransform)
        {
            // Check the camera index.
            if (node.camera < 0 || node.camera >= m_gltf->cameras.size())
                return;

            // Lookup the gltf camera.
            auto& camera = m_gltf->cameras[node.camera];

            // Create a new Radeon ProRender camera.
            rpr_camera rprCamera = nullptr;
            rprContextCreateCamera(m_context, &rprCamera);

            // Check for AMD_RPR_camera extension.
            amd::AMD_RPR_Camera ext;
            if (ImportExtension(camera.extensions, ext))
            {
                // Import camera parameters.
                rprCameraSetMode(rprCamera, static_cast<rpr_camera_mode>(ext.cameraMode));
                rprCameraSetApertureBlades(rprCamera, ext.apertureBlades);
                rprCameraSetExposure(rprCamera, ext.exposure);
                rprCameraSetFocusDistance(rprCamera, ext.focusDistance);
                rprCameraSetFocalLength(rprCamera, ext.focalLength);
                rprCameraSetFocalTilt(rprCamera, ext.focalTilt);
                rprCameraSetFStop(rprCamera, ext.fstop);
                rprCameraSetIPD(rprCamera, ext.ipd);
                rprCameraSetLensShift(rprCamera, ext.lensShift[0], ext.lensShift[1]);
                rprCameraSetOrthoWidth(rprCamera, ext.orthoWidth);
                rprCameraSetOrthoHeight(rprCamera, ext.orthoHeight);
                rprCameraSetSensorSize(rprCamera, ext.sensorSize[0], ext.sensorSize[1]);
                rprCameraSetTiltCorrection(rprCamera, ext.tiltCorrection[0], ext.tiltCorrection[1]);
                rprCameraLookAt(rprCamera, ext.position[0], ext.position[1], ext.position[2], ext.lookAt[0], ext.lookAt[1], ext.lookAt[2], ext.up[0], ext.up[1], ext.up[2]);
            }
            else if (camera.type == gltf::Camera::Type::PERSPECTIVE)
            {
                // Set the camera mode.
                rpr_int status = rprCameraSetMode(rprCamera, RPR_CAMERA_MODE_PERSPECTIVE);

                // Set the camera transform.
                status = rprCameraSetTransform(rprCamera, RPR_FALSE, node.matrix.data());

                // Get the Radeon ProRender camera's default sensor size.
                float sensorSize[2];
                status = rprCameraGetInfo(rprCamera, RPR_CAMERA_SENSOR_SIZE, sizeof(float) * 2, sensorSize, nullptr);

                // Convert gltf camera's vertical field of view to an appropriate focal length.
                float focalLength = 0.5f * sensorSize[1] / tan(camera.perspective.yfov * 0.5f);
                status = rprCameraSetFocalLength(rprCamera, focalLength);
            }
            else if (camera.type == gltf::Camera::Type::ORTHOGRAPHIC)
            {
                // Set the camera mode.
                rprCameraSetMode(rprCamera, FR_CAMERA_MODE_ORTHOGRAPHIC);

                // Set the camera transform.
                rprCameraSetTransform(rprCamera, RPR_FALSE, node.matrix.data());

                // Use the gltf camera's xmag and ymag as Radeon ProRender ortho width and height values.
                rprCameraSetOrthoWidth(rprCamera, camera.orthographic.xmag);
                rprCameraSetOrthoHeight(rprCamera, camera.orthographic.ymag);
            }

            // Attach the camera to the scene.
            rpr_int status = rprSceneSetCamera(rprScene, rprCamera);
        }

        void ImportNode(gltf::Scene& scene, rpr_scene rprScene, int nodeIndex, const mat4& worldTransform)
        {
            // Validate node index.
            if (nodeIndex < 0 || nodeIndex >= m_gltf->nodes.size())
                return;

            // If node has already been visited then exit.
            if (m_visitedNodes.find(nodeIndex) != m_visitedNodes.end())
            {
                std::cout << "ImportNode " << nodeIndex << " already visited\n";
                return;
            }

            // Get the referenced node.
            auto& node = m_gltf->nodes[nodeIndex];

            // Mark the node as visited.
            m_visitedNodes.insert(nodeIndex);

            // Check to see if node is a camera.
            if (node.camera != -1)
            {
                ImportCamera(rprScene, node, worldTransform);
            }
            // Check to see if node is a mesh.
            else if (node.mesh >= 0 && node.mesh < m_gltf->meshes.size())
            {
                // Determine if node is a Radeon ProRender instance.
                bool isRprInstance = false;
                if (node.extras.find("rpr.shape.parameters") != node.extras.end())
                {
                    auto& shapeParams = node.extras["rpr.shape.parameters"];
                    if (shapeParams.find("parentShapeID") != shapeParams.end())
                    {
                        // Since this is a Radeon ProRender instance, only a single rpr_shape was created by the parent Node.
                        mat4 identity;
                        auto rprParentShape = ImportMesh(rprScene, shapeParams.at("parentShapeID").get<int>(), identity);
                        if (rprParentShape)
                        {
                            // Mark that the node was a Radeon ProRender instance.
                            isRprInstance = true;

                            // Create a new Radeon ProRender instance.
                            rpr_shape rprInstance = nullptr;
                            rpr_int result = rprContextCreateInstance(m_context, rprParentShape, &rprInstance);
                            if (result == RPR_SUCCESS)
                            {
                                // Set all of the shape parameters.
                                ImportShapeParameters(node.extras, rprInstance);

                                // Calculate the node's transform.
                                mat4 nodeTransform = GetNodeTransformMatrix(node, worldTransform);

                                // Set the shape transform.
                                rprShapeSetTransform(rprInstance, RPR_FALSE, &nodeTransform.m[0][0]);

                                // Set the shape material.
                                if (node.mesh > 0 && node.mesh < m_gltf->meshes.size())
                                    if (m_gltf->meshes[node.mesh].primitives.size() == 1)
                                        ImportMaterial(rprInstance, m_gltf->meshes[node.mesh].primitives[0].material);

                                // Attach the shape to the scene.
                                rprSceneAttachShape(rprScene, rprInstance);

                                // Cache the shape by node index.
                                m_shapeCache.emplace(nodeIndex, rprInstance);
                            }
                        }
                    }
                }

                // Else load it as a regular mesh.
                if (!isRprInstance)
                {
                    ImportMesh(rprScene, nodeIndex, worldTransform);
                }
            }

            // Calculate the node's transform to pass to children as new world transform.
            mat4 nodeTransform = GetNodeTransformMatrix(node, worldTransform);

            // Import the node's children.
            for (auto& childNodeIndex : node.children)
            {
                ImportNode(scene, rprScene, childNodeIndex, nodeTransform);
            }
        }

        void ImportSceneLights(gltf::Scene& scene, rpr_scene rprScene)
        {
            // Check for the AMD_RPR_lights extension.
            amd::AMD_RPR_Lights ext;
            if (amd::ImportExtension(scene.extensions, ext))
            {
                // Iterate over all of the lights.
                for (auto& light : ext.lights)
                {
                    // Handle all light types.
                    rpr_light rprLight = nullptr;
                    switch (light.type)
                    {
                    case amd::Light::Type::POINT:
                        rprContextCreatePointLight(m_context, &rprLight);
                        rprPointLightSetRadiantPower3f(rprLight, light.point.radiantPower[0], light.point.radiantPower[1], light.point.radiantPower[2]);
                        break;

                    case amd::Light::Type::DIRECTIONAL:
                        rprContextCreateDirectionalLight(m_context, &rprLight);
                        rprDirectionalLightSetRadiantPower3f(rprLight, light.directional.radiantPower[0], light.directional.radiantPower[1], light.directional.radiantPower[2]);
                        rprDirectionalLightSetShadowSoftness(rprLight, light.directional.shadowSoftness);
                        break;

                    case amd::Light::Type::SPOT:
                        rprContextCreateSpotLight(m_context, &rprLight);
                        rprSpotLightSetRadiantPower3f(rprLight, light.spot.radiantPower[0], light.spot.radiantPower[1], light.spot.radiantPower[2]);
                        rprSpotLightSetConeShape(rprLight, light.spot.innerAngle, light.spot.outerAngle);
                        break;

                    case amd::Light::Type::ENVIRONMENT:
                        rprContextCreateEnvironmentLight(m_context, &rprLight);
                        rprEnvironmentLightSetIntensityScale(rprLight, light.environment.intensityScale);
                        if (light.environment.image != -1)
                        {
                            rpr_image img = ImportImage(light.environment.image);
                            if (img) rprEnvironmentLightSetImage(rprLight, img);
                        }
                        break;

                    case amd::Light::Type::SKY:
                        rprContextCreateSkyLight(m_context, &rprLight);
                        rprSkyLightSetTurbidity(rprLight, light.sky.turbidity);
                        rprSkyLightSetAlbedo(rprLight, light.sky.albedo);
                        rprSkyLightSetScale(rprLight, light.sky.scale);
                        break;

                    case amd::Light::Type::IES:
                        rprContextCreateIESLight(m_context, &rprLight);
                        rprIESLightSetRadiantPower3f(rprLight, light.ies.radiantPower[0], light.ies.radiantPower[1], light.ies.radiantPower[2]);
                        if (light.ies.buffer != -1)
                        {
                            auto stream = ImportBuffer(light.ies.buffer);
                            if (stream)
                            {
                                // Calculate the size of the data stream.
                                stream->seekg(0, std::ios::end);
                                auto numBytes = stream->tellg();
                                stream->seekg(0);

                                // Allocate memory and read the data.
                                char* data = new char[numBytes];
                                stream->read(data, numBytes);

                                // Pass the data off to Radeon ProRender.
                                rprIESLightSetImageFromIESdata(rprLight, data, light.ies.nx, light.ies.ny);

                                // Free the memory.
                                delete[] data;

                                // Delete stream.
                                delete stream;
                            }
                        }
                        break;
                    }

                    // Set the light transform.
                    rprLightSetTransform(rprLight, RPR_FALSE, light.transform.data());

                    // Attach the light to the scene.
                    rprSceneAttachLight(rprScene, rprLight);
                }
            }
        }

        void ImportSceneParameters(gltf::Scene& scene, rpr_scene rprScene)
        {
            if (scene.extras.find("rpr.scene.parameters") != scene.extras.end())
            {
                // Check for background image.
                auto& sceneParams = scene.extras["rpr.scene.parameters"];
                if (sceneParams.find("backgroundImage") != sceneParams.end())
                {
                    int imageIndex = sceneParams["backgroundImage"].get<int>();
                    if (imageIndex != -1)
                    {
                        rpr_image img = ImportImage(imageIndex);
                        if (img) rprSceneSetBackgroundImage(rprScene, img);
                    }
                }
            }
        }

        rpr_scene ImportScene(gltf::Scene& scene)
        {
            // Create a new Radeon ProRender scene.
            rpr_scene rprScene = nullptr;
            rpr_int result = rprContextCreateScene(m_context, &rprScene);
            if (result == RPR_SUCCESS)
            {
                // Set the scene name.
                rprObjectSetName(rprScene, scene.name.c_str());

                // Import scene parameters.
                ImportSceneParameters(scene, rprScene);

                // Import scene lights (i.e. AMD_RPR_Lights extension).
                ImportSceneLights(scene, rprScene);

                // Import all of the scene's nodes.
                for (auto& nodeIndex : scene.nodes)
                {
                    mat4 identity;
                    ImportNode(scene, rprScene, nodeIndex, identity);
                }
            }

            return rprScene;
        }

        void ImportScenes(std::vector<rpr_scene>& scenes)
        {
            // Iterate over all scene entries.
            for (auto& scene : m_gltf->scenes)
            {
                // Import gltf scene into Radeon ProRender.
                auto rprScene = ImportScene(scene);
                if (rprScene)
                    scenes.push_back(rprScene);
            }

            // Set the current scene based on the GLTF object scene index.
            if (m_gltf->scene < scenes.size())
                rprContextSetScene(m_context, scenes[m_gltf->scene]);
        }

        void ImportPostEffects()
        {
            // Import post effects from GLTF object's extensions.
            amd::AMD_RPR_Post_Effects ext;
            if (amd::ImportExtension(m_gltf->extensions, ext))
            {
                // Iterate over all post effect entries.
                for (auto& postEffect : ext.postEffects)
                {
                    // Create Radeon ProRender post effects from each entry.
                    rpr_post_effect rprPostEffect = nullptr;
                    rpr_int result = rprContextCreatePostEffect(m_context, static_cast<rpr_post_effect_type>(postEffect.type), &rprPostEffect);
                    if (result == RPR_SUCCESS)
                    {
                        switch (postEffect.type)
                        {
                        case amd::PostEffect::Type::WHITE_BALANCE:
                            rprPostEffectSetParameter1u(rprPostEffect, "colorspace", static_cast<rpr_uint>(postEffect.whiteBalance.colorSpace));
                            rprPostEffectSetParameter1f(rprPostEffect, "colortemp", postEffect.whiteBalance.colorTemperature);
                            break;

                        case amd::PostEffect::Type::SIMPLE_TONEMAP:
                            rprPostEffectSetParameter1f(rprPostEffect, "exposure", postEffect.simpleTonemap.exposure);
                            rprPostEffectSetParameter1f(rprPostEffect, "contrast", postEffect.simpleTonemap.contrast);
                            rprPostEffectSetParameter1u(rprPostEffect, "tonemap", postEffect.simpleTonemap.enableTonemap);
                            break;
                        }

                        // Attach the post effect.
                        rprContextAttachPostEffect(m_context, rprPostEffect);
                    }
                }
            }
        }

        void ImportContextParameters()
        {
            // Check to see if there's any content in "extras" parameter.
            if (m_gltf->extras.find("rpr.m_context.parameters") != m_gltf->extras.end())
            {
                // First build a lookup table of all Radeon ProRender m_context parameter names and their types.
                // Then match the name strings found in the gltf file to this lookup table in order to retrieve the parameter's index and type more easily.
                //! This should be auto-generated by parsing the Radeon ProRender XML files
                std::unordered_map<std::string, rpr_parameter_type> lookupTable;
                size_t paramCount = 0;
                rpr_int status = rprContextGetInfo(m_context, RPR_CONTEXT_PARAMETER_COUNT, sizeof(paramCount), &paramCount, nullptr);
                if (status == RPR_SUCCESS)
                {
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

                        // Store in the lookup table.
                        lookupTable.emplace(name.c_str(), type);
                    }
                }

                // Populate Radeon ProRender m_context with any parameters present.
                auto& contextParams = m_gltf->extras["rpr.m_context.parameters"];
                for (auto itr = contextParams.begin(); itr != contextParams.end(); ++itr)
                {
                    // Look up Radeon ProRender parameter type based on name key.
                    auto& item = lookupTable.find(itr.key());
                    if (item != lookupTable.end())
                    {
                        rpr_parameter_type type = item->second;
                        switch (type)
                        {
                        case RPR_PARAMETER_TYPE_UINT:
                        {
                            auto value = itr.value().get<uint32_t>();
                            rprContextSetParameter1u(m_context, itr.key().c_str(), value);
                            break;
                        }
                        case RPR_PARAMETER_TYPE_FLOAT:
                        {
                            auto value = itr.value().get<float>();
                            rprContextSetParameter1f(m_context, itr.key().c_str(), value);
                            break;
                        }
                        case RPR_PARAMETER_TYPE_FLOAT3:
                        {
                            std::vector<float> values = itr.value().get<std::vector<float>>();
                            rprContextSetParameter3f(m_context, itr.key().c_str(), values[0], values[1], values[2]);
                            break;
                        }
                        case RPR_PARAMETER_TYPE_FLOAT4:
                        {
                            std::vector<float> values = itr.value().get<std::vector<float>>();
                            rprContextSetParameter4f(m_context, itr.key().c_str(), values[0], values[1], values[2], values[3]);
                            break;
                        }
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
        std::unordered_map<int, std::istream*> m_bufferCache;
        std::unordered_map<int, rpr_image> m_imageCache;
        std::unordered_map<int, std::pair<amd::Node::Type, void*>> m_materialCache;
        std::unordered_map<int, rpr_shape> m_shapeCache;
        std::unordered_set<int> m_visitedNodes;
    };

    /*bool ImportFromGLTF(gltf::glTF& gltf, const std::string& filename, rpr_context context, rpr_material_system materialSystem, rprx_context uberMatContext, std::vector<rpr_scene>& scenes)
    {
        Importer importer(context, materialSystem, uberMatContext);
        return importer.Load(gltf, filename, scenes);
    }*/    

    bool ImportMaterialsFromGLTF(gltf::glTF& gltf, const std::string& filename, rpr_context context, rpr_material_system materialSystem, rprx_context uberMatContext, std::vector<rpr_material_node>& materialNodes, std::vector<rprx_material>& uberMaterials)
    {
        Importer importer(context, materialSystem, uberMatContext);
        return importer.Load(gltf, filename, materialNodes, uberMaterials);
    }
}

bool rprImportFromGLTF(const char* filename, rpr_context context, rpr_material_system materialSystem, rprx_context uberMatContext, rpr_scene* scene)
{
    gltf::glTF gltf;
    if (!gltf::Import(filename, gltf))
        return false;

    rpr::Importer importer(context, materialSystem, uberMatContext);
    return importer.Load(gltf, filename, scene);
}