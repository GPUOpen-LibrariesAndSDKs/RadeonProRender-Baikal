#pragma once
#ifdef ENABLE_GLTF

#include <string>
#include <array>
#include <vector>
#include <unordered_map>
#include <json.hpp>

namespace gltf
{
    // Attribute semantic property name declarations. These are not provided by the glTF schema files.
    namespace AttributeNames
    {
        static const std::string POSITION = "POSITION";
        static const std::string NORMAL = "NORMAL";
        static const std::string TANGENT = "TANGENT";

        static const std::string TEXCOORD_0 = "TEXCOORD_0";
        static const std::string TEXCOORD_1 = "TEXCOORD_1";
        static const std::string TEXCOORD_2 = "TEXCOORD_2";
        static const std::string TEXCOORD_3 = "TEXCOORD_3";
        static const std::string TEXCOORD_4 = "TEXCOORD_4";
        static const std::string TEXCOORD_5 = "TEXCOORD_5";
        static const std::string TEXCOORD_6 = "TEXCOORD_6";
        static const std::string TEXCOORD_7 = "TEXCOORD_7";

        static const std::string COLOR_0 = "COLOR_0";
        static const std::string COLOR_1 = "COLOR_1";
        static const std::string COLOR_2 = "COLOR_2";
        static const std::string COLOR_3 = "COLOR_3";
        static const std::string COLOR_4 = "COLOR_4";
        static const std::string COLOR_5 = "COLOR_5";
        static const std::string COLOR_6 = "COLOR_6";
        static const std::string COLOR_7 = "COLOR_7";

        static const std::string JOINTS_0 = "JOINTS_0";
        static const std::string JOINTS_1 = "JOINTS_1";
        static const std::string JOINTS_2 = "JOINTS_2";
        static const std::string JOINTS_3 = "JOINTS_3";
        static const std::string JOINTS_4 = "JOINTS_4";
        static const std::string JOINTS_5 = "JOINTS_5";
        static const std::string JOINTS_6 = "JOINTS_6";
        static const std::string JOINTS_7 = "JOINTS_7";

        static const std::string WEIGHTS_0 = "WEIGHTS_0";
        static const std::string WEIGHTS_1 = "WEIGHTS_1";
        static const std::string WEIGHTS_2 = "WEIGHTS_2";
        static const std::string WEIGHTS_3 = "WEIGHTS_3";
        static const std::string WEIGHTS_4 = "WEIGHTS_4";
        static const std::string WEIGHTS_5 = "WEIGHTS_5";
        static const std::string WEIGHTS_6 = "WEIGHTS_6";
        static const std::string WEIGHTS_7 = "WEIGHTS_7";
    }

    // Dictionary object with extension-specific objects.
    using Extension = std::unordered_map<std::string, nlohmann::json>;

    // Application-specific data.
    using Extras = nlohmann::json;

    // No description
    struct glTFProperty
    {
        Extension extensions;
        Extras extras;
    };

    // Indices of those attributes that deviate from their initialization value.
    struct Accessor_Sparse_Indices : glTFProperty
    {
        enum class ComponentType
        {
            UNSIGNED_BYTE = 5121,
            UNSIGNED_SHORT = 5123,
            UNSIGNED_INT = 5125,
        };

        int bufferView = -1; // The index of the bufferView with sparse indices. Referenced bufferView can't have ARRAY_BUFFER or ELEMENT_ARRAY_BUFFER target.
        int byteOffset = 0; // The offset relative to the start of the bufferView in bytes. Must be aligned.
        ComponentType componentType; // The indices data type.
    };

    // Array of size `accessor.sparse.count` times number of components storing the displaced accessor attributes pointed by `accessor.sparse.indices`.
    struct Accessor_Sparse_Values : glTFProperty
    {
        int bufferView = -1; // The index of the bufferView with sparse values. Referenced bufferView can't have ARRAY_BUFFER or ELEMENT_ARRAY_BUFFER target.
        int byteOffset = 0; // The offset relative to the start of the bufferView in bytes. Must be aligned.
    };

