#pragma once

#include "Renderers/renderer.h"
#include "Output/clwoutput.h"
#include "RadeonRays/CLW/CLWContext.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>


namespace Baikal
{
    class RendererOutputAccessor
    {
    public:
        RendererOutputAccessor(std::string output_dir, size_t width, size_t height)
                : m_output_dir(std::move(output_dir)), m_width(width), m_height(height)
        {
            m_image_data.resize(m_width * m_height);
            m_image_rgb.resize(m_width * m_height);
        };

        template<class TOutputData>
        void SaveAllOutputs(const std::vector<TOutputData>& outputs)
        {
            if (m_frame_count % kDumpPeriod == 0)
            {
                for (std::size_t idx = 0; idx < outputs.size(); ++idx)
                {
                    for (auto& output : outputs[idx].render_outputs)
                    {
                        SaveImageFromRendererOutput(idx,
                                                    output.first,
                                                    output.second.get(),
                                                    m_frame_count);
                    }
                }
            }
            ++m_frame_count;
        }

        void SaveImageFromRendererOutput(size_t device_idx,
                                         Renderer::OutputType output_type,
                                         Output *output, size_t index)
        {
            GetOutputData(output);
            NormalizeImage();
            SaveImage(device_idx, GetOutputName(output_type), index);
        }

        void LoadImageToRendererOutput(const CLWContext& context,
                                       Output* output,
                                       const std::string& image_path)
        {
            using float3 = RadeonRays::float3;
            std::ifstream ifs(image_path, std::ios::binary | std::ios::ate);
            std::ifstream::pos_type pos = ifs.tellg();

            std::vector<char> result(pos);
            ifs.seekg(0, std::ios::beg);
            ifs.read(result.data(), pos);

            auto clw_output = dynamic_cast<ClwOutput*>(output);

            context.WriteBuffer<float3>(0,
                                        clw_output->data(),
                                        reinterpret_cast<RadeonRays::float3 *>(result.data()),
                                        sizeof(RadeonRays::float3)).Wait();
        }

    private:
        struct Color
        {
            float r;
            float g;
            float b;
        };

        void GetOutputData(Output* output)
        {
            output->GetData(&m_image_data[0]);
        }

        static std::string GetOutputName(Renderer::OutputType output_type)
        {
            switch (output_type)
            {
                case Renderer::OutputType::kColor:
                    return "color";
                case Renderer::OutputType::kOpacity:
                    return "opacity";
                case Renderer::OutputType::kVisibility:
                    return "visibility";
                case Baikal::Renderer::OutputType::kMaxMultiPassOutput:
                    return "max_multi_pass_output";
                case Baikal::Renderer::OutputType::kMeshID:
                    return "mesh_id";
                case Renderer::OutputType::kAlbedo:
                    return "albedo";
                case Renderer::OutputType::kWorldPosition:
                    return "world_position";
                case Renderer::OutputType::kWorldTangent:
                    return "world_tangent";
                case Renderer::OutputType::kWorldBitangent:
                    return "world_bitangent";
                case Renderer::OutputType::kWorldShadingNormal:
                    return "world_shading_normal";
                case Renderer::OutputType::kWorldGeometricNormal:
                    return "world_shading_normal";
                case Renderer::OutputType::kViewShadingNormal:
                    return "view_shading_normal";
                case Renderer::OutputType::kBackground:
                    return "background";
                case Renderer::OutputType::kDepth:
                    return "depth";
                case Renderer::OutputType::kGloss:
                    return "gloss";
                case Renderer::OutputType::kGroupID:
                    return "group_id";
                case Renderer::OutputType::kMax:
                    return "max";
                case Renderer::OutputType::kShapeId:
                    return "shape_id";
                case Renderer::OutputType::kWireframe:
                    return "wireframe";
                default:
                    throw std::runtime_error("Output is not supported");
            }
        }

        void NormalizeImage()
        {
            std::transform(m_image_data.cbegin(), m_image_data.cend(), m_image_rgb.begin(),
                           [](RadeonRays::float3 const &v)
                           {
                               if (v.w == 0)
                               {
                                   return Color({0, 0, 0});
                               }
                               else
                               {
                                   float invw = 1.f / v.w;
                                   return Color({v.x * invw, v.y * invw, v.z * invw});
                               }
                           });
        }

        void SaveImage(size_t device_idx, std::string const& output_name, std::size_t index)
        {
            std::ostringstream path;
            path << m_output_dir
                 << "/device_" << device_idx
                 << "_frame_" << index
                 << "_" << output_name << ".bin";
            std::ofstream out(path.str(), std::ios_base::binary);
            out.write(reinterpret_cast<char const *>(m_image_rgb.data()),
                      m_width * m_height * sizeof(Color));
            std::cerr << "Written: " << path.str() << "\n";
        }

        std::vector<RadeonRays::float3> m_image_data;
        std::vector<Color> m_image_rgb;
        std::string m_output_dir;
        std::size_t m_width;
        std::size_t m_height;
        std::size_t m_frame_count = 0;
        static constexpr std::size_t kDumpPeriod = 20;
    };
}
