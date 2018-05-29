
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

#include <functional>
#include "uber_node.h"

class UberTree
{
public:
    using Ptr = std::shared_ptr<UberTree>;

    static Ptr Create(UberNode::Ptr node);

    // 'id' is an id ofthe node to add subtree ('node' or 'tree' arg)
    // 'arg_number' is a number of the argument in
    // parent node (id of this node is argument "id") which will be set
    bool AddSubTree(std::uint32_t id, std::uint32_t arg_number, UberNode::Ptr node);
    bool AddSubTree(std::uint32_t id, std::uint32_t arg_number, UberTree::Ptr tree);

    void ExcludeSubTree(UberNode::Ptr node);

    bool IsValid() const;

protected:
    UberTree(UberNode::Ptr node);

private:
    void BuildTree(UberNode::Ptr root);

    std::vector<UberNode::Ptr> m_nodes;
};