    // The index of the node and TRS property that an animation channel targets.
    struct Animation_Channel_Target : glTFProperty
    {
        enum class Path
        {
            TRANSLATION = 0,
            ROTATION = 1,
            SCALE = 2,
            WEIGHTS = 3,
        };

        int node = -1; // The index of the node to target.
        Path path; // The name of the node's TRS property to modify, or the "weights" of the Morph Targets it instantiates.
    };

    // Combines input and output accessors with an interpolation algorithm to define a keyframe graph (but not its target).
    struct Animation_Sampler : glTFProperty
    {
        enum class Interpolation
        {
            LINEAR = 0,
            STEP = 1,
            CATMULLROMSPLINE = 2,
            CUBICSPLINE = 3,
        };

        int input = -1; // The index of an accessor containing keyframe input values, e.g., time.
        Interpolation interpolation; // Interpolation algorithm.
        int output = -1; // The index of an accessor, containing keyframe output values.
    };

    // Metadata about the glTF asset.
    struct Asset : glTFProperty
    {
        std::string copyright; // A copyright message suitable for display to credit the content creator.
        std::string generator; // Tool that generated this glTF model.  Useful for debugging.
        std::string version; // The glTF version that this asset targets.
        std::string minVersion; // The minimum glTF version that this asset targets.
    };

    // An orthographic camera containing properties to create an orthographic projection matrix.
    struct Camera_Orthographic : glTFProperty
    {
        float xmag; // The floating-point horizontal magnification of the view. Must not be zero.
        float ymag; // The floating-point vertical magnification of the view. Must not be zero.
        float zfar; // The floating-point distance to the far clipping plane. `zfar` must be greater than `znear`.
        float znear; // The floating-point distance to the near clipping plane.
    };

    // A perspective camera containing properties to create a perspective projection matrix.
    struct Camera_Perspective : glTFProperty
    {
        float aspectRatio; // The floating-point aspect ratio of the field of view.
        float yfov; // The floating-point vertical field of view in radians.
        float zfar; // The floating-point distance to the far clipping plane.
        float znear; // The floating-point distance to the near clipping plane.
    };

    // No description
    struct glTFChildOfRootProperty : glTFProperty
    {
        std::string name; // The user-defined name of this object.
    };

    // Geometry to be rendered with the given material.
    struct Mesh_Primitive : glTFProperty
    {
        enum class Mode
        {
            POINTS = 0,
            LINES = 1,
            LINE_LOOP = 2,
            LINE_STRIP = 3,
            TRIANGLES = 4,
            TRIANGLE_STRIP = 5,
            TRIANGLE_FAN = 6,
        };

        std::unordered_map<std::string, int> attributes; // A dictionary object, where each key corresponds to mesh attribute semantic and each value is the index of the accessor containing attribute's data.
        int indices = -1; // The index of the accessor that contains the indices.
        int material = -1; // The index of the material to apply to this primitive when rendering.
        Mode mode; // The type of primitives to render.        
        std::vector<std::unordered_map<std::string, int>> targets; // An array of Morph Targets, each  Morph Target is a dictionary mapping attributes (only `POSITION`, `NORMAL`, and `TANGENT` supported) to their deviations in the Morph Target.
    };

    // Reference to a texture.
    struct TextureInfo : glTFProperty
    {
        int index = -1; // The index of the texture.
        int texCoord = 0; // The set index of texture's TEXCOORD attribute used for texture coordinate mapping.
    };

    // Sparse storage of attributes that deviate from their initialization value.
    struct Accessor_Sparse : glTFProperty
    {
        int count = 0; // Number of entries stored in the sparse array.
        Accessor_Sparse_Indices indices; // Index array of size `count` that points to those accessor attributes that deviate from their initialization value. Indices must strictly increase.
        Accessor_Sparse_Values values; // Array of size `count` times number of components, storing the displaced accessor attributes pointed by `indices`. Substituted values must have the same `componentType` and number of components as the base accessor.
    };

