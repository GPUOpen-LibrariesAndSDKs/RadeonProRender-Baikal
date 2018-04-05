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

#include "scene_io.h"
#include "image_io.h"
#include "SceneGraph/scene1.h"
#include "SceneGraph/shape.h"
#include "SceneGraph/material.h"
#include "SceneGraph/light.h"
#include "SceneGraph/texture.h"
#include "math/mathutils.h"

#include <string>
#include <map>
#include <set>
#include <cassert>

#include "Utils/tiny_obj_loader.h"
#include "Utils/log.h"

namespace Baikal
{
    // Obj scene loader
    class SceneIoObj : public SceneIo::Loader
    {
    public:
        // Load scene from file
        Scene1::Ptr LoadScene(std::string const& filename, std::string const& basepath) const override;
        SceneIoObj() : SceneIo::Loader("obj", this)
        {
            SceneIo::RegisterLoader("objm", this);
        }
        ~SceneIoObj()
        {
            SceneIo::UnregisterLoader("objm");
        }

    private:
        Material::Ptr TranslateMaterial(ImageIo const& image_io, tinyobj::material_t const& mat, std::string const& basepath, Scene1& scene) const;

        mutable std::map<std::string, Material::Ptr> m_material_cache;
    };

    // Create static object to register loader. This object will be used as loader
    static SceneIoObj obj_loader;


