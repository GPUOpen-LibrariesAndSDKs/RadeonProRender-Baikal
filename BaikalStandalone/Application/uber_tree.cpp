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

#include "uber_tree.h"
#include <algorithm>
#include <queue>

void UberTree::BuildTree(UberNode::Ptr root)
{
    std::queue<UberNode::Ptr> queue;
    queue.push(root);

    while (!queue.empty())
    {
        auto parent = queue.back();
        for (int i = 0; i < (int)queue.front()->GetType(); i++)
        {
            auto child = UberNode::Create(parent->GetArg(i), parent);
            parent->SetChild(i, child->GetId());
            queue.push(child);
        }
        m_nodes.push_back(queue.back());
        queue.pop();
    }
}

bool UberTree::IsValid() const
{
    for (auto item : m_nodes)
    {
        if (!item->IsValid())
            return false;
    }
    return true;
}

bool UberTree::AddSubTree(std::uint32_t id, std::uint32_t arg_number, UberNode::Ptr node)
{
    // check that tree doesn't contain this node already
    for (auto item : m_nodes)
    {
        if (item->GetId() == node->GetId())
            return false;
    }

    // build tree for input node
    Ptr tree = Create(node);
    // add this tree as sub tree to main tree
    return AddSubTree(id, arg_number, tree);
}

bool UberTree::AddSubTree(std::uint32_t id, std::uint32_t arg_number, UberTree::Ptr tree)
{
    auto iter = std::find_if(m_nodes.begin(), m_nodes.end(),
        [id](UberNode::Ptr node) 
        {
            return node->GetId() == id;
        });

    if (iter == m_nodes.end())
        return false;

    // node should support arguments
    if ((*iter)->GetType() == NodeType::kNoneArgs)
        return false;
    // check 'arg_number' and NodeType compatibility
    if (arg_number >= (std::uint32_t)(*iter)->GetType())
        return false;

    (*iter)->SetChild(arg_number, m_nodes[0]->GetId());
    // update material input maps, if tree is valid
    if (IsValid())
    {
        (*iter)->SetArg(tree->m_nodes[0]->m_input_map, arg_number);
    }
    return true;
}

void UberTree::ExcludeSubTree(UberNode::Ptr node)
{
    int id = node->GetId();

    auto iter = std::find_if(
        m_nodes.begin(), m_nodes.end(),
        [id](UberNode::Ptr node)
        { return id == node->GetId(); });

    auto parent = (*iter)->m_parent;

    // can not exclude root
    if (parent == nullptr)
        return;

    int arg_num = 0;
    for (size_t i = 0; i < parent->m_children.size(); i++)
    {
        if (parent->m_children[i].second == id)
        {
            parent->m_children[i].second = INVALID_ID;
            parent->SetArg(nullptr, (std::uint32_t)i);
            break;
        }
    }
}

int UberTree::GetRootId() const
{
    return (*m_nodes.begin())->GetId();
}

void UberTree::Synchronize()
{
    if (!IsValid())
        return;

    std::queue<UberNode::Ptr> queue;
    queue.push(m_nodes[0]);

    while (!queue.empty())
    {
        auto parent = queue.back()->m_parent;
        if (!parent)
        {
            auto children = queue.back()->m_parent->GetChildren();
            for (auto child : children)
            {
                int id = child.second;
                auto node = FindNode(id);

                if (node == nullptr)
                {
                    throw std::logic_error
                        ("UberTree::Synchronize(...): missed necessary id in tree");
                }

                parent->SetArg(node->GetArg(child.first), child.first);
            }
        }
        queue.pop();
    }
}

UberNode::Ptr UberTree::FindNode(int id)
{
    auto iter = std::find_if(
        m_nodes.begin(), m_nodes.end(),
        [id](UberNode::Ptr node)
        { return id == node->GetId(); });

    if (iter == m_nodes.end())
        return nullptr;

    return *iter;
}

UberTree::UberTree(UberNode::Ptr node)
{ BuildTree(node); }


namespace {
    struct UberTreeConcrete: public UberTree
    {
        UberTreeConcrete(UberNode::Ptr node) :
            UberTree(node)
        {   }
    };
}

UberTree::Ptr UberTree::Create(UberNode::Ptr node)
{
    return std::make_shared<UberTreeConcrete>(node);
}