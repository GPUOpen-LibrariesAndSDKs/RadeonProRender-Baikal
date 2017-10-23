/*****************************************************************************\
*
*  Module Name    RprSupport.cpp
*  Project        RRP support library
*
*  Description    RRP support library implementation file
*
*  Copyright 2017 Advanced Micro Devices, Inc.
*
*  All rights reserved.  This notice is intended as a precaution against
*  inadvertent publication and does not imply publication or any waiver
*  of confidentiality.  The year included in the foregoing notice is the
*  year of creation of the work.
\*****************************************************************************/
#include "RprSupport.h"
#include "MaterialFactory.h"
#include "Material.h"

#include <cstdint>
#include <string>
#include <sstream>
#include <algorithm>
#include <memory>

namespace RprSupport
{
    class Logger
    {
    public:
        void Clear() { m_log.clear(); }

        void Mute() { m_muted = true; }
        void Unmute() { m_muted = false; }

        template <typename T> Logger& Print(T const& message)
        {
            if (!m_muted)
            {
                std::ostringstream oss;
                oss << message;
                m_log.append(oss.str());
            }

            return *this;
        }

        std::size_t size() const { return m_log.size(); }

        std::size_t CopyBuffer(std::uint32_t count, char* dst) const
        {
            if (count <= size())
            {
                std::copy(m_log.cbegin(), m_log.cend(), dst);
                return size();
            }
            else
            {
                std::copy(m_log.cend() - count, m_log.cend(), dst);
                return count;
            }
        }

    private:
        std::string m_log;
        bool m_muted = true;
    };

    struct Context
    {
        std::unique_ptr<MaterialFactory> material_factory;
        std::map<rprx_material, std::set<rpr_shape>> shape_bindings;
        std::map<rprx_material, std::set<std::pair<rpr_material_node, std::string>>> material_bindings;
        Logger logger;
    };
}


using namespace RprSupport;

inline void ThrowIfRprFailed(rpr_int status, std::string const& message)
{
    if (status != RPR_SUCCESS)
        throw std::runtime_error(message);
}

rpr_int rprxCreateContext(rpr_material_system material_system, rpr_uint flags, rprx_context* out_context)
{
    try
    {
        auto context = new Context;
        context->material_factory.reset(MaterialFactory::CreateDefault(material_system));

        if (flags & RPRX_FLAGS_ENABLE_LOGGING)
        {
            context->logger.Unmute();
        }

        *out_context = reinterpret_cast<rprx_context>(context);
    }
    catch (std::runtime_error& e)
    {
        return RPR_ERROR_INTERNAL_ERROR;
    }

    return RPR_SUCCESS;
}

rpr_int rprxCreateMaterial(rprx_context context, rprx_material_type type, rprx_material* out_material)
{
    auto ctx = reinterpret_cast<Context*>(context);

    if (!out_material)
    {
        ctx->logger.Print("nullptr passed as output argument");
        return RPR_ERROR_INVALID_PARAMETER;
    }

    try
    {
        *out_material = reinterpret_cast<rprx_material>(ctx->material_factory->CreateMaterial(type));
    }
    catch (std::runtime_error& e)
    {
        ctx->logger.Print(e.what());
        return RPR_ERROR_INTERNAL_ERROR;
    }

    return RPR_SUCCESS;
}

rpr_int rprxMaterialDelete(rprx_context context, rprx_material material)
{
    auto ctx = reinterpret_cast<Context*>(context);

    // Remove material if it is bound to anything
    {
        auto iter = ctx->shape_bindings.find(material);

        if (iter != ctx->shape_bindings.cend())
        {
            for (auto& shape : iter->second)
            {
                rprShapeSetMaterial(shape, nullptr);
            }

            ctx->shape_bindings.erase(iter);
        }
    }
    {
        auto iter = ctx->material_bindings.find(material);

        if (iter != ctx->material_bindings.cend())
        {
            for (auto& pair : iter->second)
            {
                rprMaterialNodeSetInputN(pair.first, pair.second.c_str(), nullptr);
            }

            ctx->material_bindings.erase(iter);
        }
    }

    delete reinterpret_cast<Material*>(material);
    return RPR_SUCCESS;
}

