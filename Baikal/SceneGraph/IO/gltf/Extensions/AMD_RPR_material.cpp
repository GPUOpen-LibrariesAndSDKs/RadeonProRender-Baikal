#include "AMD_RPR_Material.h"

namespace amd
{
    void from_json(const nlohmann::json& json, Input& object)
    {
    	if (json.find("name") != json.end()) object.name = json.at("name").get<decltype(object.name)>();
		const std::unordered_map<std::string, int> typeFromString = { { "FLOAT4", 0 } , { "UINT", 1 } , { "NODE", 2 } , { "IMAGE", 3 }  };
    	if (json.find("type") != json.end()) object.type = static_cast<decltype(object.type)>(typeFromString.at(json.at("type").get<std::string>()));
        if (json.find("value") != json.end())
        {
            if (object.type == Input::Type::FLOAT4) object.value.array = json.at("value").get<decltype(object.value.array)>();
            else object.value.integer = json.at("value").get<decltype(object.value.integer)>();
        }
    }
    
    void to_json(nlohmann::json& json, const Input& object)
    {
    	if (object.name.size() > 0) json.emplace("name", object.name);
		const std::array<std::string, 4> typeToString = { "FLOAT4" , "UINT" , "NODE" , "IMAGE"  };
    	json.emplace("type", typeToString[static_cast<int>(object.type)]);
        if (object.type == Input::Type::FLOAT4) json.emplace("value", object.value.array);
        else json.emplace("value", object.value.integer);
    }
    
    void from_json(const nlohmann::json& json, Node& object)
    {
    	if (json.find("name") != json.end()) object.name = json.at("name").get<decltype(object.name)>();
    	const std::unordered_map<std::string, int> typeFromString = { { "UBER", 0 } , { "DIFFUSE", 1 } , { "MICROFACET", 2 } , { "REFLECTION", 3 } , { "REFRACTION", 4 } , { "MICROFACET_REFRACTION", 5 } , { "TRANSPARENT", 6 } , { "EMISSIVE", 7 } , { "WARD", 8 } , { "ADD", 9 } , { "BLEND", 10 } , { "ARITHMETIC", 11 } , { "FRESNEL", 12 } , { "NORMAL_MAP", 13 } , { "IMAGE_TEXTURE", 14 } , { "NOISE2D_TEXTURE", 15 } , { "DOT_TEXTURE", 16 } , { "GRADIENT_TEXTURE", 17 } , { "CONSTANT_TEXTURE", 18 } , { "INPUT_LOOKUP", 19 } , { "STANDARD ", 20 } , { "BLEND_VALUE", 21 } , { "PASSTHROUGH", 22 } , { "ORENNAYAR", 23 } , { "FRESNEL_SCHLICK", 24 } , { "DIFFUSE_REFRACTION", 25 } , { "BUMP_MAP", 26 } , { "VOLUME", 27 } , { "MICROFACET_ANISOTROPIC_REFLECTION", 28 } , { "MICROFACET_ANISOTROPIC_REFRACTION", 29 } , { "TWOSIDED", 30 } , { "UV_PROJECT", 31 }  };
    	if (json.find("type") != json.end()) object.type = static_cast<decltype(object.type)>(typeFromString.at(json.at("type").get<std::string>()));
    	if (json.find("inputs") != json.end()) object.inputs = json.at("inputs").get<decltype(object.inputs)>();
    }
    
    void to_json(nlohmann::json& json, const Node& object)
    {
    	if (object.name.size() > 0) json.emplace("name", object.name);
    	const std::array<std::string, 32> typeToString = { "UBER" , "DIFFUSE" , "MICROFACET" , "REFLECTION" , "REFRACTION" , "MICROFACET_REFRACTION" , "TRANSPARENT" , "EMISSIVE" , "WARD" , "ADD" , "BLEND" , "ARITHMETIC" , "FRESNEL" , "NORMAL_MAP" , "IMAGE_TEXTURE" , "NOISE2D_TEXTURE" , "DOT_TEXTURE" , "GRADIENT_TEXTURE" , "CONSTANT_TEXTURE" , "INPUT_LOOKUP" , "STANDARD " , "BLEND_VALUE" , "PASSTHROUGH" , "ORENNAYAR" , "FRESNEL_SCHLICK" , "DIFFUSE_REFRACTION" , "BUMP_MAP" , "VOLUME" , "MICROFACET_ANISOTROPIC_REFLECTION" , "MICROFACET_ANISOTROPIC_REFRACTION" , "TWOSIDED" , "UV_PROJECT"  };
    	json.emplace("type", typeToString[static_cast<int>(object.type)]);
    	if (object.inputs.size() > 0) json.emplace("inputs", object.inputs);
    }
    
    void from_json(const nlohmann::json& json, AMD_RPR_Material& object)
    {
    	if (json.find("nodes") != json.end()) object.nodes = json.at("nodes").get<decltype(object.nodes)>();
    }
    
    void to_json(nlohmann::json& json, const AMD_RPR_Material& object)
    {
    	if (object.nodes.size() > 0) json.emplace("nodes", object.nodes);
    }

    bool ImportExtension(gltf::Extension& extensions, AMD_RPR_Material& ext)
    {
        // Check to see if the extension is present.
        if (extensions.find("AMD_RPR_material") == extensions.end())
            return false;
        
        // Deserialize the json object into the extension.        
        ext = extensions["AMD_RPR_material"];
        
        // Return success.
        return true;
    }

    bool ExportExtension(const AMD_RPR_Material& ext, gltf::Extension& extensions)
    {
        // Add the extension to the json object.
        extensions["AMD_RPR_material"] = ext;
        
        // Return success.
        return true;
    }
} // End namespace amd