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

#include "cl_program.h"

#include <assert.h>

#include "cl_program_manager.h"

using namespace Baikal;

void CLProgram::SetSource(const std::string &source, const std::string &compilation_options)
{
    m_program_source = source;
    m_compilation_options = compilation_options;
    ParseSource(m_program_source);
}

void CLProgram::ParseSource(const std::string &source)
{
    std::string::size_type offset = 0;
    std::string::size_type position = 0;
    std::string find_str("#include");
    while ((position = source.find(find_str, offset)) != std::string::npos)
    {
        std::string::size_type end_position = source.find(">", position);
        assert(end_position != std::string::npos);
        std::string fname = source.substr(position, end_position - position);
        position = fname.find("<");
        assert(position != std::string::npos);
        fname = fname.substr(position + 1, fname.length() - position);
        offset = end_position;

        m_program_manager->LoadHeader(fname);
        m_required_headers.push_back(fname);
        ParseSource(m_program_manager->ReadHeader(fname));
    }
}
