#pragma once

#include <base/scene/visitors/node_visitor.hpp>

#include <vector>
#include <limits>

#include <string_view>

#include <base/vulkan/buffer.hpp>

namespace vrts
{
    struct Context;

    class   Node;
    struct  MeshNode;
}

namespace vrts
{
    class SceneGeometryReferencesGetter final :
        public NodeVisitor
    {
        void process(Node* ptr_node)            override;
        void process(MeshNode* ptr_node)        override;
        void process(SkinnedMeshNode* ptr_node) override;

        Buffer createBuffer(const std::vector<VkDeviceAddress>& references, const std::string_view name) const;

    public:
        SceneGeometryReferencesGetter(const Context* ptr_context);

        [[nodiscard]] size_t getGeometryCount() const;

        [[nodiscard]] Buffer getVertexBuffersReferences()   const;
        [[nodiscard]] Buffer getIndexBuffersReferences()    const;

    private:
        std::vector<VkDeviceAddress> _vertex_buffers_references;
        std::vector<VkDeviceAddress> _index_buffers_references;

        const Context* _ptr_context = nullptr;
    };
}