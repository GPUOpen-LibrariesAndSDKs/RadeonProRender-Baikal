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

#include "WrapObject/SceneObject.h"
#include "WrapObject/ShapeObject.h"
#include "WrapObject/LightObject.h"
#include "WrapObject/CameraObject.h"
#include "SceneGraph/scene1.h"
#include "SceneGraph/light.h"
#include "SceneGraph/shape.h"
#include "SceneGraph/material.h"
#include "SceneGraph/iterator.h"

#include <assert.h>

SceneObject::SceneObject()
    : m_scene(nullptr)
{
    m_scene = new Baikal::Scene1();
}
SceneObject::~SceneObject()
{
    delete m_scene;
    m_scene = nullptr;
}

void SceneObject::Clear()
{
	m_shapes.clear();
    m_lights.clear();

    //remove lights
    for (std::unique_ptr<Baikal::Iterator> it_light(m_scene->CreateLightIterator()); it_light->IsValid();)
    {
        Baikal::LightCPtr light = it_light->ItemAs<const Baikal::Light>().lock();
        m_scene->DetachLight(light);
        it_light = m_scene->CreateLightIterator();
    }

    //remove shapes
    for (std::unique_ptr<Baikal::Iterator> it_shape(m_scene->CreateShapeIterator()); it_shape->IsValid();)
    {
        Baikal::ShapeCPtr shape = it_shape->ItemAs<const Baikal::Mesh>().lock();
        m_scene->DetachShape(shape);
        it_shape = m_scene->CreateShapeIterator();
    }
}

void SceneObject::AttachShape(ShapeObject* shape)
{
	//check is mesh already in scene
	auto it = std::find(m_shapes.begin(), m_shapes.end(), shape);
	if (it != m_shapes.end())
	{
		return;
	}
	m_shapes.push_back(shape);

    m_scene->AttachShape(shape->GetShape());
}

void SceneObject::DetachShape(ShapeObject* shape)
{
	//check is mesh in scene
	auto it = std::find(m_shapes.begin(), m_shapes.end(), shape);
	if (it == m_shapes.end())
	{
		return;
	}
	m_shapes.erase(it);
	m_scene->DetachShape(shape->GetShape());
}

void SceneObject::AttachLight(LightObject* light)
{
    //check is light already in scene
    auto it = std::find(m_lights.begin(), m_lights.end(), light);
    if (it != m_lights.end())
    {
        return;
    }
    m_lights.push_back(light);

    m_scene->AttachLight(light->GetLight());
}

void SceneObject::DetachLight(LightObject* light)
{
    //check is light in scene
    auto it = std::find(m_lights.begin(), m_lights.end(), light);
    if (it == m_lights.end())
    {
        return;
    }
    m_lights.erase(it);
    m_scene->DetachLight(light->GetLight());
}

void SceneObject::SetCamera(CameraObject* cam)
{
    m_current_camera = cam;
    Baikal::CameraPtr baikal_cam = cam ? cam->GetCamera() : nullptr;
    m_scene->SetCamera(baikal_cam);
}

void SceneObject::GetShapeList(void* out_list)
{
	memcpy(out_list, m_shapes.data(), m_shapes.size() * sizeof(ShapeObject*));
}

void SceneObject::GetLightList(void* out_list)
{
    memcpy(out_list, m_lights.data(), m_lights.size() * sizeof(LightObject*));
}


void SceneObject::AddEmissive()
{
    //TODO: check scene isn't changed
    //recreate emissive if scene is dirty
    if (!m_scene->GetDirtyFlags())
	{
		return;
	}
    RemoveEmissive();

	//handle emissive materials
	for (std::unique_ptr<Baikal::Iterator> it_shape(m_scene->CreateShapeIterator()); it_shape->IsValid(); it_shape->Next())
	{
		Baikal::ShapeCPtr shape = it_shape->ItemAs<const Baikal::Shape>().lock();
		std::shared_ptr<Baikal::Mesh const> mesh = std::dynamic_pointer_cast<Baikal::Mesh const>(shape);
		//instance case
		if (!mesh)
		{
            std::shared_ptr<Baikal::Instance const> inst = std::dynamic_pointer_cast<Baikal::Instance const>(shape);
			assert(inst);
			mesh = std::dynamic_pointer_cast<Baikal::Mesh const>(inst->GetBaseShape());
		}
		assert(mesh);

		const Baikal::MaterialCPtr mat = mesh->GetMaterial();
		if (!mat)
		{
			continue;
		}
		//fine shapes with emissive material
		if (mat->HasEmission())
		{
			// Add area light for each polygon of emissive mesh
			for (int l = 0; l < mesh->GetNumIndices() / 3; ++l)
			{
				std::shared_ptr<Baikal::AreaLight> light(new Baikal::AreaLight(mesh, l));
				m_scene->AttachLight(light);
				m_emmisive_lights.push_back(light);
			}
		}
	}
}

void SceneObject::RemoveEmissive()
{
	for (auto light : m_emmisive_lights)
	{
		m_scene->DetachLight(light);
	}
	m_emmisive_lights.clear();
}

bool SceneObject::IsDirty()
{
    auto dirty = m_scene->GetDirtyFlags();
    return dirty != Baikal::Scene1::kNone;
}
