#ifdef ENABLE_GLTF

#include "AMD_RPR_Lights.h"

namespace amd
{
    void from_json(const nlohmann::json& json, Light_Directional& object)
    {
    	if (json.find("radiantPower") != json.end()) object.radiantPower = json.at("radiantPower").get<decltype(object.radiantPower)>();
    	if (json.find("shadowSoftness") != json.end()) object.shadowSoftness = json.at("shadowSoftness").get<decltype(object.shadowSoftness)>();
    }
    
    void to_json(nlohmann::json& json, const Light_Directional& object)
    {
    	if (object.radiantPower != decltype(object.radiantPower) { 1, 1, 1 }) json.emplace("radiantPower", object.radiantPower);
    	json.emplace("shadowSoftness", object.shadowSoftness);
    }
    
    void from_json(const nlohmann::json& json, Light_Point& object)
    {
    	if (json.find("radiantPower") != json.end()) object.radiantPower = json.at("radiantPower").get<decltype(object.radiantPower)>();
    }
    
    void to_json(nlohmann::json& json, const Light_Point& object)
    {
    	if (object.radiantPower != decltype(object.radiantPower) { 1, 1, 1 }) json.emplace("radiantPower", object.radiantPower);
    }
    
    void from_json(const nlohmann::json& json, Light_Sky& object)
    {
    	if (json.find("turbidity") != json.end()) object.turbidity = json.at("turbidity").get<decltype(object.turbidity)>();
    	if (json.find("albedo") != json.end()) object.albedo = json.at("albedo").get<decltype(object.albedo)>();
    	if (json.find("scale") != json.end()) object.scale = json.at("scale").get<decltype(object.scale)>();
    }
    
    void to_json(nlohmann::json& json, const Light_Sky& object)
    {
    	json.emplace("turbidity", object.turbidity);
    	json.emplace("albedo", object.albedo);
    	json.emplace("scale", object.scale);
    }
    
    void from_json(const nlohmann::json& json, Light_Spot& object)
    {
    	if (json.find("radiantPower") != json.end()) object.radiantPower = json.at("radiantPower").get<decltype(object.radiantPower)>();
    	if (json.find("innerAngle") != json.end()) object.innerAngle = json.at("innerAngle").get<decltype(object.innerAngle)>();
    	if (json.find("outerAngle") != json.end()) object.outerAngle = json.at("outerAngle").get<decltype(object.outerAngle)>();
    }
    
    void to_json(nlohmann::json& json, const Light_Spot& object)
    {
    	if (object.radiantPower != decltype(object.radiantPower) { 1, 1, 1 }) json.emplace("radiantPower", object.radiantPower);
    	json.emplace("innerAngle", object.innerAngle);
    	json.emplace("outerAngle", object.outerAngle);
    }
    
    void from_json(const nlohmann::json& json, Light_Environment& object)
    {
    	if (json.find("image") != json.end()) object.image = json.at("image").get<decltype(object.image)>();
    	if (json.find("intensityScale") != json.end()) object.intensityScale = json.at("intensityScale").get<decltype(object.intensityScale)>();
    }
    
    void to_json(nlohmann::json& json, const Light_Environment& object)
    {
    	if (object.image != -1) json.emplace("image", object.image);
    	json.emplace("intensityScale", object.intensityScale);
    }
    
    void from_json(const nlohmann::json& json, Light_Ies& object)
    {
    	if (json.find("buffer") != json.end()) object.buffer = json.at("buffer").get<decltype(object.buffer)>();
    	if (json.find("nx") != json.end()) object.nx = json.at("nx").get<decltype(object.nx)>();
    	if (json.find("ny") != json.end()) object.ny = json.at("ny").get<decltype(object.ny)>();
    	if (json.find("radiantPower") != json.end()) object.radiantPower = json.at("radiantPower").get<decltype(object.radiantPower)>();
    }
    
