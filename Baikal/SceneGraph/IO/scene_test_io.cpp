#include "scene_io.h"
#include "image_io.h"
#include "../scene1.h"
#include "../shape.h"
#include "../material.h"
#include "../light.h"
#include "../texture.h"
#include "math/mathutils.h"

#include <vector>
#include <memory>

#define _USE_MATH_DEFINES
#include <math.h>

namespace Baikal
{
    // Create fake test IO
    class SceneIoTest : public SceneIo
    {
    public:
        // Load scene (this class uses filename to determine what scene to generate)
        std::unique_ptr<Scene1> LoadScene(std::string const& filename, std::string const& basepath) const override;
    };
    
    // Create test IO
    std::unique_ptr<SceneIo> SceneIo::CreateSceneIoTest()
    {
        return std::unique_ptr<SceneIo>(new SceneIoTest());
    }
    
    
    // Create spehere mesh
    Mesh* CreateSphere(std::uint32_t lat, std::uint32_t lon, float r, RadeonRays::float3 const& c)
    {
        auto num_verts = (lat - 2) * lon + 2;
        auto num_tris = (lat - 2) * (lon - 1 ) * 2;
        
        std::vector<RadeonRays::float3> vertices(num_verts);
        std::vector<RadeonRays::float3> normals(num_verts);
        std::vector<RadeonRays::float2> uvs(num_verts);
        std::vector<std::uint32_t> indices (num_tris * 3);

        
        auto t = 0U;
        for(auto j = 1U; j < lat - 1; j++)
            for(auto i = 0U; i < lon; i++)
            {
                float theta = float(j) / (lat - 1) * (float)M_PI; 
                float phi   = float(i) / (lon - 1 ) * (float)M_PI * 2;
                vertices[t].x =  r * sinf(theta) * cosf(phi) + c.x;
                vertices[t].y =  r * cosf(theta) + c.y;
                vertices[t].z = r * -sinf(theta) * sinf(phi) + c.z;
                normals[t].x = sinf(theta) * cosf(phi);
                normals[t].y = cosf(theta);
                normals[t].z = -sinf(theta) * sinf(phi);
                uvs[t].x = float(j) / (lat - 1);
                uvs[t].y = float(i) / (lon - 1);
                ++t;
            }
        
        vertices[t].x=c.x; vertices[t].y = c.y + r; vertices[t].z = c.z;
        normals[t].x=0; normals[t].y = 1; normals[t].z = 0;
        uvs[t].x=0; uvs[t].y = 0;
        ++t;
        vertices[t].x=c.x; vertices[t].y = c.y-r; vertices[t].z = c.z;
        normals[t].x=0; normals[t].y = -1; normals[t].z = 0;
        uvs[t].x=1; uvs[t].y = 1;
        ++t;

        t = 0U;
        for(auto j = 0U; j < lat - 3; j++)
            for(auto i = 0U; i < lon - 1; i++)
            {
                indices[t++] = j * lon + i;
                indices[t++] = (j + 1) * lon + i + 1;
                indices[t++] = j * lon + i + 1;
                
                indices[t++] = j * lon + i;
                indices[t++] = (j + 1) * lon + i;
                indices[t++] = (j + 1) * lon + i + 1;
            }
        
        for(auto i = 0U; i < lon - 1; i++)
        {
            indices[t++] = (lat - 2) * lon;
            indices[t++] = i;
            indices[t++] = i + 1;
            indices[t++] = (lat - 2) * lon + 1;
            indices[t++] = (lat - 3) * lon + i + 1;
            indices[t++] = (lat - 3) * lon + i;
        }
        
        auto mesh = new Mesh();
        mesh->SetVertices(&vertices[0], vertices.size());
        mesh->SetNormals(&normals[0], normals.size());
        mesh->SetUVs(&uvs[0], uvs.size());
        mesh->SetIndices(&indices[0], indices.size());

        return mesh;
    }
    
    
    // Create quad
    Mesh* CreateQuad(std::vector<RadeonRays::float3> const& vertices, bool flip)
    {
        using namespace RadeonRays;
        
        auto u1 = normalize(vertices[1] - vertices[0]);
        auto u2 = normalize(vertices[3] - vertices[0]);
        auto n = -cross(u1, u2);
        
        if (flip)
        {
            n = -n;
        }
        
        float3 normals[] = { n, n, n, n };
        
        float2 uvs[] =
        {
            float2(0, 0),
            float2(1, 0),
            float2(1, 1),
            float2(0, 1)
        };
        
        std::uint32_t indices[] =
        {
            0, 1, 2,
            0, 2, 3
        };
        
        auto mesh = new Mesh();
        mesh->SetVertices(&vertices[0], 4);
        mesh->SetNormals(normals, 4);
        mesh->SetUVs(uvs, 4);
        mesh->SetIndices(indices, 6);
     
        return mesh;
    }
    
