#pragma once

#include <base/vulkan/acceleration_structure.hpp>

#include <base/scene/visitors/node_visitor.hpp>

#include <string_view>

namespace vrts
{
    struct Context;
    struct Mesh;
    struct Buffer;
}

namespace vrts
{
    class ASBuilder final :
        public NodeVisitor
    {
        AccelerationStructure buildBLAS(
            std::string_view    name,
            const Buffer&       vertex_buffer,
            uint32_t            vertex_count,
            const Buffer&       index_buffer,
            uint32_t            index_count
        );

        void process(Node* ptr_node)            override;
        void process(MeshNode* ptr_node)        override;
        void process(SkinnedMeshNode* ptr_node) override;

    public:
        ASBuilder(const Context* ptr_context);

    private:
        const Context* _ptr_context;

        uint32_t _custom_index = 0;

        std::optional<Buffer> _identity_matrix;
    };
}