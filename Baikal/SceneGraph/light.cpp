#include "light.h"
#include "SceneGraph/scene1.h"
#include "SceneGraph/texture.h"

namespace Baikal
{
    AreaLight::AreaLight(Shape const* shape, std::size_t idx)
        : m_shape(shape)
        , m_prim_idx(idx)
    {
    }

    RadeonRays::float3 Light::GetPosition() const
    {
        return m_p;
    }

    void Light::SetPosition(RadeonRays::float3 const& p)
    {
        m_p = p;
        SetDirty(true);
    }

    RadeonRays::float3 Light::GetDirection() const
    {
        return m_d;
    }

    void Light::SetDirection(RadeonRays::float3 const& d)
    {
        m_d = normalize(d);
        SetDirty(true);
    }

    Iterator* Light::CreateTextureIterator() const
    {
        return new EmptyIterator();
    }

    RadeonRays::float3 Light::GetEmittedRadiance() const
    {
        return m_e;
    }

    void Light::SetEmittedRadiance(RadeonRays::float3 const& e)
    {
        m_e = e;
        SetDirty(true);
    }

    void SpotLight::SetConeShape(RadeonRays::float2 angles)
    {
        m_angles = angles;
        SetDirty(true);
    }

    RadeonRays::float2 SpotLight::GetConeShape() const
    {
        return m_angles;
    }

    void ImageBasedLight::SetTexture(Texture const* texture)
    {
        m_texture = texture;
        SetDirty(true);
    }

    Texture const* ImageBasedLight::GetTexture() const
    {
        return m_texture;
    }

    std::size_t AreaLight::GetPrimitiveIdx() const
    {
        return m_prim_idx;
    }

    Shape const* AreaLight::GetShape() const
    {
        return m_shape;
    }

    ImageBasedLight::ImageBasedLight()
        : m_texture(nullptr)
        , m_multiplier(1.f)
    {
    }

    float ImageBasedLight::GetMultiplier() const
    {
        return m_multiplier;
    }

    void ImageBasedLight::SetMultiplier(float m)
    {
        m_multiplier = m;
        SetDirty(true);
    }

    Iterator* ImageBasedLight::CreateTextureIterator() const
    {
        std::set<Texture const*> result;

        if (m_texture)
        {
            result.insert(m_texture);
        }

        return new ContainerIterator<std::set<Texture const*>>(std::move(result));
    }


    RadeonRays::float3 PointLight::GetPower(Scene1 const& scene) const
    {
        return 4.f * PI * GetEmittedRadiance();
    }

    RadeonRays::float3 SpotLight::GetPower(Scene1 const& scene) const
    {
        auto cone = GetConeShape();
        return 2.f * PI * GetEmittedRadiance() * (1.f - 0.5f * (cone.x + cone.y));
    }

    RadeonRays::float3 DirectionalLight::GetPower(Scene1 const& scene) const
    {
        auto scene_radius = scene.GetRadius();
        return PI * GetEmittedRadiance() * scene_radius * scene_radius;
    }

    RadeonRays::float3 ImageBasedLight::GetPower(Scene1 const& scene) const
    {
        auto scene_radius = scene.GetRadius();
        auto avg = m_texture->ComputeAverageValue();
        return PI * avg * scene_radius * scene_radius;
    }

    RadeonRays::float3 AreaLight::GetPower(Scene1 const& scene) const
    {
        auto mesh = static_cast<Mesh const*>(m_shape);
        auto indices = mesh->GetIndices();
        auto vertices = mesh->GetVertices();

        auto i0 = indices[m_prim_idx * 3];
        auto i1 = indices[m_prim_idx * 3 + 1];
        auto i2 = indices[m_prim_idx * 3 + 2];

        auto v0 = vertices[i0];
        auto v1 = vertices[i1];
        auto v2 = vertices[i2];

        float area = 0.5f * std::sqrt(cross(v2 - v0, v1 - v0).sqnorm());
        return PI * GetEmittedRadiance() * area;
    }
}