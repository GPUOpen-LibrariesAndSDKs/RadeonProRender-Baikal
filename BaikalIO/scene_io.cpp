#include "scene_io.h"
#include "image_io.h"
#include "SceneGraph/scene1.h"
#include "SceneGraph/shape.h"
#include "SceneGraph/material.h"
#include "SceneGraph/light.h"
#include "SceneGraph/texture.h"
#include "math/mathutils.h"

#include <string>
#include <map>
#include <set>
#include <cassert>

#include "Utils/log.h"

namespace Baikal
{
    std::map<std::string, SceneIo::Loader*> SceneIo::m_loaders;

    void SceneIo::RegisterLoader(const std::string& ext, SceneIo::Loader *loader)
    {
        m_loaders[ext] = loader;
    }

    void SceneIo::UnregisterLoader(const std::string& ext)
    {
        m_loaders.erase(ext);
    }

    Scene1::Ptr SceneIo::LoadScene(std::string const& filename, std::string const& basepath)
    {
        auto ext = filename.substr(filename.rfind(".") + 1);

        auto loader_it = m_loaders.find(ext);
        if (loader_it == m_loaders.end())
        {
            throw std::runtime_error("No loader for \"" + filename + "\" has been found.");
        }

        return loader_it->second->LoadScene(filename, basepath);
    }

    void SceneIo::SaveScene(Scene1 const& scene, std::string const& filename, std::string const& basepath)
    {
        auto ext = filename.substr(filename.rfind("."));

        auto loader_it = m_loaders.find(ext);
        if (loader_it == m_loaders.end())
        {
            throw std::runtime_error("No serializer for \"" + filename + "\" has been found.");
        }

        return loader_it->second->SaveScene(scene, filename, basepath);
    }

    Texture::Ptr SceneIo::Loader::LoadTexture(ImageIo const& io, Scene1& scene, std::string const& basepath, std::string const& name) const
    {
        auto iter = m_texture_cache.find(name);

        if (iter != m_texture_cache.cend())
        {
            return iter->second;
        }
        else
        {
            try
            {
                LogInfo("Loading ", name, "\n");
                auto texture = io.LoadImage(basepath + name);
                texture->SetName(name);
                m_texture_cache[name] = texture;
                return texture;
            }
            catch (std::runtime_error)
            {
                LogInfo("Missing texture: ", name, "\n");
                return nullptr;
            }
        }
    }

    SceneIo::Loader::Loader(const std::string& ext, SceneIo::Loader *loader) :
        m_ext(ext)

    {
        SceneIo::RegisterLoader(m_ext, loader);
    }

    SceneIo::Loader::~Loader()
    {
        SceneIo::UnregisterLoader(m_ext);
    }

}
