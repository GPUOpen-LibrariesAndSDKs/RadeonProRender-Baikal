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


#pragma once

#include <set>

#include "SceneGraph/scene1.h"
#include "SceneGraph/Collector/collector.h"
#include "SceneGraph/clwscene.h"

namespace Baikal
{
    class UberV2Material;
    class CLInputMapGenerator
    {
    public:
        void Generate(const Collector& input_map_collector, const Collector& input_map_leaf_collector);
        const std::string& GetGeneratedSource() const
        {
            return m_source_code;
        }

    private:
        void GenerateSingleInput(std::shared_ptr<Baikal::InputMap> input, const Collector& input_map_leaf_collector);
        void GenerateInputSource(std::shared_ptr<Baikal::InputMap> input, const Collector& input_map_leaf_collector);
        std::string m_source_code;
        std::string m_read_functions;
        std::string m_float4_selector;
        std::string m_float_selector;
        std::set<uint32_t> m_generated_inputs;
    };
}
