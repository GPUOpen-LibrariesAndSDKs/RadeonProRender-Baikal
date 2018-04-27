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

class LightTest : public BasicTest
{
public:
    virtual void SetUp() override
    {
        BasicTest::SetUp();
        CreateScene(SceneType::kThreeSpheres);
    }

    void AddPointLight(float3 pos, float3 color)
    {
        rpr_light light = nullptr;
        ASSERT_EQ(rprContextCreatePointLight(m_context, &light), RPR_SUCCESS);
        matrix lightm = translation(pos);
        ASSERT_EQ(rprLightSetTransform(light, true, &lightm.m00), RPR_SUCCESS);
        ASSERT_EQ(rprPointLightSetRadiantPower3f(light, color.x, color.y, color.z), RPR_SUCCESS);
        AddLight(light);

    }

    void AddDirectionalLight(float yaw, float pitch, float3 color)
    {
        rpr_light light = nullptr;
        ASSERT_EQ(rprContextCreateDirectionalLight(m_context, &light), RPR_SUCCESS);
        matrix lightm = rotation_x(pitch) * rotation_y(yaw);
        ASSERT_EQ(rprLightSetTransform(light, false, &lightm.m00), RPR_SUCCESS);
        ASSERT_EQ(rprDirectionalLightSetRadiantPower3f(light, color.x, color.y, color.z), RPR_SUCCESS);
        AddLight(light);
    }

    //void AddSpotLight(float3 pos, float yaw, float pitch, float3 color)
    //{
    //    rpr_light light = nullptr;
    //    ASSERT_EQ(rprContextCreateSpotLight(m_context, &light), RPR_SUCCESS);
    //    matrix lightm = translation(pos) * rotation_x(static_cast<float>(M_PI_2)); // * rotation_x(pitch) * rotation_y(yaw);
    //    ASSERT_EQ(rprLightSetTransform(light, true, &lightm.m00), RPR_SUCCESS);
    //    ASSERT_EQ(rprSpotLightSetRadiantPower3f(light, color.x, color.y, color.z), RPR_SUCCESS);
    //    //ASSERT_EQ(rprSpotLightSetConeShape(light, (float)M_PI_4, (float)M_PI_2), RPR_SUCCESS);
    //    AddLight(light);
    //}

    void AddAreaLight()
    {
        AddEmissiveMaterial("emission", float3(2.0f, 2.0f, 2.0f));
        AddPlane("plane1", float3(0.0f, 6.0f, 0.0f), float2(2.0f, 2.0f), float3(0, -1.0f, 0.0f));
        ApplyMaterialToObject("plane1", "emission");
    }

};