rpr_int rprxMaterialSetParameterN(rprx_context context, rprx_material material, rprx_parameter parameter, rpr_material_node  node)
{
    auto ctx = reinterpret_cast<Context*>(context);

    try
    {
        reinterpret_cast<Material*>(material)->SetInput(parameter, node);
    }
    catch (std::runtime_error& e)
    {
        ctx->logger.Print(e.what());
        return RPR_ERROR_INTERNAL_ERROR;
    }

    return RPR_SUCCESS;
}

rpr_int rprxMaterialSetParameterU(rprx_context context, rprx_material material, rprx_parameter parameter, rpr_uint value)
{
    auto ctx = reinterpret_cast<Context*>(context);

    try
    {
        reinterpret_cast<Material*>(material)->SetInput(parameter, value);
    }
    catch (std::runtime_error& e)
    {
        ctx->logger.Print(e.what());
        return RPR_ERROR_INTERNAL_ERROR;
    }

    return RPR_SUCCESS;
}

rpr_int rprxMaterialSetParameterF(rprx_context context, rprx_material material, rprx_parameter parameter, rpr_float x, rpr_float y, rpr_float z, rpr_float w)
{
    auto ctx = reinterpret_cast<Context*>(context);

    try
    {
        reinterpret_cast<Material*>(material)->SetInput(parameter, { x, y, z, w });
    }
    catch (std::runtime_error& e)
    {
        ctx->logger.Print(e.what());
        return RPR_ERROR_INTERNAL_ERROR;
    }

    return RPR_SUCCESS;
}

rpr_int rprxMaterialCommit(rprx_context context, rprx_material material)
{
    auto ctx = reinterpret_cast<Context*>(context);

    try
    {
        // Compile material into bundle
        auto bundle = reinterpret_cast<Material*>(material)->Compile();

        {
            // Find objects this material is bound to
            auto iter = ctx->shape_bindings.find(material);

            // If we have at least one object
            if (iter != ctx->shape_bindings.cend())
            {
                // Update all the objects with potentially new material nodes
                for (auto& shape : iter->second)
                {
                    auto status = rprShapeSetMaterial(shape, bundle.surface_material);
                    ThrowIfRprFailed(status, "Error: RPR - rprShapeSetMaterial failed");

                    status = rprShapeSetVolumeMaterial(shape, bundle.volume_material);
                    ThrowIfRprFailed(status, "Error: RPR - rprShapeSetVolumeMaterial failed");

                    status = rprShapeSetDisplacementMaterial(shape, bundle.displacement_material );
                    ThrowIfRprFailed(status, "Error: RPR - rprShapeSetDisplacementMaterial failed");
                }
            }
        }

        {
            // Find materials this material is bound to
            auto iter = ctx->material_bindings.find(material);

            // If we have at least one object
            if (iter != ctx->material_bindings.cend())
            {
                // Update all the objects with potentially new material nodes
                for (auto& pair : iter->second)
                {
                    auto status = rprMaterialNodeSetInputN(pair.first, pair.second.c_str(), bundle.surface_material);
                    ThrowIfRprFailed(status, "Error: RPR - rprMaterialNodeSetInputN failed");
                }
            }
        }
    }
    catch (std::runtime_error& e)
    {
        ctx->logger.Print(e.what());
        return RPR_ERROR_INTERNAL_ERROR;
    }

    return RPR_SUCCESS;
}

rpr_int rprxShapeAttachMaterial(rprx_context context, rpr_shape shape, rprx_material material)
{
    auto ctx = reinterpret_cast<Context*>(context);

    if (!shape)
    {
        ctx->logger.Print("Error: rprxShapeAttachMaterial - shape argument can't be nullptr");
        return RPR_ERROR_INVALID_PARAMETER;
    }

    if (!material)
    {
        ctx->logger.Print("Error: rprxShapeAttachMaterial - material argument can't be nullptr");
        return RPR_ERROR_INVALID_PARAMETER;
    }

    auto iter = ctx->shape_bindings.find(material);

    if (iter != ctx->shape_bindings.cend())
    {
        iter->second.emplace(shape);
    }
    else
    {
        ctx->shape_bindings.insert(std::make_pair(material, std::set<rpr_shape>{shape}));
    }

    return RPR_SUCCESS;
}

