#include "scene_io.h"
#include "image_io.h"
#include "../scene1.h"
#include "../shape.h"
#include "../material.h"
#include "../light.h"
#include "../texture.h"
#include "math/mathutils.h"

#include <string>
#include <map>

#include "Utils/tiny_obj_loader.h"
#include "Utils/log.h"

namespace Baikal
{
    // Obj scene loader
    class SceneIoObj : public SceneIo
    {
    public:
        // Load scene from file
        std::unique_ptr<Scene1> LoadScene(std::string const& filename, std::string const& basepath) const override;
    private:
        Material const* TranslateMaterial(ImageIo const& image_io, tinyobj::material_t const& mat, std::string const& basepath, Scene1& scene) const;

        mutable std::map<std::string, Material const*> m_material_cache;
    };

    std::unique_ptr<SceneIo> SceneIo::CreateSceneIoObj()
    {
        return std::unique_ptr<SceneIo>(new SceneIoObj());
    }

    Texture const* SceneIo::LoadTexture(ImageIo const& io, Scene1& scene, std::string const& basepath, std::string const& name) const
    {
        auto iter = m_texture_cache.find(name);

        if (iter != m_texture_cache.cend())
        {
            return iter->second;
        }
        else
        {
            LogInfo("Loading ", name, "\n");
            auto texture = io.LoadImage(basepath + name);
            texture->SetName(name);
            scene.AttachAutoreleaseObject(texture);
            m_texture_cache[name] = texture;
            return texture;
        }
    }

    Material const* SceneIoObj::TranslateMaterial(ImageIo const& image_io, tinyobj::material_t const& mat, std::string const& basepath, Scene1& scene) const
    {
        auto iter = m_material_cache.find(mat.name);

        if (iter != m_material_cache.cend())
        {
            return iter->second;
        }
        else
        {
            RadeonRays::float3 emission(mat.emission[0], mat.emission[1], mat.emission[2]);

            Material* material = nullptr;

            // Check if this is emissive
            if (emission.sqnorm() > 0)
            {
                // If yes create emissive brdf
                material = new SingleBxdf(SingleBxdf::BxdfType::kEmissive);

                // Set albedo
                if (!mat.diffuse_texname.empty())
                {
                    auto texture = LoadTexture(image_io, scene, basepath, mat.diffuse_texname);
                    material->SetInputValue("albedo", texture);
                }
                else
                {
                    material->SetInputValue("albedo", emission);
                }
            }
            else
            {
                auto s = RadeonRays::float3(mat.specular[0], mat.specular[1], mat.specular[2]);

                if ((s.sqnorm() > 0 || !mat.specular_texname.empty()))
                {
                    // Otherwise create lambert
                    material = new MultiBxdf(MultiBxdf::Type::kFresnelBlend);
                    material->SetInputValue("ior", RadeonRays::float4(1.5f, 1.5f, 1.5f, 1.5f));

                    Material* diffuse = new SingleBxdf(SingleBxdf::BxdfType::kLambert);
                    Material* specular = new SingleBxdf(SingleBxdf::BxdfType::kMicrofacetGGX);

                    specular->SetInputValue("roughness", 0.01f);

                    // Set albedo
                    if (!mat.diffuse_texname.empty())
                    {
                        auto texture = LoadTexture(image_io, scene, basepath, mat.diffuse_texname);
                        diffuse->SetInputValue("albedo", texture);
                    }
                    else
                    {
                        diffuse->SetInputValue("albedo", RadeonRays::float3(mat.diffuse[0], mat.diffuse[1], mat.diffuse[2]));
                    }

                    // Set albedo
                    if (!mat.specular_texname.empty())
                    {
                        auto texture = LoadTexture(image_io, scene, basepath, mat.specular_texname);
                        specular->SetInputValue("albedo", texture);
                    }
                    else
                    {
                        specular->SetInputValue("albedo", s);
                    }

                    // Set normal
                    if (!mat.bump_texname.empty())
                    {
                        auto texture = LoadTexture(image_io, scene, basepath, mat.bump_texname);
                        diffuse->SetInputValue("normal", texture);
                        specular->SetInputValue("normal", texture);
                    }

                    diffuse->SetName(mat.name + "-diffuse");
                    specular->SetName(mat.name + "-specular");

                    material->SetInputValue("base_material", diffuse);
                    material->SetInputValue("top_material", specular);

                    scene.AttachAutoreleaseObject(diffuse);
                    scene.AttachAutoreleaseObject(specular);
                }
                else
                {
                    // Otherwise create lambert
                    Material* diffuse = new SingleBxdf(SingleBxdf::BxdfType::kLambert);

                    // Set albedo
                    if (!mat.diffuse_texname.empty())
                    {
                        auto texture = LoadTexture(image_io, scene, basepath, mat.diffuse_texname);
                        diffuse->SetInputValue("albedo", texture);
                    }
                    else
                    {
                        diffuse->SetInputValue("albedo", RadeonRays::float3(mat.diffuse[0], mat.diffuse[1], mat.diffuse[2]));
                    }

                    // Set normal
                    if (!mat.bump_texname.empty())
                    {
                        auto texture = LoadTexture(image_io, scene, basepath, mat.bump_texname);
                        diffuse->SetInputValue("normal", texture);
                    }

                    material = diffuse;
                }
            }

            // Set material name
            material->SetName(mat.name);

            m_material_cache[mat.name] = material;

            scene.AttachAutoreleaseObject(material);

            return material;
        }
    }

