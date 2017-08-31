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
#include "monte_carlo_renderer.h"
#include "Output/clwoutput.h"
#include "Estimators/estimator.h"

#include <numeric>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <cstdint>
#include <random>
#include <algorithm>

#include "Utils/sobol.h"
#include "math/int2.h"

#ifdef RR_EMBED_KERNELS
#include "./Kernels/CL/cache/kernels.h"
#endif

namespace Baikal
{
    using namespace RadeonRays;

    int constexpr kMaxLightSamples = 1;
    int constexpr kTileSizeX = 1920;
    int constexpr kTileSizeY = 1080;

    // Constructor
    MonteCarloRenderer::MonteCarloRenderer(CLWContext context, std::unique_ptr<Estimator> estimator)
        : Baikal::ClwClass(context, "../Baikal/Kernels/CL/monte_carlo_renderer.cl")
        , m_estimator(std::move(estimator))
        , m_sample_counter(0u)
    {
    }

    void MonteCarloRenderer::Clear(RadeonRays::float3 const& val, Output& output) const
    {
        static_cast<ClwOutput&>(output).Clear(val);
    }

    void MonteCarloRenderer::Render(ClwScene const& scene)
    {
        auto output = FindFirstNonZeroOutput();
        auto output_size = int2(output->width(), output->height());

        if (output_size.x > kTileSizeX || output_size.y > kTileSizeY)
        {
            auto num_tiles_x = (output_size.x + kTileSizeX - 1) / kTileSizeX;
            auto num_tiles_y = (output_size.y + kTileSizeY - 1) / kTileSizeY;

            for (auto x = 0; x < num_tiles_x; ++x)
                for (auto y = 0; y < num_tiles_y; ++y)
                {
                    auto tile_offset = int2(x * kTileSizeX, y * kTileSizeY);
                    auto tile_size = int2(std::min(kTileSizeX, output_size.x - tile_offset.x),
                        std::min(kTileSizeY, output_size.y - tile_offset.y));

                    RenderTile(scene, tile_offset, tile_size);
                }
        }
        else
        {
            RenderTile(scene, int2(), output_size);
        }

        ++m_sample_counter;
    }

    // Render the scene into the output
    void MonteCarloRenderer::RenderTile(ClwScene const& scene, int2 const& tile_origin, int2 const& tile_size)
    {
        // Number of rays to generate
        auto output = static_cast<ClwOutput*>(GetOutput(OutputType::kColor));

        if (output)
        {
            auto num_rays = tile_size.x * tile_size.y;
            auto output_size = int2(output->width(), output->height());

            GenerateTileDomain(output_size, tile_origin, tile_size, tile_size);
            GeneratePrimaryRays(scene, *output, tile_size);

            m_estimator->Estimate(
                scene,
                num_rays,
                Estimator::QualityLevel::kStandard,
                output->data());
        }

        // Check if we have other outputs, than color
        bool aov_pass_needed = (FindFirstNonZeroOutput(false) != nullptr);
        if (aov_pass_needed)
        {
            FillAOVs(scene, tile_origin, tile_size);
            GetContext().Flush(0);
        }
    }

    void MonteCarloRenderer::GenerateTileDomain(
        int2 const& output_size, 
        int2 const& tile_origin,
        int2 const& tile_size, 
        int2 const& subtile_size
    )
    {
        // Fetch kernel
        CLWKernel generate_kernel = GetKernel("GenerateTileDomain");

        // Set kernel parameters
        int argc = 0;
        generate_kernel.SetArg(argc++, output_size.x);
        generate_kernel.SetArg(argc++, output_size.y);
        generate_kernel.SetArg(argc++, tile_origin.x);
        generate_kernel.SetArg(argc++, tile_origin.y);
        generate_kernel.SetArg(argc++, tile_size.x);
        generate_kernel.SetArg(argc++, tile_size.y);
        generate_kernel.SetArg(argc++, subtile_size.x);
        generate_kernel.SetArg(argc++, subtile_size.y);
        generate_kernel.SetArg(argc++, m_estimator->GetOutputIndexBuffer());
        generate_kernel.SetArg(argc++, m_estimator->GetRayCountBuffer());

        // Run shading kernel
        {
            size_t gs[] = { static_cast<size_t>((tile_size.x + 7) / 8 * 8), static_cast<size_t>((tile_size.y + 7) / 8 * 8) };
            size_t ls[] = { 8, 8 };

            GetContext().Launch2D(0, gs, ls, generate_kernel);
        }
    }

    Output* MonteCarloRenderer::FindFirstNonZeroOutput(bool include_color) const
    {
        // Find first non-zero output
        auto current_output = include_color ? GetOutput(Renderer::OutputType::kColor) : nullptr;
        if (!current_output)
        {
            for (auto i = 1U; i < static_cast<std::uint32_t>(Renderer::OutputType::kMax); ++i)
            {
                current_output = GetOutput(static_cast<Renderer::OutputType>(i));

                if (current_output)
                {
                    break;
                }
            }
        }

        return current_output;
    }

    void MonteCarloRenderer::SetOutput(OutputType type, Output* output)
    {
        if (output)
        {
            auto required_size = output->width() * output->height();

            if (required_size > m_estimator->GetWorkBufferSize())
            {
                m_estimator->SetWorkBufferSize(required_size);
            }
        }

        Renderer::SetOutput(type, output);
    }


