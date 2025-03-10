#include <base/scene/scene.hpp>
#include <base/logger/logger.hpp>
#include <base/scene/material_manager.hpp>
#include <base/scene/node.hpp>

#include <base/math.hpp>

#include <utility>
#include <format>
#include <algorithm>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

#include <base/private/stb.hpp>

#include <ranges>

#include <cstddef> 

namespace vrts::utils
{
    static constexpr auto buffer_usage_flags = 
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT 
            |   VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT 
            |   VK_BUFFER_USAGE_TRANSFER_DST_BIT 
            |   VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;

    glm::mat4 cast(const aiMatrix4x4& matrix)
    {
        return glm::mat4(
            glm::vec4(matrix.a1, matrix.a2, matrix.a3, matrix.a4),
            glm::vec4(matrix.b1, matrix.b2, matrix.b3, matrix.b4),
            glm::vec4(matrix.c1, matrix.c2, matrix.c3, matrix.c4),
            glm::vec4(matrix.d1, matrix.d2, matrix.d3, matrix.d4)
        );
    }

    glm::vec4 cast(const aiVector2D& vec)
    {
        return glm::vec4(vec.x, vec.y, 0.0f, 0.0f);
    }

    glm::vec4 cast(const aiVector3D& vec)
    {
        return glm::vec4(vec.x, vec.y, vec.z, 0.0f);
    }

    glm::vec4 cast(const aiColor3D& col)
    {
        return glm::vec4(col.r, col.g, col.b, 0.0f);
    }

    glm::quat cast(const aiQuaternion& quat)
    {
        return glm::quat(quat.w, quat.x, quat.y, quat.z);
    }

    template<typename T>
    Buffer createBuffer(
        const Context*          ptr_context,
        const std::string_view  name,
        std::span<T>            data,
        VkBufferUsageFlags      usage_flags
    )
    {
        assert(usage_flags & VK_BUFFER_USAGE_TRANSFER_DST_BIT);

        auto buffer = Buffer::Builder(ptr_context)
            .vkSize(data.size_bytes())
            .vkUsage(usage_flags)
            .name(name)
            .build();

        Buffer::writeData(buffer,data);

        return buffer;
    }
    
    Buffer createBuffer(
        const Context*          ptr_context,
        const std::string_view  name,
        VkDeviceSize            size,
        VkBufferUsageFlags      usage_flags
    )
    {
        assert(usage_flags & VK_BUFFER_USAGE_TRANSFER_DST_BIT);

        return Buffer::Builder(ptr_context)
            .vkUsage(usage_flags)
            .vkSize(size)
            .name(name)
            .build();
    }
}

namespace vrts
{
    class ScopedTransform
    {
    public:
        explicit ScopedTransform(Scene::Importer* ptr_importer, const glm::mat4 transform) :
            _ptr_importer   (ptr_importer),
            _prev_transform (ptr_importer->_current_state.transform)
        {
            _ptr_importer->_current_state.transform *= transform;
        }

        ~ScopedTransform()
        {
            _ptr_importer->_current_state.transform = _prev_transform;
        }

    private:
        Scene::Importer*    _ptr_importer;
        glm::mat4           _prev_transform;
    };
}

namespace vrts
{
    Scene::Scene(
        Model&&                     model, 
        std::vector<Light>&&        lights, 
        const Camera&               camera,
        std::optional<Animator>&&   animator
    ) :
        _model              (std::move(model)),
        _lights             (std::move(lights)),
        _camera_controller  (camera)
    {
        std::swap(_animator, animator);
    }

    Model& Scene::getModel() noexcept
    {
        return _model;
    }
    
    const Model& Scene::getModel() const noexcept
    {
        return _model;
    }

    CameraController& Scene::getCameraController() noexcept
    {
        return _camera_controller;
    }

    const std::vector<Light>& Scene::getLights() const noexcept
    {
        return _lights;
    }

    void Scene::processEvent(const SDL_Event* ptr_event)
    {
        _camera_controller.process(ptr_event);
    }

    void Scene::updateCamera()
    {
        _camera_controller.update();
    }

