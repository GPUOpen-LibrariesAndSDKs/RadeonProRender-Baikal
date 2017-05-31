
#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm> 
#include <functional> 
#include <cctype>
#include <locale>

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

                specular->SetInputValue("roughness", 0.025f);

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


	// trim from start
	static inline std::string &ltrim(std::string &s) {
		s.erase(s.begin(), std::find_if(s.begin(), s.end(),
			std::not1(std::ptr_fun<int, int>(std::isspace))));
		return s;
	}

	// trim from end
	static inline std::string &rtrim(std::string &s) {
		s.erase(std::find_if(s.rbegin(), s.rend(),
			std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
		return s;
	}

	// trim from both ends
	static inline std::string &trim(std::string &s) {
		return ltrim(rtrim(s));
	}

    Scene1* SceneIoObj::LoadScene(std::string const& filename, std::string const& basepath) const
    {
        using namespace tinyobj;

        auto image_io(ImageIo::CreateImageIo());

        // Loader data
        std::vector<shape_t> objshapes;
        std::vector<material_t> objmaterials;

        // Try loading file
		std::cout << "Loading OBJ  ..." << std::endl;
        std::string res = LoadObj(objshapes, objmaterials, filename.c_str(), basepath.c_str());
		std::cout << "OBJ loaded." << std::endl;
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

        // Construct all Baikal shapes 
		
		/// @todo: for testing, load curves from a hard-coded custom formatted text file:
		std::cout << "Constructing curves  ..." << std::endl;

		Curves* curves = new Curves();
		{
			std::string cvsFile = "../Resources/chief/chief_lonoise.cvs";
			std::ifstream inFile(cvsFile);
			
			std::vector<RadeonRays::float4> curve_vertices;
			std::vector<std::uint32_t> curve_indices;

			float cvRadius = 0.002f; // hard-coded for now, as not exported!
			
			bool atRoot = true;
			size_t prev_index = 0;
			std::string line;
			while (std::getline(inFile, line))
			{
				line = trim(line);
				if (line.empty()) continue;
				if (line.find("curve") != std::string::npos) 	
				{
					atRoot = true;
					continue;
				}

				float pos[3];
				std::stringstream ss(line);
				for(int i=0; i<3; i++) ss >> pos[i];
				
				// @todo: allow for a radius ramp between root and tip.
				RadeonRays::float4 cv(pos[0], pos[1], pos[2], cvRadius);
				if (atRoot)
				{
					curve_vertices.push_back(cv);
					prev_index = curve_vertices.size()-1;
					atRoot = false;
					continue;
				}
				
				curve_vertices.push_back(cv);
				curve_indices.push_back(prev_index);
				curve_indices.push_back(prev_index+1);

				prev_index++;
			}

			curves->SetVertices(&curve_vertices[0], curve_vertices.size());
			curves->SetIndices(&curve_indices[0], curve_indices.size());
			
			Material* hairMaterial = new SingleBxdf(SingleBxdf::BxdfType::kHair);
			curves->SetMaterial(hairMaterial);

			// Attach to the scene
			scene->AttachShape(curves);

			// Attach for autorelease
			scene->AttachAutoreleaseObject(curves);
		}
		std::cout << "Curves constructed ..." << std::endl;

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

            if (idx >= 0)
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

        // TODO: temporary code, add IBL
        //Texture* ibl_texture = image_io->LoadImage("../Resources/Textures/Stonewall_Ref.hdr");
		//Texture* ibl_texture = image_io->LoadImage("../Resources/Textures/HDR_112_River_Road_2_Ref.hdr");
		Texture* ibl_texture = image_io->LoadImage("../Resources/Textures/HDR_041_Path_Ref.hdr");
		
        scene->AttachAutoreleaseObject(ibl_texture);

        ImageBasedLight* ibl = new ImageBasedLight();
        ibl->SetTexture(ibl_texture);
        ibl->SetMultiplier(0.2f);
        scene->AttachAutoreleaseObject(ibl);

        // TODO: temporary code to add directional light
        DirectionalLight* light = new DirectionalLight();
        light->SetDirection(RadeonRays::normalize(RadeonRays::float3(1.1f, -0.6f, 1.2f)));
        light->SetEmittedRadiance(3.5f * RadeonRays::float3(1.f, 1.f, 1.f));
        scene->AttachAutoreleaseObject(light);

        DirectionalLight* light1 = new DirectionalLight();
        light1->SetDirection(RadeonRays::float3(0.3f, -1.f, -0.5f));
        light1->SetEmittedRadiance(RadeonRays::float3(1.f, 0.8f, 0.65f));
        scene->AttachAutoreleaseObject(light1);

        scene->AttachLight(light);
        //scene->AttachLight(light1);
        scene->AttachLight(ibl);

        return scene;
    }
}
