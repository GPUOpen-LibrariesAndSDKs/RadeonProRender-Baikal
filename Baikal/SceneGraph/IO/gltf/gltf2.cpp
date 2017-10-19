#pragma once
#include <fstream>
#include "gltf2.h"

namespace gltf
{
    void from_json(const nlohmann::json& json, glTFProperty& object)
    {
    	if (json.find("extensions") != json.end()) object.extensions = json.at("extensions").get<decltype(object.extensions)>();
    	if (json.find("extras") != json.end()) object.extras = json.at("extras").get<decltype(object.extras)>();
    }
    
    void to_json(nlohmann::json& json, const glTFProperty& object)
    {
    	if (object.extensions.size() > 0) json.emplace("extensions", object.extensions);
    	if (object.extras.size() > 0) json.emplace("extras", object.extras);
    }
    
    void from_json(const nlohmann::json& json, Accessor_Sparse_Indices& object)
    {
    	if (json.find("bufferView") != json.end()) object.bufferView = json.at("bufferView").get<decltype(object.bufferView)>();
    	if (json.find("byteOffset") != json.end()) object.byteOffset = json.at("byteOffset").get<decltype(object.byteOffset)>();
    	if (json.find("componentType") != json.end()) object.componentType = json.at("componentType").get<decltype(object.componentType)>();
    	if (json.find("extensions") != json.end()) object.extensions = json.at("extensions").get<decltype(object.extensions)>();
    	if (json.find("extras") != json.end()) object.extras = json.at("extras").get<decltype(object.extras)>();
    }
    
    void to_json(nlohmann::json& json, const Accessor_Sparse_Indices& object)
    {
    	if (object.bufferView != -1) json.emplace("bufferView", object.bufferView);
    	json.emplace("byteOffset", object.byteOffset);
    	json.emplace("componentType", object.componentType);
    	if (object.extensions.size() > 0) json.emplace("extensions", object.extensions);
    	if (object.extras.size() > 0) json.emplace("extras", object.extras);
    }
    
    void from_json(const nlohmann::json& json, Accessor_Sparse_Values& object)
    {
    	if (json.find("bufferView") != json.end()) object.bufferView = json.at("bufferView").get<decltype(object.bufferView)>();
    	if (json.find("byteOffset") != json.end()) object.byteOffset = json.at("byteOffset").get<decltype(object.byteOffset)>();
    	if (json.find("extensions") != json.end()) object.extensions = json.at("extensions").get<decltype(object.extensions)>();
    	if (json.find("extras") != json.end()) object.extras = json.at("extras").get<decltype(object.extras)>();
    }
    
    void to_json(nlohmann::json& json, const Accessor_Sparse_Values& object)
    {
    	if (object.bufferView != -1) json.emplace("bufferView", object.bufferView);
    	json.emplace("byteOffset", object.byteOffset);
    	if (object.extensions.size() > 0) json.emplace("extensions", object.extensions);
    	if (object.extras.size() > 0) json.emplace("extras", object.extras);
    }
    
    void from_json(const nlohmann::json& json, Animation_Channel_Target& object)
    {
    	if (json.find("node") != json.end()) object.node = json.at("node").get<decltype(object.node)>();
    	const std::unordered_map<std::string, int> pathFromString = { { "translation", 0 } , { "rotation", 1 } , { "scale", 2 } , { "weights", 3 }  };
    	if (json.find("path") != json.end()) object.path = static_cast<decltype(object.path)>(pathFromString.at(json.at("path").get<std::string>()));
    	if (json.find("extensions") != json.end()) object.extensions = json.at("extensions").get<decltype(object.extensions)>();
    	if (json.find("extras") != json.end()) object.extras = json.at("extras").get<decltype(object.extras)>();
    }
    
    void to_json(nlohmann::json& json, const Animation_Channel_Target& object)
    {
    	if (object.node != -1) json.emplace("node", object.node);
    	const std::array<std::string, 4> pathToString = { "translation" , "rotation" , "scale" , "weights"  };
    	json.emplace("path", pathToString[static_cast<int>(object.path)]);
    	if (object.extensions.size() > 0) json.emplace("extensions", object.extensions);
    	if (object.extras.size() > 0) json.emplace("extras", object.extras);
    }
    
    void from_json(const nlohmann::json& json, Animation_Sampler& object)
    {
    	if (json.find("input") != json.end()) object.input = json.at("input").get<decltype(object.input)>();
    	const std::unordered_map<std::string, int> interpolationFromString = { { "LINEAR", 0 } , { "STEP", 1 } , { "CATMULLROMSPLINE", 2 } , { "CUBICSPLINE", 3 }  };
    	if (json.find("interpolation") != json.end()) object.interpolation = static_cast<decltype(object.interpolation)>(interpolationFromString.at(json.at("interpolation").get<std::string>()));
    	if (json.find("output") != json.end()) object.output = json.at("output").get<decltype(object.output)>();
    	if (json.find("extensions") != json.end()) object.extensions = json.at("extensions").get<decltype(object.extensions)>();
    	if (json.find("extras") != json.end()) object.extras = json.at("extras").get<decltype(object.extras)>();
    }
    
    void to_json(nlohmann::json& json, const Animation_Sampler& object)
    {
    	if (object.input != -1) json.emplace("input", object.input);
    	const std::array<std::string, 4> interpolationToString = { "LINEAR" , "STEP" , "CATMULLROMSPLINE" , "CUBICSPLINE"  };
    	json.emplace("interpolation", interpolationToString[static_cast<int>(object.interpolation)]);
    	if (object.output != -1) json.emplace("output", object.output);
    	if (object.extensions.size() > 0) json.emplace("extensions", object.extensions);
    	if (object.extras.size() > 0) json.emplace("extras", object.extras);
    }
    
