#include "scene_object.h"

namespace Baikal
{
    std::uint32_t g_next_id = 0;

    SceneObject::SceneObject()
        : m_dirty(false), m_id(g_next_id++)
    {
    }

    void SceneObject::ResetId()
    {
        g_next_id = 0;
    }
}
