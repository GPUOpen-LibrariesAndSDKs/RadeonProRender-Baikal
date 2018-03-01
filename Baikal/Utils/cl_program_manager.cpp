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

#include "cl_program_manager.h"

#include <fstream>
using namespace Baikal;

uint32_t CLProgramManager::m_next_program_id = 0;

std::string ReadFile(const std::string &fname)
{
    std::ifstream file(fname);
    std::string str;

    file.seekg(0, std::ios::end);
    str.reserve(file.tellg());
    file.seekg(0, std::ios::beg);

    str.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return str;
}
CLProgramManager::CLProgramManager(const std::string &cache_path) :
    m_cache_path(cache_path)
{

}

uint32_t CLProgramManager::CreateProgram(CLWContext context, const std::string &fname, const std::string &options)
{
    CLProgram prg(this, m_next_program_id++);
    prg.SetSource(ReadFile(fname), options);
    m_programs.insert(std::make_pair(prg.GetId(), prg));
    return prg.GetId();
}

void CLProgramManager::AddHeader(const std::string &header, const std::string &source)
{
    m_headers.insert(std::make_pair(header, source));
}

void CLProgramManager::LoadHeader(const std::string &header)
{
    std::string header_source = ReadFile(header);
    m_headers.insert(std::make_pair(header, header_source));
}

const std::string& CLProgramManager::ReadHeader(const std::string &header) const
{
    auto it = m_headers.find(header);
    return (it != m_headers.end()) ? it->second : "";
}

CLWProgram CLProgramManager::GetProgram(uint32_t id)
{
    
}

void CLProgramManager::CompileProgram(uint32_t id)
{

}