    void from_json(const nlohmann::json& json, Asset& object)
    {
    	if (json.find("copyright") != json.end()) object.copyright = json.at("copyright").get<decltype(object.copyright)>();
    	if (json.find("generator") != json.end()) object.generator = json.at("generator").get<decltype(object.generator)>();
    	if (json.find("version") != json.end()) object.version = json.at("version").get<decltype(object.version)>();
    	if (json.find("minVersion") != json.end()) object.minVersion = json.at("minVersion").get<decltype(object.minVersion)>();
    	if (json.find("extensions") != json.end()) object.extensions = json.at("extensions").get<decltype(object.extensions)>();
    	if (json.find("extras") != json.end()) object.extras = json.at("extras").get<decltype(object.extras)>();
    }
    
    void to_json(nlohmann::json& json, const Asset& object)
    {
    	if (object.copyright.size() > 0) json.emplace("copyright", object.copyright);
    	if (object.generator.size() > 0) json.emplace("generator", object.generator);
    	if (object.version.size() > 0) json.emplace("version", object.version);
    	if (object.minVersion.size() > 0) json.emplace("minVersion", object.minVersion);
    	if (object.extensions.size() > 0) json.emplace("extensions", object.extensions);
    	if (object.extras.size() > 0) json.emplace("extras", object.extras);
    }
    
    void from_json(const nlohmann::json& json, Camera_Orthographic& object)
    {
    	if (json.find("xmag") != json.end()) object.xmag = json.at("xmag").get<decltype(object.xmag)>();
    	if (json.find("ymag") != json.end()) object.ymag = json.at("ymag").get<decltype(object.ymag)>();
    	if (json.find("zfar") != json.end()) object.zfar = json.at("zfar").get<decltype(object.zfar)>();
    	if (json.find("znear") != json.end()) object.znear = json.at("znear").get<decltype(object.znear)>();
    	if (json.find("extensions") != json.end()) object.extensions = json.at("extensions").get<decltype(object.extensions)>();
    	if (json.find("extras") != json.end()) object.extras = json.at("extras").get<decltype(object.extras)>();
    }
    
    void to_json(nlohmann::json& json, const Camera_Orthographic& object)
    {
    	json.emplace("xmag", object.xmag);
    	json.emplace("ymag", object.ymag);
    	json.emplace("zfar", object.zfar);
    	json.emplace("znear", object.znear);
    	if (object.extensions.size() > 0) json.emplace("extensions", object.extensions);
    	if (object.extras.size() > 0) json.emplace("extras", object.extras);
    }
    
    void from_json(const nlohmann::json& json, Camera_Perspective& object)
    {
    	if (json.find("aspectRatio") != json.end()) object.aspectRatio = json.at("aspectRatio").get<decltype(object.aspectRatio)>();
    	if (json.find("yfov") != json.end()) object.yfov = json.at("yfov").get<decltype(object.yfov)>();
    	if (json.find("zfar") != json.end()) object.zfar = json.at("zfar").get<decltype(object.zfar)>();
    	if (json.find("znear") != json.end()) object.znear = json.at("znear").get<decltype(object.znear)>();
    	if (json.find("extensions") != json.end()) object.extensions = json.at("extensions").get<decltype(object.extensions)>();
    	if (json.find("extras") != json.end()) object.extras = json.at("extras").get<decltype(object.extras)>();
    }
    
    void to_json(nlohmann::json& json, const Camera_Perspective& object)
    {
    	json.emplace("aspectRatio", object.aspectRatio);
    	json.emplace("yfov", object.yfov);
    	json.emplace("zfar", object.zfar);
    	json.emplace("znear", object.znear);
    	if (object.extensions.size() > 0) json.emplace("extensions", object.extensions);
    	if (object.extras.size() > 0) json.emplace("extras", object.extras);
    }
    
    void from_json(const nlohmann::json& json, glTFChildOfRootProperty& object)
    {
    	if (json.find("name") != json.end()) object.name = json.at("name").get<decltype(object.name)>();
    }
    
    void to_json(nlohmann::json& json, const glTFChildOfRootProperty& object)
    {
    	if (object.name.size() > 0) json.emplace("name", object.name);
    }
    
    void from_json(const nlohmann::json& json, Mesh_Primitive& object)
    {
    	if (json.find("attributes") != json.end()) object.attributes = json.at("attributes").get<decltype(object.attributes)>();
    	if (json.find("indices") != json.end()) object.indices = json.at("indices").get<decltype(object.indices)>();
    	if (json.find("material") != json.end()) object.material = json.at("material").get<decltype(object.material)>();
    	if (json.find("mode") != json.end()) object.mode = json.at("mode").get<decltype(object.mode)>();
    	if (json.find("targets") != json.end()) object.targets = json.at("targets").get<decltype(object.targets)>();
    	if (json.find("extensions") != json.end()) object.extensions = json.at("extensions").get<decltype(object.extensions)>();
    	if (json.find("extras") != json.end()) object.extras = json.at("extras").get<decltype(object.extras)>();
    }
    
