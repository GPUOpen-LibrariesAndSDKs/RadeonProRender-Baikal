#include "AMD_RPR_Post_Effects.h"

namespace amd
{
    void from_json(const nlohmann::json& json, GammaCorrection& object)
    {
    }
    
    void to_json(nlohmann::json& json, const GammaCorrection& object)
    {
    }
    
    void from_json(const nlohmann::json& json, Normalization& object)
    {
    }
    
    void to_json(nlohmann::json& json, const Normalization& object)
    {
    }
    
    void from_json(const nlohmann::json& json, SimpleTonemap& object)
    {
    	if (json.find("exposure") != json.end()) object.exposure = json.at("exposure").get<decltype(object.exposure)>();
    	if (json.find("contrast") != json.end()) object.contrast = json.at("contrast").get<decltype(object.contrast)>();
    	if (json.find("enableTonemap") != json.end()) object.enableTonemap = json.at("enableTonemap").get<decltype(object.enableTonemap)>();
    }
    
    void to_json(nlohmann::json& json, const SimpleTonemap& object)
    {
    	json.emplace("exposure", object.exposure);
    	json.emplace("contrast", object.contrast);
    	json.emplace("enableTonemap", object.enableTonemap);
    }
    
    void from_json(const nlohmann::json& json, Tonemap& object)
    {
    }
    
    void to_json(nlohmann::json& json, const Tonemap& object)
    {
    }
    
    void from_json(const nlohmann::json& json, WhiteBalance& object)
    {
    	if (json.find("colorSpace") != json.end()) object.colorSpace = json.at("colorSpace").get<decltype(object.colorSpace)>();
    	if (json.find("colorTemperature") != json.end()) object.colorTemperature = json.at("colorTemperature").get<decltype(object.colorTemperature)>();
    }
    
    void to_json(nlohmann::json& json, const WhiteBalance& object)
    {
    	json.emplace("colorSpace", object.colorSpace);
    	json.emplace("colorTemperature", object.colorTemperature);
    }
    
    void from_json(const nlohmann::json& json, PostEffect& object)
    {
        if (json.find("type") != json.end()) object.type = json.at("type").get<decltype(object.type)>();

        switch (object.type)
        {
        case PostEffect::Type::TONE_MAP:
            if (json.find("tonemap") != json.end()) object.tonemap = json.at("tonemap").get<decltype(object.tonemap)>();
            break;

        case PostEffect::Type::WHITE_BALANCE:
            if (json.find("whiteBalance") != json.end()) object.whiteBalance = json.at("whiteBalance").get<decltype(object.whiteBalance)>();
            break;

        case PostEffect::Type::SIMPLE_TONEMAP:
            if (json.find("simpleTonemap") != json.end()) object.simpleTonemap = json.at("simpleTonemap").get<decltype(object.simpleTonemap)>();
            break;

        case PostEffect::Type::NORMALIZATION:
            if (json.find("normalization") != json.end()) object.normalization = json.at("normalization").get<decltype(object.normalization)>();
            break;

        case PostEffect::Type::GAMMA_CORRECTION:
            if (json.find("gammaCorrection") != json.end()) object.gammaCorrection = json.at("gammaCorrection").get<decltype(object.gammaCorrection)>();
            break;
        }
    }
    
    void to_json(nlohmann::json& json, const PostEffect& object)
    {
        switch (object.type)
        {
        case PostEffect::Type::TONE_MAP:
            json.emplace("tonemap", object.tonemap);
            break;

        case PostEffect::Type::WHITE_BALANCE:
            json.emplace("whiteBalance", object.whiteBalance);
            break;

        case PostEffect::Type::SIMPLE_TONEMAP:
            json.emplace("simpleTonemap", object.simpleTonemap);
            break;

        case PostEffect::Type::NORMALIZATION:
            json.emplace("normalization", object.normalization);
            break;

        case PostEffect::Type::GAMMA_CORRECTION:
            json.emplace("gammaCorrection", object.gammaCorrection);
            break;
        }

    	json.emplace("type", object.type);
    }
    
    void from_json(const nlohmann::json& json, AMD_RPR_Post_Effects& object)
    {
    	if (json.find("postEffects") != json.end()) object.postEffects = json.at("postEffects").get<decltype(object.postEffects)>();
    }
    
    void to_json(nlohmann::json& json, const AMD_RPR_Post_Effects& object)
    {
    	if (object.postEffects.size() > 0) json.emplace("postEffects", object.postEffects);
    }

    bool ImportExtension(gltf::Extension& extensions, AMD_RPR_Post_Effects& ext)
    {
        // Check to see if the extension is present.
        if (extensions.find("AMD_RPR_post_effects") == extensions.end())
            return false;
        
        // Deserialize the json object into the extension.        
        ext = extensions["AMD_RPR_post_effects"];
        
        // Return success.
        return true;
    }

    bool ExportExtension(const AMD_RPR_Post_Effects& ext, gltf::Extension& extensions)
    {
        // Add the extension to the json object.
        extensions["AMD_RPR_post_effects"] = ext;
        
        // Return success.
        return true;
    }
} // End namespace amd