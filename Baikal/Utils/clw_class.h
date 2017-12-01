#pragma once

#include <string>
#include <numeric>
#include <vector>
#include <fstream>
#include <regex>
#include <sstream>
#include <unordered_map>

#include "CLW.h"
#include "version.h"

namespace Baikal
{
    class ClwClass
    {
    public:
        //create from file
        ClwClass(CLWContext context,
            std::string const& cl_file,
            std::string const& opts = "",
            std::string const& cache_path = "");

        //create from memory
        ClwClass(CLWContext context,
            const char* data,
            const char* includes[],
            std::size_t inc_num,
            std::string const& opts = "",
            std::string const& cache_path = "");

        virtual ~ClwClass() = default;


    protected:
        CLWContext GetContext() const { return m_context; }
        CLWKernel GetKernel(std::string const& name, std::string const& opts = "");
        void SetDefaultBuildOptions(std::string const& opts);
        std::string GetDefaultBuildOpts() const { return m_default_opts; }
        std::string GetFullBuildOpts() const;
        CLWProgram CreateProgram(std::string const& filename, std::string const& opts, CLWContext context);
        std::string GetFilenameHash(std::string const& filename, std::string const& opts, CLWContext context) const;

        bool LoadBinaries(std::string const& name, std::vector<std::uint8_t>& data) const;
        void SaveBinaries(std::string const& name, std::vector<std::uint8_t>& data) const;

    private:
        void AddCommonOptions(std::string& opts) const;

        // Context to build programs for
        CLWContext m_context;
        // Mapping of build options to programs
        std::unordered_map<std::string, CLWProgram> m_programs;
        // Default build options
        std::string m_default_opts;
        // Name of program CL file
        std::string m_cl_file;
        // Binary cache path
        std::string m_cache_path;
    };

    inline void ClwClass::AddCommonOptions(std::string& opts) const {
        opts.append(" -cl-mad-enable -cl-fast-relaxed-math "
            "-cl-std=CL1.2 -I . ");

        opts.append(
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
    }

    inline std::string ClwClass::GetFullBuildOpts() const {
        auto options = m_default_opts;
        AddCommonOptions(options);
        return options;
    }

    inline ClwClass::ClwClass(
        CLWContext context,
        std::string const& cl_file,
        std::string const& opts,
        std::string const& cache_path)
    : m_context(context)
    , m_cl_file(cl_file)
    , m_default_opts(opts)
    , m_cache_path(cache_path)
    {
        auto program = CreateProgram(cl_file, m_default_opts, m_context);
        m_programs.emplace(std::make_pair(m_default_opts, program));
    }

    inline ClwClass::ClwClass(CLWContext context,
        const char* data,
        const char* includes[],
        std::size_t inc_num,
        std::string const& opts,
        std::string const& cache_path)
        : m_context(context)
        , m_cl_file("")
        , m_default_opts(opts)
        , m_cache_path(cache_path)
    {
        auto options = opts;
        AddCommonOptions(options);

        std::string src("");
        //append all includes and data
        for (int i = 0; i < inc_num; ++i)
        {
            src += includes[i];
        }
        src += data;
        auto program = CLWProgram::CreateFromSource(src.data(), src.length(), m_default_opts.c_str(), m_context);
        m_programs.emplace(std::make_pair(m_default_opts, program));
    }

    inline void ClwClass::SetDefaultBuildOptions(std::string const& opts) {
        m_default_opts = opts;
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

    inline CLWProgram ClwClass::CreateProgram(std::string const& filename, std::string const& opts, CLWContext context)
    {
        CLWProgram result;

        auto options = opts;
        AddCommonOptions(options);

        // Try from cache first
        if (!m_cache_path.empty())
        {
            auto hash = GetFilenameHash(filename, options, context);

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
                result = CLWProgram::CreateFromBinary(&binaries, &size, context);
            }
            else
            {
                result = CLWProgram::CreateFromFile(filename.c_str(), options.c_str(), context);
                // Save binaries
                result.GetBinaries(0, binary);
                SaveBinaries(cached_program_path, binary);
            }
        }
        else
        {
            result = CLWProgram::CreateFromFile(filename.c_str(), options.c_str(), context);
        }

        return result;
    }

    inline CLWKernel ClwClass::GetKernel(std::string const& name, std::string const& opts)
    {
        std::string options = opts.empty() ? m_default_opts : opts;

        auto iter = m_programs.find(options);

        if (iter != m_programs.cend()) {
            return iter->second.GetKernel(name);
        } else {
            auto program = CreateProgram(m_cl_file, options, m_context);
            m_programs.emplace(std::make_pair(options, program));
            return program.GetKernel(name);
        }
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
        name.append(BAIKAL_VERSION);

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