    void to_json(nlohmann::json& json, const Mesh_Primitive& object)
    {
    	json.emplace("attributes", object.attributes);
    	if (object.indices != -1) json.emplace("indices", object.indices);
    	if (object.material != -1) json.emplace("material", object.material);
    	json.emplace("mode", object.mode);
    	if (object.targets.size() > 0) json.emplace("targets", object.targets);
    	if (object.extensions.size() > 0) json.emplace("extensions", object.extensions);
    	if (object.extras.size() > 0) json.emplace("extras", object.extras);
    }
    
    void from_json(const nlohmann::json& json, TextureInfo& object)
    {
    	if (json.find("index") != json.end()) object.index = json.at("index").get<decltype(object.index)>();
    	if (json.find("texCoord") != json.end()) object.texCoord = json.at("texCoord").get<decltype(object.texCoord)>();
    	if (json.find("extensions") != json.end()) object.extensions = json.at("extensions").get<decltype(object.extensions)>();
    	if (json.find("extras") != json.end()) object.extras = json.at("extras").get<decltype(object.extras)>();
    }
    
    void to_json(nlohmann::json& json, const TextureInfo& object)
    {
    	if (object.index != -1) json.emplace("index", object.index);
    	json.emplace("texCoord", object.texCoord);
    	if (object.extensions.size() > 0) json.emplace("extensions", object.extensions);
    	if (object.extras.size() > 0) json.emplace("extras", object.extras);
    }
    
    void from_json(const nlohmann::json& json, Accessor_Sparse& object)
    {
    	if (json.find("count") != json.end()) object.count = json.at("count").get<decltype(object.count)>();
    	if (json.find("indices") != json.end()) object.indices = json.at("indices").get<decltype(object.indices)>();
    	if (json.find("values") != json.end()) object.values = json.at("values").get<decltype(object.values)>();
    	if (json.find("extensions") != json.end()) object.extensions = json.at("extensions").get<decltype(object.extensions)>();
    	if (json.find("extras") != json.end()) object.extras = json.at("extras").get<decltype(object.extras)>();
    }
    
    void to_json(nlohmann::json& json, const Accessor_Sparse& object)
    {
    	json.emplace("count", object.count);
    	json.emplace("indices", object.indices);
    	json.emplace("values", object.values);
    	if (object.extensions.size() > 0) json.emplace("extensions", object.extensions);
    	if (object.extras.size() > 0) json.emplace("extras", object.extras);
    }
    
    void from_json(const nlohmann::json& json, Animation_Channel& object)
    {
    	if (json.find("sampler") != json.end()) object.sampler = json.at("sampler").get<decltype(object.sampler)>();
    	if (json.find("target") != json.end()) object.target = json.at("target").get<decltype(object.target)>();
    	if (json.find("extensions") != json.end()) object.extensions = json.at("extensions").get<decltype(object.extensions)>();
    	if (json.find("extras") != json.end()) object.extras = json.at("extras").get<decltype(object.extras)>();
    }
    
    void to_json(nlohmann::json& json, const Animation_Channel& object)
    {
    	if (object.sampler != -1) json.emplace("sampler", object.sampler);
    	json.emplace("target", object.target);
    	if (object.extensions.size() > 0) json.emplace("extensions", object.extensions);
    	if (object.extras.size() > 0) json.emplace("extras", object.extras);
    }
    
    void from_json(const nlohmann::json& json, Buffer& object)
    {
    	if (json.find("uri") != json.end()) object.uri = json.at("uri").get<decltype(object.uri)>();
    	if (json.find("byteLength") != json.end()) object.byteLength = json.at("byteLength").get<decltype(object.byteLength)>();
    	if (json.find("name") != json.end()) object.name = json.at("name").get<decltype(object.name)>();
    	if (json.find("extensions") != json.end()) object.extensions = json.at("extensions").get<decltype(object.extensions)>();
    	if (json.find("extras") != json.end()) object.extras = json.at("extras").get<decltype(object.extras)>();
    }
    
    void to_json(nlohmann::json& json, const Buffer& object)
    {
    	if (object.uri.size() > 0) json.emplace("uri", object.uri);
    	json.emplace("byteLength", object.byteLength);
    	json.emplace("name", object.name);
    	if (object.extensions.size() > 0) json.emplace("extensions", object.extensions);
    	if (object.extras.size() > 0) json.emplace("extras", object.extras);
    }
    
    void from_json(const nlohmann::json& json, BufferView& object)
    {
    	if (json.find("buffer") != json.end()) object.buffer = json.at("buffer").get<decltype(object.buffer)>();
    	if (json.find("byteOffset") != json.end()) object.byteOffset = json.at("byteOffset").get<decltype(object.byteOffset)>();
    	if (json.find("byteLength") != json.end()) object.byteLength = json.at("byteLength").get<decltype(object.byteLength)>();        
        if (json.find("byteStride") != json.end()) object.byteStride = json.at("byteStride").get<decltype(object.byteStride)>();
        if (json.find("target") != json.end()) object.target = json.at("target").get<decltype(object.target)>();
    	if (json.find("name") != json.end()) object.name = json.at("name").get<decltype(object.name)>();
    	if (json.find("extensions") != json.end()) object.extensions = json.at("extensions").get<decltype(object.extensions)>();
    	if (json.find("extras") != json.end()) object.extras = json.at("extras").get<decltype(object.extras)>();

        //if (object.target == BufferView::Target::ELEMENT_ARRAY_BUFFER && object.byteStride == 0) object.byteStride = sizeof(uint32_t);
    }
    
