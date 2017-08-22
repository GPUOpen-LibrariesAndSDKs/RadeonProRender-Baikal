#include "scene_binary_io.h"
#include "SceneGraph/scene1.h"
#include "SceneGraph/iterator.h"
#include "SceneGraph/shape.h"
#include "SceneGraph/material.h"
#include "SceneGraph/light.h"
#include "SceneGraph/texture.h"
#include "SceneGraph/IO/image_io.h"
#include "math/mathutils.h"
#include "Utils/log.h"

#include <fstream>

namespace Baikal
{
    std::unique_ptr<SceneIo> SceneIo::CreateSceneIoBinary()
    {
        return std::unique_ptr<SceneIo>(new SceneBinaryIo());
    }

    std::unique_ptr<Scene1> SceneBinaryIo::LoadScene(std::string const& filename, std::string const& basepath) const
    {
        Scene1* scene = new Scene1;
        auto image_io(ImageIo::CreateImageIo());

        std::map<std::string, Material*> mats;

        struct less
        {
            bool operator ()(RadeonRays::float3 const& v1, RadeonRays::float3 const& v2) const {
                return v1.x < v2.x;
            }
        };

        std::map<RadeonRays::float3, Material*, less> c2mats;

        std::string full_path = filename;

        std::ifstream in(full_path, std::ios::binary | std::ios::in);

        if (!in)
        {
            throw std::runtime_error("Cannot open file for reading");
        }

        std::uint32_t num_meshes = 0;
        in.read((char*)&num_meshes, sizeof(std::uint32_t));

        LogInfo("Number of objects: ", num_meshes, "\n");

        for (auto i = 0U; i < num_meshes; ++i)
        {
            auto mesh = new Mesh();

            std::uint32_t num_indices = 0;
            in.read((char*)&num_indices, sizeof(std::uint32_t));

            std::uint32_t num_vertices = 0;
            in.read((char*)&num_vertices, sizeof(std::uint32_t));


            std::uint32_t num_normals = 0;
            in.read((char*)&num_normals, sizeof(std::uint32_t));


            std::uint32_t num_uvs = 0;
            in.read((char*)&num_uvs, sizeof(std::uint32_t));

            {
                std::vector<std::uint32_t> indices(num_indices);
                in.read((char*)&indices[0], num_indices * sizeof(std::uint32_t));

                mesh->SetIndices(&indices[0], num_indices);
            }

            {
                std::vector<RadeonRays::float3> vertices(num_vertices);
                in.read((char*)&vertices[0], num_vertices * sizeof(RadeonRays::float3));

                mesh->SetVertices(&vertices[0], num_vertices);
            }

            {
                std::vector<RadeonRays::float3> normals(num_normals);
                in.read((char*)&normals[0], num_normals * sizeof(RadeonRays::float3));

                mesh->SetNormals(&normals[0], num_normals);
            }

            {
                std::vector<RadeonRays::float2> uvs(num_uvs);
                in.read((char*)&uvs[0], num_uvs * sizeof(RadeonRays::float2));

                mesh->SetUVs(&uvs[0], num_uvs);
            }


            {
                std::uint32_t flag = 0;
                in.read(reinterpret_cast<char*>(&flag), sizeof(flag));

                if (!flag)
                {
                    RadeonRays::float3 albedo;
                    in.read(reinterpret_cast<char*>(&albedo.x), sizeof(RadeonRays::float3));

                    /*auto iter = c2mats.find(albedo);

                    if (iter != c2mats.cend())
                    {
                        material = iter->second;
                    }
                    else
                    {
                        material = new SingleBxdf(SingleBxdf::BxdfType::kLambert);
                        material->SetInputValue("albedo", albedo);
                        c2mats[albedo] = material;
                        scene->AttachAutoreleaseObject(material);
                    }*/
                }
                else
                {
                    std::uint32_t size = 0;
                    in.read(reinterpret_cast<char*>(&size), sizeof(size));

                    std::vector<char> buff(size);
                    in.read(&buff[0], sizeof(char) * size);

                    //std::string name(buff.cbegin(), buff.cend());

                    //auto iter = mats.find(name);

                    //if (iter != mats.cend())
                    //{
                    //    material = iter->second;
                    //}
                    //else
                    //{
                    //    LogInfo("Loading texture ", name, "\n");
                    //    auto texture = image_io->LoadImage(basepath + name);
                    //    //material = new SingleBxdf(SingleBxdf::BxdfType::kLambert);
                    //    //material->SetInputValue("albedo", texture);
                    //    //mats[name] = material;
                    //    //scene->AttachAutoreleaseObject(texture);
                    //    //scene->AttachAutoreleaseObject(material);
                    //}

                }

                mesh->SetMaterial(nullptr);
            }

            scene->AttachShape(ShapePtr(mesh));
        }



        TexturePtr ibl_texture = TexturePtr(image_io->LoadImage("../Resources/Textures/Canopus_Ground_4k.exr"));

        ImageBasedLight* ibl = new ImageBasedLight();
        ibl->SetTexture(ibl_texture);
        ibl->SetMultiplier(1.f);

        // TODO: temporary code to add directional light
        DirectionalLight* light = new DirectionalLight();
        light->SetDirection(RadeonRays::normalize(RadeonRays::float3(-1.1f, -0.6f, -0.4f)));
        light->SetEmittedRadiance(7.f * RadeonRays::float3(1.f, 0.95f, 0.92f));

        DirectionalLight* light1 = new DirectionalLight();
        light1->SetDirection(RadeonRays::float3(0.3f, -1.f, -0.5f));
        light1->SetEmittedRadiance(RadeonRays::float3(1.f, 0.8f, 0.65f));

        scene->AttachLight(LightPtr(light));
        //scene->AttachLight(light1);
        scene->AttachLight(LightPtr(ibl));

        return std::unique_ptr<Scene1>(scene);
    }

