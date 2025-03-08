#pragma once

#include <base/scene/model.hpp>
#include <base/scene/mesh.hpp>

#include <base/camera.hpp>

#include <assimp/scene.h>

#include <SDL2/SDL_events.h>

#include <filesystem>
#include <numeric>

#include <stdexcept>

#include <tuple>
#include <map>

namespace vrts
{
    struct Context;
    struct MeshNode;
}

namespace vrts
{
    struct Light
    {
        alignas(16) glm::vec3 pos;
        alignas(16) glm::vec3 col;
    };

    class Scene
    {
    public:
        class Importer;

    public:
        [[nodiscard]] Model&                    getModel()              noexcept;
        [[nodiscard]] const Model&              getModel()              const noexcept;
        [[nodiscard]] CameraController&         getCameraController()   noexcept;
        [[nodiscard]] const std::vector<Light>& getLights()             const noexcept;
        
        [[nodiscard]]
        bool hasAnimator() const;

        [[nodiscard]]
        Animator& getAnimator();

        void processEvent(const SDL_Event* ptr_event);
        void updateCamera();

        void applyTransform(const glm::mat4& transform);

        void addRect(
            const Context*      ptr_context,
            const glm::vec3&    color,
            const glm::vec3&    emissive, 
            const glm::mat4&    transform
        );

    private:
        explicit Scene(
            Model&&                     model, 
            std::vector<Light>&&        lights, 
            const Camera&               camera,
            std::optional<Animator>&&   animator
        );

        [[nodiscard]]
        static std::unique_ptr<MeshNode> makeRectNode(
            const Context*      ptr_context,
            const glm::mat4&    transform
        );

    private:
        Model _model;

        std::vector<Light> _lights;

        CameraController _camera_controller; 

        std::optional<Animator> _animator;
    };

    class Scene::Importer
    {
        using Vertices = std::pair<std::vector<uint32_t>, std::vector<Attributes>>;

        friend class ScopedTransform;

    public:
        Importer(const Context* ptr_context) noexcept;

        Importer(Importer&& importer)       = delete;
        Importer(const Importer& importer)  = delete;

        Importer& operator = (Importer&& importer)      = delete;
        Importer& operator = (const Importer& importer) = delete;

        Importer& path(const std::filesystem::path path);
        Importer& vkMemoryTypeIndex(uint32_t memory_type_index) noexcept;
        Importer& viewport(uint32_t width, uint32_t heigth)     noexcept;

        [[nodiscard]] Scene import();

    private:
        size_t getMeshCount() const;

        void processMaterial(const aiScene* ptr_scene, const aiMaterial* ptr_material);
        void processAnimation(const aiMesh* ptr_mesh, std::span<SkinningData> skinning_data);
        void processNode(const aiScene* ptr_scene, const aiNode* ptr_node);

        void getKeyFrames(const aiAnimation* ptr_animation);
        void getAnimation(const aiScene* ptr_scene);

        [[nodiscard]]
        Mesh createMesh (
            const std::string_view      name,
            const std::span<uint32_t>   indices,
            const std::span<Attributes> attributes
        ) const;
        
        [[nodiscard]]
        SkinnedMesh createMesh (
            const std::string_view          name,
            const std::span<uint32_t>       indices,
            const std::span<Attributes>     attributes,
            const std::span<SkinningData>   skinning_data
        ) const;


        [[nodiscard]]
        Image getImage(
            const aiTexture*    ptr_texture, 
            VkFilter            filter_for_mipmap, 
            int32_t             channels_per_pixel
        ) const;

        [[nodiscard]]
        Image getDefaultTexture(int32_t channels_per_pixel);

        [[nodiscard]]
        Image getTexture(
            const aiScene*      ptr_scene, 
            const aiMaterial*   ptr_material, 
            aiTextureType       texuture_type,
            int32_t             channels_per_pixel,
            VkFilter            filter
        );

        void add(
            const std::string_view          name, 
            const std::span<uint32_t>       indices, 
            const std::span<Attributes>     attributes, 
            const std::span<SkinningData>   skinning_data
        );

        void add(const aiLight* ptr_light);

        void validate() const;

    private:
        std::filesystem::path _path;

        const Context* _ptr_context;

        uint32_t _memory_type_index = std::numeric_limits<uint32_t>::max();

        uint32_t _width     = 0;
        uint32_t _height    = 0;

        MaterialManager _material_manager;
        
        std::unique_ptr<Node> _ptr_root_node;

        std::vector<Light>                      _processed_lights;
        std::map<std::string, const aiLight*>   _scene_lights;

        struct 
        {
            glm::mat4 transform = glm::mat4(1.0f);
        } _current_state;

        struct
        {
            BoneRegistry        bone_infos;
            std::vector<Bone>   bones;
            
            AnimationHierarchiry::Node  root_node;
            AnimationHierarchiry::Node* ptr_current_node = &root_node;

            std::optional<Animator> animator;
        } _animation;
    };
}