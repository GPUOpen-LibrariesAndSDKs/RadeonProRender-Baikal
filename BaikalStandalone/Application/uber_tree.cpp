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

#include "uber_tree.h"
#include <algorithm>
#include <queue>

////////////////////////////////////////////
//// UberTree implementation
////////////////////////////////////////////

void UberTree::BuildTree(InputMap::Ptr input_map)
{
    auto root = UberNode::Create(input_map, nullptr);
    std::queue<UberNode::Ptr> queue;

    queue.push(root);

    while (queue.empty())
    {
        auto node = queue.back();

        for (auto i = 0u; i < node->GetArgNumber(); i++)
        {
            auto child = UberNode::Create(node->GetArg(i), node);
            node->SetChild(i, child);
            queue.push(node);
        }
        queue.pop();
    }

}

bool UberTree::AddTree(std::uint32_t id, std::uint32_t arg_number, UberTree::Ptr tree)
{
    if (arg_number > MAX_ARGS - 1)
        return false;

    UberNode::Ptr parent = nullptr;

    for (const auto& node : m_nodes)
    {
        if (node->GetId() == id)
            parent = node;
    }

    if (!parent)
        return false; // there is no Node with such id (argument 'id')

    auto child = tree->m_nodes.begin();

    m_nodes.insert(
        m_nodes.end(),
        tree->m_nodes.begin(),
        tree->m_nodes.end());

    parent->SetChild(arg_number, *child);
    (*child)->m_parent = parent;
    tree = nullptr;
    return true;
}

std::vector<UberTree::Ptr> UberTree::ExcludeNode(std::uint32_t id)
{
    UberNode::Ptr removed_node = nullptr;

    for (const auto& node : m_nodes)
    {
        if (node->GetId() == id)
            removed_node = node;
    }

    if (!removed_node)
    {
        throw std::logic_error("UberTree::ExcludeNode(...): attempt to exclude nonexistent node");
    }

    auto parent = removed_node->m_parent;

    if (parent)
    {
        for (auto i = 0u; i < parent->GetArgNumber(); i++)
        {
            auto child = parent->m_children[i];
            if (child->GetId() == id)
                parent->m_children[i] = nullptr;
        }
    }

    std::vector<UberTree::Ptr> trees;
    for (auto i = 0u; i < removed_node->GetArgNumber(); i++)
    {
        std::vector<UberNode::Ptr> tree;
        auto child = removed_node->GetChildren(i);
        child->m_parent = nullptr;

        std::queue<UberNode::Ptr> queue;
        queue.push(child);
        while (queue.empty())
        {
            for (auto i = 0u; i < queue.back()->GetArgNumber(); i++)
            {
                queue.push(queue.back()->m_children[i]);
            }
            m_nodes.erase(std::find(m_nodes.begin(), m_nodes.end(), queue.back()));
            tree.push_back(queue.back());
            queue.pop();
        }

        trees.push_back(std::make_shared<UberTree>(UberTree(tree)));
    }

    return trees;
}

bool UberTree::IsValid() const
{
    for (const auto& node : m_nodes)
    {
        if (!node->IsValid())
            return false;
    }
    return true;
}

UberTree::UberTree(std::vector<UberNode::Ptr> nodes) :
    m_nodes(nodes)
{   }

UberTree::UberTree(InputMap::Ptr input_map)
{ BuildTree(input_map); }

namespace {
    struct UberTreeConcrete: public UberTree
    {
        UberTreeConcrete(InputMap::Ptr node) :
            UberTree(node)
        {   }
    };
}

UberTree::Ptr UberTree::Create(InputMap::Ptr input_map)
{
    return std::make_shared<UberTreeConcrete>(input_map);
}

////////////////////////////////////////////
//// UberTreeIterator implementation
////////////////////////////////////////////

UberTreeIterator::UberTreeIterator(UberTree::Ptr tree) :
    m_tree(tree), m_index(-1)
{
    if (m_tree && (m_tree->m_nodes.size() > 0))
        m_index = 0;
}

bool UberTreeIterator::IsValid() const
{
    if (!m_tree)
        return false;

    return (size_t)m_index < m_tree->m_nodes.size();
}

UberNode::Ptr UberTreeIterator::Item() const
{
    return m_tree->m_nodes[m_index];
}

void UberTreeIterator::Reset()
{ m_index = 0; }

void UberTreeIterator::Next()
{ m_index++; }

namespace {
    struct UberTreeIteratorConcrete : public UberTreeIterator
    {
        UberTreeIteratorConcrete(UberTree::Ptr tree) :
            UberTreeIterator(tree)
        {   }
    };
}

UberTreeIterator::Ptr UberTreeIterator::Create(UberTree::Ptr tree)
{
    return std::make_shared<UberTreeIteratorConcrete>(tree);
}