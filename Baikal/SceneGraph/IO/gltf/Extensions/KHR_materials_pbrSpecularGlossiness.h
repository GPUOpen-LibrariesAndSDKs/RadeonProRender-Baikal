#pragma once
#ifdef ENABLE_GLTF

#include "SceneGraph/IO/gltf/gltf2.h"

namespace khr
{
    // Reference to a texture.
    struct TextureInfo : gltf::glTFProperty
    {
    	int index = -1; // The index of the texture.
    	int texCoord = 0; // The set index of texture's TEXCOORD attribute used for texture coordinate mapping.
    };
    
    // glTF extension that defines the specular-glossiness material model from Physically-Based Rendering (PBR) methodology.
    struct KHR_Materials_PbrSpecularGlossiness : gltf::glTFProperty
    {
    	std::array<float, 4> diffuseFactor = { 1.0f, 1.0f, 1.0f, 1.0f }; // The reflected diffuse factor of the material.
    	TextureInfo diffuseTexture; // The diffuse texture.
    	std::array<float, 3> specularFactor = { 1.0f, 1.0f, 1.0f }; // The specular RGB color of the material.
    	float glossinessFactor = 1.0f; // The glossiness or smoothness of the material.
    	TextureInfo specularGlossinessTexture; // The specular-glossiness texture.
    };

    // Parses the specified glTF extension json string.
    bool ImportExtension(gltf::Extension& extensions, KHR_Materials_PbrSpecularGlossiness& ext);

    // Serializes the specified glTF extension to a json string.
    bool ExportExtension(const KHR_Materials_PbrSpecularGlossiness& ext, gltf::Extension& extensions);

} // End namespace khr

#endif //ENABLE_GLTF
