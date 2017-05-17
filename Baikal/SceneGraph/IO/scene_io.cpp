#include "scene_io.h"
#include "image_io.h"
#include "../scene1.h"
#include "../shape.h"
#include "../material.h"
#include "../light.h"
#include "../texture.h"


#include "Utils/tiny_obj_loader.h"

namespace Baikal
{
    // Obj scene loader
    class SceneIoObj : public SceneIo
    {
    public:
        // Load scene from file
        Scene1* LoadScene(std::string const& filename, std::string const& basepath) const override;
    private:
        Material const* TranslateMaterial(ImageIo const& image_io, tinyobj::material_t const& mat, std::string const& basepath, Scene1& scene) const;

    };

    SceneIo* SceneIo::CreateSceneIoObj()
    {
        return new SceneIoObj();
    }


    Material const* SceneIoObj::TranslateMaterial(ImageIo const& image_io, tinyobj::material_t const& mat, std::string const& basepath, Scene1& scene) const
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
                auto texture = image_io.LoadImage(basepath + mat.diffuse_texname);
                material->SetInputValue("albedo", texture);
                scene.AttachAutoreleaseObject(texture);
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
                    auto texture = image_io.LoadImage(basepath + mat.diffuse_texname);
                    diffuse->SetInputValue("albedo", texture);
                    scene.AttachAutoreleaseObject(texture);
                }
                else
                {
                    diffuse->SetInputValue("albedo", RadeonRays::float3(mat.diffuse[0], mat.diffuse[1], mat.diffuse[2]));
                }

                // Set albedo
                if (!mat.specular_texname.empty())
                {
                    auto texture = image_io.LoadImage(basepath + mat.specular_texname);
                    specular->SetInputValue("albedo", texture);
                    scene.AttachAutoreleaseObject(texture);
                }
                else
                {
                    specular->SetInputValue("albedo", s);
                }

                // Set normal
                if (!mat.normal_texname.empty())
                {
                    auto texture = image_io.LoadImage(basepath + mat.normal_texname);
                    diffuse->SetInputValue("normal", texture);
                    specular->SetInputValue("normal", texture);
                    scene.AttachAutoreleaseObject(texture);
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
                    auto texture = image_io.LoadImage(basepath + mat.diffuse_texname);
                    diffuse->SetInputValue("albedo", texture);
                    scene.AttachAutoreleaseObject(texture);
                }
                else
                {
                    diffuse->SetInputValue("albedo", RadeonRays::float3(mat.diffuse[0], mat.diffuse[1], mat.diffuse[2]));
                }

                // Set normal
                if (!mat.normal_texname.empty())
                {
                    auto texture = image_io.LoadImage(basepath + mat.normal_texname);
                    diffuse->SetInputValue("normal", texture);
                    scene.AttachAutoreleaseObject(texture);
                }

                material = diffuse;
            }
        }

        // Set material name
        material->SetName(mat.name);

        scene.AttachAutoreleaseObject(material);

        return material;
    }

    Scene1* SceneIoObj::LoadScene(std::string const& filename, std::string const& basepath) const
    {
        using namespace tinyobj;

        auto image_io(ImageIo::CreateImageIo());

        // Loader data
        std::vector<shape_t> objshapes;
        std::vector<material_t> objmaterials;

        // Try loading file
        std::string res = LoadObj(objshapes, objmaterials, filename.c_str(), basepath.c_str());
        if (res != "")
        {
            throw std::runtime_error(res);
        }

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

		/// @todo: for testing, create a scene with curves only.
		///        As to handle a scene with both meshes and curves, we will need an
		///        intersector which supports both.
		{
			Curves* curves = new Curves();

			// make a test grid of curves ("fur")
			float scale = 1.f;
			int gridRes = 10;
			int cvsPerCurve = 10;
			float dl = scale/float(gridRes);
			float cvRadius = dl/20.f;

			std::vector<RadeonRays::float4> curve_vertices;
			std::vector<std::uint32_t> curve_indices;

			for (size_t i=0; i<gridRes; i++)
			for (size_t j=0; j<gridRes; j++)
			{
				float x = float(i)*dl;
				float z = float(j)*dl;
				RadeonRays::float4 rootCv(x, 0.f, z, cvRadius);
				curve_vertices.push_back(rootCv);
				size_t prev_index = curve_vertices.size()-1;

				for (size_t n=1; n<cvsPerCurve; ++n)
				{
					RadeonRays::float4 segment(0.5f*dl, dl, 0.f);
					RadeonRays::float4 cv = rootCv + float(n)*segment;
					cv.w = cvRadius;
					curve_vertices.push_back(cv);

					size_t numVertices = curve_vertices.size();
					curve_indices.push_back(prev_index);
					curve_indices.push_back(prev_index+1);

					prev_index++;
				}
			}

			curves->SetVertices(&curve_vertices[0], curve_vertices.size());
			curves->SetIndices(&curve_indices[0], curve_indices.size());

			// Attach to the scene
			scene->AttachShape(curves);

			// Attach for autorelease
			scene->AttachAutoreleaseObject(curves);
		}

		/*
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
            auto idx = objshapes[s].mesh.material_ids[0];

            if (idx > 0)
            {
                mesh->SetMaterial(materials[idx]);
            }

            // Attach to the scene
            scene->AttachShape(mesh);

            // Attach for autorelease
            scene->AttachAutoreleaseObject(mesh);

            // If the mesh has emissive material we need to add area light for it
            if (idx > 0 && emissives.find(materials[idx]) != emissives.cend())
            {
                // Add area light for each polygon of emissive mesh
                for (int l = 0; l < mesh->GetNumIndices() / 3 ;++l)
                {
                    AreaLight* light = new AreaLight(mesh, l);
                    scene->AttachLight(light);
                    scene->AttachAutoreleaseObject(light);
                }
            }
        }
		*/

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
        light->SetEmittedRadiance(3.5f * RadeonRays::float3(1.f, 1.f, 1.f));
        scene->AttachAutoreleaseObject(light);

        DirectionalLight* light1 = new DirectionalLight();
        light1->SetDirection(RadeonRays::float3(0.3f, -1.f, -0.5f));
        light1->SetEmittedRadiance(RadeonRays::float3(1.f, 0.8f, 0.65f));
        scene->AttachAutoreleaseObject(light1);

        //scene->AttachLight(light);
        scene->AttachLight(light1);
        scene->AttachLight(ibl);

        return scene;
    }
}