    void Scene::applyTransform(const glm::mat4& transform)
    {
        _model.applyTransform(transform);

        for (auto& light: _lights)
            light.pos = transform * glm::vec4(light.pos, 1.0);
    }
}

namespace vrts
{
    void Scene::addRect(
        const Context*      ptr_context,
        const glm::vec3&    color,
        const glm::vec3&    emissive, 
        const glm::mat4&    transform
    )
    {
        _model.addNode(Scene::makeRectNode(ptr_context, glm::transpose(transform)));

        static uint32_t rect_material_id = 0;
        ++rect_material_id;

        auto material_name = std::format("Rect material #{}", rect_material_id);

        auto albedo_image = Image::Builder(ptr_context)
            .fillColor(color)
            .generateMipmap(false)
            .size(1, 1)
            .vkFormat(VK_FORMAT_R8G8B8A8_UNORM)
            .vkFilter(VK_FILTER_NEAREST)
            .build();

        auto emissive_image = Image::Builder(ptr_context)
            .fillColor(emissive)
            .generateMipmap(false)
            .size(1, 1)
            .vkFormat(VK_FORMAT_R16G16B16A16_SFLOAT)
            .vkFilter(VK_FILTER_NEAREST)
            .build();

        auto metallic_image = Image::Builder(ptr_context)
            .fillColor(glm::vec3(0))
            .generateMipmap(false)
            .size(1, 1)
            .vkFormat(VK_FORMAT_R8_UNORM)
            .vkFilter(VK_FILTER_NEAREST)
            .build();

        auto roughness_image = Image::Builder(ptr_context)
            .fillColor(glm::vec3(1))
            .generateMipmap(false)
            .size(1, 1)
            .vkFormat(VK_FORMAT_R8_UNORM)
            .vkFilter(VK_FILTER_NEAREST)
            .build();

        auto normal_map_image = Image::Builder(ptr_context)
            .fillColor(glm::vec3(0, 0, 1))
            .generateMipmap(false)
            .size(1, 1)
            .vkFormat(VK_FORMAT_R8G8B8A8_UNORM)
            .vkFilter(VK_FILTER_NEAREST)
            .build();

        VkUtils::setName(ptr_context->device_handle, albedo_image, VK_OBJECT_TYPE_IMAGE, std::format("{}: albedo", material_name));
        VkUtils::setName(ptr_context->device_handle, emissive_image, VK_OBJECT_TYPE_IMAGE, std::format("{}: emissive", material_name));
        VkUtils::setName(ptr_context->device_handle, metallic_image, VK_OBJECT_TYPE_IMAGE, std::format("{}: metallic", material_name));
        VkUtils::setName(ptr_context->device_handle, roughness_image, VK_OBJECT_TYPE_IMAGE, std::format("{}: roughness", material_name));
        VkUtils::setName(ptr_context->device_handle, normal_map_image, VK_OBJECT_TYPE_IMAGE, std::format("{}: normal_map", material_name));

        _model.getMaterialManager().add(Material
        {
            std::move(albedo_image),
            std::move(normal_map_image),
            std::move(metallic_image),
            std::move(roughness_image),
            std::move(emissive_image) 
        });
    }

