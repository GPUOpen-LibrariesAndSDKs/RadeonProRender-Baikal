#pragma once
#include "gltf2.h"

namespace amd
{
    // Tonemap post effect.
    struct Tonemap
    {
    };
    
    // White balance post effect.
    struct WhiteBalance
    {
    	enum class ColorSpace
    	{
    		SRGB = 1,
    		ADOBE_RGB = 2,
    		REC2020 = 3,
    		DCIP3 = 4,
    	};
    
    	ColorSpace colorSpace; // Color space to mode.
    	float colorTemperature = 6500.0f; // Color temperature value.
    };
    
    // Simple tonemap post effect.
    struct SimpleTonemap
    {
    	float exposure = 0.0f; // Exposure value.
    	float contrast = 1.0f; // Contrast value.
    	int enableTonemap = 0; // Enable tonemap operator.
    };
    
    // Normalization post effect.
    struct Normalization
    {
    };
    
    // Gamma correction post effect.
    struct GammaCorrection
    {
    };
    
    // A Radeon ProRender post effect.
    struct PostEffect : gltf::glTFChildOfRootProperty
    {
    	enum class Type
    	{
    		TONE_MAP = 0,
    		WHITE_BALANCE = 1,
    		SIMPLE_TONEMAP = 2,
    		NORMALIZATION = 3,
    		GAMMA_CORRECTION = 4,
    	};
    
    	Tonemap tonemap;
    	WhiteBalance whiteBalance;
    	SimpleTonemap simpleTonemap;
    	Normalization normalization;
    	GammaCorrection gammaCorrection;
    	Type type; // The post effect type.
    };
    
    // AMD Radeon ProRender post effects glTF extension
    struct AMD_RPR_Post_Effects
    {
    	std::vector<PostEffect> postEffects; // An array of AMD Radeon ProRender post effects.
    };

    // Parses the specified glTF extension json string.
    bool ImportExtension(gltf::Extension& extensions, AMD_RPR_Post_Effects& ext);

    // Serializes the specified glTF extension to a json string.
    bool ExportExtension(const AMD_RPR_Post_Effects& ext, gltf::Extension& extensions);

} // End namespace amd