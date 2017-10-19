#ifdef ENABLE_GLTF

#pragma once
#include "SceneGraph/IO/gltf/gltf2.h"

namespace amd
{
    // Point light properties.
    struct Light_Point
    {
    	std::array<float, 3> radiantPower = { 1.0f, 1.0f, 1.0f }; // Radiant power.
    };
    
    // Directional light properties.
    struct Light_Directional
    {
    	std::array<float, 3> radiantPower = { 1.0f, 1.0f, 1.0f }; // Radiant power.
    	float shadowSoftness = 0.0f; // Softness of cast shadows.
    };
    
    // Spot light properties.
    struct Light_Spot
    {
    	std::array<float, 3> radiantPower = { 1.0f, 1.0f, 1.0f }; // Radiant power.
    	float innerAngle = 0.0f; // Inner angle of the cone in radians.
    	float outerAngle = 0.0f; // Outer angle of the cone in radians.
    };
    
    // Environment light properties.
    struct Light_Environment
    {
    	int image = -1; // Image index for environment light
    	float intensityScale = 1.0f; // Environment light intensity scale.
    };
    
    // Sky light properties.
    struct Light_Sky
    {
    	float turbidity = 1.0f; // Sky light turbidity value.
    	float albedo = 1.0f; // Sky light albedo value.
    	float scale = 1.0f; // Sky light scale value.
    };
    
    // IES light properties.
    struct Light_Ies
    {
    	int buffer = -1; // Buffer index for IES light data
    	int nx = 0; // Desired resolution of the output image along the x-axis.
    	int ny = 0; // Desired resolution of the output image along the y-axis.
    	std::array<float, 3> radiantPower = { 1.0f, 1.0f, 1.0f }; // Radiant power.
    };
    
    // A point, spot, directional, ies, sky or environment light.
    struct Light : gltf::glTFChildOfRootProperty
    {
    	enum class Type
    	{
    		POINT = 0,
    		DIRECTIONAL = 1,
    		SPOT = 2,
    		ENVIRONMENT = 3,
    		SKY = 4,
    		IES = 5,
    	};
    
    	Light_Point point;
    	Light_Directional directional;
    	Light_Spot spot;
    	Light_Environment environment;
    	Light_Sky sky;
    	Light_Ies ies;
    	std::array<float, 16> transform = { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f }; // Light transform matrix.
    	Type type; // The light type.
    };
    
    // AMD Radeon ProRender lights glTF extension
    struct AMD_RPR_Lights
    {
    	std::vector<Light> lights; // An array of AMD Radeon ProRender lights.
    };

    // Parses the specified glTF extension json string.
    bool ImportExtension(gltf::Extension& extensions, AMD_RPR_Lights& ext);

    // Serializes the specified glTF extension to a json string.
    bool ExportExtension(const AMD_RPR_Lights& ext, gltf::Extension& extensions);

} // End namespace amd

#endif //ENABLE_GLTF