    void to_json(nlohmann::json& json, const Light_Ies& object)
    {
    	if (object.buffer != -1) json.emplace("buffer", object.buffer);
    	json.emplace("nx", object.nx);
    	json.emplace("ny", object.ny);
    	if (object.radiantPower != decltype(object.radiantPower) { 1, 1, 1 }) json.emplace("radiantPower", object.radiantPower);
    }
    
    void from_json(const nlohmann::json& json, Light& object)
    {
        const std::unordered_map<std::string, int> typeFromString = { { "POINT", 0 } ,{ "DIRECTIONAL", 1 } ,{ "SPOT", 2 } ,{ "ENVIRONMENT", 3 } ,{ "SKY", 4 } ,{ "IES", 5 } };
        if (json.find("type") != json.end()) object.type = static_cast<decltype(object.type)>(typeFromString.at(json.at("type").get<std::string>()));

        switch (object.type)
        {
        case Light::Type::POINT:
            if (json.find("point") != json.end()) object.point = json.at("point").get<decltype(object.point)>();
            break;

        case Light::Type::DIRECTIONAL:
            if (json.find("directional") != json.end()) object.directional = json.at("directional").get<decltype(object.directional)>();
            break;

        case Light::Type::SPOT:
            if (json.find("spot") != json.end()) object.spot = json.at("spot").get<decltype(object.spot)>();
            break;

        case Light::Type::ENVIRONMENT:
            if (json.find("environment") != json.end()) object.environment = json.at("environment").get<decltype(object.environment)>();
            break;

        case Light::Type::SKY:
            if (json.find("sky") != json.end()) object.sky = json.at("sky").get<decltype(object.sky)>();
            break;

        case Light::Type::IES:
            if (json.find("ies") != json.end()) object.ies = json.at("ies").get<decltype(object.ies)>();
            break;
        }
    	
        if (json.find("transform") != json.end()) object.transform = json.at("transform").get<decltype(object.transform)>();
    }
    
    void to_json(nlohmann::json& json, const Light& object)
    {
        switch (object.type)
        {
        case Light::Type::POINT:
            json.emplace("point", object.point);
            break;

        case Light::Type::DIRECTIONAL:
            json.emplace("directional", object.directional);
            break;

        case Light::Type::SPOT:
            json.emplace("spot", object.spot);
            break;

        case Light::Type::ENVIRONMENT:
            json.emplace("environment", object.environment);
            break;

        case Light::Type::SKY:
            json.emplace("sky", object.sky);
            break;

        case Light::Type::IES:
            json.emplace("ies", object.ies);
            break;
        }

    	if (object.transform != decltype(object.transform) { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 }) json.emplace("transform", object.transform);
    	const std::array<std::string, 6> typeToString = { "POINT" , "DIRECTIONAL" , "SPOT" , "ENVIRONMENT" , "SKY" , "IES"  };
    	json.emplace("type", typeToString[static_cast<int>(object.type)]);
    }
    
    void from_json(const nlohmann::json& json, AMD_RPR_Lights& object)
    {
    	if (json.find("lights") != json.end()) object.lights = json.at("lights").get<decltype(object.lights)>();
    }
    
    void to_json(nlohmann::json& json, const AMD_RPR_Lights& object)
    {
    	if (object.lights.size() > 0) json.emplace("lights", object.lights);
    }

    bool ImportExtension(gltf::Extension& extensions, AMD_RPR_Lights& ext)
    {
        // Check to see if the extension is present.
        if (extensions.find("AMD_RPR_lights") == extensions.end())
            return false;
        
        // Deserialize the json object into the extension.        
        ext = extensions["AMD_RPR_lights"];
        
        // Return success.
        return true;
    }

    bool ExportExtension(const AMD_RPR_Lights& ext, gltf::Extension& extensions)
    {
        // Add the extension to the json object.
        extensions["AMD_RPR_lights"] = ext;
        
        // Return success.
        return true;
    }
} // End namespace amd

#endif //ENABLE_GLTF