    void to_json(nlohmann::json& json, const BufferView& object)
    {
    	if (object.buffer != -1) json.emplace("buffer", object.buffer);
    	json.emplace("byteOffset", object.byteOffset);
    	json.emplace("byteLength", object.byteLength);
    	json.emplace("byteStride", object.byteStride);
    	json.emplace("target", object.target);
    	json.emplace("name", object.name);
    	if (object.extensions.size() > 0) json.emplace("extensions", object.extensions);
    	if (object.extras.size() > 0) json.emplace("extras", object.extras);
    }
    
    void from_json(const nlohmann::json& json, Camera& object)
    {
    	if (json.find("orthographic") != json.end()) object.orthographic = json.at("orthographic").get<decltype(object.orthographic)>();
    	if (json.find("perspective") != json.end()) object.perspective = json.at("perspective").get<decltype(object.perspective)>();
    	const std::unordered_map<std::string, int> typeFromString = { { "perspective", 0 } , { "orthographic", 1 }  };
    	if (json.find("type") != json.end()) object.type = static_cast<decltype(object.type)>(typeFromString.at(json.at("type").get<std::string>()));
    	if (json.find("name") != json.end()) object.name = json.at("name").get<decltype(object.name)>();
    	if (json.find("extensions") != json.end()) object.extensions = json.at("extensions").get<decltype(object.extensions)>();
    	if (json.find("extras") != json.end()) object.extras = json.at("extras").get<decltype(object.extras)>();
    }
    
    void to_json(nlohmann::json& json, const Camera& object)
    {
    	json.emplace("orthographic", object.orthographic);
    	json.emplace("perspective", object.perspective);
    	const std::array<std::string, 2> typeToString = { "perspective" , "orthographic"  };
    	json.emplace("type", typeToString[static_cast<int>(object.type)]);
    	json.emplace("name", object.name);
    	if (object.extensions.size() > 0) json.emplace("extensions", object.extensions);
    	if (object.extras.size() > 0) json.emplace("extras", object.extras);
    }
    
    void from_json(const nlohmann::json& json, Image& object)
    {
    	if (json.find("uri") != json.end()) object.uri = json.at("uri").get<decltype(object.uri)>();
    	const std::unordered_map<std::string, int> mimeTypeFromString = { { "image/jpeg", 0 } , { "image/png", 1 }  };
    	if (json.find("mimeType") != json.end()) object.mimeType = static_cast<decltype(object.mimeType)>(mimeTypeFromString.at(json.at("mimeType").get<std::string>()));
    	if (json.find("bufferView") != json.end()) object.bufferView = json.at("bufferView").get<decltype(object.bufferView)>();
    	if (json.find("name") != json.end()) object.name = json.at("name").get<decltype(object.name)>();
    	if (json.find("extensions") != json.end()) object.extensions = json.at("extensions").get<decltype(object.extensions)>();
    	if (json.find("extras") != json.end()) object.extras = json.at("extras").get<decltype(object.extras)>();
    }
    
    void to_json(nlohmann::json& json, const Image& object)
    {
    	if (object.uri.size() > 0) json.emplace("uri", object.uri);
    	const std::array<std::string, 2> mimeTypeToString = { "image/jpeg" , "image/png"  };
    	json.emplace("mimeType", mimeTypeToString[static_cast<int>(object.mimeType)]);
    	if (object.bufferView != -1) json.emplace("bufferView", object.bufferView);
    	json.emplace("name", object.name);
    	if (object.extensions.size() > 0) json.emplace("extensions", object.extensions);
    	if (object.extras.size() > 0) json.emplace("extras", object.extras);
    }
    
    void from_json(const nlohmann::json& json, Material_NormalTextureInfo& object)
    {
    	if (json.find("index") != json.end()) object.index = json.at("index").get<decltype(object.index)>();
    	if (json.find("texCoord") != json.end()) object.texCoord = json.at("texCoord").get<decltype(object.texCoord)>();
    	if (json.find("scale") != json.end()) object.scale = json.at("scale").get<decltype(object.scale)>();
    	if (json.find("extensions") != json.end()) object.extensions = json.at("extensions").get<decltype(object.extensions)>();
    	if (json.find("extras") != json.end()) object.extras = json.at("extras").get<decltype(object.extras)>();
    }
    
    void to_json(nlohmann::json& json, const Material_NormalTextureInfo& object)
    {
    	if (object.index != -1) json.emplace("index", object.index);
    	json.emplace("texCoord", object.texCoord);
    	json.emplace("scale", object.scale);
    	if (object.extensions.size() > 0) json.emplace("extensions", object.extensions);
    	if (object.extras.size() > 0) json.emplace("extras", object.extras);
    }
    
    void from_json(const nlohmann::json& json, Material_OcclusionTextureInfo& object)
    {
    	if (json.find("index") != json.end()) object.index = json.at("index").get<decltype(object.index)>();
    	if (json.find("texCoord") != json.end()) object.texCoord = json.at("texCoord").get<decltype(object.texCoord)>();
    	if (json.find("strength") != json.end()) object.strength = json.at("strength").get<decltype(object.strength)>();
    	if (json.find("extensions") != json.end()) object.extensions = json.at("extensions").get<decltype(object.extensions)>();
    	if (json.find("extras") != json.end()) object.extras = json.at("extras").get<decltype(object.extras)>();
    }
    
