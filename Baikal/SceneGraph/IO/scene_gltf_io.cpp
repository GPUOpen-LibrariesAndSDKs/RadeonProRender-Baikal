
#include "scene_io.h"

#ifdef ENABLE_GLTF
#include <cassert>
#include <iostream>

#include "FreeImage.h"

#include "Baikal/SceneGraph/IO/image_io.h"
#include "Baikal/SceneGraph/scene1.h"
#include "Baikal/SceneGraph/light.h"

#include "RadeonProRender.h"
#include "RprSupport.h"
#include "ProRenderGLTF.h"
#include "Rpr/Export.h"

using namespace Baikal;

namespace Baikal
{

    // glTF scene loader
    class SceneGltfIo : public SceneIo
    {
    public:

        // Load scene from file
        virtual Scene1::Ptr LoadScene(std::string const& filename, std::string const& basepath) const override;
    private:

    };

    Scene1::Ptr SceneGltfIo::LoadScene(std::string const& filename, std::string const& basepath) const
    {
        rpr_context context = nullptr;
        rprx_context uber_context = nullptr;
        rpr_material_system sys = nullptr;
        rpr_scene scene = nullptr;
        auto res = rprCreateContext(RPR_API_VERSION, nullptr, 0, RPR_CREATION_FLAGS_ENABLE_GPU0, nullptr, nullptr, &context);
        res = rprContextCreateMaterialSystem(context, NULL, &sys);
        res = rprxCreateContext(sys, 0, &uber_context);
        if (!rprImportFromGLTF(filename.c_str(), context, sys, uber_context, &scene))
        {
            throw std::runtime_error("Failed to load GLTF scene:" + filename + "\n");
        }

        Baikal::Scene1::Ptr baikal_scene = ExportFromRpr(scene);
        if (baikal_scene->GetNumLights() == 0)
        {
            auto image_io(ImageIo::CreateImageIo());

            // TODO: temporary code, add IBL
            auto ibl_texture = image_io->LoadImage("../Resources/Textures/studio015.hdr");

            auto ibl = ImageBasedLight::Create();
            ibl->SetTexture(ibl_texture);
            ibl->SetMultiplier(1.f);

            // TODO: temporary code to add directional light
            auto light = SpotLight::Create();
            light->SetPosition({ 0.f, 5.f, 3.f });

            light->SetDirection(RadeonRays::normalize(RadeonRays::float3(0.f, -1.f, 0.f)));
            light->SetEmittedRadiance(300.f * RadeonRays::float3(1.f, 0.95f, 0.92f));

            auto light1 = DirectionalLight::Create();
            light1->SetDirection(RadeonRays::float3(0.3f, -1.f, -0.5f));
            light1->SetEmittedRadiance(RadeonRays::float3(1.f, 0.8f, 0.65f));
            
            auto point_light = PointLight::Create();
            point_light->SetPosition({ 0.f, 2.f, 3.f });
            point_light->SetDirection(RadeonRays::float3(0.3f, -1.f, -0.5f));
            point_light->SetEmittedRadiance(RadeonRays::float3(10.f, 10.8f, 10.65f));

            baikal_scene->AttachLight(ibl);
            //baikal_scene->AttachLight(light);
            //baikal_scene->AttachLight(light1);
            //baikal_scene->AttachLight(point_light);
        }

        rprxDeleteContext(uber_context);
        rprObjectDelete(scene);
        rprObjectDelete(sys);
        rprObjectDelete(context);

        return baikal_scene;
    }


    std::unique_ptr<Baikal::SceneIo> SceneIo::CreateSceneIoGltf()
    {
        return std::unique_ptr<Baikal::SceneIo>(new SceneGltfIo());
    }
} //Baikal
#else

namespace Baikal
{
    std::unique_ptr<SceneIo> SceneIo::CreateSceneIoGltf()
    {
        throw std::runtime_error("GLTF support is disabled. Please build with --gltf premake option.\n");
        return nullptr;
    }
}

#endif //ENABLE_GLTF
