#pragma once

#include <string>
#include "CLW.h"

#if defined(_WIN32) || defined (WIN32)
    #define NOMINMAX
    #include <windows.h>
    #include <algorithm>
#endif

namespace Baikal
{
    class ClwClass
    {
    public:
        ClwClass(CLWContext context,
                 std::string const& cl_file,
                 std::string const& opts = "");
        virtual ~ClwClass();

        void Rebuild(std::string const& opts);
        static void Update();
    protected:
        CLWContext GetContext() const { return m_context; }
        CLWKernel GetKernel(std::string const& name);
        std::string GetBuildOpts() const { return m_buildopts; }
        
    private:
        CLWContext m_context;
        CLWProgram m_program;
        std::string m_buildopts;
        std::string m_cl_file;
        std::string m_opts;

#if defined(_WIN32) || defined (WIN32)
        FILETIME    m_file_time;
        static std::vector<ClwClass*> m_clw_classes;
#endif
    };
    
    inline ClwClass::ClwClass(
        CLWContext context,
        std::string const& cl_file,
        std::string const& opts
                              )
    : m_context(context)
    , m_cl_file(cl_file)
    , m_opts(opts)
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

        auto cmdopts = m_buildopts;
        cmdopts.append(opts);
        m_program = CLWProgram::CreateFromFile(cl_file.c_str(),
                                               cmdopts.c_str(), m_context);

#if defined(_WIN32) || defined (WIN32)
        WIN32_FILE_ATTRIBUTE_DATA fileAttrData = {0};
        GetFileAttributesEx(cl_file.c_str(), GetFileExInfoStandard, &fileAttrData);

        m_file_time = fileAttrData.ftLastWriteTime;

        m_clw_classes.push_back(this);
#endif
    }

    inline ClwClass::~ClwClass()
    {
#if defined(_WIN32) || defined (WIN32)
        auto it = std::find(m_clw_classes.begin(), m_clw_classes.end(), this);

        assert(it != m_clw_classes.end());

        if (it != m_clw_classes.end())
        {
            m_clw_classes.erase(it);
        }
#endif
    }

    inline void ClwClass::Rebuild(std::string const& opts)
    {
        bool file_time_changed = false;

#if defined(_WIN32) || defined (WIN32)
        WIN32_FILE_ATTRIBUTE_DATA fileAttrData = { 0 };
        GetFileAttributesEx(m_cl_file.c_str(), GetFileExInfoStandard, &fileAttrData);

        FILETIME file_time = fileAttrData.ftLastWriteTime;

        file_time_changed = (m_file_time.dwHighDateTime != file_time.dwHighDateTime) || (m_file_time.dwLowDateTime != file_time.dwLowDateTime);
#endif

        if (m_opts != opts || file_time_changed)
        {
            auto cmdopts = m_buildopts;
            m_file_time = file_time;

            cmdopts.append(opts);
            m_opts = opts;
            m_program = CLWProgram::CreateFromFile(m_cl_file.c_str(),
                cmdopts.c_str(), m_context);
        }
    }

    inline CLWKernel ClwClass::GetKernel(std::string const& name)
    {
        return m_program.GetKernel(name);
    }
}