    void to_json(nlohmann::json& json, const Material_OcclusionTextureInfo& object)
    {
    	if (object.index != -1) json.emplace("index", object.index);
    	json.emplace("texCoord", object.texCoord);
    	json.emplace("strength", object.strength);
    	if (object.extensions.size() > 0) json.emplace("extensions", object.extensions);
    	if (object.extras.size() > 0) json.emplace("extras", object.extras);
    }
    
    void from_json(const nlohmann::json& json, Material_PbrMetallicRoughness& object)
    {
    	if (json.find("baseColorFactor") != json.end()) object.baseColorFactor = json.at("baseColorFactor").get<decltype(object.baseColorFactor)>();
    	if (json.find("baseColorTexture") != json.end()) object.baseColorTexture = json.at("baseColorTexture").get<decltype(object.baseColorTexture)>();
    	if (json.find("metallicFactor") != json.end()) object.metallicFactor = json.at("metallicFactor").get<decltype(object.metallicFactor)>();
    	if (json.find("roughnessFactor") != json.end()) object.roughnessFactor = json.at("roughnessFactor").get<decltype(object.roughnessFactor)>();
    	if (json.find("metallicRoughnessTexture") != json.end()) object.metallicRoughnessTexture = json.at("metallicRoughnessTexture").get<decltype(object.metallicRoughnessTexture)>();
    	if (json.find("extensions") != json.end()) object.extensions = json.at("extensions").get<decltype(object.extensions)>();
    	if (json.find("extras") != json.end()) object.extras = json.at("extras").get<decltype(object.extras)>();
    }
    
    void to_json(nlohmann::json& json, const Material_PbrMetallicRoughness& object)
    {
    	if (object.baseColorFactor != decltype(object.baseColorFactor) { 1.0, 1.0, 1.0, 1.0 }) json.emplace("baseColorFactor", object.baseColorFactor);
    	json.emplace("baseColorTexture", object.baseColorTexture);
    	json.emplace("metallicFactor", object.metallicFactor);
    	json.emplace("roughnessFactor", object.roughnessFactor);
    	json.emplace("metallicRoughnessTexture", object.metallicRoughnessTexture);
    	if (object.extensions.size() > 0) json.emplace("extensions", object.extensions);
    	if (object.extras.size() > 0) json.emplace("extras", object.extras);
    }
    
    void from_json(const nlohmann::json& json, Mesh& object)
    {
    	if (json.find("primitives") != json.end()) object.primitives = json.at("primitives").get<decltype(object.primitives)>();
    	if (json.find("weights") != json.end()) object.weights = json.at("weights").get<decltype(object.weights)>();
    	if (json.find("name") != json.end()) object.name = json.at("name").get<decltype(object.name)>();
    	if (json.find("extensions") != json.end()) object.extensions = json.at("extensions").get<decltype(object.extensions)>();
    	if (json.find("extras") != json.end()) object.extras = json.at("extras").get<decltype(object.extras)>();
    }
    
    void to_json(nlohmann::json& json, const Mesh& object)
    {
    	if (object.primitives.size() > 0) json.emplace("primitives", object.primitives);
    	if (object.weights.size() > 0) json.emplace("weights", object.weights);
    	json.emplace("name", object.name);
    	if (object.extensions.size() > 0) json.emplace("extensions", object.extensions);
    	if (object.extras.size() > 0) json.emplace("extras", object.extras);
    }
    
    void from_json(const nlohmann::json& json, Node& object)
    {
    	if (json.find("camera") != json.end()) object.camera = json.at("camera").get<decltype(object.camera)>();
    	if (json.find("children") != json.end()) object.children = json.at("children").get<decltype(object.children)>();
    	if (json.find("skin") != json.end()) object.skin = json.at("skin").get<decltype(object.skin)>();
    	if (json.find("matrix") != json.end()) object.matrix = json.at("matrix").get<decltype(object.matrix)>();
    	if (json.find("mesh") != json.end()) object.mesh = json.at("mesh").get<decltype(object.mesh)>();
    	if (json.find("rotation") != json.end()) object.rotation = json.at("rotation").get<decltype(object.rotation)>();
    	if (json.find("scale") != json.end()) object.scale = json.at("scale").get<decltype(object.scale)>();
    	if (json.find("translation") != json.end()) object.translation = json.at("translation").get<decltype(object.translation)>();
    	if (json.find("weights") != json.end()) object.weights = json.at("weights").get<decltype(object.weights)>();
    	if (json.find("name") != json.end()) object.name = json.at("name").get<decltype(object.name)>();
    	if (json.find("extensions") != json.end()) object.extensions = json.at("extensions").get<decltype(object.extensions)>();
    	if (json.find("extras") != json.end()) object.extras = json.at("extras").get<decltype(object.extras)>();
    }
    
    void to_json(nlohmann::json& json, const Node& object)
    {
    	if (object.camera != -1) json.emplace("camera", object.camera);
    	if (object.children.size() > 0) json.emplace("children", object.children);
    	if (object.skin != -1) json.emplace("skin", object.skin);
    	if (object.matrix != decltype(object.matrix) { 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0 }) json.emplace("matrix", object.matrix);
    	if (object.mesh != -1) json.emplace("mesh", object.mesh);
    	if (object.rotation != decltype(object.rotation) { 0.0, 0.0, 0.0, 1.0 }) json.emplace("rotation", object.rotation);
    	if (object.scale != decltype(object.scale) { 1.0, 1.0, 1.0 }) json.emplace("scale", object.scale);
    	if (object.translation != decltype(object.translation) { 0.0, 0.0, 0.0 }) json.emplace("translation", object.translation);
    	if (object.weights.size() > 0) json.emplace("weights", object.weights);
    	json.emplace("name", object.name);
    	if (object.extensions.size() > 0) json.emplace("extensions", object.extensions);
    	if (object.extras.size() > 0) json.emplace("extras", object.extras);
    }
    