    std::unique_ptr<Scene1> SceneIoTest::LoadScene(std::string const& filename, std::string const& basepath) const
    {
        using namespace RadeonRays;
        
        Scene1* scene(new Scene1);
        
        auto image_io(ImageIo::CreateImageIo());
        
        if (filename == "quad+ibl")
        {
            MeshPtr quad(CreateQuad(
            {
                RadeonRays::float3(-5, 0, -5),
                RadeonRays::float3(5, 0, -5),
                RadeonRays::float3(5, 0, 5),
                RadeonRays::float3(-5, 0, 5),
            }
            , false));
            
            scene->AttachShape(quad);
            
            TexturePtr ibl_texture(image_io->LoadImage("../Resources/Textures/studio015.hdr"));
            
            ImageBasedLightPtr ibl(new ImageBasedLight());
            ibl->SetTexture(TexturePtr(ibl_texture));
            ibl->SetMultiplier(1.f);
            scene->AttachLight(ibl);
            
            SingleBxdfPtr green(new SingleBxdf(SingleBxdf::BxdfType::kLambert));
            green->SetInputValue("albedo", float4(0.1f, 0.2f, 0.1f, 1.f));
            
            SingleBxdfPtr spec(new SingleBxdf(SingleBxdf::BxdfType::kMicrofacetGGX));
            spec->SetInputValue("albedo", float4(0.9f, 0.9f, 0.9f, 1.f));
            spec->SetInputValue("roughness", float4(0.002f, 0.002f, 0.002f, 1.f));
            
            MultiBxdfPtr mix(new MultiBxdf(MultiBxdf::Type::kFresnelBlend));
            mix->SetInputValue("base_material", green);
            mix->SetInputValue("top_material", spec);
            mix->SetInputValue("ior", float4(1.33f, 1.33f, 1.33f, 1.33f));
            
            quad->SetMaterial(mix);
        }
        else if (filename == "sphere+ibl")
        {
            MeshPtr mesh(CreateSphere(64, 32, 2.f, float3()));
            scene->AttachShape(mesh);
            
            TexturePtr ibl_texture(image_io->LoadImage("../Resources/Textures/studio015.hdr"));
            
            ImageBasedLightPtr ibl(new ImageBasedLight());
            ibl->SetTexture(TexturePtr(ibl_texture));
            ibl->SetMultiplier(1.f);
            scene->AttachLight(ibl);
            
            SingleBxdfPtr green(new SingleBxdf(SingleBxdf::BxdfType::kLambert));
            green->SetInputValue("albedo", float4(0.1f, 0.2f, 0.1f, 1.f));
            
            SingleBxdfPtr spec(new SingleBxdf(SingleBxdf::BxdfType::kMicrofacetGGX));
            spec->SetInputValue("albedo", float4(0.9f, 0.9f, 0.9f, 1.f));
            spec->SetInputValue("roughness", float4(0.002f, 0.002f, 0.002f, 1.f));
            
            MultiBxdfPtr mix(new MultiBxdf(MultiBxdf::Type::kFresnelBlend));
            mix->SetInputValue("base_material", green);
            mix->SetInputValue("top_material", spec);
            mix->SetInputValue("ior", float4(1.33f, 1.33f, 1.33f, 1.33f));

            mesh->SetMaterial(mix);
        }
        else if (filename == "sphere+plane+area")
        {
            MeshPtr mesh(CreateSphere(64, 32, 2.f, float3(0.f, 2.5f, 0.f)));
            scene->AttachShape(mesh);

            SingleBxdfPtr grey(new SingleBxdf(SingleBxdf::BxdfType::kMicrofacetGGX));
            grey->SetInputValue("albedo", float4(0.99f, 0.99f, 0.99f, 1.f));
            grey->SetInputValue("roughness", float4(0.025f, 0.025f, 0.025f, 1.f));

            MeshPtr floor(CreateQuad(
                                    {
                                        RadeonRays::float3(-8, 0, -8),
                                        RadeonRays::float3(8, 0, -8),
                                        RadeonRays::float3(8, 0, 8),
                                        RadeonRays::float3(-8, 0, 8),
                                    }
                                    , false));
            scene->AttachShape(floor);
            
            floor->SetMaterial(grey);
            mesh->SetMaterial(grey);

            SingleBxdfPtr emissive(new SingleBxdf(SingleBxdf::BxdfType::kEmissive));
            emissive->SetInputValue("albedo", 1.f * float4(3.1f, 3.f, 2.8f, 1.f));
            
            MeshPtr light(CreateQuad(
                                     {
                                         RadeonRays::float3(-2, 6, -2),
                                         RadeonRays::float3(2, 6, -2),
                                         RadeonRays::float3(2, 6, 2),
                                         RadeonRays::float3(-2, 6, 2),
                                     }
                                     , true));
            scene->AttachShape(light);
            
            light->SetMaterial(emissive);

            AreaLightPtr l1(new AreaLight(light, 0));
            AreaLightPtr l2(new AreaLight(light, 1));

            scene->AttachLight(l1);
            scene->AttachLight(l2);
        }
        else if (filename == "sphere+plane+ibl")
        {
            MeshPtr mesh(CreateSphere(64, 32, 2.f, float3(0.f, 2.2f, 0.f)));
            scene->AttachShape(mesh);

            SingleBxdfPtr refract(new SingleBxdf(SingleBxdf::BxdfType::kMicrofacetRefractionGGX));
            refract->SetInputValue("albedo", float4(0.7f, 1.f, 0.7f, 1.f));
            refract->SetInputValue("ior", float4(1.5f, 1.5f, 1.5f, 1.f));
            refract->SetInputValue("roughness", float4(0.02f, 0.02f, 0.02f, 1.f));


            SingleBxdfPtr spec(new SingleBxdf(SingleBxdf::BxdfType::kMicrofacetGGX));
            spec->SetInputValue("albedo", float4(0.7f, 1.f, 0.7f, 1.f));
            spec->SetInputValue("roughness", float4(0.02f, 0.02f, 0.02f, 1.f));

            MultiBxdfPtr mix(new MultiBxdf(MultiBxdf::Type::kFresnelBlend));
            mix->SetInputValue("base_material", refract);
            mix->SetInputValue("top_material", spec);
            mix->SetInputValue("ior", float4(1.5f, 1.5f, 1.5f, 1.5f));

            mesh->SetMaterial(mix);

            MeshPtr floor(CreateQuad(
                                     {
                                         RadeonRays::float3(-8, 0, -8),
                                         RadeonRays::float3(8, 0, -8),
                                         RadeonRays::float3(8, 0, 8),
                                         RadeonRays::float3(-8, 0, 8),
                                     }
                                     , false));
            scene->AttachShape(floor);
            
            TexturePtr ibl_texture(image_io->LoadImage("../Resources/Textures/sky.hdr"));
            
            ImageBasedLightPtr ibl(new ImageBasedLight());
            ibl->SetTexture(ibl_texture);
            ibl->SetMultiplier(3.f);
            scene->AttachLight(ibl);
        }
        else if (filename == "100spheres+plane+ibl+disney")
        {
            MeshPtr mesh(CreateSphere(64, 32, 0.9f, float3(0.f, 1.0f, 0.f)));
            scene->AttachShape(mesh);
            
            std::vector<std::string> params =
            {
                "metallic",
                "roughness",
                "anisotropy",
                "subsurface",
                "specular",
                "specular_tint",
                "clearcoat",
                "clearcoat_gloss",
                "sheen",
                "sheen_tint"
            };
            
            for (int i = 0; i < 10; ++i)
            {
                auto color = 0.5f * float3(rand_float(), rand_float(), rand_float()) +
                float3(0.5f, 0.5f, 0.5f);
                for (int j = 0; j < 10; ++j)
                {
                    DisneyBxdfPtr disney(new DisneyBxdf());
                    disney->SetInputValue("albedo", color);
                    
                    if (params[i] == "roughness")
                        disney->SetInputValue("metallic", float4(1.0f));
                    
                    if (params[i] == "metallic")
                        disney->SetInputValue("roughness", float4(0.2f));
                    
                    if (params[i] == "anisotropy")
                    {
                        disney->SetInputValue("roughness", float4(0.4f));
                        disney->SetInputValue("metallic", float4(0.75f));
                        disney->SetInputValue("specular", float4(0.f));
                        disney->SetInputValue("clearcoat", float4(0.f));
                    }
                    
                    if (params[i] == "subsurface")
                    {
                        disney->SetInputValue("roughness", float4(0.5f));
                        disney->SetInputValue("metallic", float4(0.f));
                        disney->SetInputValue("specular", float4(0.f));
                        disney->SetInputValue("clearcoat", float4(0.f));
                    }
                    
                    if (params[i] == "clearcoat" || params[i] == "clearcoat_gloss")
                    {
                        disney->SetInputValue("roughness", float4(0.0f));
                        disney->SetInputValue("metallic", float4(0.0f));
                        disney->SetInputValue("clearcoat", float4(1.0f));
                        disney->SetInputValue("clearcoat_gloss", float4(0.5f));
                        disney->SetInputValue("specular", float4(0.f));
                    }
                    
                    if (params[i] == "specular" || params[i] == "specular_tint")
                    {
                        disney->SetInputValue("roughness", float4(0.f));
                        disney->SetInputValue("metallic", float4(0.f));
                        disney->SetInputValue("clearcoat", float4(0.f));
                        disney->SetInputValue("specular", float4(1.f));
                    }
                    
                    if (params[i] == "sheen" || params[i] == "sheen_tint")
                    {
                        disney->SetInputValue("roughness", float4(0.f));
                        disney->SetInputValue("metallic", float4(0.0f));
                        disney->SetInputValue("clearcoat", float4(0.f));
                        disney->SetInputValue("specular", float4(0.f));
                    }
                    
                    float3 value = float3( j / 10.f, j / 10.f, j / 10.f);
                    disney->SetInputValue(params[i], value);
                    
                    InstancePtr instance(new Instance(mesh));
                    matrix t = RadeonRays::translation(float3(i * 2 - 9, 0.f, j * 2 - 9));
                    instance->SetTransform(t);
                    scene->AttachShape(instance);
                    
                    instance->SetMaterial(disney);
                }
            }
            
            
            MeshPtr floor(CreateQuad(
                                     {
                                         RadeonRays::float3(-15, 0, -15),
                                         RadeonRays::float3(15, 0, -15),
                                         RadeonRays::float3(15, 0, 15),
                                         RadeonRays::float3(-15, 0, 15),
                                     }
                                     , false));
            scene->AttachShape(floor);
            
            TexturePtr ibl_texture(image_io->LoadImage("../Resources/Textures/sky.hdr"));
            
            ImageBasedLightPtr ibl(new ImageBasedLight());
            ibl->SetTexture(ibl_texture);
            ibl->SetMultiplier(3.f);
            scene->AttachLight(ibl);
        }
        
        return std::unique_ptr<Scene1>(scene);
    }
}

