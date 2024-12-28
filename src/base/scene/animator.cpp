#include <base/scene/animator.hpp>
#include <base/logger/logger.hpp>

#include <algorithm>

namespace vrts
{
    Bone::Bone(
        std::string_view        name,
        uint32_t                id,
        BoneTransformTrack&&    bone_transform_track
    ) :
        _name       (name),
        _id         (id),
        _sampler    (std::move(bone_transform_track))
    { }

    void Bone::update(float time)
    {
        _transform = _sampler.getTransform(time);
    }

    std::string_view Bone::getName() const
    {
        return _name;
    }

    const glm::mat4& Bone::getTransform() const noexcept
    {
        return _transform;
    }
}

namespace vrts
{
    Bone::Builder& Bone::Builder::name(std::string_view name)
    {
        _name = name;
        return *this;
    }

    Bone::Builder& Bone::Builder::id(uint32_t bone_id)
    {
        _id = bone_id;
        return *this;
    }

    Bone::Builder& Bone::Builder::positionKeys(std::vector<PositionKey>&& position_keys)
    {
        std::swap(_bone_transform_track.position_keys, position_keys);
        return *this;
    }

    Bone::Builder& Bone::Builder::rotationKeys(std::vector<RotationKey>&& rotation_keys)
    {
        std::swap(_bone_transform_track.rotation_keys, rotation_keys);
        return *this;
    }
    
    Bone::Builder& Bone::Builder::scaleKeys(std::vector<ScaleKey>&& scale_keys)
    {
        std::swap(_bone_transform_track.scale_keys, scale_keys);
        return *this;
    }

    void Bone::Builder::validate() const
    {
        if (_bone_transform_track.position_keys.empty())
            log::error("[Bone::Builder] Position keys is empty");

        if (_bone_transform_track.rotation_keys.empty())
            log::error("[Bone::Builder] Rotation keys is empty");
        
        if (_bone_transform_track.scale_keys.empty())
            log::error("[Bone::Builder] Scale keys is empty");
    }

    Bone Bone::Builder::build()
    {
        validate();
        return Bone(_name, _id, std::move(_bone_transform_track));
    }
}

namespace vrts
{
    std::optional<BoneInfo> BoneRegistry::get(std::string_view name) const
    {
        if (auto res = _bones.find(name); res != std::end(_bones))
            return res->second;
        
        return std::nullopt;
    }

    size_t BoneRegistry::boneCount() const
    {
        return _bones.size();
    }

    void BoneRegistry::add(std::string_view name, const BoneInfo& bone)
    {
        _bones.insert(std::make_pair(name, bone));
    }
}

namespace vrts
{
    AnimationSampler::AnimationSampler(BoneTransformTrack&& track) :
        _track (std::move(track))
    { }

    float AnimationSampler::getScaleFactor(float t1, float t2, float t)
    {
        if (constexpr auto eps = 0.0001f; std::abs(t1 - t2) < eps) 
            log::error("[Animator::Bone] Failed calculate scale factor because t1 and t2 is equal");
        
        return glm::clamp((t - t1) / (t2 - t1), 0.0f, 1.0f);
    }

    glm::mat4 AnimationSampler::getKeyPosition(float time) const
    {
        if (_track.position_keys.size() == 1)
            return glm::translate(glm::mat4(1.0f), _track.position_keys[0].pos);

        const auto index0 = getIndex(_track.position_keys, time);
        const auto index1 = index0 + 1;

        if (index1 == _track.position_keys.size())
            return glm::translate(glm::mat4(1.0f), _track.position_keys[0].pos);

        const auto t0 = _track.position_keys[index0].time_stamp;
        const auto t1 = _track.position_keys[index1].time_stamp;

        const auto pos0 = _track.position_keys[index0].pos;
        const auto pos1 = _track.position_keys[index1].pos;

        return glm::translate(glm::mat4(1.0f), glm::mix(pos0, pos1, getScaleFactor(t0, t1, time)));
    }

