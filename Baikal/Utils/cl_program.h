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

#include <string>
#include <vector>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include "CLWProgram.h"
#include "CLWContext.h"

namespace Baikal
{
    class CLProgramManager;
    class CLProgram
    {
    public:
        CLProgram() = default;
        CLProgram(const CLProgramManager *program_manager, uint32_t id, CLWContext context);
        bool IsDirty() const { return m_is_dirty; }
        void SetDirty() { m_is_dirty = true; }
        uint32_t GetId() const { return m_id; }
        void SetSource(const std::string &source);
        CLWProgram GetCLWProgram(const std::string &opts);

        bool IsHeaderNeeded(const std::string &header_name) const;

        const std::string& GetFullSource();
        CLWProgram Compile(const std::string &opts);

    private:
        void ParseSource(const std::string &source);
        void BuildSource(const std::string &source);

        const CLProgramManager *m_program_manager;
        std::string m_compiled_source;
        std::string m_program_source;
        std::unordered_set<std::string> m_required_headers;

        std::unordered_map<std::string, CLWProgram> m_programs;

        bool m_is_dirty = true;
        uint32_t m_id;
        CLWContext m_context;
        std::set<std::string> m_included_headers;

    };
}
