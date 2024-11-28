#include <base/scene/animator.hpp>
#include <base/logger/logger.hpp>

namespace sample_vk
{
    Bone::Bone(
        std::string_view            name,
        uint32_t                    id,
        std::vector<PositionKey>&&  position_keys,
        std::vector<RotationKey>&&  rotation_keys,
        std::vector<ScaleKey>&&     scale_keys
    ) :
        _name           (name),
        _id             (id),
        _position_keys  (std::move(position_keys)),
        _rotation_keys  (std::move(rotation_keys)),
        _scale_keys     (std::move(scale_keys))
    { }

    float Bone::getScaleFactor(float t1, float t2, float t)
    {
        if (constexpr auto eps = 0.0001; std::abs(t1 - t2) < eps) 
            log::appError("[Animator::Bone] Failed calculate scale factor because t1 and t2 is equal");
        
        return (t - t1) / (t2 - t1);
    }

    size_t Bone::getPositionIndex(float time) const
    {
        for (size_t i = 0; i < _position_keys.size() - 1; ++i)
        {
            if (_position_keys[i + 1].time_stamp > time)
                return i;
        }

        return _position_keys.size() - 1;
    }

    size_t Bone::getRotationIndex(float time) const
    {
        for (size_t i = 0; i < _rotation_keys.size() - 1; ++i)
        {
            if (_rotation_keys[i + 1].time_stamp > time)
                return i;
        }

        return _rotation_keys.size() - 1;
    }

    size_t Bone::getScaleIndex(float time) const
    {
        for (size_t i = 0; i < _scale_keys.size() - 1; ++i)
        {
            if (_scale_keys[i + 1].time_stamp > time)
                return i;
        }

        return _scale_keys.size() - 1;
    }

    glm::mat4 Bone::getKeyPosition(float time) const
    {
        if (_position_keys.size() == 1)
            return glm::translate(glm::mat4(1.0f), _position_keys[0].pos);

        auto index0 = getPositionIndex(time);
        auto index1 = index0 + 1;

        if (index1 == _position_keys.size())
            return glm::translate(glm::mat4(1.0f), _position_keys[0].pos);

        auto t0 = _position_keys[index0].time_stamp;
        auto t1 = _position_keys[index1].time_stamp;

        auto pos0 = _position_keys[index0].pos;
        auto pos1 = _position_keys[index1].pos;

        return glm::translate(glm::mat4(1.0f), glm::mix(pos0, pos1, getScaleFactor(t0, t1, time)));
    }

    glm::mat4 Bone::getKeyRotation(float time) const
    {
        if (_rotation_keys.size() == 1)
            return glm::mat4_cast(_rotation_keys[0].rotate);

        auto index0 = getRotationIndex(time);
        auto index1 = index0 + 1;

        if (index1 == _rotation_keys.size())
            return glm::mat4_cast(_rotation_keys[0].rotate);

        auto t0 = _rotation_keys[index0].time_stamp;
        auto t1 = _rotation_keys[index1].time_stamp;

        auto rot0 = _rotation_keys[index0].rotate;
        auto rot1 = _rotation_keys[index1].rotate;

        return glm::mat4_cast(glm::slerp(rot0, rot1, getScaleFactor(t0, t1, time)));
    }

    glm::mat4 Bone::getKeyScale(float time) const
    {
        if (_scale_keys.size() == 1)
            return glm::scale(glm::mat4(1.0f), _scale_keys[0].scale);

        auto index0 = getScaleIndex(time);
        auto index1 = index0 + 1;

        if (index1 == _scale_keys.size())
            return glm::scale(glm::mat4(1.0f), _scale_keys[0].scale);

        auto t0 = _scale_keys[index0].time_stamp;
        auto t1 = _scale_keys[index1].time_stamp;

        auto scale0 = _scale_keys[index0].scale;
        auto scale1 = _scale_keys[index1].scale;

        return glm::scale(glm::mat4(1.0f), glm::mix(scale0, scale1, getScaleFactor(t0, t1, time)));
    }

    void Bone::update(float time)
    {
        auto translation    = getKeyPosition(time);
        auto rotation       = getKeyRotation(time);
        auto scale          = getKeyScale(time);

        _local_transform = translation * rotation * scale;
    }

    const std::string& Bone::getName() const noexcept
    {
        return _name;
    }

    const glm::mat4& Bone::getLocalTransform() const noexcept
    {
        return _local_transform;
    }
}

namespace sample_vk
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
        std::swap(_position_keys, position_keys);
        return *this;
    }

    Bone::Builder& Bone::Builder::rotationKeys(std::vector<RotationKey>&& rotation_keys)
    {
        std::swap(_rotation_keys, rotation_keys);
        return *this;
    }
    
    Bone::Builder& Bone::Builder::scaleKeys(std::vector<ScaleKey>&& scale_keys)
    {
        std::swap(_scale_keys, scale_keys);
        return *this;
    }

    void Bone::Builder::validate() const
    {
        if (_position_keys.empty())
            log::appError("[Bone::Builder] Position keys is empty");

        if (_rotation_keys.empty())
            log::appError("[Bone::Builder] Rotation keys is empty");
        
        if (_scale_keys.empty())
            log::appError("[Bone::Builder] Scale keys is empty");
    }

    Bone Bone::Builder::build()
    {
        validate();
        return Bone(_name, _id, std::move(_position_keys), std::move(_rotation_keys), std::move(_scale_keys));
    }
}

namespace sample_vk
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

namespace sample_vk
{
    Animator::Animator
    (
        std::vector<Bone>&&             bones,
        BoneRegistry&&                  bone_registry,
        AnimationHierarchiry::Node&&    root_node, 
        float                           duration, 
        float                           ticks_per_second
    ) :
        _bones              (std::move(bones)),
        _bone_registry      (std::move(_bone_registry)),
        _root_node          (std::move(root_node)),
        _duration           (duration),
        _ticks_per_second   (ticks_per_second)
    {
        _final_bone_matrices.resize(_bone_registry.boneCount());
    }

    void Animator::update(float delta_time)
    {
        _current_time += _ticks_per_second * delta_time;
        _current_time = std::fmod(_current_time, _duration);

        calculateBoneTransform(_root_node, glm::mat4(1.0f));
    }

    void Animator::calculateBoneTransform(AnimationHierarchiry::Node& node, const glm::mat4& parent_transform)
    {
        auto node_name      = node.name;
        auto node_transform = node.transform;

        auto bone = std::find_if(std::begin(_bones), std::end(_bones), [&node_name] (const Bone& bone)
        {
            return node_name == bone.getName();
        });

        if (bone != std::end(_bones)) 
        {
            bone->update(_current_time);
            node_transform = bone->getLocalTransform();
        }

        auto global_transform = parent_transform * node_transform;

        if (auto bone = _bone_registry.get(node_name); bone)
            _final_bone_matrices[bone->bone_id] = global_transform * bone->offset;

        for (auto& child: node.children)
            calculateBoneTransform(child, global_transform);
    }
}

namespace sample_vk
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
            log::appError("[Animator::Builder] Haven't bones");

        constexpr auto eps = 0.001f;

        if (_duration < eps)
            log::appError("[Animator::Builder] Duration is invalid: {}", _duration);

        if (_ticks_per_second < eps)
            log::appError("[Animator::Builder] Tikcs per second is invalid: {}", _ticks_per_second);
    }

    Animator Animator::Builder::build()
    {
        validate();
        return Animator(std::move(_bones), std::move(_bone_registry), std::move(_root_node), _duration, _ticks_per_second);
    }
}