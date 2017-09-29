#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <regex>
#include <sstream>

#include "CLW.h"

namespace Baikal
{
    class ClwClass
    {
    public:
        ClwClass(CLWContext context,
            std::string const& cl_file,
            std::string const& opts = "",
            std::string const& cache_path = "");

        virtual ~ClwClass() = default;

        void Rebuild(std::string const& opts);
    protected:
        CLWContext GetContext() const { return m_context; }
        CLWKernel GetKernel(std::string const& name);
        std::string GetBuildOpts() const { return m_buildopts; }
        void CreateProgram(std::string const& filename, std::string const& opts, CLWContext context);
        std::string GetFilenameHash(std::string const& filename, std::string const& opts, CLWContext context) const;

        bool LoadBinaries(std::string const& name, std::vector<std::uint8_t>& data) const;
        void SaveBinaries(std::string const& name, std::vector<std::uint8_t>& data) const;

    private:
        CLWContext m_context;
        CLWProgram m_program;
        std::string m_buildopts;
        std::string m_cl_file;
        std::string m_opts;
        std::string m_cache_path;
    };
    
    inline ClwClass::ClwClass(
        CLWContext context,
        std::string const& cl_file,
        std::string const& opts,
        std::string const& cache_path)
    : m_context(context)
    , m_cl_file(cl_file)
    , m_opts(opts)
    , m_cache_path(cache_path)
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

        CreateProgram(cl_file, cmdopts, m_context);
    }

    inline uint32_t jenkins_one_at_a_time_hash(char const *key, size_t len)
    {
        uint32_t hash, i;
        for (hash = i = 0; i < len; ++i)
        {
            hash += key[i];
            hash += (hash << 10);
            hash ^= (hash >> 6);
        }
        hash += (hash << 3);
        hash ^= (hash >> 11);
        hash += (hash << 15);
        return hash;
    }

    inline void ClwClass::CreateProgram(std::string const& filename, std::string const& opts, CLWContext context)
    {
        // Try from cache first
        if (!m_cache_path.empty())
        {
            auto hash = GetFilenameHash(filename, opts, context);

            auto cached_program_path = m_cache_path;
            cached_program_path.append("/");
            cached_program_path.append(hash);
            cached_program_path.append(".bin");

            std::vector<std::uint8_t> binary;
            if (LoadBinaries(cached_program_path, binary))
            {
                // Create from binary
                std::size_t size = binary.size();
                auto binaries = &binary[0];
                m_program = CLWProgram::CreateFromBinary(&binaries, &size, context);
            }
            else
            {
                m_program = CLWProgram::CreateFromFile(filename.c_str(), opts.c_str(), context);
                // Save binaries
                m_program.GetBinaries(0, binary);
                SaveBinaries(cached_program_path, binary);
            }
        }
        else
        {
            m_program = CLWProgram::CreateFromFile(filename.c_str(), opts.c_str(), context);
        }
    }

    inline void ClwClass::Rebuild(std::string const& opts)
    {
        if (m_opts != opts)
        {
            auto cmdopts = m_buildopts;
            cmdopts.append(opts);
            m_opts = opts;
            CreateProgram(m_cl_file, cmdopts, m_context);
        }
    }

    inline CLWKernel ClwClass::GetKernel(std::string const& name)
    {
        return m_program.GetKernel(name);
    }

    inline std::string ClwClass::GetFilenameHash(std::string const& filename, std::string const& opts, CLWContext context) const
    {
        std::regex delimiter("\\\\");
        std::regex forbidden("(\\\\)|[\\./:<>\\\"\\|\\?\\*]");
        auto fullpath = std::regex_replace(filename, delimiter, "/");

        auto filename_start = fullpath.find_last_of('/');

        if (filename_start == std::string::npos)
            filename_start = 0;
        else
            filename_start += 1;

        auto filename_end = fullpath.find_last_of('.');

        if (filename_end == std::string::npos)
            filename_end = fullpath.size();

        auto name = fullpath.substr(filename_start, filename_end - filename_start);

        auto device_name = context.GetDevice(0).GetName();

        device_name = std::regex_replace(device_name, forbidden, "_");
        device_name.erase(
            std::remove_if(device_name.begin(), device_name.end(), isspace),
            device_name.end());

        name.append("_");
        name.append(device_name);

        auto extra = context.GetDevice(0).GetVersion();
        extra.append(opts);

        std::ostringstream oss;
        oss << jenkins_one_at_a_time_hash(extra.c_str(), extra.size());

        name.append("_");
        name.append(oss.str());

        return name;
    }

    inline bool ClwClass::LoadBinaries(std::string const& name, std::vector<std::uint8_t>& data) const
    {
        std::ifstream in(name, std::ios::in | std::ios::binary);

        if (in)
        {
            data.clear();

            std::streamoff beg = in.tellg();

            in.seekg(0, std::ios::end);

            std::streamoff fileSize = in.tellg() - beg;

            in.seekg(0, std::ios::beg);

            data.resize(static_cast<unsigned>(fileSize));

            in.read((char*)&data[0], fileSize);

            return true;
        }
        else
        {
            return false;
        }
    }

    inline void ClwClass::SaveBinaries(std::string const& name, std::vector<std::uint8_t>& data) const
    {
        std::ofstream out(name, std::ios::out | std::ios::binary);

        if (out)
        {
            out.write((char*)&data[0], data.size());
        }
    }
}
