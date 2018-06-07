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

class GraphScheme
{
public:
    using Ptr = std::shared_ptr<GraphScheme>;

    struct Link
    {
        std::uint32_t src_id;
        std::uint32_t dst_id;
    };

    struct Node
    {
        std::uint32_t id;
        std::string name;
        RadeonRays::int2 pos; // position of the top left corner
        RadeonRays::int2 size;
    };

    static Ptr Create(
        std::vector<UberTree>& trees,
        RadeonRays::int2 root_pos = RadeonRays::int2(0, 0));

    const std::vector<Node>& GetNodes() const;
    const std::vector<Link>& GetLinks() const;

protected:
    GraphScheme(std::vector<UberTree>& trees, RadeonRays::int2 root_pos);

private:
    void ComputeInitialCords(std::vector<UberTree>& trees, RadeonRays::int2 root_pos);

    std::vector<Node> m_nodes;
    std::vector<Link> links;
};