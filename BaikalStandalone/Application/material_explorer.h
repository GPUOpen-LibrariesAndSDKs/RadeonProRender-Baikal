
/**********************************************************************
 Copyright (c) 2017 Advanced Micro Devices, Inc. All rights reserved.

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

#include "ImGUI/imgui.h"
#include "SceneGraph/uberv2material.h"
#include "uber_graph.h"

class MaterialExplorer
{
public:
    using InputMap = Baikal::InputMap;
    using Ptr = std::shared_ptr<MaterialExplorer>;
    using LayerDesc = std::pair<Baikal::UberV2Material::Layers, std::vector<std::string>>;

    static Ptr Create(InputMap::Ptr input_map);
    static std::vector<LayerDesc> GetUberLayersDesc();

protected:
    MaterialExplorer(InputMap::Ptr input_map);

private:
    UberGraph::Ptr m_uber_graph;
};