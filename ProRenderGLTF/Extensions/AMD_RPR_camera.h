#pragma once
#include "gltf2.h"

namespace amd
{
    // AMD Radeon ProRender camera glTF extension
    struct AMD_RPR_Camera
    {
    	enum class CameraMode
    	{
    		PERSPECTIVE = 1,
    		ORTHOGRAPHIC = 2,
    		LATITUDE_LONGITUDE_360 = 3,
    		LATITUDE_LONGITUDE_STEREO = 4,
    		CUBEMAP = 5,
    		CUBEMAP_STEREO = 6,
    		FISHEYE = 7,
    	};
    
    	int apertureBlades = 0; // Number of aperture blades 4 to 32.
    	CameraMode cameraMode; // Camera mode
    	float exposure = 0.0f; // Exposure value
    	float focusDistance = 1.0f; // Focus distance in meters, default is 1m.
    	float focalLength = 0.0f; // Focal length in millimeters, default is 35mm.
    	float focalTilt = 1.0f; // Focal tilt.
    	float fstop = 3.402823e+38f; // F-stop value in mm^-1, default is infinity.
    	float ipd = 0.063f; // Camera ipd
    	std::array<float, 2> lensShift = { 0.0f, 0.0f }; // Lens shift values (shiftX and shiftY).
    	std::array<float, 3> lookAt = { 0.0f, 0.0f, 0.0f }; // Camera look at.
    	float orthoHeight = 1.0f; // View volume height in meters.
    	float orthoWidth = 1.0f; // View volume width in meters.
    	std::array<float, 3> position = { 0.0f, 0.0f, 3.5f }; // Camera position.
    	std::array<float, 2> sensorSize = { 36.0f, 24.0f }; // Sensor width and height in millimeters.
    	std::array<float, 2> tiltCorrection = { 0.0f, 0.0f }; // Lens tilt correction values (tiltX and tiltY).
    	std::array<float, 3> up = { 0.0f, 0.0f, 1.0f }; // Camera up vector.
    };

    // Parses the specified glTF extension json string.
    bool ImportExtension(gltf::Extension& extensions, AMD_RPR_Camera& ext);

    // Serializes the specified glTF extension to a json string.
    bool ExportExtension(const AMD_RPR_Camera& ext, gltf::Extension& extensions);

} // End namespace amd