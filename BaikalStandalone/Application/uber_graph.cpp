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

#include "uber_graph.h"

UberGraph::UberGraph(UberNode::InputMap::Ptr input_map)
{
    m_trees.push_back(
        UberTree::Create(
            UberNode::Create(input_map, nullptr)));
}

namespace {
    struct UberGraphConcrete : public UberGraph
    {
        UberGraphConcrete(UberNode::InputMap::Ptr input_map) :
            UberGraph(input_map)
        {  }

    };
}

UberGraph::Ptr UberGraph::Create(UberNode::InputMap::Ptr input_map)
{
    return std::make_shared<UberGraphConcrete>(input_map);
}

void UberGraph::RemoveNode(UberNode::Ptr node)
{
    return RemoveNode(node->GetId());
}

void UberGraph::RemoveNode(int node_id)
{
    for (auto tree : m_trees)
    {
        auto node = tree->FindNode(node_id);
        if (node)
        {
            tree->ExcludeSubTree(node);
        }
    }
}

void UberGraph::RemoveSubTree(UberTree::Ptr tree)
{
    for (auto iter = m_trees.begin(); iter != m_trees.end(); iter++)
    {
        if ((*iter)->GetRootId() == tree->GetRootId())
        {
            m_trees.erase(iter);
            return;
        }
    }

    RemoveNode(tree->GetRootId());
}

void UberGraph::AddTree(UberTree::Ptr tree)
{
    m_trees.push_back(tree);
}

void UberGraph::AddTree(UberNode::Ptr node)
{
    m_trees.push_back(UberTree::Create(node));
}

bool UberGraph::AddSubTree(int id, int arg_number, UberTree::Ptr tree)
{
    for (auto item : m_trees)
    {
        if (!item->FindNode(id))
        {
            item->AddSubTree(id, arg_number, tree);
            return true;
        }
    }
    return false;
}

bool UberGraph::AddSubTree(int id, int arg_number, UberNode::Ptr node)
{
    return AddSubTree(id, arg_number, UberTree::Create(node));
}