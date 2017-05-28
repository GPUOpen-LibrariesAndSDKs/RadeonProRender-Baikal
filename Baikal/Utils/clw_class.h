#pragma once

#include <string>
#include "CLW.h"

namespace Baikal
{
    class ClwClass
    {
    public:
        ClwClass(CLWContext context, std::string const& cl_file);
        virtual ~ClwClass();
        
    protected:
        CLWContext GetContext() { return m_context; }
        CLWKernel GetKernel(std::string const& name);
        
        
    private:
        CLWContext m_context;
        CLWProgram m_program;
    };
    
    inline ClwClass::ClwClass(CLWContext context, std::string const& cl_file)
    : m_context(context)
    {
        std::string buildopts;
        
        buildopts.append(" -cl-mad-enable -cl-fast-relaxed-math "
                         "-cl-std=CL1.2 -I . ");
        
        buildopts.append(
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
        
        m_program = CLWProgram::CreateFromFile(cl_file.c_str(),
                                               buildopts.c_str(), m_context);
    }
    
    inline CLWKernel ClwClass::GetKernel(std::string const& name)
    {
        return m_program.GetKernel(name);
    }
}
