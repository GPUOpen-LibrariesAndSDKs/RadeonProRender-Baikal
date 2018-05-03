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

#pragma once

#include "basic.h"

class MaterialTest : public BasicTest
{
public:
    virtual void SetUp() override
    {
        BasicTest::SetUp();
        CreateScene(SceneType::kSphereAndPlane);
        AddEnvironmentLight("../Resources/Textures/studio015.hdr");
    }

};
/*
TEST_F(MaterialTest, Material_Diffuse)
{
    std::vector<float3> colors =
    {
        float3(0.9f, 0.2f, 0.1f),
        float3(0.1f, 0.9f, 0.1f),
        float3(0.3f, 0.2f, 0.8f)
    };

    std::string resource_dir = "../Resources/Textures/";
    std::string ext = ".jpg";
    std::vector<std::string> texture_names =
    {
        "test_albedo1",
        "test_albedo2",
        "test_albedo3"
    };

    for (auto const& c : colors)
    {
        const rpr_material_node sphere_mtl = GetMaterial("sphere_mtl");
        ASSERT_EQ(rprMaterialNodeSetInputF_ext(sphere_mtl, RPR_UBER_MATERIAL_DIFFUSE_COLOR, c.x, c.y, c.z, 0.0f), RPR_SUCCESS);

        ClearFramebuffer();
        Render();
        SaveAndCompare("%.2f_%.2f_%.2f", c.x, c.y, c.z);
    }

    // TODO: CHECK IS THIS RIGHT?
    rpr_material_node albedo;
    ASSERT_EQ(rprMaterialSystemCreateNode(m_matsys, RPR_MATERIAL_NODE_ARITHMETIC, &albedo), RPR_SUCCESS);
    ASSERT_EQ(rprMaterialNodeSetInputU(albedo, "op", RPR_MATERIAL_NODE_OP_ADD), RPR_SUCCESS);

    rpr_material_node inputTexture;
    ASSERT_EQ(rprMaterialSystemCreateNode(m_matsys, RPR_MATERIAL_NODE_IMAGE_TEXTURE, &inputTexture), RPR_SUCCESS);
    ASSERT_EQ(rprMaterialNodeSetInputN(albedo, "color0", inputTexture), RPR_SUCCESS);

    rpr_material_node material;
    ASSERT_EQ(rprMaterialSystemCreateNode(m_matsys, RPR_MATERIAL_NODE_UBERV2, &material), RPR_SUCCESS);
    ASSERT_EQ(rprMaterialNodeSetInputN_ext(material, RPR_UBER_MATERIAL_DIFFUSE_COLOR, albedo), RPR_SUCCESS);
    ASSERT_EQ(rprMaterialNodeSetInputU_ext(material, RPR_UBER_MATERIAL_LAYERS, RPR_UBER_MATERIAL_LAYER_DIFFUSE), RPR_SUCCESS);

    AddMaterial("mtl_with_albedo", material);
    ApplyMaterialToObject("sphere", "mtl_with_albedo");

    for (auto const& tex_name : texture_names)
    {
        ASSERT_EQ(rprMaterialNodeSetInputImageData(inputTexture, "data", FindImage(resource_dir + tex_name + ext)), RPR_SUCCESS);

        ClearFramebuffer();
        Render();
        SaveAndCompare("%s", tex_name.c_str());
    }

}
*/
TEST_F(MaterialTest, Material_Reflect)
{
    std::vector<float3> colors =
    {
        float3(0.9f, 0.2f, 0.1f),
        float3(0.1f, 0.9f, 0.1f),
        float3(0.3f, 0.2f, 0.8f)
    };

    std::string resource_dir = "../Resources/Textures/";
    std::string ext = ".jpg";
    std::vector<std::string> texture_names =
    {
        "test_albedo1",
        "test_albedo2",
        "test_albedo3"
    };

    std::vector<float> iors =
    {
        0.f, 1.1f, 1.3f, 1.6f, 2.2f, 5.f
    };

    const rpr_material_node sphere_mtl = GetMaterial("sphere_mtl");
    ASSERT_EQ(rprMaterialNodeSetInputU_ext(sphere_mtl, RPR_UBER_MATERIAL_LAYERS, RPR_UBER_MATERIAL_LAYER_DIFFUSE | RPR_UBER_MATERIAL_LAYER_REFLECTION), RPR_SUCCESS);
    ASSERT_EQ(rprMaterialNodeSetInputF_ext(sphere_mtl, RPR_UBER_MATERIAL_REFLECTION_ROUGHNESS, 0.0f, 0.0f, 0.0f, 0.0f), RPR_SUCCESS);
    ASSERT_EQ(rprMaterialNodeSetInputF_ext(sphere_mtl, RPR_UBER_MATERIAL_REFLECTION_METALNESS, 0.0f, 0.0f, 0.0f, 0.0f), RPR_SUCCESS);
    
    //for (auto const& c : colors)
    //{
    //    ASSERT_EQ(rprMaterialNodeSetInputF_ext(sphere_mtl, RPR_UBER_MATERIAL_DIFFUSE_COLOR, c.x, c.y, c.z, 0.0f), RPR_SUCCESS);

    //    for (auto& ior : iors)
    //    {
    //        ASSERT_EQ(rprMaterialNodeSetInputF_ext(sphere_mtl, RPR_UBER_MATERIAL_REFLECTION_IOR, ior, ior, ior, ior), RPR_SUCCESS);
    //        
    //        ClearFramebuffer();
    //        Render();
    //        SaveAndCompare("%f_%f_%f_%f", c.x, c.y, c.z, ior);
    //    }
    //}
    
    rpr_material_node inputTexture;
    ASSERT_EQ(rprMaterialSystemCreateNode(m_matsys, RPR_MATERIAL_NODE_IMAGE_TEXTURE, &inputTexture), RPR_SUCCESS);
    ASSERT_EQ(rprMaterialNodeSetInputImageData(inputTexture, "data", FindImage("../Resources/Textures/test_albedo3.jpg")), RPR_SUCCESS);

    rpr_material_node material;
    ASSERT_EQ(rprMaterialSystemCreateNode(m_matsys, RPR_MATERIAL_NODE_UBERV2, &material), RPR_SUCCESS);
    ASSERT_EQ(rprMaterialNodeSetInputU_ext(material, RPR_UBER_MATERIAL_LAYERS, RPR_UBER_MATERIAL_LAYER_REFLECTION), RPR_SUCCESS);
    ASSERT_EQ(rprMaterialNodeSetInputN_ext(material, RPR_UBER_MATERIAL_REFLECTION_COLOR, inputTexture), RPR_SUCCESS);
    ASSERT_EQ(rprMaterialNodeSetInputF_ext(material, RPR_UBER_MATERIAL_REFLECTION_ROUGHNESS, 0.0f, 0.0f, 0.0f, 0.0f), RPR_SUCCESS);
    ASSERT_EQ(rprMaterialNodeSetInputF_ext(material, RPR_UBER_MATERIAL_REFLECTION_METALNESS, 0.0f, 0.0f, 0.0f, 0.0f), RPR_SUCCESS);

    AddMaterial("mtl_with_reflection_tex", material);
    ApplyMaterialToObject("sphere", "mtl_with_reflection_tex");

    for (auto const& tex_name : texture_names)
    {
        ASSERT_EQ(rprMaterialNodeSetInputImageData(inputTexture, "data", FindImage(resource_dir + tex_name + ext)), RPR_SUCCESS);

        for (auto& ior : iors)
        {
            ASSERT_EQ(rprMaterialNodeSetInputF_ext(sphere_mtl, RPR_UBER_MATERIAL_REFLECTION_IOR, ior, ior, ior, ior), RPR_SUCCESS);

            ClearFramebuffer();
            Render();
            SaveAndCompare("%s_%f", tex_name.c_str(), ior);
        }
    }

}