    std::unique_ptr<Scene1> SceneIoObj::LoadScene(std::string const& filename, std::string const& basepath) const
    {
        using namespace tinyobj;

        auto image_io(ImageIo::CreateImageIo());

        // Loader data
        std::vector<shape_t> objshapes;
        std::vector<material_t> objmaterials;

        // Try loading file
        LogInfo("Loading a scene from OBJ: ", filename, " ... ");
        std::string err;
        auto res = LoadObj(objshapes, objmaterials, err, filename.c_str(), basepath.c_str());
        if (!res)
        {
            throw std::runtime_error(err);
        }
        LogInfo("Success\n");

        // Allocate scene
        Scene1* scene(new Scene1);

        // Enumerate and translate materials
        // Keep track of emissive subset
        std::set<Material const*> emissives;
        std::vector<Material const*> materials(objmaterials.size());
        for (int i = 0; i < (int)objmaterials.size(); ++i)
        {
            // Translate material
            materials[i] = TranslateMaterial(*image_io, objmaterials[i], basepath, *scene);

            // Add to emissive subset if needed
            if (materials[i]->HasEmission())
            {
                emissives.insert(materials[i]);
            }
        }

        // Enumerate all shapes in the scene
        for (int s = 0; s < (int)objshapes.size(); ++s)
        {
            // Create empty mesh
            Mesh* mesh = new Mesh();

            // Set vertex and index data
            auto num_vertices = objshapes[s].mesh.positions.size() / 3;
            mesh->SetVertices(&objshapes[s].mesh.positions[0], num_vertices);

            auto num_normals = objshapes[s].mesh.normals.size() / 3;
            mesh->SetNormals(&objshapes[s].mesh.normals[0], num_normals);

            auto num_uvs = objshapes[s].mesh.texcoords.size() / 2;

            // If we do not have UVs, generate zeroes
            if (num_uvs)
            {
                mesh->SetUVs(&objshapes[s].mesh.texcoords[0], num_uvs);
            }
            else
            {
                std::vector<RadeonRays::float2> zero(num_vertices);
                std::fill(zero.begin(), zero.end(), RadeonRays::float2(0, 0));
                mesh->SetUVs(&zero[0], num_vertices);
            }

            // Set indices
            auto num_indices = objshapes[s].mesh.indices.size();
            mesh->SetIndices(reinterpret_cast<std::uint32_t const*>(&objshapes[s].mesh.indices[0]), num_indices);

            // Set material

            for (int i = 0; i < objshapes[s].mesh.material_ids.size() - 1; ++i)
            {
                if (objshapes[s].mesh.material_ids[i] != objshapes[s].mesh.material_ids[i + 1])
                    LogInfo("Warning: Group detected\n");
            }

            auto idx = objshapes[s].mesh.material_ids[0];

            if (idx >= 0)
            {
                mesh->SetMaterial(materials[idx]);
            }

            // Attach to the scene
            scene->AttachShape(mesh);

            // Attach for autorelease
            scene->AttachAutoreleaseObject(mesh);

            // If the mesh has emissive material we need to add area light for it
            if (idx >= 0 && emissives.find(materials[idx]) != emissives.cend())
            {
                // Add area light for each polygon of emissive mesh
                for (int l = 0; l < mesh->GetNumIndices() / 3; ++l)
                {
                    AreaLight* light = new AreaLight(mesh, l);
                    scene->AttachLight(light);
                    scene->AttachAutoreleaseObject(light);
                }
            }
        }

        // TODO: temporary code, add IBL
        Texture* ibl_texture = image_io->LoadImage("../Resources/Textures/studio015.hdr");
        scene->AttachAutoreleaseObject(ibl_texture);

        ImageBasedLight* ibl = new ImageBasedLight();
        ibl->SetTexture(ibl_texture);
        ibl->SetMultiplier(1.f);
        scene->AttachAutoreleaseObject(ibl);

        // TODO: temporary code to add directional light
        DirectionalLight* light = new DirectionalLight();
        light->SetDirection(RadeonRays::normalize(RadeonRays::float3(-1.1f, -0.6f, -0.2f)));
        light->SetEmittedRadiance(1.f * RadeonRays::float3(1.f, 0.95f, 0.92f));
        scene->AttachAutoreleaseObject(light);

        DirectionalLight* light1 = new DirectionalLight();
        light1->SetDirection(RadeonRays::float3(0.3f, -1.f, -0.5f));
        light1->SetEmittedRadiance(RadeonRays::float3(1.f, 0.8f, 0.65f));
        scene->AttachAutoreleaseObject(light1);

        scene->AttachLight(light);
        //scene->AttachLight(light1);
        scene->AttachLight(ibl);

        return std::unique_ptr<Scene1>(scene);
    }
}