    Material::Ptr SceneIoObj::TranslateMaterial(ImageIo const& image_io, tinyobj::material_t const& mat, std::string const& basepath, Scene1& scene) const
    {
        auto iter = m_material_cache.find(mat.name);

        if (iter != m_material_cache.cend())
        {
            return iter->second;
        }
        else
        {
            RadeonRays::float3 emission(mat.emission[0], mat.emission[1], mat.emission[2]);

            Material::Ptr material = nullptr;

            // Check if this is emissive
            if (emission.sqnorm() > 0)
            {
                // If yes create emissive brdf
                material = SingleBxdf::Create(SingleBxdf::BxdfType::kEmissive);

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
                    material = MultiBxdf::Create(MultiBxdf::Type::kFresnelBlend);
                    material->SetInputValue("ior", RadeonRays::float4(1.5f, 1.5f, 1.5f, 1.5f));

                    auto diffuse = SingleBxdf::Create(SingleBxdf::BxdfType::kLambert);
                    auto specular = SingleBxdf::Create(SingleBxdf::BxdfType::kMicrofacetGGX);

                    specular->SetInputValue("roughness", RadeonRays::float4(0.01f));

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
                        diffuse->SetInputValue("bump", texture);
                        specular->SetInputValue("bump", texture);
                    }

                    diffuse->SetName(mat.name + "-diffuse");
                    specular->SetName(mat.name + "-specular");

                    material->SetInputValue("base_material", diffuse);
                    material->SetInputValue("top_material", specular);
                }
                else
                {
                    // Otherwise create lambert
                    auto diffuse = SingleBxdf::Create(SingleBxdf::BxdfType::kLambert);

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
                        diffuse->SetInputValue("bump", texture);
                    }

                    material = diffuse;
                }
            }

            // Set material name
            material->SetName(mat.name);
            //material->SetThin(true);

            m_material_cache.emplace(std::make_pair(mat.name, material));

            return material;
        }
    }

    Scene1::Ptr SceneIoObj::LoadScene(std::string const& filename, std::string const& basepath) const
    {
        using namespace tinyobj;

        auto image_io(ImageIo::CreateImageIo());

        // Loader data
        std::vector<shape_t> objshapes;
        std::vector<material_t> objmaterials;

        // Try loading file
        LogInfo("Loading a scene from OBJ: ", filename, " ... ");
        std::string err;
        auto res = LoadObj(objshapes, objmaterials, err, filename.c_str(), basepath.c_str(), triangulation|calculate_normals);
        if (!res)
        {
            throw std::runtime_error(err);
        }
        LogInfo("Success\n");

        // Allocate scene
        auto scene = Scene1::Create();

        // Enumerate and translate materials
        // Keep track of emissive subset
        std::set<Material::Ptr> emissives;
        std::vector<Material::Ptr> materials(objmaterials.size());
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
            const auto& shape = objshapes[s];

            // Find all materials used by this shape.
            std::set<int> used_materials(std::begin(shape.mesh.material_ids), std::end(shape.mesh.material_ids));

            // Split the mesh into multiple meshes, each with only one material.
            for (int used_material : used_materials)
            {
                // Map from old index to new index.
                std::map<unsigned int, unsigned int> used_indices;

                // Remapped indices.
                std::vector<unsigned int> indices;

                // Collected vertex/normal/texcoord data.
                std::vector<float> vertices, normals, texcoords;

                // Go through each face in the mesh.
                for (size_t i = 0; i < shape.mesh.material_ids.size(); ++i)
                {
                    // Skip faces which don't use the current material.
                    if (shape.mesh.material_ids[i] != used_material) continue;

                    const int num_face_vertices = shape.mesh.num_vertices[i];
                    assert(num_face_vertices == 3 && "expected triangles");
                    // For each vertex index of this face.
                    for (int j = 0; j < num_face_vertices; ++j)
                    {
                        const unsigned int old_index = shape.mesh.indices[num_face_vertices * i + j];
                        // Collect vertex/normal/texcoord data. Avoid inserting the same data twice.
                        auto result = used_indices.emplace(old_index, (unsigned int)(vertices.size() / 3));
                        if (result.second) // Did insert?
                        {
                            // Push the new data.
                            for (int k = 0; k < 3; ++k)
                                vertices.push_back(shape.mesh.positions[3 * old_index + k]);
                            for (int k = 0; k < 3; ++k)
                                normals.push_back(shape.mesh.normals[3 * old_index + k]);
                            if (!shape.mesh.texcoords.empty())
                                for (int k = 0; k < 2; ++k)
                                    texcoords.push_back(shape.mesh.texcoords[2 * old_index + k]);
                        }
                        const unsigned int new_index = result.first->second;
                        indices.push_back(new_index);
                    }
                }

                // Create empty mesh
                auto mesh = Mesh::Create();

                // Set vertex and index data
                auto num_vertices = vertices.size() / 3;
                mesh->SetVertices(&vertices[0], num_vertices);

                auto num_normals = normals.size() / 3;
                mesh->SetNormals(&normals[0], num_normals);

                auto num_uvs = texcoords.size() / 2;

                // If we do not have UVs, generate zeroes
                if (num_uvs)
                {
                    mesh->SetUVs(&texcoords[0], num_uvs);
                }
                else
                {
                    std::vector<RadeonRays::float2> zero(num_vertices);
                    std::fill(zero.begin(), zero.end(), RadeonRays::float2(0, 0));
                    mesh->SetUVs(&zero[0], num_vertices);
                }

                // Set indices
                auto num_indices = indices.size();
                mesh->SetIndices(reinterpret_cast<std::uint32_t const*>(&indices[0]), num_indices);

                // Set material

                if (used_material >= 0)
                {
                    mesh->SetMaterial(materials[used_material]);
                }

                // Attach to the scene
                scene->AttachShape(mesh);

                // If the mesh has emissive material we need to add area light for it
                if (used_material >= 0 && emissives.find(materials[used_material]) != emissives.cend())
                {
                    // Add area light for each polygon of emissive mesh
                    for (int l = 0; l < mesh->GetNumIndices() / 3; ++l)
                    {
                        auto light = AreaLight::Create(mesh, l);
                        scene->AttachLight(light);
                    }
                }
            }
        }

        // TODO: temporary code, add IBL
        auto ibl_texture = image_io->LoadImage("../Resources/Textures/studio015.hdr");
        //auto ibl_texture1 = image_io->LoadImage("../Resources/Textures/sky.hdr");

        auto ibl = ImageBasedLight::Create();
        ibl->SetTexture(ibl_texture);
        //ibl->SetReflectionTexture(ibl_texture1);
        //ibl->SetBackgroundTexture(ibl_texture1);
        ibl->SetMultiplier(1.f);

        // TODO: temporary code to add directional light
        auto light = DirectionalLight::Create();
        light->SetDirection(RadeonRays::normalize(RadeonRays::float3(-1.1f, -0.6f, -0.2f)));
        light->SetEmittedRadiance(30.f * RadeonRays::float3(1.f, 0.95f, 0.92f));

        auto light1 = DirectionalLight::Create();
        auto d = RadeonRays::float3(-0.1f, -1.f, -1.f);
        d.normalize();
        light1->SetDirection(d);
        light1->SetEmittedRadiance(3.f * RadeonRays::float3(1.f, 0.8f, 0.65f));

        //scene->AttachLight(light);
        //scene->AttachLight(light1);
        scene->AttachLight(ibl);

        return scene;
    }


}
