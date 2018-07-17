#include "scene_object.h"
#include <cassert>

namespace Baikal
{
    static std::uint32_t g_next_id = 0;
    static int g_scene_controller_id = -1;

    SceneObject::SceneObject()
        : m_dirty(0), m_id(g_next_id++)
    {
    }

    bool SceneObject::IsDirty() const
    {
        bool dirty = false;
        if (g_scene_controller_id == -1)
        {
            dirty = (m_dirty != 0);
        }
        else
        {
            dirty = (m_dirty & (1 << g_scene_controller_id)) == (1 << g_scene_controller_id);
        }

        return dirty;
    }

    void SceneObject::SetDirty(bool dirty) const
    {
        if (dirty)
        {
            m_dirty = 0xFFFFFFFF;
        }
        else
        {
            if (g_scene_controller_id == -1)
            {
                m_dirty = 0;
            }
            else
            {
                m_dirty &= ~(1 << g_scene_controller_id);
            }
        }
    }

    void SceneObject::ResetId()
    {
        g_next_id = 0;
    }

    void SceneObject::SetSceneControllerId(std::uint32_t controller_id)
    {
        assert(controller_id >= 0 && controller_id < sizeof(std::uint32_t) * 8);
        g_scene_controller_id = static_cast<int>(controller_id);
    }

    void SceneObject::ResetSceneControllerId()
    {
        g_scene_controller_id = -1;
    }

}
