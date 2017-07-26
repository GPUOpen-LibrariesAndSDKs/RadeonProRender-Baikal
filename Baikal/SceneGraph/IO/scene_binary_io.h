#include "scene_io.h"
#include <vector>
#include <memory>

#define _USE_MATH_DEFINES
#include <math.h>

namespace Baikal
{
    // Create fake test IO
    class SceneBinaryIo : public SceneIo
    {
    public:
        SceneBinaryIo() = default;
        // Load scene (this class uses filename to determine what scene to generate)
        std::unique_ptr<Scene1> LoadScene(std::string const& filename, std::string const& basepath) const override;
        void SaveScene(Scene1 const& scene, std::string const& filename, std::string const& basepath) const override;
    };
}