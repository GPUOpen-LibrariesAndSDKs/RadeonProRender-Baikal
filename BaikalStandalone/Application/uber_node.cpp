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


#include "uber_node.h"

using namespace Baikal;

////////////////////////////////////////
// UberNode_OneArg implementation
////////////////////////////////////////

#define GET_ONE_ARG(X)\
    {\
        auto input_map = std::dynamic_pointer_cast<InputMap_OneArg<X>>(m_input_map);\
        if (!input_map)\
            throw std::runtime_error("UberNode_OneArg::GetArg(...): invalid dynamic_cast");\
        return input_map->GetArg();\
    }\

#define SET_ONE_ARG(X)\
    {\
        auto OneArg_InputMap = std::dynamic_pointer_cast<InputMap_Sin>(m_input_map);\
        if (!OneArg_InputMap)\
            throw std::runtime_error("UberNode_OneArg::SetArg(...): invalid dynamic_cast");\
        OneArg_InputMap->SetArg(input_map);\
    }\

// Get InputMap_OneArg child
UberNode::InputMap::Ptr UberNode_OneArg::GetArg()
{
    switch (m_input_map->m_type)
    {
        case InputMap::InputMapType::kSin:
            GET_ONE_ARG(InputMap::InputMapType::kSin);
        case InputMap::InputMapType::kCos:
            GET_ONE_ARG(InputMap::InputMapType::kCos);
        case InputMap::InputMapType::kTan:
            GET_ONE_ARG(InputMap::InputMapType::kTan);
        case InputMap::InputMapType::kAsin:
            GET_ONE_ARG(InputMap::InputMapType::kAsin);
        case InputMap::InputMapType::kAcos:
            GET_ONE_ARG(InputMap::InputMapType::kAcos);
        case InputMap::InputMapType::kAtan:
            GET_ONE_ARG(InputMap::InputMapType::kAtan);
        case InputMap::InputMapType::kLength3:
            GET_ONE_ARG(InputMap::InputMapType::kLength3);
        case InputMap::InputMapType::kNormalize3:
            GET_ONE_ARG(InputMap::InputMapType::kNormalize3);
        case InputMap::InputMapType::kFloor:
            GET_ONE_ARG(InputMap::InputMapType::kFloor);
        case InputMap::InputMapType::kAbs:
            GET_ONE_ARG(InputMap::InputMapType::kAbs);
        default:
            return nullptr;
    }
}

// Set InputMap_OneArg child
void UberNode_OneArg::SetArg(UberNode::InputMap::Ptr input_map)
{
    switch (input_map->m_type)
    {
        case InputMap::InputMapType::kSin:
            SET_ONE_ARG(InputMap::InputMapType::kSin);
        case InputMap::InputMapType::kCos:
            SET_ONE_ARG(InputMap::InputMapType::kCos);
        case InputMap::InputMapType::kTan:
            SET_ONE_ARG(InputMap::InputMapType::kTan);
        case InputMap::InputMapType::kAsin:
            SET_ONE_ARG(InputMap::InputMapType::kAsin);
        case InputMap::InputMapType::kAcos:
            SET_ONE_ARG(InputMap::InputMapType::kAcos);
        case InputMap::InputMapType::kAtan:
            SET_ONE_ARG(InputMap::InputMapType::kAtan);
        case InputMap::InputMapType::kLength3:
            SET_ONE_ARG(InputMap::InputMapType::kLength3);
        case InputMap::InputMapType::kNormalize3:
            SET_ONE_ARG(InputMap::InputMapType::kNormalize3);
        case InputMap::InputMapType::kFloor:
            SET_ONE_ARG(InputMap::InputMapType::kFloor);
        case InputMap::InputMapType::kAbs:
            SET_ONE_ARG(InputMap::InputMapType::kAbs);
        case InputMap::InputMapType::kSelect:
            SET_ONE_ARG(InputMap::InputMapType::kSelect);
        case InputMap::InputMapType::kShuffle:
            SET_ONE_ARG(InputMap::InputMapType::kShuffle);
        case InputMap::InputMapType::kMatMul:
            SET_ONE_ARG(InputMap::InputMapType::kMatMul);
    }
}


