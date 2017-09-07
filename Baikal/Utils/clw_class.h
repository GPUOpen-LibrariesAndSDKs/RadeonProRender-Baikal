#pragma once

#include <string>
#include "CLW.h"

namespace Baikal
{
    class ClwClass
    {
    public:
        ClwClass(CLWContext context,
                 std::string const& cl_file,
                 std::string const& opts = "");
        virtual ~ClwClass() = default;
        
    protected:
        CLWContext GetContext() const { return m_context; }
        CLWKernel GetKernel(std::string const& name);
        std::string GetBuildOpts() const { return m_buildopts; }
        
        
    private:
        CLWContext m_context;
        CLWProgram m_program;
        std::string m_buildopts;
    };
    
    inline ClwClass::ClwClass(
        CLWContext context,
        std::string const& cl_file,
        std::string const& opts
                              )
    : m_context(context)
    {
        m_buildopts.append(" -cl-mad-enable -cl-fast-relaxed-math "
                         "-cl-std=CL1.2 -I . ");
        
        m_buildopts.append(
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
        
        m_buildopts.append(opts);
        
        m_program = CLWProgram::CreateFromFile(cl_file.c_str(),
                                               m_buildopts.c_str(), m_context);
    }
    
    inline CLWKernel ClwClass::GetKernel(std::string const& name)
    {
        return m_program.GetKernel(name);
    }
}