    void from_json(const nlohmann::json& json, Sampler& object)
    {
    	if (json.find("magFilter") != json.end()) object.magFilter = json.at("magFilter").get<decltype(object.magFilter)>();
    	if (json.find("minFilter") != json.end()) object.minFilter = json.at("minFilter").get<decltype(object.minFilter)>();
    	if (json.find("wrapS") != json.end()) object.wrapS = json.at("wrapS").get<decltype(object.wrapS)>();
    	if (json.find("wrapT") != json.end()) object.wrapT = json.at("wrapT").get<decltype(object.wrapT)>();
    	if (json.find("name") != json.end()) object.name = json.at("name").get<decltype(object.name)>();
    	if (json.find("extensions") != json.end()) object.extensions = json.at("extensions").get<decltype(object.extensions)>();
    	if (json.find("extras") != json.end()) object.extras = json.at("extras").get<decltype(object.extras)>();
    }
    
    void to_json(nlohmann::json& json, const Sampler& object)
    {
    	json.emplace("magFilter", object.magFilter);
    	json.emplace("minFilter", object.minFilter);
    	json.emplace("wrapS", object.wrapS);
    	json.emplace("wrapT", object.wrapT);
    	json.emplace("name", object.name);
    	if (object.extensions.size() > 0) json.emplace("extensions", object.extensions);
    	if (object.extras.size() > 0) json.emplace("extras", object.extras);
    }
    
    void from_json(const nlohmann::json& json, Scene& object)
    {
    	if (json.find("nodes") != json.end()) object.nodes = json.at("nodes").get<decltype(object.nodes)>();
    	if (json.find("name") != json.end()) object.name = json.at("name").get<decltype(object.name)>();
    	if (json.find("extensions") != json.end()) object.extensions = json.at("extensions").get<decltype(object.extensions)>();
    	if (json.find("extras") != json.end()) object.extras = json.at("extras").get<decltype(object.extras)>();
    }
    
    void to_json(nlohmann::json& json, const Scene& object)
    {
    	if (object.nodes.size() > 0) json.emplace("nodes", object.nodes);
    	json.emplace("name", object.name);
    	if (object.extensions.size() > 0) json.emplace("extensions", object.extensions);
    	if (object.extras.size() > 0) json.emplace("extras", object.extras);
    }
    
    void from_json(const nlohmann::json& json, Skin& object)
    {
    	if (json.find("inverseBindMatrices") != json.end()) object.inverseBindMatrices = json.at("inverseBindMatrices").get<decltype(object.inverseBindMatrices)>();
    	if (json.find("skeleton") != json.end()) object.skeleton = json.at("skeleton").get<decltype(object.skeleton)>();
    	if (json.find("joints") != json.end()) object.joints = json.at("joints").get<decltype(object.joints)>();
    	if (json.find("name") != json.end()) object.name = json.at("name").get<decltype(object.name)>();
    	if (json.find("extensions") != json.end()) object.extensions = json.at("extensions").get<decltype(object.extensions)>();
    	if (json.find("extras") != json.end()) object.extras = json.at("extras").get<decltype(object.extras)>();
    }
    
    void to_json(nlohmann::json& json, const Skin& object)
    {
    	if (object.inverseBindMatrices != -1) json.emplace("inverseBindMatrices", object.inverseBindMatrices);
    	if (object.skeleton != -1) json.emplace("skeleton", object.skeleton);
    	if (object.joints.size() > 0) json.emplace("joints", object.joints);
    	json.emplace("name", object.name);
    	if (object.extensions.size() > 0) json.emplace("extensions", object.extensions);
    	if (object.extras.size() > 0) json.emplace("extras", object.extras);
    }
    
    void from_json(const nlohmann::json& json, Texture& object)
    {
    	if (json.find("sampler") != json.end()) object.sampler = json.at("sampler").get<decltype(object.sampler)>();
    	if (json.find("source") != json.end()) object.source = json.at("source").get<decltype(object.source)>();
    	if (json.find("name") != json.end()) object.name = json.at("name").get<decltype(object.name)>();
    	if (json.find("extensions") != json.end()) object.extensions = json.at("extensions").get<decltype(object.extensions)>();
    	if (json.find("extras") != json.end()) object.extras = json.at("extras").get<decltype(object.extras)>();
    }
    
    void to_json(nlohmann::json& json, const Texture& object)
    {
    	if (object.sampler != -1) json.emplace("sampler", object.sampler);
    	if (object.source != -1) json.emplace("source", object.source);
    	json.emplace("name", object.name);
    	if (object.extensions.size() > 0) json.emplace("extensions", object.extensions);
    	if (object.extras.size() > 0) json.emplace("extras", object.extras);
    }
    