rpr_int rprxShapeGetMaterial(rprx_context context, rpr_shape shape, rprx_material* material)
{
	auto ctx = reinterpret_cast<Context*>(context);

    if (!shape)
    {
        ctx->logger.Print("Error: rprxShapeAttachMaterial - shape argument can't be nullptr");
        return RPR_ERROR_INVALID_PARAMETER;
    }

	if (!material)
    {
        ctx->logger.Print("Error: rprxShapeAttachMaterial - material argument can't be nullptr");
        return RPR_ERROR_INVALID_PARAMETER;
    }

	
	*material = nullptr;

	for (auto& iMat : ctx->shape_bindings)
    {
		for (auto& iShape : iMat.second)
		{
			if ( iShape == shape )
			{
				*material = iMat.first;
				return RPR_SUCCESS;
			}
		}
    }

    return RPR_SUCCESS;

}

rpr_int rprxShapeDetachMaterial(rprx_context context, rpr_shape shape, rprx_material material)
{
    auto ctx = reinterpret_cast<Context*>(context);

    if (!shape)
    {
        ctx->logger.Print("Error: rprxShapeAttachMaterial - shape argument can't be nullptr");
        return RPR_ERROR_INVALID_PARAMETER;
    }

    if (!material)
    {
        ctx->logger.Print("Error: rprxShapeAttachMaterial - material argument can't be nullptr");
        return RPR_ERROR_INVALID_PARAMETER;
    }

    auto iter = ctx->shape_bindings.find(material);

    if (iter != ctx->shape_bindings.cend())
    {
        iter->second.erase(shape);

        if (iter->second.empty())
        {
            ctx->shape_bindings.erase(iter);
        }
    }

    return RPR_SUCCESS;
}

rpr_int rprxMaterialAttachMaterial(rprx_context context, rpr_material_node node, rpr_char const* parameter, rprx_material material)
{
    auto ctx = reinterpret_cast<Context*>(context);

    if (!node)
    {
        ctx->logger.Print("Error: rprxMaterialAttachMaterial - node argument can't be nullptr");
        return RPR_ERROR_INVALID_PARAMETER;
    }

    if (!material)
    {
        ctx->logger.Print("Error: rprxShapeAttachMaterial - material argument can't be nullptr");
        return RPR_ERROR_INVALID_PARAMETER;
    }

    if (!parameter)
    {
        ctx->logger.Print("Error: rprxShapeAttachMaterial - parameter argument can't be nullptr");
        return RPR_ERROR_INVALID_PARAMETER;
    }

    auto iter = ctx->material_bindings.find(material);

    if (iter != ctx->material_bindings.cend())
    {
        iter->second.emplace(std::make_pair(node, parameter));
    }
    else
    {
        ctx->material_bindings.insert(std::make_pair(material, std::set<std::pair<rpr_material_node, std::string>>{std::make_pair(node, parameter)}));
    }

    return RPR_SUCCESS;
}

rpr_int rprxMaterialDetachMaterial(rprx_context context, rpr_material_node node, rpr_char const* parameter, rprx_material material)
{
    auto ctx = reinterpret_cast<Context*>(context);

    if (!node)
    {
        ctx->logger.Print("Error: rprxMaterialAttachMaterial - node argument can't be nullptr");
        return RPR_ERROR_INVALID_PARAMETER;
    }

    if (!material)
    {
        ctx->logger.Print("Error: rprxShapeAttachMaterial - material argument can't be nullptr");
        return RPR_ERROR_INVALID_PARAMETER;
    }

    if (!parameter)
    {
        ctx->logger.Print("Error: rprxShapeAttachMaterial - parameter argument can't be nullptr");
        return RPR_ERROR_INVALID_PARAMETER;
    }

    auto iter = ctx->material_bindings.find(material);

    if (iter != ctx->material_bindings.cend())
    {
        auto binding = iter->second.find(std::make_pair(node, parameter));

        if (binding != iter->second.cend())
        {
            iter->second.erase(binding);
        }

        if (iter->second.empty())
        {
            ctx->material_bindings.erase(iter);
        }
    }

    return RPR_SUCCESS;
}