    glm::mat4 AnimationSampler::getKeyRotation(float time) const
    {
        if (_track.rotation_keys.size() == 1)
            return glm::mat4_cast(_track.rotation_keys[0].rotate);

        const auto index0 = getIndex(_track.rotation_keys, time);
        const auto index1 = index0 + 1;

        if (index1 == _track.rotation_keys.size())
            return glm::mat4_cast(_track.rotation_keys[0].rotate);

        const auto t0 = _track.rotation_keys[index0].time_stamp;
        const auto t1 = _track.rotation_keys[index1].time_stamp;

        const auto rot0 = _track.rotation_keys[index0].rotate;
        const auto rot1 = _track.rotation_keys[index1].rotate;

        return glm::mat4(glm::normalize(glm::slerp(rot0, rot1, getScaleFactor(t0, t1, time))));
    }

    glm::mat4 AnimationSampler::getKeyScale(float time) const
    {
        if (_track.scale_keys.size() == 1)
            return glm::scale(glm::mat4(1.0f), _track.scale_keys[0].scale);

        const auto index0 = getIndex(_track.scale_keys, time);
        const auto index1 = index0 + 1;

        if (index1 == _track.scale_keys.size())
            return glm::scale(glm::mat4(1.0f), _track.scale_keys[0].scale);

        const auto t0 = _track.scale_keys[index0].time_stamp;
        const auto t1 = _track.scale_keys[index1].time_stamp;

        const auto scale0 = _track.scale_keys[index0].scale;
        const auto scale1 = _track.scale_keys[index1].scale;

        return glm::scale(glm::mat4(1.0f), glm::mix(scale0, scale1, getScaleFactor(t0, t1, time)));
    }

    glm::mat4 AnimationSampler::getTransform(float time) const
    {
        const auto translation  = getKeyPosition(time);
        const auto rotation     = getKeyRotation(time);
        const auto scale        = getKeyScale(time);

        return translation * rotation * scale;
    }
}

namespace vrts
{
    void Animator::update(float delta_time)
    {
        _current_time += _ticks_per_second * delta_time;
        _current_time = std::fmod(_current_time, _duration);

        calculateBoneTransform(_root_node, glm::mat4(1.0f));
    }

    std::span<const glm::mat4> Animator::getFinalBoneMatrices() const
    {
        return _final_bones_matrices;
    }

    void Animator::calculateBoneTransform(const AnimationHierarchiry::Node& node, const glm::mat4& parent_transform)
    {
        auto node_transform     = node.transform;
        const auto& node_name   = node.name;

        const auto bone = std::find_if(std::begin(_bones), std::end(_bones), [&node_name] (const Bone& bone)
        {
            return node_name == bone.getName();
        });

        if (bone != std::end(_bones)) 
        {
            bone->update(_current_time);
            node_transform = bone->getTransform();
        }

        const auto transform = parent_transform * node_transform;

        if (const auto bone = _bone_registry.get(node_name); bone)
        _final_bones_matrices[bone->id] = transform * bone->offset;

        for (const auto& child: node.children)
            calculateBoneTransform(child, transform);
    }
}

namespace vrts
{
    Animator::Builder& Animator::Builder::bones(std::vector<Bone>&& bones)
    {
        std::swap(_bones, bones);
        return *this;
    }

    Animator::Builder& Animator::Builder::boneRegistry(BoneRegistry&& bone_registry)
    {
        std::swap(_bone_registry, bone_registry);
        return *this;
    }

    Animator::Builder& Animator::Builder::animationHierarchiryRootNode(AnimationHierarchiry::Node&& root_node)
    {
        std::swap(_root_node, root_node);
        return *this;
    }

    Animator::Builder& Animator::Builder::time(float duration, float ticks_per_second)
    {
        _duration = duration;
        _ticks_per_second = ticks_per_second;
        return *this;
    }

    void Animator::Builder::validate() const
    {
        if (_bones.empty()) 
            log::error("[Animator::Builder] Haven't bones");

        constexpr auto eps = 0.001f;

        if (_duration < eps)
            log::error("[Animator::Builder] Duration is invalid: {}", _duration);

        if (_ticks_per_second < eps)
            log::error("[Animator::Builder] Tikcs per second is invalid: {}", _ticks_per_second);
    }

    Animator Animator::Builder::build()
    {
        validate();

        Animator animator;
        animator._bones                     = std::move(_bones);
        animator._bone_registry             = std::move(_bone_registry);
        animator._root_node                 = std::move(_root_node);
        animator._duration                  = _duration;
        animator._ticks_per_second          = _ticks_per_second;

        animator._final_bones_matrices.resize(animator._bone_registry.boneCount(), glm::mat4(1.0f));

        return animator;
    }
}