    void SceneBinaryIo::SaveScene(Scene1 const& scene, std::string const& filename, std::string const& basepath) const
    {
        std::string full_path = filename;

        std::ofstream out(full_path, std::ios::binary | std::ios::out);

        if (!out)
        {
            throw std::runtime_error("Cannot open file for writing");
        }

        auto default_material = MaterialPtr(new SingleBxdf(SingleBxdf::BxdfType::kLambert));

        auto num_shapes = (std::uint32_t)scene.GetNumShapes();
        out.write((char*)&num_shapes, sizeof(std::uint32_t));

        auto shape_iter = scene.CreateShapeIterator();

        for (; shape_iter->IsValid(); shape_iter->Next())
        {
            auto mesh = shape_iter->ItemAs<Mesh const>().lock();
            auto num_indices = (std::uint32_t)mesh->GetNumIndices();
            out.write((char*)&num_indices, sizeof(std::uint32_t));

            auto num_vertices = (std::uint32_t)mesh->GetNumVertices();
            out.write((char*)&num_vertices, sizeof(std::uint32_t));

            auto num_normals = (std::uint32_t)mesh->GetNumNormals();
            out.write((char*)&num_normals, sizeof(std::uint32_t));

            auto num_uvs = (std::uint32_t)mesh->GetNumUVs();
            out.write((char*)&num_uvs, sizeof(std::uint32_t));

            out.write((char const*)mesh->GetIndices(), mesh->GetNumIndices() * sizeof(std::uint32_t));
            out.write((char const*)mesh->GetVertices(), mesh->GetNumVertices() * sizeof(RadeonRays::float3));
            out.write((char const*)mesh->GetNormals(), mesh->GetNumNormals() * sizeof(RadeonRays::float3));
            out.write((char const*)mesh->GetUVs(), mesh->GetNumUVs() * sizeof(RadeonRays::float2));

            auto material = mesh->GetMaterial();

            if (!material)
            {
                material = default_material;
            }

            auto diffuse = dynamic_cast<SingleBxdf const*>(material.get());

            if (!diffuse)
            {
                diffuse = dynamic_cast<SingleBxdf const*>(material->GetInputValue("base_material").mat_value.get());
            }

            if (!diffuse)
            {
                throw std::runtime_error("Material not supported");
            }

            auto albedo = diffuse->GetInputValue("albedo");

            if (albedo.type == Material::InputType::kFloat4)
            {
                std::uint32_t flag = 0;
                out.write(reinterpret_cast<char const*>(&flag), sizeof(flag));
                out.write(reinterpret_cast<char const*>(&albedo.float_value.x), sizeof(RadeonRays::float3));
            }
            else
            {
                std::uint32_t flag = 1;

                auto name = albedo.tex_value->GetName();
                //LogInfo("Saving texture ", name, "\n");
                auto size = name.size();

                out.write(reinterpret_cast<char const*>(&flag), sizeof(flag));
                out.write(reinterpret_cast<char const*>(&size), sizeof(size));
                out.write(name.c_str(), sizeof(char) * size);
            }

        }
    }
}