    std::unique_ptr<MeshNode> Scene::makeRectNode(
        const Context*      ptr_context,
        const glm::mat4&    transform
    )
    {
        static uint32_t rect_id = 0;
        ++rect_id;

        const std::array positions = 
        {
            glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
            glm::vec4(1.0f, 0.0f, 0.0f, 1.0f),
            glm::vec4(0.0f, 1.0f, 0.0f, 1.0f),
            glm::vec4(1.0f, 1.0f, 0.0f, 1.0f)
        };

        const std::array uvs = 
        {
            glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
            glm::vec4(1.0f, 0.0f, 0.0f, 0.0f),
            glm::vec4(0.0f, 1.0f, 0.0f, 0.0f),
            glm::vec4(1.0f, 0.0f, 0.0f, 0.0f)
        };

        const glm::vec4 normal  (0, 0, 1, 0);
        const glm::vec4 tangent (1, 0, 0, 0);

        std::vector<Attributes> attributes;

        Attributes attr = { };
        attr.normal     = normal;
        attr.tangent    = tangent;

        for (auto i: std::views::iota(0, 4))
        {
            attr.pos    = positions[i];
            attr.uv     = uvs[i];

            attributes.push_back(attr);
        }

        auto name = std::format("rect #{}", rect_id);

        std::array indices = {0u, 1u, 2u, 1u, 3u, 2u};

        Mesh mesh;

        mesh.index_count    = indices.size();
        mesh.vertex_count   = attributes.size();

        mesh.index_buffer = utils::createBuffer
        (
            ptr_context,
            std::format("[Index buffer]: {}", name), 
            std::span<uint32_t>(indices), 
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT | utils::buffer_usage_flags
        );

        mesh.vertex_buffer = utils::createBuffer
        (
            ptr_context,
            std::format("[Vertex buffer]: {}", name), 
            std::span(attributes), 
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | utils::buffer_usage_flags
        );

        return std::make_unique<MeshNode>(name, transform, std::move(mesh));
    }

    bool Scene::hasAnimator() const
    {
        return _animator != std::nullopt;
    }

    Animator& Scene::getAnimator()
    {
        return _animator.value();
    }
}

namespace vrts
{
    Scene::Importer& Scene::Importer::path(const std::filesystem::path path)
    {
        _path = path;
        return *this;
    }

    Scene::Importer& Scene::Importer::vkMemoryTypeIndex(uint32_t memory_type_index) noexcept
    {
        _memory_type_index = memory_type_index;
        return *this;
    }

    Scene::Importer& Scene::Importer::viewport(uint32_t width, uint32_t height) noexcept
    {
        _width  = width;
        _height = height;
        return *this;
    }

    void Scene::Importer::validate() const
    {
        if (!_ptr_context)
            log::error("[Scene::Importer] Not driver.");

        if (!std::filesystem::exists(_path))
            log::error("[Scene::Importer] Not find file: {}.", _path.string());
        
        if (!_width || !_height)
            log::error("[Scene::Importer] Viewport can't be with empty sizes.");
    }

    static auto getVertexAndIndexCount(const aiScene* ptr_scene, const aiNode* ptr_node)
        -> std::pair<size_t, size_t>
    {
        constexpr size_t num_indices_per_face = 3;

        size_t index_count  = 0;
        size_t vertex_count = 0;

        for (auto i: std::views::iota(0u, ptr_node->mNumMeshes))
        {
            const auto ptr_mesh = ptr_scene->mMeshes[ptr_node->mMeshes[i]];

            if (!ptr_mesh->HasFaces() || !ptr_mesh->HasPositions() || !ptr_mesh->HasTextureCoords(0))
                continue;

            vertex_count    += ptr_mesh->mNumVertices;
            index_count     += ptr_mesh->mNumFaces * num_indices_per_face;
        }

        return std::make_pair(index_count, vertex_count);
    }

    Scene::Importer::Importer(const Context* ptr_context) noexcept :
        _ptr_context (ptr_context)
    { }

    Mesh Scene::Importer::createMesh(
        const std::string_view      name,
        const std::span<uint32_t>   indices,
        const std::span<Attributes> attributes
    ) const
    {
        Mesh mesh;

        mesh.index_count    = indices.size();
        mesh.vertex_count   = attributes.size();

        mesh.index_buffer = utils::createBuffer
        (
            _ptr_context,
            std::format("[Index buffer]: {}", name), 
            indices, 
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT | utils::buffer_usage_flags
        );

        mesh.vertex_buffer = utils::createBuffer
        (
            _ptr_context,
            std::format("[Vertex buffer]: {}", name), 
            attributes, 
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | utils::buffer_usage_flags
        );

        return mesh;
    }

