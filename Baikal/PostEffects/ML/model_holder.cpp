#include "PostEffects/ML/model_holder.h"

#include <stdexcept>
#include <sstream>

#ifdef _WIN32
#   include <windows.h>
#else
#   include <dlfcn.h>
#endif


namespace
{
#ifdef _WIN32
    constexpr wchar_t const* kSharedObject = L"model_runner.dll";
#else
    constexpr char const* kSharedObject = "libmodel_runner.so";
#endif
    constexpr char const* kLoadModelFn = "LoadModel";
    decltype(LoadModel)* g_load_model = nullptr;
}

namespace Baikal
{
    namespace PostEffects
    {
        ModelHolder::ModelHolder()
        {
            if (g_load_model != nullptr)
            {
                return;
            }

#ifdef _WIN32
            auto get_last_error = []
            {
                LPSTR buffer = nullptr;
                auto size = FormatMessageA(
                    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                    nullptr,
                    GetLastError(),
                    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                    (LPSTR)&buffer,
                    0,
                    nullptr);
                std::string message(buffer, size);
                LocalFree(buffer);
                return message;
            };

            auto library = LoadLibraryW(kSharedObject);
            if (library == nullptr)
            {
                throw std::runtime_error("Library error: " + get_last_error());
            }

            auto symbol = GetProcAddress(library, kLoadModelFn);
            if (symbol == nullptr)
            {
                throw std::runtime_error(std::string("Symbol error: ") + get_last_error());
            }
#else
            auto library = dlopen(kSharedObject, RTLD_NOW);
            if (library == nullptr)
            {
                throw std::runtime_error(std::string("Library error: ") + dlerror());
            }

            auto symbol = dlsym(library, kLoadModelFn);
            if (symbol == nullptr)
            {
                throw std::runtime_error(std::string("Symbol error: ") + dlerror());
            }
#endif

            g_load_model = reinterpret_cast<decltype(LoadModel)*>(symbol);
        }

        ModelHolder::ModelHolder(std::string const& model_path,
                                 float gpu_memory_fraction,
                                 std::string const& visible_devices)
        : ModelHolder()
        {
            Reset(model_path, gpu_memory_fraction, visible_devices);
        }

        void ModelHolder::Reset(std::string const& model_path,
                                float gpu_memory_fraction,
                                std::string const& visible_devices)
        {
            m_model.reset(g_load_model(model_path.c_str(),
                                       gpu_memory_fraction,
                                       visible_devices.c_str()));
        }
    }
}
