#include <base/scene/visitors/scene_geometry_references_getter.hpp>
#include <base/scene/node.hpp>

namespace vrts
{
    SceneGeometryReferencesGetter::SceneGeometryReferencesGetter(const Context* ptr_context) :
        _ptr_context(ptr_context)
    {
        if (!ptr_context)
            log::error("[SceneGeometryReferencesGetter]: ptr_context is null.");
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
        _vertex_buffers_references.push_back(ptr_node->mesh.vertex_buffer->getAddress());
        _index_buffers_references.push_back(ptr_node->mesh.index_buffer->getAddress());

        process(static_cast<Node*>(ptr_node));
    }

    void SceneGeometryReferencesGetter::process(SkinnedMeshNode* ptr_node)
    {
        _vertex_buffers_references.push_back(ptr_node->mesh.processed_vertex_buffer->getAddress());
        _index_buffers_references.push_back(ptr_node->mesh.index_buffer->getAddress());

        process(static_cast<Node*>(ptr_node));
    }

    size_t SceneGeometryReferencesGetter::getGeometryCount() const
    {
        return _vertex_buffers_references.size();
    }

    Buffer SceneGeometryReferencesGetter::createBuffer(const std::vector<VkDeviceAddress>& references, const std::string_view name) const
    {
        if (references.empty())
            log::error("[SceneGeometryReferencesGetter]: Not references.");

        auto references_buffer = Buffer::Builder(_ptr_context)
            .vkSize(references.size() * sizeof(VkDeviceAddress))
            .vkUsage(VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
            .name(name)
            .build();

        Buffer::writeData(references_buffer, std::span(references));

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