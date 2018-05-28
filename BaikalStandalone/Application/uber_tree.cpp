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
#include <queue>

UberTree::UberTree(UberNode::Ptr node)
{
    BuildTree(node);
}

void UberTree::BuildTree(UberNode::Ptr root)
{
    std::queue<UberNode::Ptr> queue;
    queue.push(root);

    while (!queue.empty())
    {
        for (auto item : queue.front()->m_children)
            queue.push(item);
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
    // build tree for input node
    Ptr tree = std::make_shared<UberTree>(node);
    // add this tree as sub tree to main tree
    return AddSubTree(id, arg_number, tree);
}

bool UberTree::AddSubTree(std::uint32_t id, std::uint32_t arg_number, UberTree::Ptr node)
{
    auto iter = std::find_if(m_nodes.begin(), m_nodes.end(),
        [id](UberNode::Ptr node) 
        {
            return node->GetId() == id;
        });

    if (iter == m_nodes.end())
        return false;

    // if argument number is bigger than 
    if (arg_number > (std::uint32_t)(*iter)->GetType())
        return false;

    return false;
}