    SkinnedMesh Scene::Importer::createMesh(
        const std::string_view          name,
        const std::span<uint32_t>       indices,
        const std::span<Attributes>     attributes,
        const std::span<SkinningData>   skinning_data
    ) const
    {
        static int skinned_mesh_index = 0;
        ++skinned_mesh_index;

        SkinnedMesh mesh;

        mesh.index_count    = indices.size();
        mesh.vertex_count   = attributes.size();

        mesh.index_buffer = utils::createBuffer
        (
            _ptr_context,
            std::format("[SkinnedMesh][Index buffer]: {}", name), 
            indices, 
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT | utils::buffer_usage_flags
        );

        mesh.source_vertex_buffer = utils::createBuffer
        (
            _ptr_context,
            std::format("[SkinnedMesh][Source vertex buffer]: {}", name), 
            attributes, 
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | utils::buffer_usage_flags
        );

        mesh.skinning_buffer = utils::createBuffer
        (
            _ptr_context,
            std::format("[SkinnedMesh][Skinning buffer]: {}", name), 
            skinning_data, 
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | utils::buffer_usage_flags 
        );

        mesh.processed_vertex_buffer = utils::createBuffer
        (
            _ptr_context,
            std::format("[SkinnedMesh][Processed vertex buffer]: {}", skinned_mesh_index),
            static_cast<VkDeviceSize>(attributes.size() * sizeof(Attributes)),
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | utils::buffer_usage_flags
        );

        return mesh;
    }

    void Scene::Importer::add(
        const std::string_view          name, 
        const std::span<uint32_t>       indices, 
        const std::span<Attributes>     attributes,
        const std::span<SkinningData>   skinning_data
    )
    {
        if (skinning_data.empty())
        {
            _ptr_root_node->children.push_back(
                std::make_unique<MeshNode>(
                    name, 
                    _current_state.transform, 
                    createMesh(name, indices, attributes)
                )
            );
        }
        else 
        {
            _ptr_root_node->children.push_back(
                std::make_unique<SkinnedMeshNode>(
                    name, 
                    _current_state.transform, 
                    createMesh(name, indices, attributes, skinning_data)
                )
            );
        }
    }

    void Scene::Importer::add(const aiLight* ptr_light)
    {
        log::info("[Scene::Importer]\t - Process light: {}", ptr_light->mName.C_Str());

        glm::vec3 pos (
            _current_state.transform[0][3],
            _current_state.transform[1][3],
            _current_state.transform[2][3]
        );

        _processed_lights.push_back(Light 
            {
                pos,
                utils::cast(ptr_light->mColorDiffuse)
            }
        );
    }

    Image Scene::Importer::getImage(const aiTexture* ptr_texture, VkFilter filter_for_mipmap, int32_t channels_per_pixel) const
    {
        auto ptr_compressed_image   = reinterpret_cast<uint8_t*>(ptr_texture->pcData);
        auto compressed_image_size  = ptr_texture->mHeight == 0 ? 
            ptr_texture->mWidth : 
            ptr_texture->mWidth * ptr_texture->mHeight;

        return Image::Decoder(_ptr_context)
            .compressedImage(ptr_compressed_image, compressed_image_size)
            .vkFilter(filter_for_mipmap)
            .channelsPerPixel(channels_per_pixel)
            .decode();
    }

    Image Scene::Importer::getDefaultTexture(int32_t channels_per_pixel)
    {
        /// @todo Сделать четыре статические текстуры и просто возвращать их тут.

        constexpr std::array formats
        {
            VK_FORMAT_R8_UNORM,
            VK_FORMAT_R8G8_UNORM,
            VK_FORMAT_R8G8B8_UNORM,
            VK_FORMAT_R8G8B8A8_UNORM
        };

        auto image = Image::Builder(_ptr_context)
            .size(1, 1)
            .vkFormat(formats[channels_per_pixel - 1])
            .vkFilter(VK_FILTER_NEAREST)
            .build();

        std::vector<uint8_t> image_row_data (channels_per_pixel);

        memset(image_row_data.data(), 0, channels_per_pixel);

        ImageWriteData write_data;
        write_data.height   = 1;
        write_data.width    = 1;
        write_data.format   = image.format;
        write_data.ptr_data = image_row_data.data();

        image.writeData(write_data);

        return image;
    }