    // Targets an animation's sampler at a node's property.
    struct Animation_Channel : glTFProperty
    {
        int sampler = -1; // The index of a sampler in this animation used to compute the value for the target.
        Animation_Channel_Target target; // The index of the node and TRS property to target.
    };

    // A buffer points to binary geometry, animation, or skins.
    struct Buffer : glTFChildOfRootProperty
    {
        std::string uri; // The uri of the buffer.
        int byteLength = 0; // The length of the buffer in bytes.
    };

    // A view into a buffer generally representing a subset of the buffer.
    struct BufferView : glTFChildOfRootProperty
    {
        enum class Target
        {
            ARRAY_BUFFER = 34962,
            ELEMENT_ARRAY_BUFFER = 34963,
        };

        int buffer = -1; // The index of the buffer.
        int byteOffset = 0; // The offset into the buffer in bytes.
        int byteLength = 0; // The length of the bufferView in bytes.
        int byteStride = 0; // The stride, in bytes.
        Target target; // The target that the GPU buffer should be bound to.
    };

    // A camera's projection.  A node can reference a camera to apply a transform to place the camera in the scene.
    struct Camera : glTFChildOfRootProperty
    {
        enum class Type
        {
            PERSPECTIVE = 0,
            ORTHOGRAPHIC = 1,
        };

        Camera_Orthographic orthographic; // An orthographic camera containing properties to create an orthographic projection matrix.
        Camera_Perspective perspective; // A perspective camera containing properties to create a perspective projection matrix.
        Type type; // Specifies if the camera uses a perspective or orthographic projection.
    };

    // Image data used to create a texture. Image can be referenced by URI or `bufferView` index. `mimeType` is required in the latter case.
    struct Image : glTFChildOfRootProperty
    {
        enum class MimeType
        {
            IMAGE_JPEG = 0,
            IMAGE_PNG = 1,
        };

        std::string uri; // The uri of the image.
        MimeType mimeType; // The image's MIME type.
        int bufferView = -1; // The index of the bufferView that contains the image. Use this instead of the image's uri property.
    };

    // No description
    struct Material_NormalTextureInfo : TextureInfo
    {
        int index = -1; // The index of the texture.
        int texCoord = 0; // The set index of texture's TEXCOORD attribute used for texture coordinate mapping.
        float scale = 1.0f; // The scalar multiplier applied to each normal vector of the normal texture.
    };

    // No description
    struct Material_OcclusionTextureInfo : TextureInfo
    {
        int index = -1; // The index of the texture.
        int texCoord = 0; // The set index of texture's TEXCOORD attribute used for texture coordinate mapping.
        float strength = 1.0f; // A scalar multiplier controlling the amount of occlusion applied.
    };

    // A set of parameter values that are used to define the metallic-roughness material model from Physically-Based Rendering (PBR) methodology.
    struct Material_PbrMetallicRoughness : glTFProperty
    {
        std::array<float, 4> baseColorFactor = { 1.0f, 1.0f, 1.0f, 1.0f }; // The material's base color factor.
        TextureInfo baseColorTexture; // The base color texture.
        float metallicFactor = 1.0f; // The metalness of the material.
        float roughnessFactor = 1.0f; // The roughness of the material.
        TextureInfo metallicRoughnessTexture; // The metallic-roughness texture.
    };

    // A set of primitives to be rendered.  A node can contain one mesh.  A node's transform places the mesh in the scene.
    struct Mesh : glTFChildOfRootProperty
    {
        std::vector<Mesh_Primitive> primitives; // An array of primitives, each defining geometry to be rendered with a material.
        std::vector<float> weights; // Array of weights to be applied to the Morph Targets.
    };

