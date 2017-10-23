#pragma once
#include "gltf2.h"

namespace amd
{
    // A Radeon ProRender material node input property.
    struct Input
    {
    	enum class Type
    	{
    		FLOAT4 = 0,
    		UINT = 1,
    		NODE = 2,
    		IMAGE = 3,
    	};
    
    	std::string name; // The user-defined name of this object.
    	Type type; // The input value type.
    	union
    	{
    		int integer;
    		std::array<float, 4> array;
    	} value; // The input value.
    };
    
    // A Radeon ProRender material node or uber material.
    struct Node
    {
    	enum class Type
    	{
    		UBER = 0,
    		DIFFUSE = 1,
    		MICROFACET = 2,
    		REFLECTION = 3,
    		REFRACTION = 4,
    		MICROFACET_REFRACTION = 5,
    		TRANSPARENT = 6,
    		EMISSIVE = 7,
    		WARD = 8,
    		ADD = 9,
    		BLEND = 10,
    		ARITHMETIC = 11,
    		FRESNEL = 12,
    		NORMAL_MAP = 13,
    		IMAGE_TEXTURE = 14,
    		NOISE2D_TEXTURE = 15,
    		DOT_TEXTURE = 16,
    		GRADIENT_TEXTURE = 17,
    		CONSTANT_TEXTURE = 18,
    		INPUT_LOOKUP = 19,
    		STANDARD  = 20,
    		BLEND_VALUE = 21,
    		PASSTHROUGH = 22,
    		ORENNAYAR = 23,
    		FRESNEL_SCHLICK = 24,
    		DIFFUSE_REFRACTION = 25,
    		BUMP_MAP = 26,
    		VOLUME = 27,
    		MICROFACET_ANISOTROPIC_REFLECTION = 28,
    		MICROFACET_ANISOTROPIC_REFRACTION = 29,
    		TWOSIDED = 30,
    		UV_PROJECT = 31,
    	};
    
    	std::string name; // The user-defined name of this object.
    	Type type; // The material node type.
    	std::vector<Input> inputs; // RPR material inputs
    };
    
    // AMD Radeon ProRender material glTF extension
    struct AMD_RPR_Material
    {
    	std::vector<Node> nodes; // An array of AMD Radeon ProRender material nodes.
    };

    // Parses the specified glTF extension json string.
    bool ImportExtension(gltf::Extension& extensions, AMD_RPR_Material& ext);

    // Serializes the specified glTF extension to a json string.
    bool ExportExtension(const AMD_RPR_Material& ext, gltf::Extension& extensions);

} // End namespace amd