#pragma once

#include <base/vulkan/acceleration_structure.hpp>

#include <base/scene/visitors/node_visitor.hpp>

#include <string_view>

namespace sample_vk
{
    struct Context;

    class   Node;
    struct  MeshNode;

    struct Mesh;

    struct Buffer;
}

namespace sample_vk
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