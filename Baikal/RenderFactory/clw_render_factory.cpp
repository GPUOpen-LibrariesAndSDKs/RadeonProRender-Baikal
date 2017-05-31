#include "clw_render_factory.h"

#include "Output/clwoutput.h"
#include "Renderers/ptrenderer.h"
#include "PostEffects/bilateral_denoiser.h"

namespace Baikal
{
    ClwRenderFactory::ClwRenderFactory(CLWContext context, int device_index)
    : m_context (context)
    , m_device_index(device_index)
    {
    }
    
    // Create a renderer of specified type
    std::unique_ptr<Renderer> ClwRenderFactory::CreateRenderer(
                                                    RendererType type) const
    {
        switch (type)
        {
            case RendererType::kUnidirectionalPathTracer:
                return std::unique_ptr<Renderer>(new PtRenderer(m_context,
                                                                0, 5));
            default:
                throw std::runtime_error("Renderer not supported");
        }
    }
    
    // Create an output of specified type
    std::unique_ptr<Output> ClwRenderFactory::CreateOutput(std::uint32_t w,
                                                           std::uint32_t h)
                                                           const
    {
        return std::unique_ptr<Output>(new ClwOutput(m_context, w, h));
    }
    
    // Create post effect of specified type
    std::unique_ptr<PostEffect> ClwRenderFactory::CreatePostEffect(
                                                    PostEffectType type) const
    {
        switch (type)
        {
            case PostEffectType::kBilateralDenoiser:
                return std::unique_ptr<PostEffect>(
                                            new BilateralDenoiser(m_context));
            default:
                throw std::runtime_error("PostEffect not supported");
        }
    }
    
    std::unique_ptr<RenderFactory> RenderFactory::CreateClwRenderFactory(
                                        CLWContext context, int device_index)
    {
        return std::unique_ptr<RenderFactory>(new ClwRenderFactory(context,
                                                            device_index));
    }
}
