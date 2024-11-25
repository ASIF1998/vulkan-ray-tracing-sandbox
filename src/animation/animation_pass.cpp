#include <animation/animation_pass.hpp>

using namespace sample_vk;
using namespace animation;

SkinnedMesh::SkinnedMesh(const Context* ptr_context, const Mesh& source_mesh) :
    _source_mesh(source_mesh)
{
    _animated_mesh.index_count  = _source_mesh.index_count;
    _animated_mesh.vertex_count = _source_mesh.vertex_count;

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