////////////////////////////////////////
// UberNode_Select implementation
////////////////////////////////////////

Baikal::InputMap_Select::Selection UberNode_Select::GetSelection()
{
    auto input_map = std::dynamic_pointer_cast<InputMap_Select>(m_input_map);
    if (!input_map)
        throw std::runtime_error("UberNode_OneArg::GetSelection(...): invalid dynamic_cast");
    return input_map->GetSelection();
}

// set selection parametr (Not InputMap)
void UberNode_Select::SetSelection(Baikal::InputMap_Select::Selection selection)
{
    auto input_map = std::dynamic_pointer_cast<InputMap_Select>(m_input_map);
    if (!input_map)
        throw std::runtime_error("UberNode_OneArg::GetSelection(...): invalid dynamic_cast");
    input_map->SetSelection(selection);
}


namespace {
    struct UberNode_OneArgConcrete : public UberNode_OneArg
    {
        UberNode_OneArgConcrete(InputMap::Ptr input_map, Ptr parent) :
            UberNode_OneArg(input_map, parent)
        {  }
    };
    struct UberNode_SelectConcrete : public UberNode_Select
    {
        UberNode_SelectConcrete(InputMap::Ptr input_map, Ptr parent) :
            UberNode_Select(input_map, parent)
        {  }
    };
    struct UberNode_ShuffleConcrete : public UberNode_Shuffle
    {
        UberNode_ShuffleConcrete(InputMap::Ptr input_map, Ptr parent) :
            UberNode_Shuffle(input_map, parent)
        {  }
    };
    struct UberNode_MatmulConcrete : public UberNode_Matmul
    {
        UberNode_MatmulConcrete(InputMap::Ptr input_map, Ptr parent) :
            UberNode_Matmul(input_map, parent)
        {  }
    };
    struct UberNode_TwoArgsConcrete : public UberNode_TwoArgs
    {
        UberNode_TwoArgsConcrete(InputMap::Ptr input_map, Ptr parent) :
            UberNode_TwoArgs(input_map, parent)
        {  }
    };
    struct UberNode_LerpConcrete : public UberNode_Lerp
    {
        UberNode_LerpConcrete(InputMap::Ptr input_map, Ptr parent) :
            UberNode_Lerp(input_map, parent)
        {  }
    };
    struct UberNode_Shuffle2Concrete : public UberNode_Shuffle2
    {
        UberNode_Shuffle2Concrete(InputMap::Ptr input_map, Ptr parent) :
            UberNode_Shuffle2(input_map, parent)
        {  }
    };
    struct UberNode_ThreeArgsConcrete : public UberNode_ThreeArgs
    {
        UberNode_ThreeArgsConcrete(InputMap::Ptr input_map, Ptr parent) :
            UberNode_ThreeArgs(input_map, parent)
        {  }
    };
    struct UberNode_FloatConcrete : public UberNode_Float
    {
        UberNode_FloatConcrete(InputMap::Ptr input_map, Ptr parent) :
            UberNode_Float(input_map, parent)
        {  }
    };
    struct UberNode_Float3Concrete : public UberNode_Float3
    {
        UberNode_Float3Concrete(InputMap::Ptr input_map, Ptr parent) :
            UberNode_Float3(input_map, parent)
        {  }
    };
    struct UberNode_SamplerConcrete : public UberNode_Sampler
    {
        UberNode_SamplerConcrete(InputMap::Ptr input_map, Ptr parent) :
            UberNode_Sampler(input_map, parent)
        {  }
    };
}

