
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

#pragma once 

#include "Baikal/SceneGraph/inputmap.h"
#include "Baikal/SceneGraph/inputmaps.h"

enum class UberNodeType
{
    kNone = 0, // invalid type
    kLeafNode_Float,
    kLeafNode_Float3,
    kLeafNode_Texture,
    kInputOneArg,
    kInputOneArg_Select,
    kInputOneArg_Shuffle,
    kInputOneArg_MatMul,
    kInputTwoArg,
    kInputTwoArg_Shuffle2,
    kInputThreeArg_Remap
};

class UberNode
{
public:
    using Ptr = std::shared_ptr<UberNode>;
    using InputMap = Baikal::InputMap;

    // for root element parent is nullptr
    UberNode(InputMap::Ptr input_map, Ptr parent);

    // data accessors, returns nullptr in case of type mismatch
    template <class type>
    InputMap::Ptr GetFirstArg()
    {
        auto input_map = std::dynamic_pointer_cast<InputMap_OneArg<type>>(m_input_map);

        if (!input_map)
            return nullptr;

        return input_map->GetArg();
    }

    InputMap::Ptr GetSecondArg();
    InputMap::Ptr GetThirdArg();
    float GetFloat();
    RadeonRays::float3 GetFloat3();
    Baikal::Texture::Ptr GetTexture();

    // input map type accessor
    UberNodeType GetType() const;

private:
    std::uint32_t m_id;
    InputMap::Ptr m_input_map;
    Ptr m_parent;
    std::vector<Ptr> m_children;
};
