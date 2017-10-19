#include "AMD_RPR_Camera.h"

namespace amd
{
    void from_json(const nlohmann::json& json, AMD_RPR_Camera& object)
    {
    	if (json.find("apertureBlades") != json.end()) object.apertureBlades = json.at("apertureBlades").get<decltype(object.apertureBlades)>();
    	if (json.find("cameraMode") != json.end()) object.cameraMode = json.at("cameraMode").get<decltype(object.cameraMode)>();
    	if (json.find("exposure") != json.end()) object.exposure = json.at("exposure").get<decltype(object.exposure)>();
    	if (json.find("focusDistance") != json.end()) object.focusDistance = json.at("focusDistance").get<decltype(object.focusDistance)>();
    	if (json.find("focalLength") != json.end()) object.focalLength = json.at("focalLength").get<decltype(object.focalLength)>();
    	if (json.find("focalTilt") != json.end()) object.focalTilt = json.at("focalTilt").get<decltype(object.focalTilt)>();
    	if (json.find("fstop") != json.end()) object.fstop = json.at("fstop").get<decltype(object.fstop)>();
    	if (json.find("ipd") != json.end()) object.ipd = json.at("ipd").get<decltype(object.ipd)>();
    	if (json.find("lensShift") != json.end()) object.lensShift = json.at("lensShift").get<decltype(object.lensShift)>();
    	if (json.find("lookAt") != json.end()) object.lookAt = json.at("lookAt").get<decltype(object.lookAt)>();
    	if (json.find("orthoHeight") != json.end()) object.orthoHeight = json.at("orthoHeight").get<decltype(object.orthoHeight)>();
    	if (json.find("orthoWidth") != json.end()) object.orthoWidth = json.at("orthoWidth").get<decltype(object.orthoWidth)>();
    	if (json.find("position") != json.end()) object.position = json.at("position").get<decltype(object.position)>();
    	if (json.find("sensorSize") != json.end()) object.sensorSize = json.at("sensorSize").get<decltype(object.sensorSize)>();
    	if (json.find("tiltCorrection") != json.end()) object.tiltCorrection = json.at("tiltCorrection").get<decltype(object.tiltCorrection)>();
    	if (json.find("up") != json.end()) object.up = json.at("up").get<decltype(object.up)>();
    }
    
    void to_json(nlohmann::json& json, const AMD_RPR_Camera& object)
    {
    	json.emplace("apertureBlades", object.apertureBlades);
    	json.emplace("cameraMode", object.cameraMode);
    	json.emplace("exposure", object.exposure);
    	json.emplace("focusDistance", object.focusDistance);
    	json.emplace("focalLength", object.focalLength);
    	json.emplace("focalTilt", object.focalTilt);
    	json.emplace("fstop", object.fstop);
    	json.emplace("ipd", object.ipd);
    	if (object.lensShift != decltype(object.lensShift) { 0.0, 0.0 }) json.emplace("lensShift", object.lensShift);
    	if (object.lookAt != decltype(object.lookAt) { 0.0, 0.0, 0.0 }) json.emplace("lookAt", object.lookAt);
    	json.emplace("orthoHeight", object.orthoHeight);
    	json.emplace("orthoWidth", object.orthoWidth);
    	if (object.position != decltype(object.position) { 0.0, 0.0, 3.5 }) json.emplace("position", object.position);
    	if (object.sensorSize != decltype(object.sensorSize) { 36.0, 24.0 }) json.emplace("sensorSize", object.sensorSize);
    	if (object.tiltCorrection != decltype(object.tiltCorrection) { 0.0, 0.0 }) json.emplace("tiltCorrection", object.tiltCorrection);
    	if (object.up != decltype(object.up) { 0.0, 0.0, 1.0 }) json.emplace("up", object.up);
    }

    bool ImportExtension(gltf::Extension& extensions, AMD_RPR_Camera& ext)
    {
        // Check to see if the extension is present.
        if (extensions.find("AMD_RPR_camera") == extensions.end())
            return false;
        
        // Deserialize the json object into the extension.        
        ext = extensions["AMD_RPR_camera"];
        
        // Return success.
        return true;
    }

    bool ExportExtension(const AMD_RPR_Camera& ext, gltf::Extension& extensions)
    {
        // Add the extension to the json object.
        extensions["AMD_RPR_camera"] = ext;
        
        // Return success.
        return true;
    }
} // End namespace amd