    Image Scene::Importer::getTexture(
        const aiScene*      ptr_scene, 
        const aiMaterial*   ptr_material, 
        aiTextureType       texuture_type, 
        int32_t             channels_per_pixel,
        VkFilter            filter
    )
    {
        aiString texture_name;
        if (ptr_material->Get(AI_MATKEY_TEXTURE(texuture_type, 0), texture_name) != aiReturn_SUCCESS)
            return getDefaultTexture(channels_per_pixel);

        if (texture_name.C_Str()[0] != '*')
            return getDefaultTexture(channels_per_pixel);

        auto ptr_texture = ptr_scene->mTextures[std::stoi(texture_name.C_Str() + 1)];

        if (!ptr_texture)
            return getDefaultTexture(channels_per_pixel);

        return getImage(ptr_texture, filter, channels_per_pixel);
    }

    void Scene::Importer::processMaterial(const aiScene* ptr_scene, const aiMaterial* ptr_material)
    {
        Material material;

        material.albedo     = getTexture(ptr_scene, ptr_material, aiTextureType_DIFFUSE, 4, VK_FILTER_LINEAR);
        material.normal_map = getTexture(ptr_scene, ptr_material, aiTextureType_NORMALS, 4, VK_FILTER_NEAREST);
        material.metallic   = getTexture(ptr_scene, ptr_material, aiTextureType_METALNESS, 1, VK_FILTER_LINEAR);
        material.roughness  = getTexture(ptr_scene, ptr_material, aiTextureType_DIFFUSE_ROUGHNESS, 1, VK_FILTER_LINEAR);
        material.emissive   = getTexture(ptr_scene, ptr_material, aiTextureType_EMISSIVE, 4, VK_FILTER_LINEAR);

        std::string material_name = ptr_material->GetName().C_Str();

        VkUtils::setName(_ptr_context->device_handle, material.albedo.value(), VK_OBJECT_TYPE_IMAGE, material_name + "_albedo");
        VkUtils::setName(_ptr_context->device_handle, material.normal_map.value(), VK_OBJECT_TYPE_IMAGE, material_name + "_normal_map");
        VkUtils::setName(_ptr_context->device_handle, material.metallic.value(), VK_OBJECT_TYPE_IMAGE, material_name + "_metallic");
        VkUtils::setName(_ptr_context->device_handle, material.roughness.value(), VK_OBJECT_TYPE_IMAGE, material_name + "_roughness");
        VkUtils::setName(_ptr_context->device_handle, material.emissive.value(), VK_OBJECT_TYPE_IMAGE, material_name + "_emissive");

        _material_manager.add(std::move(material));
    }

    size_t Scene::Importer::getMeshCount() const
    {
        return _ptr_root_node->children.size();
    }

    void Scene::Importer::processAnimation(const aiMesh* ptr_mesh, std::span<SkinningData> skinning_data)
    {
        const std::span bones (ptr_mesh->mBones, ptr_mesh->mNumBones);
        for (const auto& bone: bones)
        {
            auto        bone_id     = std::numeric_limits<uint32_t>::infinity();
            std::string bone_name   = bone->mName.C_Str();

            if (auto info = _animation.bone_infos.get(bone_name); info)
                bone_id = info->id;
            else
            {
                BoneInfo bone_info;
                bone_id             = static_cast<uint32_t>(_animation.bone_infos.boneCount());
                bone_info.id        = bone_id;
                bone_info.offset    = glm::transpose(utils::cast(bone->mOffsetMatrix));

                _animation.bone_infos.add(bone_name, bone_info);
            }

            const std::span weights (bone->mWeights, bone->mNumWeights);
            for (const auto& weight_info: weights)
            {
                auto vertex_id  = weight_info.mVertexId;
                auto weight     = weight_info.mWeight;

                if (vertex_id >= ptr_mesh->mNumVertices)
                {
                    log::warning("[Scene::Importer] failed load bone weight: {} >= {}", vertex_id, ptr_mesh->mNumVertices);
                    continue;
                }

                auto& vert_skin_data = skinning_data[vertex_id];
                for (auto k: std::views::iota(0u, 4u))
                {
                    if (vert_skin_data.bone_ids[k] > 0 || vert_skin_data.weights[k] > 0.0)
                        continue;

                    vert_skin_data.bone_ids[k]  = bone_id;
                    vert_skin_data.weights[k]   = weight;
                    break;
                }
            }
        }

        log::info("[Scene::Importer]\t\t - Bone count: {}", _animation.bone_infos.boneCount());
    }

