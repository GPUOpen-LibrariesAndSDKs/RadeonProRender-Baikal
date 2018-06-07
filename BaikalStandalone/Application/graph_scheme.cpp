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

#include "graph_scheme.h"

namespace {
    struct GraphSchemeConcrete : public GraphScheme
    {
        GraphSchemeConcrete(std::vector<UberTree>& trees, RadeonRays::int2 root_pos) :
            GraphScheme(trees, root_pos)
        {   }
    };
}

GraphScheme::GraphScheme(std::vector<UberTree>& trees, RadeonRays::int2 root_pos)
{ /* Not implemented */ }

GraphScheme::Ptr GraphScheme::Create(std::vector<UberTree>& trees, RadeonRays::int2 root_pos)
{
    return std::make_shared<GraphScheme>(
        GraphSchemeConcrete(trees, root_pos));
}

void GraphScheme::ComputeInitialCords(std::vector<UberTree>& trees, RadeonRays::int2 root_pos)
{

}