    void MonteCarloRenderer::FillAOVs(ClwScene const& scene, int2 const& tile_origin, int2 const& tile_size)
    {
        // Find first non-zero AOV to get buffer dimensions
        auto output = FindFirstNonZeroOutput();
        auto output_size = int2(output->width(), output->height());

        // Generate tile domain
        GenerateTileDomain(output_size, tile_origin, tile_size, tile_size);

        // Generate primary
        GeneratePrimaryRays(scene, *output, tile_size);

        auto num_rays = tile_size.x * tile_size.y;

        // Intersect ray batch
        m_estimator->TraceFirstHit(scene, num_rays);

        CLWKernel fill_kernel = GetKernel("FillAOVs");

        auto argc = 0U;
        fill_kernel.SetArg(argc++, m_estimator->GetRayBuffer());
        fill_kernel.SetArg(argc++, m_estimator->GetFirstHitBuffer());
        fill_kernel.SetArg(argc++, m_estimator->GetOutputIndexBuffer());
        fill_kernel.SetArg(argc++, m_estimator->GetRayCountBuffer());
        fill_kernel.SetArg(argc++, scene.vertices);
        fill_kernel.SetArg(argc++, scene.normals);
        fill_kernel.SetArg(argc++, scene.uvs);
        fill_kernel.SetArg(argc++, scene.indices);
        fill_kernel.SetArg(argc++, scene.shapes);
        fill_kernel.SetArg(argc++, scene.materialids);
        fill_kernel.SetArg(argc++, scene.materials);
        fill_kernel.SetArg(argc++, scene.textures);
        fill_kernel.SetArg(argc++, scene.texturedata);
        fill_kernel.SetArg(argc++, scene.envmapidx);
        fill_kernel.SetArg(argc++, scene.lights);
        fill_kernel.SetArg(argc++, scene.num_lights);
        fill_kernel.SetArg(argc++, rand_uint());
        fill_kernel.SetArg(argc++, m_estimator->GetRandomBuffer(Estimator::RandomBufferType::kRandomSeed));
        fill_kernel.SetArg(argc++, m_estimator->GetRandomBuffer(Estimator::RandomBufferType::kSobolLUT));
        fill_kernel.SetArg(argc++, m_sample_counter);
        for (auto i = 1U; i < static_cast<std::uint32_t>(Renderer::OutputType::kMax); ++i)
        {
            if (auto aov = static_cast<ClwOutput*>(GetOutput(static_cast<Renderer::OutputType>(i))))
            {
                fill_kernel.SetArg(argc++, 1);
                fill_kernel.SetArg(argc++, aov->data());
            }
            else
            {
                fill_kernel.SetArg(argc++, 0);
                // This is simply a dummy buffer
                fill_kernel.SetArg(argc++, m_estimator->GetRayCountBuffer());
            }
        }

        // Run AOV kernel
        {
            int globalsize = tile_size.x * tile_size.y;
            GetContext().Launch1D(0, ((globalsize + 63) / 64) * 64, 64, fill_kernel);
        }
    }


    void MonteCarloRenderer::GeneratePrimaryRays(
        ClwScene const& scene, 
        Output const& output, 
        int2 const& tile_size
    )
    {
        // Fetch kernel
        std::string kernel_name = (scene.camera_type == CameraType::kDefault) ? "PerspectiveCamera_GeneratePaths" : "PerspectiveCameraDof_GeneratePaths";

        auto genkernel = GetKernel(kernel_name);

        // Set kernel parameters
        int argc = 0;
        genkernel.SetArg(argc++, scene.camera);
        genkernel.SetArg(argc++, output.width());
        genkernel.SetArg(argc++, output.height());
        genkernel.SetArg(argc++, m_estimator->GetOutputIndexBuffer());
        genkernel.SetArg(argc++, m_estimator->GetRayCountBuffer());
        genkernel.SetArg(argc++, (int)rand_uint());
        genkernel.SetArg(argc++, m_sample_counter);
        genkernel.SetArg(argc++, m_estimator->GetRayBuffer());
        genkernel.SetArg(argc++, m_estimator->GetRandomBuffer(Estimator::RandomBufferType::kRandomSeed));
        genkernel.SetArg(argc++, m_estimator->GetRandomBuffer(Estimator::RandomBufferType::kSobolLUT));

        {
            int globalsize = tile_size.x * tile_size.y;
            GetContext().Launch1D(0, ((globalsize + 63) / 64) * 64, 64, genkernel);
        }
    }

    CLWKernel MonteCarloRenderer::GetCopyKernel()
    {
        return GetKernel("ApplyGammaAndCopyData");
    }

    CLWKernel MonteCarloRenderer::GetAccumulateKernel()
    {
        return GetKernel("AccumulateData");
    }

    void MonteCarloRenderer::SetRandomSeed(std::uint32_t seed)
    {
        m_estimator->SetRandomSeed(seed);
    }

    void MonteCarloRenderer::Benchmark(ClwScene const& scene, Estimator::RayTracingStats& stats)
    {
        auto output = static_cast<ClwOutput*>(GetOutput(OutputType::kColor));

        int num_rays = output->width() * output->height();
        int2 tile_size = int2(output->width(), output->height());

        GenerateTileDomain(tile_size, int2(), tile_size, int2());
        GeneratePrimaryRays(scene, *output, tile_size);

        m_estimator->Benchmark(scene, num_rays, stats);
    }

    void MonteCarloRenderer::SetMaxBounces(std::uint32_t max_bounces)
    {
        m_estimator->SetMaxBounces(max_bounces);
    }
}