/*

TEST_F(LightTest, Light_PointLight)
{
    std::vector<float3> positions
    {
        float3( 3.0f, 6.0f,  0.0f),
        float3(-2.0f, 6.0f, -1.0f),
        float3( 0.0f, 6.0f, -3.0f)
    };

    std::vector<float3> colors
    {
        float3(3.0f, 0.1f, 0.1f),
        float3(0.1f, 3.0f, 0.1f),
        float3(0.1f, 0.1f, 3.0f)
    };

    for (size_t i = 0; i < 3; ++i)
    {
        AddPointLight(positions[i], colors[i] * 10.0f);
    }

    ClearFramebuffer();
    Render();
    SaveAndCompare("1");

    RemoveLight(0);
    ClearFramebuffer();
    Render();
    SaveAndCompare("2");

}

TEST_F(LightTest, Light_PointLightMany)
{
    const size_t num_lights = 16;
    const float step = (float)(2.f * M_PI / num_lights);

    for (size_t i = 0; i < num_lights; ++i)
    {
        float x = 5.0f * std::cos(i * step);
        float y = 5.0f;
        float z = 5.0f * std::sin(i * step);
        float f = (float)i / num_lights;
        float3 color = f * float3(1.f, 0.f, 0.f) + (1.f - f) * float3(0.f, 1.f, 0.f);

        AddPointLight(float3(x, y, z), color * 10.0f);
    }

    ClearFramebuffer();
    Render();
    SaveAndCompare();
}

TEST_F(LightTest, Light_DirectionalLight)
{
    std::vector<std::pair<float, float>> directions
    {
        { 0.0f,   (float)M_PI / 3.0f },
        { (float)M_PI / 3.0f * 2.0f, (float)M_PI / 4.0f },
        { (float)M_PI / 3.0f * 4.0f, (float)M_PI / 6.0f }
    };

    std::vector<float3> colors
    {
        float3(3.0f, 0.1f, 0.1f),
        float3(0.1f, 3.0f, 0.1f),
        float3(0.1f, 0.1f, 3.0f)
    };

    for (size_t i = 0; i < 3; ++i)
    {
        AddDirectionalLight(directions[i].first, directions[i].second, colors[i]);
    }

    ClearFramebuffer();
    Render();
    SaveAndCompare("1");

    RemoveLight(0);
    ClearFramebuffer();
    Render();
    SaveAndCompare("2");

}

// Black screen... Not working
TEST_F(LightTest, Light_SpotLight)
{
    std::vector<std::pair<float, float>> directions
    {
        { 0.0f,   (float)M_PI / 3.0f },
        { (float)M_PI / 3.0f * 2.0f, (float)M_PI / 4.0f },
        { (float)M_PI / 3.0f * 4.0f, (float)M_PI / 6.0f }
    };

    std::vector<float3> colors
    {
        float3(3.0f, 0.1f, 0.1f),
        float3(0.1f, 3.0f, 0.1f),
        float3(0.1f, 0.1f, 3.0f)
    };

    //for (int i = 0; i < 4; ++i)
    //{
    //    for (int j = 0; j < 4; ++j)
    //    {
            //AddSpotLight(float3(i * 2.0f - 4.0f, 6.0f, j * 2.0f - 4.0f), 0, 0, float3(100, 100, 100));
            //AddSpotLight(float3(3.0f, 6.0f, 0.0f), 0, 0, float3(100, 100, 100));
    
            //AddPointLight(float3(i * 2.0f, 6.0f, j * 2.0f), float3(100, 100, 100));
            ClearFramebuffer();
            Render();
            //SaveAndCompare("%d_%d", i, j);
            SaveAndCompare();
            //RemoveLight(0);
            //RemoveLight(1);
    //    }
    //}
    //ClearFramebuffer();
    //Render();
    //SaveAndCompare("1");

    //RemoveLight(0);
    //ClearFramebuffer();
    //Render();
    //SaveAndCompare("2");

}


TEST_F(LightTest, Light_AreaLight)
{
    AddAreaLight();
    ClearFramebuffer();
    Render();
    SaveAndCompare();
}

TEST_F(LightTest, Light_ImageBasedLight)
{
    AddEnvironmentLight("../Resources/Textures/studio015.hdr");
    ClearFramebuffer();
    Render();
    SaveAndCompare("1");
    RemoveLight(0);
    RemoveImage(0);

    AddEnvironmentLight("../Resources/Textures/test_albedo1.jpg");
    ClearFramebuffer();
    Render();
    SaveAndCompare("2");

}

*/

//TEST_F(LightTest, Light_OverrideReflection)
//{
//    rpr_image image_reflection = nullptr;
//    ASSERT_EQ(rprContextCreateImageFromFile(m_context, "../Resources/Textures/test_albedo1.jpg", &image_reflection), RPR_SUCCESS);
//    AddEnvironmentLight(image_reflection);
//    //ASSERT_EQ(rprSceneSetEnvironmentOverride(m_scene, RPR_SCENE_ENVIRONMENT_OVERRIDE_REFLECTION, m_lights[0]), RPR_SUCCESS);
//
//    rpr_image image_background = nullptr;
//    ASSERT_EQ(rprContextCreateImageFromFile(m_context, "../Resources/Textures/test_albedo3.jpg", &image_background), RPR_SUCCESS);
//    AddEnvironmentLight(image_background);
//    //ASSERT_EQ(rprSceneSetEnvironmentOverride(m_scene, RPR_SCENE_ENVIRONMENT_OVERRIDE_REFRACTION, m_lights[1]), RPR_SUCCESS);
//
//    ClearFramebuffer();
//    Render();
//    SaveAndCompare();
//
//}

class Ttest : public ::testing::Test
{
public:
    static constexpr int kRenderIterations = 32;