    void from_json(const nlohmann::json& json, Accessor& object)
    {
    	if (json.find("bufferView") != json.end()) object.bufferView = json.at("bufferView").get<decltype(object.bufferView)>();
    	if (json.find("byteOffset") != json.end()) object.byteOffset = json.at("byteOffset").get<decltype(object.byteOffset)>();
    	if (json.find("componentType") != json.end()) object.componentType = json.at("componentType").get<decltype(object.componentType)>();
    	if (json.find("normalized") != json.end()) object.normalized = json.at("normalized").get<decltype(object.normalized)>();
    	if (json.find("count") != json.end()) object.count = json.at("count").get<decltype(object.count)>();
    	const std::unordered_map<std::string, int> typeFromString = { { "SCALAR", 0 } , { "VEC2", 1 } , { "VEC3", 2 } , { "VEC4", 3 } , { "MAT2", 4 } , { "MAT3", 5 } , { "MAT4", 6 }  };
    	if (json.find("type") != json.end()) object.type = static_cast<decltype(object.type)>(typeFromString.at(json.at("type").get<std::string>()));
    	if (json.find("max") != json.end()) object.max = json.at("max").get<decltype(object.max)>();
    	if (json.find("min") != json.end()) object.min = json.at("min").get<decltype(object.min)>();
    	if (json.find("sparse") != json.end()) object.sparse = json.at("sparse").get<decltype(object.sparse)>();
    	if (json.find("name") != json.end()) object.name = json.at("name").get<decltype(object.name)>();
    	if (json.find("extensions") != json.end()) object.extensions = json.at("extensions").get<decltype(object.extensions)>();
    	if (json.find("extras") != json.end()) object.extras = json.at("extras").get<decltype(object.extras)>();
    }
    
    void to_json(nlohmann::json& json, const Accessor& object)
    {
    	if (object.bufferView != -1) json.emplace("bufferView", object.bufferView);
    	json.emplace("byteOffset", object.byteOffset);
    	json.emplace("componentType", object.componentType);
    	json.emplace("normalized", object.normalized);
    	json.emplace("count", object.count);
    	const std::array<std::string, 7> typeToString = { "SCALAR" , "VEC2" , "VEC3" , "VEC4" , "MAT2" , "MAT3" , "MAT4"  };
    	json.emplace("type", typeToString[static_cast<int>(object.type)]);
    	if (object.max.size() > 0) json.emplace("max", object.max);
    	if (object.min.size() > 0) json.emplace("min", object.min);
    	json.emplace("sparse", object.sparse);
    	json.emplace("name", object.name);
    	if (object.extensions.size() > 0) json.emplace("extensions", object.extensions);
    	if (object.extras.size() > 0) json.emplace("extras", object.extras);
    }
    
    void from_json(const nlohmann::json& json, Animation& object)
    {
    	if (json.find("channels") != json.end()) object.channels = json.at("channels").get<decltype(object.channels)>();
    	if (json.find("samplers") != json.end()) object.samplers = json.at("samplers").get<decltype(object.samplers)>();
    	if (json.find("name") != json.end()) object.name = json.at("name").get<decltype(object.name)>();
    	if (json.find("extensions") != json.end()) object.extensions = json.at("extensions").get<decltype(object.extensions)>();
    	if (json.find("extras") != json.end()) object.extras = json.at("extras").get<decltype(object.extras)>();
    }
    
    void to_json(nlohmann::json& json, const Animation& object)
    {
    	if (object.channels.size() > 0) json.emplace("channels", object.channels);
    	if (object.samplers.size() > 0) json.emplace("samplers", object.samplers);
    	json.emplace("name", object.name);
    	if (object.extensions.size() > 0) json.emplace("extensions", object.extensions);
    	if (object.extras.size() > 0) json.emplace("extras", object.extras);
    }
    
    void from_json(const nlohmann::json& json, Material& object)
    {
    	if (json.find("name") != json.end()) object.name = json.at("name").get<decltype(object.name)>();
    	if (json.find("extensions") != json.end()) object.extensions = json.at("extensions").get<decltype(object.extensions)>();
    	if (json.find("extras") != json.end()) object.extras = json.at("extras").get<decltype(object.extras)>();
    	if (json.find("pbrMetallicRoughness") != json.end()) object.pbrMetallicRoughness = json.at("pbrMetallicRoughness").get<decltype(object.pbrMetallicRoughness)>();
    	if (json.find("normalTexture") != json.end()) object.normalTexture = json.at("normalTexture").get<decltype(object.normalTexture)>();
    	if (json.find("occlusionTexture") != json.end()) object.occlusionTexture = json.at("occlusionTexture").get<decltype(object.occlusionTexture)>();
    	if (json.find("emissiveTexture") != json.end()) object.emissiveTexture = json.at("emissiveTexture").get<decltype(object.emissiveTexture)>();
    	if (json.find("emissiveFactor") != json.end()) object.emissiveFactor = json.at("emissiveFactor").get<decltype(object.emissiveFactor)>();
    	const std::unordered_map<std::string, int> alphaModeFromString = { { "OPAQUE", 0 } , { "MASK", 1 } , { "BLEND", 2 }  };
    	if (json.find("alphaMode") != json.end()) object.alphaMode = static_cast<decltype(object.alphaMode)>(alphaModeFromString.at(json.at("alphaMode").get<std::string>()));
    	if (json.find("alphaCutoff") != json.end()) object.alphaCutoff = json.at("alphaCutoff").get<decltype(object.alphaCutoff)>();
    	if (json.find("doubleSided") != json.end()) object.doubleSided = json.at("doubleSided").get<decltype(object.doubleSided)>();
    }
    
