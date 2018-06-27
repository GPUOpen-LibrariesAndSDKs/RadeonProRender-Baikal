#pragma once

#include "BaikalOld/SceneGraph/material.h"

#include "SceneGraph/uberv2material.h"

#include <set>

class MaterialConverter
{
public:
    static std::set<Baikal::UberV2Material::Ptr> TranslateMaterials(std::set<BaikalOld::Material::Ptr> const& old_materials);

private:
    static Baikal::Texture::Format TranslateFormat(BaikalOld::Texture::Format old_format);
    static Baikal::InputMap::Ptr TranslateInput(BaikalOld::Material::Ptr old_mtl, std::string const& input_name);
    static Baikal::UberV2Material::Ptr TranslateSingleBxdfMaterial(BaikalOld::SingleBxdf::Ptr old_mtl);
    static Baikal::UberV2Material::Ptr MergeFresnelBlendMaterials(Baikal::UberV2Material::Ptr base, Baikal::UberV2Material::Ptr top, BaikalOld::Material::Ptr mtl);
    static Baikal::UberV2Material::Ptr MergeMixMaterials(Baikal::UberV2Material::Ptr base, Baikal::UberV2Material::Ptr top, BaikalOld::Material::Ptr mtl);
    static Baikal::UberV2Material::Ptr TranslateFresnelBlend(BaikalOld::MultiBxdf::Ptr mtl);
    static Baikal::UberV2Material::Ptr TranslateMix(BaikalOld::MultiBxdf::Ptr mtl);
    static Baikal::UberV2Material::Ptr TranslateMultiBxdfMaterial(BaikalOld::MultiBxdf::Ptr mtl);
    static Baikal::UberV2Material::Ptr TranslateMaterial(BaikalOld::Material::Ptr mtl);

};
