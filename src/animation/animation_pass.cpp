#include <animation/animation_pass.hpp>

#include <base/scene/node.hpp>

#include <base/logger/logger.hpp>

namespace sample_vk::animation
{
    SkinnedMesh::SkinnedMesh(const Context* ptr_context, const Mesh* ptr_source_mesh) :
        _ptr_source_mesh(ptr_source_mesh)
    {
        if (!ptr_source_mesh)
            log::vkError("[SkinnedMesh::SkinnedMesh] Pointer to source mesh is null");

        _animated_mesh.index_count  = _ptr_source_mesh->index_count;
        _animated_mesh.vertex_count = _ptr_source_mesh->vertex_count;

        constexpr auto shared_usage_flags = 
                    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT 
                |   VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT 
                |   VK_BUFFER_USAGE_TRANSFER_DST_BIT 
                |   VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;

        const auto memory_index = MemoryProperties::getMemoryIndex(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
            .or_else([]
            {
                constexpr std::optional invalid_memory_index = std::numeric_limits<uint32_t>::infinity();
                
                log::appError("[SkinnedMesh::SkinnedMesh] Failed get memory index for create buffers");

                return invalid_memory_index;
            })
            .value();

        static int index_for_current_buffers = 0;
        ++index_for_current_buffers;

        _animated_mesh.index_buffer = Buffer::make(
            ptr_context, 
            _animated_mesh.index_count, 
            memory_index,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT | shared_usage_flags, 
            std::format("[SkinnedMesh][IndexBuffer]: {}", index_for_current_buffers)
        );

        _animated_mesh.vertex_buffer = Buffer::make(
            ptr_context, 
            _animated_mesh.vertex_count, 
            memory_index,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | shared_usage_flags,
            std::format("[SkinnedMesh][VertexBuffer]: {}", index_for_current_buffers)
        );
    }

    SkinnedMesh::SkinnedMesh(SkinnedMesh&& skinned_mesh) :
        _ptr_source_mesh    (skinned_mesh._ptr_source_mesh),
        _animated_mesh      (std::move(skinned_mesh._animated_mesh))
    { }

    SkinnedMesh& SkinnedMesh::operator = (SkinnedMesh&& skinned_mesh)
    {
        _ptr_source_mesh    = skinned_mesh._ptr_source_mesh;
        _animated_mesh      = std::move(skinned_mesh._animated_mesh);
        return *this;
    }
}

namespace sample_vk::animation
{
    AnimationPass::AnimationPass(std::vector<SkinnedMesh>&& meshes) :
        _meshes (std::move(meshes))
    { }

    AnimationPass::AnimationPass(AnimationPass&& animation_pass) :
        _meshes(std::move(animation_pass._meshes))
    { }

    AnimationPass& AnimationPass::operator = (AnimationPass&& animation_pass)
    {
        _meshes = std::move(animation_pass._meshes);
        return *this;
    }

    void AnimationPass::process()
    {
        /// @todo
    }
}

namespace sample_vk::animation
{
    AnimationPass::Builder::Builder(const Context* ptr_context) :
        _ptr_context(ptr_context)
    {
        if (!ptr_context)
            log::vkError("[AnimationPass::Builder] Vulkan context is null");
    }

    void AnimationPass::Builder::process(Node* ptr_node)
    {
        for (const auto& ptr_child: ptr_node->children)
            process(ptr_child.get());
    }

    void AnimationPass::Builder::process(MeshNode* ptr_node)
    {
        SkinnedMesh skinned_mesg (_ptr_context, &ptr_node->mesh);
    }

    AnimationPass AnimationPass::Builder::build()
    {
        return AnimationPass(std::move(_meshes));
    }
}