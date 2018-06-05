
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

#include "uber_tree.h"

class UberGraph
{
public:
    using Ptr = std::shared_ptr<UberGraph>;

    static Ptr Create(Baikal::InputMap::Ptr input_map);

    void RemoveNode(int node_id);
    void RemoveNode(UberNode::Ptr node);
    void RemoveSubTree(UberTree::Ptr tree);

    void AddTree(UberTree::Ptr tree);

    bool AddSubTree(int id, int arg_number, UberTree::Ptr tree);

    const std::vector<UberTree::Ptr>& GetTrees() const
    { return m_trees; }

protected:
    UberGraph(Baikal::InputMap::Ptr input_map);

private:
    std::vector<UberTree::Ptr> m_trees;
};
