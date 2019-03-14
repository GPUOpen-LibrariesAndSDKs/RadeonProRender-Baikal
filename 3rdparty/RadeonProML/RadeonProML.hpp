#include <RadeonProML.h>

#include <array>
#include <stdexcept>
#include <string>


namespace RadeonProML {

class Image
{
public:
    Image() = default;

    explicit Image(ml_image image)
        : m_image(image)
    {
    }

    Image(Image&& other) noexcept
        : m_image(other.m_image)
    {
    }

    ~Image()
    {
        if (m_image != nullptr)
        {
            mlReleaseImage(m_image);
        }
    }

    Image& operator=(Image&& other) noexcept
    {
        m_image = other.m_image;
        other.m_image = nullptr;
        return *this;
    }

    void* Map(size_t* size = nullptr)
    {
        return mlMapImage(m_image, size);
    }

    void Unmap(void* data)
    {
        mlUnmapImage(m_image, data);
    }

    operator ml_image() const
    {
        return m_image;
    }

private:
    ml_image m_image = nullptr;
};


class Model
{
public:
    Model() = default;

    explicit Model(ml_model model)
        : m_model(model)
    {
    }

    Model(Model&& other) noexcept
        : m_model(other.m_model)
    {
    }

    ~Model()
    {
        if (m_model != nullptr)
        {
            mlReleaseModel(m_model);
        }
    }

    Model& operator =(Model&& other) noexcept
    {
        m_model = other.m_model;
        other.m_model = nullptr;
        return *this;
    }

#define CHECK_ML_STATUS(OP) CheckStatus(OP, #OP)

    void SetInputInfo(const ml_image_info& info)
    {
        CHECK_ML_STATUS(mlSetModelInputInfo(m_model, &info) == ML_OK);
    }

    ml_image_info GetInputInfo() const
    {
        ml_image_info info;
        CHECK_ML_STATUS(mlGetModelInfo(m_model, &info, nullptr) == ML_OK);
        return info;
    }

    ml_image_info GetOutputInfo() const
    {
        ml_image_info info;
        CHECK_ML_STATUS(mlGetModelInfo(m_model, nullptr, &info) == ML_OK);
        return info;
    }

    void Infer(const Image& input, const Image& output)
    {
        CHECK_ML_STATUS(mlInfer(m_model, input, output) == ML_OK);
    }

#undef CHECK_ML_STATUS

private:
    void CheckStatus(bool status, const char* op_name) const
    {
        if (!status)
        {
            std::string func_name(op_name);
            const auto end_pos = func_name.find('(');
            func_name.erase(end_pos != std::string::npos ? end_pos : func_name.size());

            throw std::runtime_error(func_name + " failed: " + mlGetLastError(nullptr));
        }
    }

    ml_model m_model = nullptr;
};


class Context
{
public:
    Context() = default;

    explicit Context(ml_context context)
        : m_context(context)
    {
    }

    Context(Context&& other) noexcept
        : m_context(other.m_context)
    {
    }

    ~Context()
    {
        if (m_context != nullptr)
        {
            mlReleaseContext(m_context);
        }
    }

    Context& operator =(Context&& other) noexcept
    {
        m_context = other.m_context;
        other.m_context = nullptr;
        return *this;
    }

    Model CreateModel(const ml_model_params& params)
    {
        auto model = mlCreateModel(m_context, &params);
        CheckStatus(model != nullptr, "mlCreateModel");
        return Model(model);
    }

    Image CreateImage(const ml_image_info& info, ml_access_mode mode)
    {
        auto image = mlCreateImage(m_context, &info, mode);
        CheckStatus(image != nullptr, "mlCreateImage");
        return Image(image);
    }

private:
    void CheckStatus(bool status, const char* op_name) const
    {
        if (!status)
        {
            std::string func_name(op_name);
            const auto end_pos = func_name.find('(');
            func_name.erase(end_pos != std::string::npos ? end_pos : func_name.size());

            throw std::runtime_error(func_name + " failed: " + mlGetLastError(nullptr));
        }
    }

    ml_context m_context = nullptr;
};

} // namespace RadeonProML