    rpr_shape CreateSphere(rpr_context context, std::uint32_t lat, std::uint32_t lon, float r, RadeonRays::float3 const& c)
    {
        size_t num_verts = (lat - 2) * lon + 2;
        size_t num_tris = (lat - 2) * (lon - 1) * 2;

        std::vector<RadeonRays::float3> vertices(num_verts);
        std::vector<RadeonRays::float3> normals(num_verts);
        std::vector<RadeonRays::float2> uvs(num_verts);
        std::vector<std::uint32_t> indices(num_tris * 3);

        auto t = 0U;
        for (auto j = 1U; j < lat - 1; j++)
        {
            for (auto i = 0U; i < lon; i++)
            {
                float theta = float(j) / (lat - 1) * (float)M_PI;
                float phi = float(i) / (lon - 1) * (float)M_PI * 2;
                vertices[t].x = r * sinf(theta) * cosf(phi) + c.x;
                vertices[t].y = r * cosf(theta) + c.y;
                vertices[t].z = r * -sinf(theta) * sinf(phi) + c.z;
                normals[t].x = sinf(theta) * cosf(phi);
                normals[t].y = cosf(theta);
                normals[t].z = -sinf(theta) * sinf(phi);
                uvs[t].x = phi / (2 * (float)M_PI);
                uvs[t].y = theta / ((float)M_PI);
                ++t;
            }
        }

        vertices[t].x = c.x; vertices[t].y = c.y + r; vertices[t].z = c.z;
        normals[t].x = 0; normals[t].y = 1; normals[t].z = 0;
        uvs[t].x = 0; uvs[t].y = 0;
        ++t;
        vertices[t].x = c.x; vertices[t].y = c.y - r; vertices[t].z = c.z;
        normals[t].x = 0; normals[t].y = -1; normals[t].z = 0;
        uvs[t].x = 1; uvs[t].y = 1;
        ++t;

        t = 0U;
        for (auto j = 0U; j < lat - 3; j++)
        {
            for (auto i = 0U; i < lon - 1; i++)
            {
                indices[t++] = j * lon + i;
                indices[t++] = (j + 1) * lon + i + 1;
                indices[t++] = j * lon + i + 1;
                indices[t++] = j * lon + i;
                indices[t++] = (j + 1) * lon + i;
                indices[t++] = (j + 1) * lon + i + 1;
            }
        }

        for (auto i = 0U; i < lon - 1; i++)
        {
            indices[t++] = (lat - 2) * lon;
            indices[t++] = i;
            indices[t++] = i + 1;
            indices[t++] = (lat - 2) * lon + 1;
            indices[t++] = (lat - 3) * lon + i + 1;
            indices[t++] = (lat - 3) * lon + i;
        }

        std::vector<int> faces(indices.size() / 3, 3);

        rpr_shape sphere = nullptr;
        rprContextCreateMesh(context,
            (rpr_float const*)vertices.data(), vertices.size(), sizeof(RadeonRays::float3),
            (rpr_float const*)normals.data(), normals.size(), sizeof(RadeonRays::float3),
            (rpr_float const*)uvs.data(), uvs.size(), sizeof(RadeonRays::float2),
            (rpr_int const*)indices.data(), sizeof(rpr_int),
            (rpr_int const*)indices.data(), sizeof(rpr_int),
            (rpr_int const*)indices.data(), sizeof(rpr_int),
            faces.data(), faces.size(), &sphere);

        return sphere;

    }

};