    // A node in the node hierarchy.  When the node contains `skin`, all `mesh.primitives` must contain `JOINTS_0` and `WEIGHTS_0` attributes.  A node can have either a `matrix` or any combination of `translation`/`rotation`/`scale` (TRS) properties. TRS properties are converted to matrices and postmultiplied in the `T * R * S` order to compose the transformation matrix; first the scale is applied to the vertices, then the rotation, and then the translation. If none are provided, the transform is the identity. When a node is targeted for animation (referenced by an animation.channel.target), only TRS properties may be present; `matrix` will not be present.
    struct Node : glTFChildOfRootProperty
    {
        int camera = -1; // The index of the camera referenced by this node.
        std::vector<int> children; // The indices of this node's children.
        int skin = -1; // The index of the skin referenced by this node.
        std::array<float, 16> matrix = { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f }; // A floating-point 4x4 transformation matrix stored in column-major order.
        int mesh = -1; // The index of the mesh in this node.
        std::array<float, 4> rotation = { 0.0f, 0.0f, 0.0f, 1.0f }; // The node's unit quaternion rotation in the order (x, y, z, w), where w is the scalar.
        std::array<float, 3> scale = { 1.0f, 1.0f, 1.0f }; // The node's non-uniform scale.
        std::array<float, 3> translation = { 0.0f, 0.0f, 0.0f }; // The node's translation.
        std::vector<float> weights; // The weights of the instantiated Morph Target. Number of elements must match number of Morph Targets of used mesh.
    };

    // Texture sampler properties for filtering and wrapping modes.
    struct Sampler : glTFChildOfRootProperty
    {
        enum class MagFilter
        {
            NEAREST = 9728,
            LINEAR = 9729,
        };

        enum class MinFilter
        {
            NEAREST = 9728,
            LINEAR = 9729,
            NEAREST_MIPMAP_NEAREST = 9984,
            LINEAR_MIPMAP_NEAREST = 9985,
            NEAREST_MIPMAP_LINEAR = 9986,
            LINEAR_MIPMAP_LINEAR = 9987,
        };

        enum class WrapS
        {
            CLAMP_TO_EDGE = 33071,
            MIRRORED_REPEAT = 33648,
            REPEAT = 10497,
        };

        enum class WrapT
        {
            CLAMP_TO_EDGE = 33071,
            MIRRORED_REPEAT = 33648,
            REPEAT = 10497,
        };

        MagFilter magFilter; // Magnification filter.
        MinFilter minFilter; // Minification filter.
        WrapS wrapS; // s wrapping mode.
        WrapT wrapT; // t wrapping mode.
    };

    // The root nodes of a scene.
    struct Scene : glTFChildOfRootProperty
    {
        std::vector<int> nodes; // The indices of each root node.
    };

    // Joints and matrices defining a skin.
    struct Skin : glTFChildOfRootProperty
    {
        int inverseBindMatrices = -1; // The index of the accessor containing the floating-point 4x4 inverse-bind matrices.  The default is that each matrix is a 4x4 identity matrix, which implies that inverse-bind matrices were pre-applied.
        int skeleton = -1; // The index of the node used as a skeleton root. When undefined, joints transforms resolve to scene root.
        std::vector<int> joints; // Indices of skeleton nodes, used as joints in this skin.
    };

    // A texture and its sampler.
    struct Texture : glTFChildOfRootProperty
    {
        int sampler = -1; // The index of the sampler used by this texture. When undefined, a sampler with repeat wrapping and auto filtering should be used.
        int source = -1; // The index of the image used by this texture.
    };

    // A typed view into a bufferView.  A bufferView contains raw binary data.  An accessor provides a typed view into a bufferView or a subset of a bufferView similar to how WebGL's `vertexAttribPointer()` defines an attribute in a buffer.
    struct Accessor : glTFChildOfRootProperty
    {
        enum class ComponentType
        {
            BYTE = 5120,
            UNSIGNED_BYTE = 5121,
            SHORT = 5122,
            UNSIGNED_SHORT = 5123,
            UNSIGNED_INT = 5125,
            FLOAT = 5126,
        };

