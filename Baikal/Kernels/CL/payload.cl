/**********************************************************************
Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
********************************************************************/
#ifndef PAYLOAD_CL
#define PAYLOAD_CL

#define TEXTURED_INPUT(x) union { struct { float4 value; } float_value; struct { int value[4]; } int_value; } x
#define TEXTURED_INPUT_HAS_TEXTURE(x) ((x).int_value.value[3] != -1)
#define TEXTURED_INPUT_GET_COLOR(x) ((x).float_value.value.xyz)

// Matrix
typedef struct
{
    float4 m0;
    float4 m1;
    float4 m2;
    float4 m3;
} matrix4x4;

// Camera
typedef struct
{
    // Coordinate frame
    float3 forward;
    float3 right;
    float3 up;
    // Position
    float3 p;

    // Image plane width & height in current units
    float2 dim;
    // Near and far Z
    float2 zcap;
    // Focal lenght
    float focal_length;
    // Camera aspect_ratio ratio
    float aspect_ratio;
    float focus_distance;
    float aperture;
} Camera;

// Shape description
typedef struct
{
    // Shape starting index
    int startidx;
    // Start vertex
    int startvtx;
    // Start material idx
    int material_idx;
    // Number of primitives in the shape
    int volume_idx;
    // Linear motion vector
    float3 linearvelocity;
    // Angular velocity
    float4 angularvelocity;
    // Transform in row major format
    matrix4x4 transform;
    // unique shape id
    int id;
    // Follow fields for 16 byte allign
    int offset[3];
} Shape;

typedef enum
{
    kFloat3 = 0,
    kFloat = 1,
    kInt = 2
} InputMapDataType;

// Input data for input maps
typedef struct _InputMapData
{
    union
    {
        struct
        {
            float3 value;
        } float_value;
        struct
        {
            int idx;
            int placeholder[2];
            int type; //We can use it since float3 is actually float4
        } int_values;
    };
} InputMapData;

enum Bxdf
{
    kZero,
    kLambert,
    kIdealReflect,
    kIdealRefract,
    kMicrofacetBeckmann,
    kMicrofacetGGX,
    kLayered,
    kFresnelBlend,
    kMix,
    kEmissive,
    kPassthrough,
    kTranslucent,
    kMicrofacetRefractionGGX,
    kMicrofacetRefractionBeckmann,
    kDisney,
    kUberV2
};

enum UberMaterialLayers
{
    kEmissionLayer = 0x1,
    kTransparencyLayer = 0x2,
    kCoatingLayer = 0x4,
    kReflectionLayer = 0x8,
    kDiffuseLayer = 0x10,
    kRefractionLayer = 0x20,
    kSSSLayer = 0x40,
    kShadingNormalLayer = 0x80
};

// Material description
typedef struct _Material
{
    union
    {
        struct
        {
            float4 kx;
            float ni;
            float ns;
            int kxmapidx;
            int nsmapidx;
            float fresnel;
            int padding0[3];
        } simple;

        struct
        {
            float weight;
            int weight_map_idx;
            int top_brdf_idx;
            int base_brdf_idx;
            int padding1[8];
        } compound;

        struct
        {
            float4 base_color;

            float metallic;
            float subsurface;
            float specular;
            float roughness;

            float specular_tint;
            float anisotropy;
            float sheen;
            float sheen_tint;

            float clearcoat;
            float clearcoat_gloss;
            int base_color_map_idx;
            int metallic_map_idx;

            int specular_map_idx;
            int roughness_map_idx;
            int specular_tint_map_idx;
            int anisotropy_map_idx;

            int sheen_map_idx;
            int sheen_tint_map_idx;
            int clearcoat_map_idx;
            int clearcoat_gloss_map_idx;
        } disney;
#ifdef ENABLE_UBERV2
        struct
        {
            //material inputs
            int diffuse_color_input_id;
            int reflection_color_input_id;
            int reflection_roughness_input_id;
            int reflection_anisotropy_input_id;

            int reflection_anisotropy_rotation_input_id;
            int reflection_ior_input_id;
            int reflection_metalness_input_id;
            int coating_color_input_id;

            int coating_ior_input_id;
            int refraction_color_input_id;
            int refraction_roughness_input_id;
            int refraction_ior_input_id;

            int emission_color_input_id;
            int transparency_input_id;
            int sss_absorption_color_input_id;
            int sss_scatter_color_input_id;

            int sss_subsurface_color_input_id;
            int sss_absorption_distance_input_id;
            int sss_scatter_distance_input_id;
            int sss_scatter_direction_input_id;

            // Normal mapping
            int shading_normal_input_id;
            //Material parameters
            int layers;
            int refraction_ior_mode;
            int refraction_thin_surface;

            int emission_mode;
            int sss_multiscatter;
            int padding[2];

/*            float displacement;
            int displacement_map_idx;*/
        } uberv2;
#endif
    };

    int type;
    int bump_flag;
    int thin;
    int nmapidx;
    int bxdf_flags;
    int padding[3];
} Material;

enum LightType
{
    kPoint = 0x1,
    kDirectional,
    kSpot,
    kArea,
    kIbl
};

typedef struct
{
    union
    {
        // Area light
        struct
        {
            int id;
            int shapeidx;
            int primidx;
            int matidx;
            int padding0;
            // offset for 16 byte alligment
            int offset[3];
        };

        // IBL
        struct
        {
            int tex;
            int tex_reflection;
            int tex_refraction;
            int tex_transparency;
        };

        // Spot
        struct
        {
            float ia;
            float oa;
            float f;
            int padding2;
        };
    };

    float3 p;
    float3 d;
    float3 intensity;
    int type;
    float multiplier;
    int tex_background;
    int padding;
} Light;

typedef enum
    {
        kEmpty,
        kHomogeneous,
        kHeterogeneous
    } VolumeType;


typedef struct _Volume
{
    VolumeType type;
    float g;

    // Id of volume data if present
    int data;
    int extra;

    // Absorbtion
    TEXTURED_INPUT(sigma_a);
    // Scattering
    TEXTURED_INPUT(sigma_s);
    // Emission
    TEXTURED_INPUT(sigma_e);
} Volume;

/// Supported formats
enum TextureFormat
{
    UNKNOWN,
    RGBA8,
    RGBA16,
    RGBA32
};

/// Texture description
typedef
struct _Texture
{
    // Width, height and depth
    int w;
    int h;
    int d;
    // Offset in texture data array
    int dataoffset;
    // Format
    int fmt;
    int extra;
} Texture;


// Hit data
typedef struct _DifferentialGeometry
{
    // World space position
    float3 p;
    // Shading normal
    float3 n;
    // Geo normal
    float3 ng;
    // UVs
    float2 uv;
    // Derivatives
    float3 dpdu;
    float3 dpdv;
    float  area;

    matrix4x4 world_to_tangent;
    matrix4x4 tangent_to_world;

    // Material
    Material mat;
    int material_index;
    int transfer_mode;
} DifferentialGeometry;










#endif // PAYLOAD_CL