TEST_F(Ttest, Sas)
{
    rpr_int status = RPR_SUCCESS;
    rpr_context	context;
    status = rprCreateContext(RPR_API_VERSION, nullptr, 0, RPR_CREATION_FLAGS_ENABLE_GPU0, NULL, NULL, &context);
    assert(status == RPR_SUCCESS);
    rpr_material_system matsys = NULL;
    status = rprContextCreateMaterialSystem(context, 0, &matsys);
    assert(status == RPR_SUCCESS);

    rpr_scene scene = NULL; status = rprContextCreateScene(context, &scene);
    assert(status == RPR_SUCCESS);

    //material
    rpr_material_node reflective = NULL;
    ASSERT_EQ(rprMaterialSystemCreateNode(matsys, RPR_MATERIAL_NODE_UBERV2, &reflective), RPR_SUCCESS);
    ASSERT_EQ(rprMaterialNodeSetInputU_ext(reflective, RPR_UBER_MATERIAL_LAYERS, RPR_UBER_MATERIAL_LAYER_REFLECTION), RPR_SUCCESS);
    ASSERT_EQ(rprMaterialNodeSetInputF_ext(reflective, RPR_UBER_MATERIAL_REFLECTION_COLOR, 1.0f, 0.0f, 0.0f, 0.0f), RPR_SUCCESS);
    ASSERT_EQ(rprMaterialNodeSetInputF_ext(reflective, RPR_UBER_MATERIAL_REFLECTION_IOR, 2.0f, 2.0f, 2.0f, 2.0f), RPR_SUCCESS);
    ASSERT_EQ(rprMaterialNodeSetInputF_ext(reflective, RPR_UBER_MATERIAL_REFLECTION_ROUGHNESS, 0.0f, 0.0f, 0.0f, 0.0f), RPR_SUCCESS);
    ASSERT_EQ(rprMaterialNodeSetInputF_ext(reflective, RPR_UBER_MATERIAL_REFLECTION_METALNESS, 0.0f, 0.0f, 0.0f, 0.0f), RPR_SUCCESS);
    
    rpr_material_node refractive = NULL;
    ASSERT_EQ(rprMaterialSystemCreateNode(matsys, RPR_MATERIAL_NODE_UBERV2, &refractive), RPR_SUCCESS);
    ASSERT_EQ(rprMaterialNodeSetInputU_ext(refractive, RPR_UBER_MATERIAL_LAYERS, RPR_UBER_MATERIAL_LAYER_REFRACTION), RPR_SUCCESS);
    ASSERT_EQ(rprMaterialNodeSetInputF_ext(refractive, RPR_UBER_MATERIAL_REFRACTION_COLOR, 1.0f, 0.0f, 0.0f, 0.0f), RPR_SUCCESS);
    ASSERT_EQ(rprMaterialNodeSetInputF_ext(refractive, RPR_UBER_MATERIAL_REFRACTION_IOR, 2.0f, 2.0f, 2.0f, 2.0f), RPR_SUCCESS);
    ASSERT_EQ(rprMaterialNodeSetInputF_ext(refractive, RPR_UBER_MATERIAL_REFRACTION_ROUGHNESS, 0.0f, 0.0f, 0.0f, 0.0f), RPR_SUCCESS);
        
    rpr_material_node transparent = NULL;
    ASSERT_EQ(rprMaterialSystemCreateNode(matsys, RPR_MATERIAL_NODE_UBERV2, &transparent), RPR_SUCCESS);
    ASSERT_EQ(rprMaterialNodeSetInputU_ext(transparent, RPR_UBER_MATERIAL_LAYERS, RPR_UBER_MATERIAL_LAYER_TRANSPARENCY), RPR_SUCCESS);
    rprMaterialNodeSetInputF_ext(transparent, RPR_UBER_MATERIAL_TRANSPARENCY, 0.8f, 0.8f, 0.8f, 0.8f);
    
    //sphere
    rpr_shape mesh_reflective = CreateSphere(context, 64, 32, 2.f, float3());
    assert(status == RPR_SUCCESS);
    matrix translate = translation(float3(-5.0, 0.0, 0.0));
    status = rprShapeSetTransform(mesh_reflective, true, &translate.m[0][0]);
    assert(status == RPR_SUCCESS);
    status = rprSceneAttachShape(scene, mesh_reflective);
    assert(status == RPR_SUCCESS);
    status = rprShapeSetMaterial(mesh_reflective, reflective);
    assert(status == RPR_SUCCESS);

    rpr_shape mesh_refractive = CreateSphere(context, 64, 32, 2.f, float3());
    assert(status == RPR_SUCCESS);
    translate = translation(float3(0.0, 0.0, 0.0));
    status = rprShapeSetTransform(mesh_refractive, true, &translate.m[0][0]);
    assert(status == RPR_SUCCESS);
    status = rprSceneAttachShape(scene, mesh_refractive);
    assert(status == RPR_SUCCESS);
    status = rprShapeSetMaterial(mesh_refractive, refractive);
    assert(status == RPR_SUCCESS);

    rpr_shape mesh_transparent = CreateSphere(context, 64, 32, 2.f, float3());
    assert(status == RPR_SUCCESS);
    translate = translation(float3(5.0, 0.0, 0.0));
    status = rprShapeSetTransform(mesh_transparent, true, &translate.m[0][0]);
    assert(status == RPR_SUCCESS);
    status = rprSceneAttachShape(scene, mesh_transparent);
    assert(status == RPR_SUCCESS);
    status = rprShapeSetMaterial(mesh_transparent, transparent);
    assert(status == RPR_SUCCESS);
    
    rpr_light light = NULL; status = rprContextCreateEnvironmentLight(context, &light);
    assert(status == RPR_SUCCESS);
    rpr_image imageInput = NULL; status = rprContextCreateImageFromFile(context, "../Resources/Textures/studio015.hdr", &imageInput);
    assert(status == RPR_SUCCESS);
    status = rprEnvironmentLightSetImage(light, imageInput);
    assert(status == RPR_SUCCESS);
    status = rprSceneAttachLight(scene, light);
    assert(status == RPR_SUCCESS);

    rpr_image bgImage = NULL;
    status = rprContextCreateImageFromFile(context, "../Resources/Textures/test_albedo1.jpg", &bgImage);
    assert(status == RPR_SUCCESS);

    //hdr overrides
    rpr_image hdr_reflection, hdr_refraction, hdr_transparency, hdr_background;
    rpr_light light_reflection, light_refraction, light_transparency, light_background;

    status = rprContextCreateImageFromFile(context, "../Resources/Textures/test_albedo2.jpg", &hdr_reflection);
    assert(status == RPR_SUCCESS);
    status = rprContextCreateEnvironmentLight(context, &light_reflection);
    assert(status == RPR_SUCCESS);
    status = rprEnvironmentLightSetImage(light_reflection, hdr_reflection);
    assert(status == RPR_SUCCESS);

    status = rprContextCreateImageFromFile(context, "../Resources/Textures/test_albedo3.jpg", &hdr_refraction);
    assert(status == RPR_SUCCESS);
    status = rprContextCreateEnvironmentLight(context, &light_refraction);
    assert(status == RPR_SUCCESS);
    status = rprEnvironmentLightSetImage(light_refraction, hdr_refraction);
    assert(status == RPR_SUCCESS);

    status = rprContextCreateImageFromFile(context, "../Resources/Textures/sky.hdr", &hdr_transparency);
    assert(status == RPR_SUCCESS);
    status = rprContextCreateEnvironmentLight(context, &light_transparency);
    assert(status == RPR_SUCCESS);
    status = rprEnvironmentLightSetImage(light_transparency, hdr_transparency);
    assert(status == RPR_SUCCESS);

    status = rprContextCreateImageFromFile(context, "../Resources/Textures/studio015.hdr", &hdr_background);
    assert(status == RPR_SUCCESS);
    status = rprContextCreateEnvironmentLight(context, &light_background);
    assert(status == RPR_SUCCESS);
    status = rprEnvironmentLightSetImage(light_background, hdr_background);
    assert(status == RPR_SUCCESS);

    status = rprSceneSetEnvironmentOverride(scene, RPR_SCENE_ENVIRONMENT_OVERRIDE_REFLECTION, light_reflection);
    assert(status == RPR_SUCCESS);
    status = rprSceneSetEnvironmentOverride(scene, RPR_SCENE_ENVIRONMENT_OVERRIDE_REFRACTION, light_refraction);
    assert(status == RPR_SUCCESS);
    status = rprSceneSetEnvironmentOverride(scene, RPR_SCENE_ENVIRONMENT_OVERRIDE_TRANSPARENCY, light_transparency);
    assert(status == RPR_SUCCESS);
    status = rprSceneSetEnvironmentOverride(scene, RPR_SCENE_ENVIRONMENT_OVERRIDE_BACKGROUND, light_background);
    assert(status == RPR_SUCCESS);

    //camera
    rpr_camera camera = NULL; status = rprContextCreateCamera(context, &camera);
    assert(status == RPR_SUCCESS);
    status = rprCameraLookAt(camera, 0, 0, 10, 0, 0, 0, 0, 1, 0);
    assert(status == RPR_SUCCESS);
    status = rprCameraSetFocalLength(camera, 23.f);
    assert(status == RPR_SUCCESS);
    //status = rprCameraSetFStop(camera, 5.4f);
    assert(status == RPR_SUCCESS);
    status = rprSceneSetCamera(scene, camera);
    assert(status == RPR_SUCCESS);

    status = rprContextSetScene(context, scene);
    assert(status == RPR_SUCCESS);
    
    //setup out
    rpr_framebuffer_desc desc;
    desc.fb_width = 900;
    desc.fb_height = 600;

    rpr_framebuffer_format fmt = { 4, RPR_COMPONENT_TYPE_FLOAT32 };
    rpr_framebuffer frame_buffer = NULL; status = rprContextCreateFrameBuffer(context, fmt, &desc, &frame_buffer);
    assert(status == RPR_SUCCESS);
    status = rprContextSetAOV(context, RPR_AOV_COLOR, frame_buffer);
    assert(status == RPR_SUCCESS);
    status = rprFrameBufferClear(frame_buffer);
    assert(status == RPR_SUCCESS);

    for (int i = 0; i < kRenderIterations; ++i)
    {
        status = rprContextRender(context);
        assert(status == RPR_SUCCESS);
    }

    status = rprFrameBufferSaveToFile(frame_buffer, "ReferenceImages/EnvironmentOverrideTest_pass0.jpg");
    assert(status == RPR_SUCCESS);

    status = rprSceneSetEnvironmentOverride(scene, RPR_SCENE_ENVIRONMENT_OVERRIDE_BACKGROUND, nullptr);
    assert(status == RPR_SUCCESS);

    status = rprSceneSetEnvironmentOverride(scene, RPR_SCENE_ENVIRONMENT_OVERRIDE_REFLECTION, nullptr);
    assert(status == RPR_SUCCESS);

    status = rprSceneSetBackgroundImage(scene, bgImage);
    assert(status == RPR_SUCCESS);

    status = rprFrameBufferClear(frame_buffer);
    assert(status == RPR_SUCCESS);

    for (int i = 0; i < kRenderIterations; ++i)
    {
        status = rprContextRender(context);
        assert(status == RPR_SUCCESS);
    }

    status = rprFrameBufferSaveToFile(frame_buffer, "ReferenceImages/EnvironmentOverrideTest_pass1.jpg");
    assert(status == RPR_SUCCESS);

    status = rprSceneSetEnvironmentOverride(scene, RPR_SCENE_ENVIRONMENT_OVERRIDE_BACKGROUND, light_background);
    assert(status == RPR_SUCCESS);

    status = rprSceneSetEnvironmentOverride(scene, RPR_SCENE_ENVIRONMENT_OVERRIDE_REFLECTION, light_reflection);
    assert(status == RPR_SUCCESS);

    status = rprSceneSetEnvironmentOverride(scene, RPR_SCENE_ENVIRONMENT_OVERRIDE_REFRACTION, nullptr);
    assert(status == RPR_SUCCESS);

    status = rprSceneSetEnvironmentOverride(scene, RPR_SCENE_ENVIRONMENT_OVERRIDE_TRANSPARENCY, nullptr);
    assert(status == RPR_SUCCESS);

    status = rprFrameBufferClear(frame_buffer);
    assert(status == RPR_SUCCESS);

    for (int i = 0; i < kRenderIterations; ++i)
    {
        status = rprContextRender(context);
        assert(status == RPR_SUCCESS);
    }

    status = rprFrameBufferSaveToFile(frame_buffer, "ReferenceImages/EnvironmentOverrideTest_pass2.jpg");
    assert(status == RPR_SUCCESS);
    
    rpr_render_statistics rs;
    status = rprContextGetInfo(context, RPR_CONTEXT_RENDER_STATISTICS, sizeof(rpr_render_statistics), &rs, NULL);
    assert(status == RPR_SUCCESS);

    status = rprSceneDetachLight(scene, light);
    assert(status == RPR_SUCCESS);
    status = rprObjectDelete(light); light = NULL;
    assert(status == RPR_SUCCESS);
    status = rprObjectDelete(bgImage);
    assert(status == RPR_SUCCESS);
    status = rprObjectDelete(reflective);
    assert(status == RPR_SUCCESS);
    status = rprObjectDelete(refractive);
    assert(status == RPR_SUCCESS);
    status = rprObjectDelete(transparent);
    assert(status == RPR_SUCCESS);
    status = rprSceneSetCamera(scene, NULL);
    assert(status == RPR_SUCCESS);
    status = rprObjectDelete(scene); scene = NULL;
    assert(status == RPR_SUCCESS);
    status = rprObjectDelete(camera); camera = NULL;
    assert(status == RPR_SUCCESS);
    status = rprObjectDelete(frame_buffer); frame_buffer = NULL;
    assert(status == RPR_SUCCESS);
    status = rprObjectDelete(matsys); matsys = NULL;
    assert(status == RPR_SUCCESS);
    status = rprObjectDelete(context); context = NULL;
    assert(status == RPR_SUCCESS);
    
}
