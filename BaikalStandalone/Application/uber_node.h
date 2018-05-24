
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

enum class NodeType
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
    static Ptr Create(InputMap::Ptr input_map, Ptr parent);

    // input map type accessor
    virtual NodeType GetType() const
    { return NodeType::kNone; }

    // input map data type accessor
    InputMap::InputMapType GetDataType()
    { return m_input_map->m_type; }

protected:
    UberNode(InputMap::Ptr input_map, UberNode::Ptr parent);

    InputMap::Ptr m_input_map;
    Ptr m_parent;
    std::vector<Ptr> m_children;
};

// UberNode_OneArg common class
class UberNode_OneArg : public UberNode
{
public:
    // input map type accessor
    NodeType UberNode_OneArg::GetType() const override
    { return NodeType::kInputOneArg; }

    // Get InputMap_OneArg child
    UberNode::InputMap::Ptr GetArg();
    // Set InputMap_OneArg child
    void SetArg(UberNode::InputMap::Ptr);

protected:
    UberNode_OneArg(InputMap::Ptr input_map, UberNode::Ptr parent);
};

// UberNode_OneArg specific classes
class UberNode_Select : public UberNode_OneArg
{
public:
    // get selection parametr (Not InputMap)
    Baikal::InputMap_Select::Selection GetSelection();
    // set selection parametr (Not InputMap)
    void SetSelection(Baikal::InputMap_Select::Selection selection);

protected:
    UberNode_Select(InputMap::Ptr input_map, UberNode::Ptr parent);
};

class UberNode_Shuffle : public UberNode_OneArg
{
public:
    // get mask parameter (Not InputMap)
    std::array<uint32_t, 4> GetMask() const;
    // set mask parameter (Not InputMap)
    void SetMask(const std::array<uint32_t, 4> &mask);
protected:
    UberNode_Shuffle(InputMap::Ptr input_map, UberNode::Ptr parent);
};

class UberNode_Matmul : public UberNode_OneArg
{
public:
    // get matrix parameter
    void SetMatrix(const RadeonRays::matrix &mat4);
    // get matrix parameter
    RadeonRays::matrix GetMatrix() const;
protected:
    UberNode_Matmul(InputMap::Ptr input_map, UberNode::Ptr parent);
};

// UberNode_TwoArgs common class
class UberNode_TwoArgs : public UberNode
{
public:
    // input map type accessor
    NodeType GetType() const override;

    // Get InputMap_TwoArg child A
    InputMap::Ptr GetArgA();
    // Get InputMap_TwoArg child B
    InputMap::Ptr GetArgB();

protected:
    UberNode_TwoArgs(InputMap::Ptr input_map, UberNode::Ptr parent);
};

// UberNode_TwoArgs specific class
class UberNode_Lerp : public UberNode_TwoArgs
{
public:
    // get control parameter (not child argument)
    Baikal::InputMap::Ptr GetCntrol();
    // set control parameter (not child argument)
    void SetCntrol(Baikal::InputMap::Ptr control);

protected:
    UberNode_Lerp(InputMap::Ptr input_map, UberNode::Ptr parent);
};

class UberNode_Shuffle2 : public UberNode_TwoArgs
{
public:
    // get mask parameter (Not InputMap)
    std::array<uint32_t, 4> GetMask() const;
    // set mask parameter (Not InputMap)
    void SetMask(const std::array<uint32_t, 4> &mask);

protected:
    UberNode_Shuffle2(InputMap::Ptr input_map, UberNode::Ptr parent);
};

// UberNode_ThreeArgs common class
class UberNode_ThreeArgs : public UberNode
{
public:
    // input map type accessor
    NodeType GetType() const override;

    // Get InputMap_ThreeArg child A
    InputMap::Ptr GetArgA();
    // Get InputMap_ThreeArg child B
    InputMap::Ptr GetArgB();
    // Get InputMap_ThreeArg child C
    InputMap::Ptr GetArgC();

protected:
    UberNode_ThreeArgs(InputMap::Ptr input_map, UberNode::Ptr parent);
};

// lead nodes
class UberNode_Float : public UberNode
{
public:
    NodeType GetType() const override;

    void SetValue(float value);
    float GetValue() const;

protected:
    UberNode_Float(InputMap::Ptr input_map, UberNode::Ptr parent);
};

class UberNode_Float3 : public UberNode
{
public:
    NodeType GetType() const override;

    void SetValue(RadeonRays::float3 value);
    RadeonRays::float3 GetValue() const;

protected:
    UberNode_Float3(InputMap::Ptr input_map, UberNode::Ptr parent);
};

class UberNode_Sampler : public UberNode
{
public:
    NodeType GetType() const override;

    void SetValue(Baikal::Texture::Ptr value);
    Baikal::Texture::Ptr GetValue() const;

protected:
    UberNode_Sampler(InputMap::Ptr input_map, UberNode::Ptr parent);
};