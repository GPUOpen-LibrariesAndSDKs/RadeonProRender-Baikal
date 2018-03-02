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


CLProgram::CLProgram(const CLProgramManager *program_manager, uint32_t id, CLWContext context) :
    m_program_manager(program_manager), 
    m_id(id), 
    m_context(context) 
{
    m_compilation_options.append(" -cl-mad-enable -cl-fast-relaxed-math "
        "-cl-std=CL1.2 -I . ");

    m_compilation_options.append(
#if defined(__APPLE__)
        "-D APPLE "
#elif defined(_WIN32) || defined (WIN32)
        "-D WIN32 "
#elif defined(__linux__)
        "-D __linux__ "
#else
        ""
#endif
    );
#ifdef ENABLE_UBERV2
    m_compilation_options.append("-D ENABLE_UBERV2");
#endif
};

void CLProgram::SetSource(const std::string &source, const std::string &compilation_options)
{
    m_compiled_source.reserve(1024 * 1024); //Just reserve 1M for now
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
        std::string arr = m_program_manager->ReadHeader(fname);
        ParseSource(arr);
    }
}

void CLProgram::BuildSource(const std::string &source)
{
    std::string::size_type offset = 0;
    std::string::size_type position = 0;
    std::string find_str("#include");
    while ((position = source.find(find_str, offset)) != std::string::npos)
    {
        // Append not-include part of source
        if (position != offset)
            m_compiled_source += source.substr(offset, position - offset - 1);

        // Get include file name
        std::string::size_type end_position = source.find(">", position);
        assert(end_position != std::string::npos);
        std::string fname = source.substr(position, end_position - position);
        position = fname.find("<");
        assert(position != std::string::npos);
        fname = fname.substr(position + 1, fname.length() - position);
        offset = end_position + 1;

        if (m_included_headers.find(fname) == m_included_headers.end())
        {
            m_included_headers.insert(fname);
            // Append included file to source
            BuildSource(m_program_manager->ReadHeader(fname));
        }
    }

    // Append rest of the file
    m_compiled_source += source.substr(offset);
}

const std::string& CLProgram::GetFullSource()
{
    if (m_is_dirty)
    {
        m_compiled_source.clear();
        m_included_headers.clear();
        BuildSource(m_program_source);
    }

    return m_compiled_source;
}

void CLProgram::Compile()
{
    const std::string &src = GetFullSource();
    m_compiled_program = CLWProgram::CreateFromSource(src.c_str(), src.size(), m_compilation_options.c_str(), m_context);
    m_is_dirty = false;
}
