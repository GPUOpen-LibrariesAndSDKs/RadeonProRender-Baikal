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
 \file shape.h
 \author Dmitry Kozlov
 \version 1.0
 \brief Contains declaration of various shape types supported by the renderer.
 */
#pragma once

#include "math/float3.h"
#include "math/float2.h"
#include "math/matrix.h"
#include <memory>
#include <string>
#include <vector>

#include "scene_object.h"
#include "material.h"

namespace Baikal
{
    class Mesh;
    class Shape;
    class Instance;
    using ShapePtr = std::shared_ptr<Shape>;
    using ShapeCPtr = std::shared_ptr<Shape const>;
    using MeshPtr = std::shared_ptr<Mesh>;
    using InstancePtr = std::shared_ptr<Instance>;

    /**
     \brief Shape base interface.
     
     High-level interface all shape classes need to implement.
     */
    class Shape : public SceneObject
    {
    public:
        // Constructor
        Shape();
        // Destructor
        virtual ~Shape() = 0;

        // Get and set material
        void SetMaterial(MaterialCPtr material);
        MaterialCPtr GetMaterial() const;

        // Get and set transform
        void SetTransform(RadeonRays::matrix const& t);
        RadeonRays::matrix GetTransform() const;

        // Set whether a shape casts shadow or not
        void SetShadow(bool shadow);
        bool GetShadow() const;

        // Forbidden stuff
        Shape(Shape const&) = delete;
        Shape& operator = (Shape const&) = delete;
    private:
        MaterialCPtr m_material;

        RadeonRays::matrix m_transform;

        bool m_shadow;
    };



    /**
     \brief Triangle mesh class.
     
     Triangle mesh is a collection of indexed triangle.
     */
    class Mesh : public Shape
    {
    public:
        Mesh();
        
        // Set and get index array
        void SetIndices(std::uint32_t const* indices, std::size_t num_indices);
        void SetIndices(std::vector<std::uint32_t>&& indices);
        std::size_t GetNumIndices() const;
        std::uint32_t const* GetIndices() const;
        
        // Set and get vertex array
        void SetVertices(RadeonRays::float3 const* vertices, std::size_t num_vertices);
        void SetVertices(float const* vertices, std::size_t num_vertices);
        void SetVertices(std::vector<RadeonRays::float3>&& vertices);

        std::size_t GetNumVertices() const;
        RadeonRays::float3 const* GetVertices() const;
        
        // Set and get normal array
        void SetNormals(RadeonRays::float3 const* normals, std::size_t num_normals);
        void SetNormals(float const* normals, std::size_t num_normals);
        void SetNormals(std::vector<RadeonRays::float3>&& normals);

        std::size_t GetNumNormals() const;
        RadeonRays::float3 const* GetNormals() const;
        
        // Set and get UV array
        void SetUVs(RadeonRays::float2 const* uvs, std::size_t num_uvs);
        void SetUVs(float const* uvs, std::size_t num_uvs);
        void SetUVs(std::vector<RadeonRays::float2>&& uvs);
        std::size_t GetNumUVs() const;
        RadeonRays::float2 const* GetUVs() const;

        // Forbidden stuff
        Mesh(Mesh const&) = delete;
        Mesh& operator = (Mesh const&) = delete;
        
    private:
        std::vector<RadeonRays::float3> m_vertices;
        std::vector<RadeonRays::float3> m_normals;
        std::vector<RadeonRays::float2> m_uvs;
        std::vector<std::uint32_t> m_indices;
    };
    
    inline Shape::~Shape()
    {
    }
    
    inline Shape::Shape() 
        : m_material(nullptr)
        , m_shadow(true)
    {
    }
    
    inline void Shape::SetMaterial(MaterialCPtr material)
    {
        m_material = material;
        SetDirty(true);
    }
    
    inline MaterialCPtr Shape::GetMaterial() const
    {
        return m_material;
    }

    inline void Shape::SetTransform(RadeonRays::matrix const& t)
    {
        m_transform = t;
        SetDirty(true);
    }

    inline RadeonRays::matrix Shape::GetTransform() const
    {
        return m_transform;
    }

    inline void Shape::SetShadow(bool shadow)
    {
        m_shadow = shadow;
        SetDirty(true);
    }

    inline bool Shape::GetShadow() const
    {
        return m_shadow;
    }

    /**
    \brief Instance class.

    Instance references some mesh, but might have different transform and material.
    */
    class Instance : public Shape
    {
    public:
        Instance(ShapeCPtr const& base_shape);

        // Get and set base shape
        void SetBaseShape(ShapeCPtr const& base_shape);
        ShapeCPtr const GetBaseShape() const;

        // Forbidden stuff
        Instance(Instance const&) = delete;
        Instance& operator = (Instance const&) = delete;

    private:
        ShapeCPtr m_base_shape;
    };

    inline Instance::Instance(ShapeCPtr const& base_shape)
        : m_base_shape(base_shape)
    {
    }

    inline void Instance::SetBaseShape(ShapeCPtr const& base_shape)
    {
        m_base_shape = base_shape;
        SetDirty(true);
    }

    inline ShapeCPtr const Instance::GetBaseShape() const
    {
        return m_base_shape;
    }
}