// Constructors implementation
UberNode::UberNode(InputMap::Ptr input_map, UberNode::Ptr parent) :
    m_input_map(input_map), m_parent(parent)
{   }
UberNode_OneArg::UberNode_OneArg(InputMap::Ptr input_map, UberNode::Ptr parent) :
    UberNode(input_map, parent)
{   }
UberNode_Select::UberNode_Select(InputMap::Ptr input_map, UberNode::Ptr parent) :
    UberNode_OneArg(input_map, parent)
{   }
UberNode_Shuffle::UberNode_Shuffle(InputMap::Ptr input_map, UberNode::Ptr parent) :
    UberNode_OneArg(input_map, parent)
{   }
UberNode_Matmul::UberNode_Matmul(InputMap::Ptr input_map, UberNode::Ptr parent) :
    UberNode_OneArg(input_map, parent)
{   }
UberNode_TwoArgs::UberNode_TwoArgs(InputMap::Ptr input_map, UberNode::Ptr parent) :
    UberNode(input_map, parent)
{   }
UberNode_Lerp::UberNode_Lerp(InputMap::Ptr input_map, UberNode::Ptr parent) :
    UberNode_TwoArgs(input_map, parent)
{   }
UberNode_Shuffle2::UberNode_Shuffle2(InputMap::Ptr input_map, UberNode::Ptr parent) :
    UberNode_TwoArgs(input_map, parent)
{   }
UberNode_ThreeArgs::UberNode_ThreeArgs(InputMap::Ptr input_map, UberNode::Ptr parent) :
    UberNode(input_map, parent)
{   }
UberNode_Float::UberNode_Float(InputMap::Ptr input_map, UberNode::Ptr parent) : 
    UberNode(input_map, parent)
{   }
UberNode_Float3::UberNode_Float3(InputMap::Ptr input_map, UberNode::Ptr parent) :
    UberNode(input_map, parent)
{   }
UberNode_Sampler::UberNode_Sampler(InputMap::Ptr input_map, UberNode::Ptr parent) :
    UberNode(input_map, parent)
{   }

UberNode::Ptr UberNode::Create(InputMap::Ptr input_map, Ptr parent)
{
    switch (input_map->m_type)
    {
        // one arg input maps
        case InputMap::InputMapType::kSin:
        case InputMap::InputMapType::kCos:
        case InputMap::InputMapType::kTan:
        case InputMap::InputMapType::kAsin:
        case InputMap::InputMapType::kAcos:
        case InputMap::InputMapType::kAtan:
        case InputMap::InputMapType::kLength3:
        case InputMap::InputMapType::kNormalize3:
        case InputMap::InputMapType::kFloor:
        case InputMap::InputMapType::kAbs:
            return std::make_shared<UberNode_OneArgConcrete>(input_map, parent);
        case InputMap::InputMapType::kSelect:
            return std::make_shared<UberNode_SelectConcrete>(input_map, parent);
        case InputMap::InputMapType::kShuffle:
            return std::make_shared<UberNode_ShuffleConcrete>(input_map, parent);
        case InputMap::InputMapType::kMatMul:
            return std::make_shared<UberNode_MatmulConcrete>(input_map, parent);
        // two arg input maps
        case InputMap::InputMapType::kAdd:
        case InputMap::InputMapType::kSub:
        case InputMap::InputMapType::kMul:
        case InputMap::InputMapType::kDiv:
        case InputMap::InputMapType::kMin:
        case InputMap::InputMapType::kMax:
        case InputMap::InputMapType::kDot3:
        case InputMap::InputMapType::kCross3:
        case InputMap::InputMapType::kDot4:
        case InputMap::InputMapType::kCross4:
        case InputMap::InputMapType::kPow:
        case InputMap::InputMapType::kMod:
            return std::make_shared<UberNode_TwoArgsConcrete>(input_map, parent);
        case InputMap::InputMapType::kLerp:
            return std::make_shared<UberNode_LerpConcrete>(input_map, parent);
        case InputMap::InputMapType::kShuffle2:
            return std::make_shared<UberNode_Shuffle2Concrete>(input_map, parent);
    }

    if (std::dynamic_pointer_cast<InputMap_Remap>(input_map) != nullptr)
    {
        return std::make_shared<UberNode_ThreeArgsConcrete>(input_map, parent);
    }
    if (std::dynamic_pointer_cast<InputMap_ConstantFloat>(input_map) != nullptr)
    {
        return std::make_shared<UberNode_FloatConcrete>(input_map, parent);
    }
    if (std::dynamic_pointer_cast<InputMap_ConstantFloat3>(input_map) != nullptr)
    {
        return std::make_shared<UberNode_Float3Concrete>(input_map, parent);
    }
    if (std::dynamic_pointer_cast<InputMap_Sampler>(input_map) != nullptr)
    {
        return std::make_shared<UberNode_SamplerConcrete>(input_map, parent);
    }

    return nullptr;
}