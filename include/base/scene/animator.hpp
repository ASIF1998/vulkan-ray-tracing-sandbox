#pragma once

#include <base/math.hpp>

#include <string>
#include <string_view>

#include <vector>
#include <map>
#include <span>

#include <optional>

namespace sample_vk
{
    struct AnimationHierarchiry
    {
        struct Node
        {
            std::string         name;
            glm::mat4           transform = glm::mat4(1.0f);
            std::vector<Node>   children;
        };
    };

    struct PositionKey
    {
        glm::vec3   pos;
        float       time_stamp = 0.0f;
    };

    struct RotationKey
    {
        glm::quat   rotate;
        float       time_stamp = 0.0f;
    };

    struct ScaleKey
    {
        glm::vec3   scale;
        float       time_stamp = 0.0f;
    };

    struct BoneInfo
    {
        uint32_t    bone_id = std::numeric_limits<uint32_t>::infinity();
        glm::mat4   offset  = glm::mat4(1.0f);
    };

    class BoneRegistry
    {
    public:
        BoneRegistry() = default;

        BoneRegistry(BoneRegistry&& bone_registry)          = default;
        BoneRegistry(const BoneRegistry&& bone_registry)    = delete;

        BoneRegistry& operator = (BoneRegistry&& bone_registry)         = default;
        BoneRegistry& operator = (const BoneRegistry& bone_registry)    = default;

        [[nodiscard]] 
        std::optional<BoneInfo> get(std::string_view name) const;

        [[nodiscard]]
        size_t boneCount() const;

        void add(std::string_view name, const BoneInfo& bone);

    private:
        std::map<std::string, BoneInfo, std::less<void>> _bones;
    };

    class Bone
    {
        [[nodiscard]]
        static float getScaleFactor(float t1, float t2, float t);

        [[nodiscard]] size_t getPositionIndex(float time)   const;
        [[nodiscard]] size_t getRotationIndex(float time)   const;
        [[nodiscard]] size_t getScaleIndex(float time)      const;

        [[nodiscard]] glm::mat4 getKeyPosition(float time)  const;
        [[nodiscard]] glm::mat4 getKeyRotation(float time)  const;
        [[nodiscard]] glm::mat4 getKeyScale(float time)     const;

        Bone(
            std::string_view            name,
            uint32_t                    id,
            std::vector<PositionKey>&&  position_keys,
            std::vector<RotationKey>&&  rotation_keys,
            std::vector<ScaleKey>&&     scale_keys
        );

    public:
        class Builder;

    public:
        Bone(Bone&& bone)       = default;
        Bone(const Bone& bone)  = delete;

        Bone& operator = (Bone&& bone)      = default;
        Bone& operator = (const Bone& bone) = delete;

        void update(float time);

        [[nodiscard]]
        const std::string& getName() const noexcept;

        [[nodiscard]]
        const glm::mat4& getLocalTransform() const noexcept;

    private:
        std::string _name;
        uint32_t    _id;

        glm::mat4 _local_transform = glm::mat4(1.0f);

        std::vector<PositionKey>    _position_keys;
        std::vector<RotationKey>    _rotation_keys;
        std::vector<ScaleKey>       _scale_keys;
    };

    class Bone::Builder
    {
        void validate() const;

    public:
        Builder() = default;

        Builder(Builder&& builder)      = delete;
        Builder(const Builder& builder) = delete;

        Builder& operator = (Builder&& builder)         = delete;
        Builder& operator = (const Builder& builder)    = delete;

        Builder& name(std::string_view name);
        Builder& id(uint32_t bone_id);

        Builder& positionKeys(std::vector<PositionKey>&& position_keys);
        Builder& rotationKeys(std::vector<RotationKey>&& rotation_keys);
        Builder& scaleKeys(std::vector<ScaleKey>&& scale_keys);

        Bone build();

    private:
        std::string                 _name;
        uint32_t                    _id;
        std::vector<PositionKey>    _position_keys;
        std::vector<RotationKey>    _rotation_keys;
        std::vector<ScaleKey>       _scale_keys;
    };

    class Animator
    {
        void calculateBoneTransform(
            AnimationHierarchiry::Node& node, 
            const glm::mat4&            parent_transform
        );

        explicit Animator(
            std::vector<Bone>&&             bones, 
            BoneRegistry&&                  bone_registry,
            AnimationHierarchiry::Node&&    root_node,
            float                           duration, 
            float                           ticks_per_second
        );

    public:
        class Builder;

    public:
        Animator(Animator&& animator)       = default;
        Animator(const Animator& animator)  = delete;

        Animator& operator = (Animator&& animator)      = default;
        Animator& operator = (const Animator& animator) = delete;

        void update(float delta_time);

        std::span<const glm::mat4> getFinalBoneMatrices() const;

    private:
        std::vector<Bone>       _bones;
        BoneRegistry            _bone_registry;
        std::vector<glm::mat4>  _final_bones_matrices;

        AnimationHierarchiry::Node _root_node;
        
        float _duration           = 0.0f;
        float _current_time       = 0.0f;
        float _ticks_per_second   = 0.0f;
    };

    class Animator::Builder
    {
        void validate() const;

    public:
        Builder() = default;

        Builder(Builder&& builder)          = delete;
        Builder(const Builder&& builder)    = delete;

        Builder& operator = (Builder&& builder)         = delete;
        Builder& operator = (const Builder& builder)    = delete;

        Builder& bones(std::vector<Bone>&& bones);
        Builder& boneRegistry(BoneRegistry&& bone_registry);
        Builder& animationHierarchiryRootNode(AnimationHierarchiry::Node&& root_node);
        Builder& time(float duration, float ticks_per_second);

        Animator build();

    private:
        std::vector<Bone>           _bones;
        BoneRegistry                _bone_registry;
        AnimationHierarchiry::Node  _root_node;
        float                       _duration; 
        float                       _ticks_per_second;
    };
}