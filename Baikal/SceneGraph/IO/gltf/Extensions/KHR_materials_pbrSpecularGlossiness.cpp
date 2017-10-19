#ifdef ENABLE_GLTF

#include "KHR_Materials_PbrSpecularGlossiness.h"

namespace khr
{
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
    
    void from_json(const nlohmann::json& json, KHR_Materials_PbrSpecularGlossiness& object)
    {
    	if (json.find("diffuseFactor") != json.end()) object.diffuseFactor = json.at("diffuseFactor").get<decltype(object.diffuseFactor)>();
    	if (json.find("diffuseTexture") != json.end()) object.diffuseTexture = json.at("diffuseTexture").get<decltype(object.diffuseTexture)>();
    	if (json.find("specularFactor") != json.end()) object.specularFactor = json.at("specularFactor").get<decltype(object.specularFactor)>();
    	if (json.find("glossinessFactor") != json.end()) object.glossinessFactor = json.at("glossinessFactor").get<decltype(object.glossinessFactor)>();
    	if (json.find("specularGlossinessTexture") != json.end()) object.specularGlossinessTexture = json.at("specularGlossinessTexture").get<decltype(object.specularGlossinessTexture)>();
    	if (json.find("extensions") != json.end()) object.extensions = json.at("extensions").get<decltype(object.extensions)>();
    	if (json.find("extras") != json.end()) object.extras = json.at("extras").get<decltype(object.extras)>();
    }
    
    void to_json(nlohmann::json& json, const KHR_Materials_PbrSpecularGlossiness& object)
    {
    	if (object.diffuseFactor != decltype(object.diffuseFactor) { 1.0, 1.0, 1.0, 1.0 }) json.emplace("diffuseFactor", object.diffuseFactor);
    	json.emplace("diffuseTexture", object.diffuseTexture);
    	if (object.specularFactor != decltype(object.specularFactor) { 1.0, 1.0, 1.0 }) json.emplace("specularFactor", object.specularFactor);
    	json.emplace("glossinessFactor", object.glossinessFactor);
    	json.emplace("specularGlossinessTexture", object.specularGlossinessTexture);
    	if (object.extensions.size() > 0) json.emplace("extensions", object.extensions);
    	if (object.extras.size() > 0) json.emplace("extras", object.extras);
    }

    bool ImportExtension(gltf::Extension& extensions, KHR_Materials_PbrSpecularGlossiness& ext)
    {
        // Check to see if the extension is present.
        if (extensions.find("KHR_materials_pbrSpecularGlossiness") == extensions.end())
            return false;
        
        // Deserialize the json object into the extension.        
        ext = extensions["KHR_materials_pbrSpecularGlossiness"];
        
        // Return success.
        return true;
    }

    bool ExportExtension(const KHR_Materials_PbrSpecularGlossiness& ext, gltf::Extension& extensions)
    {
        // Add the extension to the json object.
        extensions["KHR_materials_pbrSpecularGlossiness"] = ext;
        
        // Return success.
        return true;
    }
} // End namespace khr

#endif //ENABLE_GLTF