        enum class Type
        {
            SCALAR = 0,
            VEC2 = 1,
            VEC3 = 2,
            VEC4 = 3,
            MAT2 = 4,
            MAT3 = 5,
            MAT4 = 6,
        };

        int bufferView = -1; // The index of the bufferView.
        int byteOffset = 0; // The offset relative to the start of the bufferView in bytes.
        ComponentType componentType; // The datatype of components in the attribute.
        bool normalized = false; // Specifies whether integer data values should be normalized.
        int count = 0; // The number of attributes referenced by this accessor.
        Type type; // Specifies if the attribute is a scalar, vector, or matrix.
        std::vector<float> max; // Maximum value of each component in this attribute.
        std::vector<float> min; // Minimum value of each component in this attribute.
        Accessor_Sparse sparse; // Sparse storage of attributes that deviate from their initialization value.
    };

    // A keyframe animation.
    struct Animation : glTFChildOfRootProperty
    {
        std::vector<Animation_Channel> channels; // An array of channels, each of which targets an animation's sampler at a node's property. Different channels of the same animation can't have equal targets.
        std::vector<Animation_Sampler> samplers; // An array of samplers that combines input and output accessors with an interpolation algorithm to define a keyframe graph (but not its target).
    };

    // The material appearance of a primitive.
    struct Material : glTFChildOfRootProperty
    {
        enum class AlphaMode
        {
            OPAQUE = 0,
            MASK = 1,
            BLEND = 2,
        };

        Material_PbrMetallicRoughness pbrMetallicRoughness; // A set of parameter values that are used to define the metallic-roughness material model from Physically-Based Rendering (PBR) methodology. When not specified, all the default values of `pbrMetallicRoughness` apply.
        Material_NormalTextureInfo normalTexture; // The normal map texture.
        Material_OcclusionTextureInfo occlusionTexture; // The occlusion map texture.
        TextureInfo emissiveTexture; // The emissive map texture.
        std::array<float, 3> emissiveFactor = { 0.0f, 0.0f, 0.0f }; // The emissive color of the material.
        AlphaMode alphaMode; // The alpha rendering mode of the material.
        float alphaCutoff = 0.5f; // The alpha cutoff value of the material.
        bool doubleSided = false; // Specifies whether the material is double sided.
    };

    // The root object for a glTF asset.
    struct glTF : glTFProperty
    {
        std::vector<std::string> extensionsUsed; // Names of glTF extensions used somewhere in this asset.
        std::vector<std::string> extensionsRequired; // Names of glTF extensions required to properly load this asset.
        std::vector<Accessor> accessors; // An array of accessors.
        std::vector<Animation> animations; // An array of keyframe animations.
        Asset asset; // Metadata about the glTF asset.
        std::vector<Buffer> buffers; // An array of buffers.
        std::vector<BufferView> bufferViews; // An array of bufferViews.
        std::vector<Camera> cameras; // An array of cameras.
        std::vector<Image> images; // An array of images.
        std::vector<Material> materials; // An array of materials.
        std::vector<Mesh> meshes; // An array of meshes.
        std::vector<Node> nodes; // An array of nodes.
        std::vector<Sampler> samplers; // An array of samplers.
        int scene = -1; // The index of the default scene.
        std::vector<Scene> scenes; // An array of scenes.
        std::vector<Skin> skins; // An array of skins.
        std::vector<Texture> textures; // An array of textures.
    };

    // Imports a gltf 2.0 file from disk.
    bool Import(const std::string& filename, glTF& gltf);

    // Exports a gltf file to disk.
    bool Export(const std::string& filename, const glTF& gltf);

} // End namespace gltf

#endif //ENABLE_GLTF
