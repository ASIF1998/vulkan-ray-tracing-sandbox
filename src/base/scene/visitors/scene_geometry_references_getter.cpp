#include <base/scene/visitors/scene_geometry_references_getter.hpp>
#include <base/scene/node.hpp>

namespace sample_vk
{
    SceneGeometryReferencesGetter::SceneGeometryReferencesGetter(const Context* ptr_context) :
        _ptr_context(ptr_context)
    {
        if (!ptr_context)
            log::vkError("[SceneGeometryReferencesGetter]: ptr_context is null.");
    }

    void SceneGeometryReferencesGetter::process(Node* ptr_node)
    { 
        if (ptr_node->children.empty())
            return ;

        for (const auto& child: ptr_node->children)
            child->visit(this);
    }

    void SceneGeometryReferencesGetter::process(MeshNode* ptr_node)
    {
        _vertex_buffers_references.push_back(ptr_node->mesh.vertex_buffer.getAddress());
        _index_buffers_references.push_back(ptr_node->mesh.index_buffer.getAddress());

        process(static_cast<Node*>(ptr_node));
    }

    size_t SceneGeometryReferencesGetter::getGeometryCount() const
    {
        return _vertex_buffers_references.size();
    }

    Buffer SceneGeometryReferencesGetter::createBuffer(const std::vector<VkDeviceAddress>& references, const std::string_view name) const
    {
        if (references.empty())
            log::vkError("[SceneGeometryReferencesGetter]: Not references.");

        auto memory_index = MemoryProperties::getMemoryIndex(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        if (!memory_index.has_value())
            log::vkError("[SceneGeometryReferencesGetter]: Not memory index for create references buffer.");

        auto references_buffer = Buffer::make(
            _ptr_context,
            references.size() * sizeof(VkDeviceAddress),
            *memory_index,
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            name.data()
        );

        auto command_buffer_for_write = VkUtils::getCommandBuffer(_ptr_context);

        Buffer::writeData(
            references_buffer, 
            std::span(references), 
            command_buffer_for_write, 
            _ptr_context->queue.handle
        );

        return references_buffer;
    }

    Buffer SceneGeometryReferencesGetter::getVertexBuffersReferences() const
    {
        return createBuffer(_vertex_buffers_references, "Vertex buffers references");
    }

    Buffer SceneGeometryReferencesGetter::getIndexBuffersReferences() const
    {
        return createBuffer(_index_buffers_references, "Index buffers references");
    }
}