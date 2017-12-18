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
#pragma once

#include "post_effect.h"

#include "CLW.h"
#include "Output/clwoutput.h"
#include "Utils/clw_class.h"

#include <string>

namespace Baikal
{
    /**
    \brief Post effects partial implementation based on CLW framework.
    */
    class ClwPostEffect : public PostEffect, protected ClwClass
    {
    public:
        // Constructor, receives CLW context
        ClwPostEffect(CLWContext context, std::string const& file_name);
        ClwPostEffect(CLWContext context, const char* data, const char* includes[], std::size_t inc_num);
    };

    inline ClwPostEffect::ClwPostEffect(CLWContext context, std::string const& file_name)
        : ClwClass(context, file_name)
    {
    }

    inline ClwPostEffect::ClwPostEffect(CLWContext context, const char* data, const char* includes[], std::size_t inc_num)
        : ClwClass(context, data, includes, inc_num)
    {
    }
}
