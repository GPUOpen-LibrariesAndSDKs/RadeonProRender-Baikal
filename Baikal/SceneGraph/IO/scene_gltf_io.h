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

/**
 \file scene_io.h
 \author Dmitry Kozlov
 \version 1.0
 \brief Contains an interface for s3d scene loading
 */
#pragma once

#include <string>
#include <unordered_set>

#include "math/matrix.h"
#include "Baikal/SceneGraph/IO/scene_io.h"
#include "Baikal/SceneGraph/material.h"
#include "SceneGraph/IO/gltf/gltf2.h"
#include "SceneGraph/IO/gltf/Extensions/Extensions.h"

namespace Baikal
{
    // glTF scene loader
    class SceneGltfIo : public SceneIo
    {
    public:

		// Load scene from file
        virtual Scene1::Ptr LoadScene(std::string const& filename, std::string const& basepath) const override;
    private:
        struct InterleavedAttributesResult
        {
            std::vector<char*> bufferData;
            std::unordered_map<std::string, char*> attributeMemoryAddresses;
        };


        RadeonRays::matrix GetNodeTransformMatrix(const gltf::Node& node, const RadeonRays::matrix& worldTransform) const;

        void ImportPostEffects() const;
        void ImportSceneParameters(Scene1::Ptr scene, int scene_index) const;
        void ImportSceneLights(Scene1::Ptr scene, int scene_index) const;
        void ImportNode(Scene1::Ptr scene, int scene_index, int node_index, RadeonRays::matrix& world_transform) const;
        void ImportCamera(Scene1::Ptr scene, gltf::Node const& node, RadeonRays::matrix const& world_transform) const;
        Shape::Ptr ImportMesh(Scene1::Ptr scene, int node_index, RadeonRays::matrix const& world_transform) const;
        void ImportShapeParameters(nlohmann::json const& extras, Shape::Ptr shape) const;
        std::istream* ImportBuffer(int bufferIndex) const;
        void ImportMaterial(Shape::Ptr shape, int materialIndex) const;
        Material::Ptr ImportMaterialNode(std::vector<amd::Node>& nodes, int nodeIndex) const;

        Texture::Ptr ImportImage(int imageIndex) const;

        InterleavedAttributesResult GetInterleavedAttributeData(const std::unordered_map<std::string, int>& attributes) const;
        void DestroyInterleavedAttributesResult(InterleavedAttributesResult& iar) const;
        void ApplyNormalMapAndEmissive(SingleBxdf::Ptr uber_material, gltf::Material& material) const;

        mutable std::string m_root_dir;
        mutable gltf::glTF m_gltf;
        mutable std::unordered_set<int> m_visited_nodes;
        mutable std::unordered_map<int, Shape::Ptr> m_shape_cache;
        mutable std::unordered_map<int, Texture::Ptr> m_image_cache;
        mutable std::unordered_map<int, std::istream*> m_buffer_cache;
        mutable std::unordered_map<int, Material::Ptr> m_material_cache;

    };

}
