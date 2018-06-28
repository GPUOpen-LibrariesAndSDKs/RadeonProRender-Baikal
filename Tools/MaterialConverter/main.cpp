#include "BaikalOld/SceneGraph/IO/material_io.h"
#include "BaikalOld/SceneGraph/iterator.h"
#include "BaikalOld/SceneGraph/material.h"

#include "material_io.h"
#include "SceneGraph/iterator.h"
#include "SceneGraph/uberv2material.h"

#include "material_converter.h"
#include "XML/tinyxml2.h"
#include <vector>

#include <iostream>
#include <fstream>
#include <algorithm>
#include <string>
#include <map>

namespace
{
    char const* kHelpMessage =
        "MaterialConverter -p <path to materials.xml and mapping.xml folder>";
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

    if (!scene_path)
    {
        std::cout << kHelpMessage << std::endl;
        return;
    }

    std::string scene_path_str = std::string(scene_path);

    std::ifstream in_materials(scene_path_str + "materials.xml");
    if (!in_materials)
    {
        std::cerr << "Failed to open materials.xml file!" << std::endl;
        return;
    }

    std::ifstream in_mapping(scene_path_str + "mapping.xml");
    if (!in_mapping)
    {
        std::cerr << "Failed to open mapping.xml file!" << std::endl;
        return;
    }

    in_materials.close();
    in_mapping.close();

    std::cout << "Loading materials.xml" << std::endl;

    auto material_io = BaikalOld::MaterialIo::CreateMaterialIoXML();
    auto mats = material_io->LoadMaterials(scene_path_str + "materials.xml");

    std::map<std::string, BaikalOld::Material::Ptr> all_materials;
    for (mats->Reset(); mats->IsValid(); mats->Next())
    {
        auto mtl = mats->ItemAs<BaikalOld::Material>();
        all_materials[mtl->GetName()] = mtl;
    }

    tinyxml2::XMLDocument doc;
    doc.LoadFile((scene_path_str + "mapping.xml").c_str());

    std::set<BaikalOld::Material::Ptr> old_materials;
    for (auto element = doc.FirstChildElement(); element; element = element->NextSiblingElement())
    {
        old_materials.emplace(all_materials[element->Attribute("to")]);
    }

    std::cout << "Converting materials" << std::endl;
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
