#include "clw_render_factory.h"

#include "Output/clwoutput.h"
#include "Renderers/monte_carlo_renderer.h"
#include "Estimators/path_tracing_estimator.h"
#include "PostEffects/bilateral_denoiser.h"

namespace Baikal
{
    ClwRenderFactory::ClwRenderFactory(CLWContext context)
    : m_context(context)
    , m_intersector(
        CreateFromOpenClContext(
            context, 
            context.GetDevice(0).GetID(), 
            context.GetCommandQueue(0)
        )
        , RadeonRays::IntersectionApi::Delete
    )
    {
    }

    // Create a renderer of specified type
    std::unique_ptr<Renderer> ClwRenderFactory::CreateRenderer(
                                                    RendererType type) const
    {
        switch (type)
        {
            case RendererType::kUnidirectionalPathTracer:
                return std::unique_ptr<Renderer>(
                    new MonteCarloRenderer(
                        m_context, 
                        std::make_unique<PathTracingEstimator>(m_context, m_intersector.get())
                        ));
            default:
                throw std::runtime_error("Renderer not supported");
        }
    }

    std::unique_ptr<Output> ClwRenderFactory::CreateOutput(std::uint32_t w,
                                                           std::uint32_t h)
                                                           const
    {
        return std::unique_ptr<Output>(new ClwOutput(m_context, w, h));
    }

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

    std::unique_ptr<SceneController<ClwScene>> ClwRenderFactory::CreateSceneController() const
    {
        return std::make_unique<ClwSceneController>(m_context, m_intersector.get());
    }
}