rpr_int rprxDeleteContext(rprx_context context)
{
    auto ctx = reinterpret_cast<Context*>(context);
    delete ctx;
    return RPR_SUCCESS;
}


rpr_int rprxGetLog(rprx_context context, rpr_char* log, size_t* size)
{
    auto ctx = reinterpret_cast<Context*>(context);

    if (!size)
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }

    if (!log)
    {
        *size = ctx->logger.size() + 1;
        return RPR_SUCCESS;
    }

    if (*size <= 0)
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }

    auto copied_count = ctx->logger.CopyBuffer(static_cast<std::uint32_t>(*size - 1), log);
    log[copied_count] = 0;

    return RPR_SUCCESS;
}

rpr_int rprxIsMaterialRprx(rprx_context context, rpr_material_node node, rpr_bool* out_result)
{
	if (!context)
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }

    auto ctx = reinterpret_cast<Context*>(context);

    if (!node)
    {
        ctx->logger.Print("Error: rprxIsMaterialRprx - node argument can't be nullptr");
        return RPR_ERROR_INVALID_PARAMETER;
    }

    *out_result = false;

    for (auto& binding : ctx->shape_bindings)
    {
        auto material = reinterpret_cast<Material const*>(binding.first);
        auto bundle = material->GetBundle();

        if (bundle.surface_material == node || bundle.volume_material == node)
        {
            *out_result = true;
			return RPR_SUCCESS;
        }
    }

    return RPR_SUCCESS;
}

rpr_int rprxMaterialGetParameterType(rprx_context context, rprx_material material, rprx_parameter parameter, rpr_parameter_type* out_type)
{
    auto ctx = reinterpret_cast<Context*>(context);

    try
    {
        auto input = reinterpret_cast<Material*>(material)->GetInput(parameter);

        switch (input.type)
        {
        case Material::InputType::kFloat4:
            *out_type = RPRX_PARAMETER_TYPE_FLOAT4;
			break;
        case Material::InputType::kUint:
            *out_type = RPRX_PARAMETER_TYPE_UINT;
			break;
        case Material::InputType::kPointer:
            *out_type = RPRX_PARAMETER_TYPE_NODE;
			break;
        }

        return RPR_SUCCESS;
    }
    catch (std::runtime_error& e)
    {
        ctx->logger.Print(e.what());
        return RPR_ERROR_INTERNAL_ERROR;
    }
}

rpr_int rprxMaterialGetParameterValue(rprx_context context, rprx_material material, rprx_parameter parameter, void* out_value)
{
    auto ctx = reinterpret_cast<Context*>(context);

    try
    {
        auto input = reinterpret_cast<Material*>(material)->GetInput(parameter);

        switch (input.type)
        {
        case Material::InputType::kFloat4:
        {
            auto out_ptr = reinterpret_cast<float*>(out_value);
            *out_ptr++ = input.value_f4.x;
            *out_ptr++ = input.value_f4.y;
            *out_ptr++ = input.value_f4.z;
            *out_ptr++ = input.value_f4.w;
			break;
        }
        case Material::InputType::kUint:
        {
            auto out_ptr = reinterpret_cast<rpr_uint*>(out_value);
            *out_ptr = input.value_ui;
			break;
        }
        case Material::InputType::kPointer:
        {
            auto out_ptr = reinterpret_cast<void**>(out_value);
            *out_ptr = input.value_ptr;
			break;
        }
        }

        return RPR_SUCCESS;
    }
    catch (std::runtime_error& e)
    {
        ctx->logger.Print(e.what());
        return RPR_ERROR_INTERNAL_ERROR;
    }
}