    void Scene::Importer::processNode(const aiScene* ptr_scene, const aiNode* ptr_node)
    {
        log::info("[Scene::Importer]\t - Process node: {}", ptr_node->mName.C_Str());

        ScopedTransform scoped_transform (this, utils::cast(ptr_node->mTransformation));

        if (auto light = _scene_lights.find(ptr_node->mName.C_Str()); light != _scene_lights.end())
            add(light->second);
       
        auto [index_count, vertex_count] = getVertexAndIndexCount(ptr_scene, ptr_node);

        if (index_count && vertex_count)
        {
            log::info("[Scene::Importer]\t\t - Index count: {}", index_count);
            log::info("[Scene::Importer]\t\t - Vertex count: {}", vertex_count);

            std::vector<uint32_t>       indices;
            std::vector<Attributes>     attributes;
            std::vector<SkinningData>   skinning_data;

            for (auto i: std::views::iota(0u, ptr_node->mNumMeshes))
            {
                const auto ptr_mesh = ptr_scene->mMeshes[ptr_node->mMeshes[i]];

                if (!ptr_mesh->HasFaces() || !ptr_mesh->HasPositions() || !ptr_mesh->HasTextureCoords(0))
                    continue;

                processMaterial(ptr_scene, ptr_scene->mMaterials[ptr_mesh->mMaterialIndex]);

                for (auto j: std::views::iota(0u, ptr_mesh->mNumVertices))
                {
                    Attributes attr;
                    attr.pos        = utils::cast(ptr_mesh->mVertices[j]);
                    attr.normal     = utils::cast(ptr_mesh->mNormals[j]);
                    attr.tangent    = utils::cast(ptr_mesh->mTangents[j]);
                    attr.uv         = utils::cast(ptr_mesh->mTextureCoords[0][j]);

                    attributes.push_back(attr);
                }

                const auto ptr_faces = ptr_mesh->mFaces;
                for (auto j: std::views::iota(0u, ptr_mesh->mNumFaces))
                {
                    indices.push_back(ptr_faces[j].mIndices[0]);
                    indices.push_back(ptr_faces[j].mIndices[1]);
                    indices.push_back(ptr_faces[j].mIndices[2]);
                }

                if (ptr_mesh->HasBones())
                {
                    skinning_data.resize(attributes.size());
                    processAnimation(ptr_mesh, skinning_data);
                }

                add(ptr_mesh->mName.C_Str(), indices, attributes, skinning_data);

                indices.clear();
                attributes.clear();
            }
        }

        _animation.ptr_current_node->name       = ptr_node->mName.C_Str();
        _animation.ptr_current_node->transform  = utils::cast(ptr_node->mTransformation);

        auto ptr_parent_node = _animation.ptr_current_node;
        
        for (auto i: std::views::iota(0u, ptr_node->mNumChildren))
        {
            _animation.ptr_current_node = &ptr_parent_node->children.emplace_back();

            processNode(ptr_scene, ptr_node->mChildren[i]);
        }
    }

