#include "BaikalOld/SceneGraph/IO/material_io.h"
#include "BaikalOld/SceneGraph/IO/scene_io.h"
#include "BaikalOld/SceneGraph/iterator.h"
#include "BaikalOld/SceneGraph/material.h"

#include "material_io.h"
#include "SceneGraph/iterator.h"
#include "SceneGraph/uberv2material.h"

#include "material_converter.h"
#include <vector>

#include <iostream>
#include <fstream>
#include <algorithm>
#include <string>

namespace
{
    char const* kHelpMessage =
        "MaterialConverter [-p path_to_models][-f model_name]";
}

static char* GetCmdOption(char** begin, char** end, const std::string& option)
{
    char** itr = std::find(begin, end, option);
    if (itr != end && ++itr != end)
    {
        return *itr;
    }
    return 0;
}

void Process(int argc, char** argv)
{
    char* scene_path = GetCmdOption(argv, argv + argc, "-p");
    char* scene_name = GetCmdOption(argv, argv + argc, "-f");

    if (!scene_path || !scene_name)
    {
        std::cout << kHelpMessage << std::endl;
        return;
    }

    std::string scene_path_str = std::string(scene_path);
    std::string scene_name_str = std::string(scene_name);

    std::cout << "Loading materials.xml" << std::endl;
    auto material_io = BaikalOld::MaterialIo::CreateMaterialIoXML();
    auto mats = material_io->LoadMaterials(scene_path_str + "materials.xml");
    std::set<BaikalOld::Material::Ptr> old_materials;

#if 1
    // Dealing with obj files
    std::unique_ptr<BaikalOld::SceneIo> scene_obj_io = BaikalOld::SceneIo::CreateSceneIoObj();

    std::cout << "Loading obj" << std::endl;
    BaikalOld::Scene1::Ptr scene = scene_obj_io->LoadScene(scene_path_str + scene_name_str, scene_path_str);
    auto mapping = material_io->LoadMaterialMapping(scene_path_str + "mapping.xml");
    material_io->ReplaceSceneMaterials(*scene, *mats, mapping);

    for (auto shape_iter = scene->CreateShapeIterator(); shape_iter->IsValid(); shape_iter->Next())
    {
        auto shape = shape_iter->ItemAs<BaikalOld::Shape>();
        auto mtl = shape->GetMaterial();
        if (!mtl)
        {
            std::cout << "Shape " << shape->GetName() << " has no material" << std::endl;
            continue;
        }
        old_materials.insert(shape->GetMaterial());
    }

    //std::vector<std::string> old_mtl_names;
    //std::ofstream mesh_mtls(scene_path + "mesh_materials.txt");
    //for (auto i: old_materials)
    //{
    //    mesh_mtls << i->GetName() << std::endl;
    //}
#else
    std::map<std::string, BaikalOld::Material::Ptr> all_materials;
    for (mats->Reset(); mats->IsValid(); mats->Next())
    {
        auto mtl = mats->ItemAs<BaikalOld::Material>();
        all_materials[mtl->GetName()] = mtl;
    }

    std::ifstream mesh_mtls(scene_path + "mesh_materials.txt");
    std::string line;
    old_materials.clear();
    while (std::getline(mesh_mtls, line))
    {
        old_materials.insert(all_materials[line]);
    }
#endif

    std::cout << "Translating materials" << std::endl;
    std::set<Baikal::UberV2Material::Ptr> new_materials = MaterialConverter::TranslateMaterials(old_materials);

    auto material_io_new = Baikal::MaterialIo::CreateMaterialIoXML();
    auto material_new_iterator = std::make_unique<Baikal::ContainerIterator<decltype(new_materials)>>(std::move(new_materials));
    std::cout << "Saving to new materials xml file" << std::endl;
    material_io_new->SaveMaterials(scene_path_str + "materials_new.xml", *material_new_iterator);

    std::cout << "Done." << std::endl;
}

int main(int argc, char** argv)
{
    try
    {
        Process(argc, argv);
    }
    catch (std::exception& ex)
    {
        std::cout << "Caught exception: " << ex.what();
    }

    return 0;
}