    void to_json(nlohmann::json& json, const Material& object)
    {
    	json.emplace("name", object.name);
    	if (object.extensions.size() > 0) json.emplace("extensions", object.extensions);
    	if (object.extras.size() > 0) json.emplace("extras", object.extras);
    	json.emplace("pbrMetallicRoughness", object.pbrMetallicRoughness);
    	json.emplace("normalTexture", object.normalTexture);
    	json.emplace("occlusionTexture", object.occlusionTexture);
    	json.emplace("emissiveTexture", object.emissiveTexture);
    	if (object.emissiveFactor != decltype(object.emissiveFactor) { 0.0, 0.0, 0.0 }) json.emplace("emissiveFactor", object.emissiveFactor);
    	const std::array<std::string, 3> alphaModeToString = { "OPAQUE" , "MASK" , "BLEND"  };
    	json.emplace("alphaMode", alphaModeToString[static_cast<int>(object.alphaMode)]);
    	json.emplace("alphaCutoff", object.alphaCutoff);
    	json.emplace("doubleSided", object.doubleSided);
    }
    
    void from_json(const nlohmann::json& json, glTF& object)
    {
    	if (json.find("extensionsUsed") != json.end()) object.extensionsUsed = json.at("extensionsUsed").get<decltype(object.extensionsUsed)>();
    	if (json.find("extensionsRequired") != json.end()) object.extensionsRequired = json.at("extensionsRequired").get<decltype(object.extensionsRequired)>();
    	if (json.find("accessors") != json.end()) object.accessors = json.at("accessors").get<decltype(object.accessors)>();
    	if (json.find("animations") != json.end()) object.animations = json.at("animations").get<decltype(object.animations)>();
    	if (json.find("asset") != json.end()) object.asset = json.at("asset").get<decltype(object.asset)>();
    	if (json.find("buffers") != json.end()) object.buffers = json.at("buffers").get<decltype(object.buffers)>();
    	if (json.find("bufferViews") != json.end()) object.bufferViews = json.at("bufferViews").get<decltype(object.bufferViews)>();
    	if (json.find("cameras") != json.end()) object.cameras = json.at("cameras").get<decltype(object.cameras)>();
    	if (json.find("images") != json.end()) object.images = json.at("images").get<decltype(object.images)>();
    	if (json.find("materials") != json.end()) object.materials = json.at("materials").get<decltype(object.materials)>();
    	if (json.find("meshes") != json.end()) object.meshes = json.at("meshes").get<decltype(object.meshes)>();
    	if (json.find("nodes") != json.end()) object.nodes = json.at("nodes").get<decltype(object.nodes)>();
    	if (json.find("samplers") != json.end()) object.samplers = json.at("samplers").get<decltype(object.samplers)>();
    	if (json.find("scene") != json.end()) object.scene = json.at("scene").get<decltype(object.scene)>();
    	if (json.find("scenes") != json.end()) object.scenes = json.at("scenes").get<decltype(object.scenes)>();
    	if (json.find("skins") != json.end()) object.skins = json.at("skins").get<decltype(object.skins)>();
    	if (json.find("textures") != json.end()) object.textures = json.at("textures").get<decltype(object.textures)>();
    	if (json.find("extensions") != json.end()) object.extensions = json.at("extensions").get<decltype(object.extensions)>();
    	if (json.find("extras") != json.end()) object.extras = json.at("extras").get<decltype(object.extras)>();
    }
    
    void to_json(nlohmann::json& json, const glTF& object)
    {
    	if (object.extensionsUsed.size() > 0) json.emplace("extensionsUsed", object.extensionsUsed);
    	if (object.extensionsRequired.size() > 0) json.emplace("extensionsRequired", object.extensionsRequired);
    	if (object.accessors.size() > 0) json.emplace("accessors", object.accessors);
    	if (object.animations.size() > 0) json.emplace("animations", object.animations);
    	json.emplace("asset", object.asset);
    	if (object.buffers.size() > 0) json.emplace("buffers", object.buffers);
    	if (object.bufferViews.size() > 0) json.emplace("bufferViews", object.bufferViews);
    	if (object.cameras.size() > 0) json.emplace("cameras", object.cameras);
    	if (object.images.size() > 0) json.emplace("images", object.images);
    	if (object.materials.size() > 0) json.emplace("materials", object.materials);
    	if (object.meshes.size() > 0) json.emplace("meshes", object.meshes);
    	if (object.nodes.size() > 0) json.emplace("nodes", object.nodes);
    	if (object.samplers.size() > 0) json.emplace("samplers", object.samplers);
    	if (object.scene != -1) json.emplace("scene", object.scene);
    	if (object.scenes.size() > 0) json.emplace("scenes", object.scenes);
    	if (object.skins.size() > 0) json.emplace("skins", object.skins);
    	if (object.textures.size() > 0) json.emplace("textures", object.textures);
    	if (object.extensions.size() > 0) json.emplace("extensions", object.extensions);
    	if (object.extras.size() > 0) json.emplace("extras", object.extras);
    }

    bool Import(const std::string& filename, glTF& gltf)
    {
        // Open the file on disk for reading
        std::ifstream file(filename);
        if (!file.good())
            return false;

        // Parse the file contents into a json object.
        nlohmann::json json;
        file >> json;
        
        // Deserialize the json into the glTF object.
        gltf = json;
        
        // Close the file.
        file.close();

        // Return success.
        return true;
    }

    bool Export(const std::string& filename, const glTF& gltf)
    {
        // Open the file on disk for writing.
        std::ofstream file(filename);
        if (!file.good())
            return false;

        // Convert the glTF object to json and save to the file.
        nlohmann::json json = gltf;
        file << std::setw(4) << json;
        
        // Close the file.
        file.close();
        
        // Return success.
        return true;
    }
} // End namespace gltf