    void Scene::Importer::getKeyFrames(const aiAnimation* ptr_animation)
    {
        for (auto i: std::views::iota(0u, ptr_animation->mNumChannels))
        {
            const auto ptr_channel = ptr_animation->mChannels[i];
            
            std::string bone_name = ptr_channel->mNodeName.C_Str();

            if (auto res = _animation.bone_infos.get(bone_name); res)
            {
                const auto id = res->id;

                std::vector<PositionKey>    position_keys;
                std::vector<RotationKey>    rotation_keys;
                std::vector<ScaleKey>       scale_keys;

                position_keys.reserve(ptr_channel->mNumPositionKeys);
                rotation_keys.reserve(ptr_channel->mNumRotationKeys);
                scale_keys.reserve(ptr_channel->mNumScalingKeys);

                const std::span positions (ptr_channel->mPositionKeys, ptr_channel->mNumPositionKeys);
                for (const auto& pos: positions)
                    position_keys.emplace_back(utils::cast(pos.mValue), static_cast<float>(pos.mTime));

                const std::span rotations (ptr_channel->mRotationKeys, ptr_channel->mNumRotationKeys);
                for (const auto& rot: rotations)
                    rotation_keys.emplace_back(utils::cast(rot.mValue), static_cast<float>(rot.mTime));

                const std::span scales (ptr_channel->mScalingKeys, ptr_channel->mNumScalingKeys);
                for (const auto& scale: scales)
                    scale_keys.emplace_back(utils::cast(scale.mValue), static_cast<float>(scale.mTime));

                _animation.bones.push_back(
                    Bone::Builder()
                        .name(bone_name)
                        .id(id)
                        .positionKeys(std::move(position_keys))
                        .rotationKeys(std::move(rotation_keys))
                        .scaleKeys(std::move(scale_keys))
                        .build()
                );
            }
        }
    }

    void Scene::Importer::getAnimation(const aiScene* ptr_scene)
    {
        if (ptr_scene->mNumAnimations > 0)
        {
            log::info("[Scene::Importer]:\t - Process animation");

#if 0
            for (auto i: std::views::iota(0u, ptr_scene->mNumAnimations))
                getKeyFrames(ptr_scene->mAnimations[i]);
#else
            const auto ptr_animation = ptr_scene->mAnimations[0];
            getKeyFrames(ptr_animation);
#endif

            auto duration           = static_cast<float>(ptr_animation->mDuration);
            auto ticks_per_second   = static_cast<float>(ptr_animation->mTicksPerSecond);

            if (constexpr auto eps = 0.001f; ticks_per_second < eps)
                ticks_per_second = 25.0f;

            log::info("[Scene::Importer]:\t\t - Duration: {}", duration);
            log::info("[Scene::Importer]:\t\t - Ticks per second: {}", ticks_per_second);

            const auto global_transform = utils::cast(ptr_scene->mRootNode->mTransformation);

            _animation.animator = Animator::Builder()
                .bones(std::move(_animation.bones))
                .boneRegistry(std::move(_animation.bone_infos))
                .animationHierarchiryRootNode(std::move(_animation.root_node))
                .time(duration, ticks_per_second)
                .build();
        }
    }

    Scene Scene::Importer::import()
    {
        validate();

        Assimp::Importer importer;

        importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_LINE | aiPrimitiveType_POINT);
        importer.SetPropertyBool(AI_CONFIG_IMPORT_COLLADA_IGNORE_UP_DIRECTION, true);

        constexpr auto assimp_read_flags = 
                aiProcess_GenNormals    
            |   aiProcess_CalcTangentSpace 
            |   aiProcess_GenUVCoords   
            |   aiProcess_JoinIdenticalVertices 
            |   aiProcess_Triangulate;

        const auto ptr_scene = importer.ReadFile(_path.string(), assimp_read_flags);

        if (!ptr_scene || !ptr_scene->mRootNode)
            log::error("[Scene::Importer]: {}", importer.GetErrorString());

        if (!ptr_scene->HasMaterials())
            log::error("[Scene::Importer]: Not materials.");

        log::info("[Scene::Importer] Start import scene: {}.", ptr_scene->mName.C_Str());

        const std::span lights (ptr_scene->mLights, ptr_scene->mNumLights);
        for (const auto& light: lights)
            _scene_lights.emplace(light->mName.C_Str(), light);

        _ptr_root_node = std::make_unique<Node>(ptr_scene->mName.C_Str(), glm::mat4(1.0f));
        processNode(ptr_scene, ptr_scene->mRootNode);
        getAnimation(ptr_scene);

        Camera camera (_width, _height);

        importer.FreeScene();

        return Scene
        (
            Model(std::move(_ptr_root_node), std::move(_material_manager)), 
            std::move(_processed_lights),
            camera,
            std::move(_animation.animator)
        );
    }
}