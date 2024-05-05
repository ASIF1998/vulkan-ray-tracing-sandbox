#pragma once

#include <base/scene/visitors/node_visitor.hpp>

#include <base/vulkan/buffer.hpp>

namespace sample_vk
{
    struct Context;

    class Node;
    class MeshNode;
}

namespace sample_vk
{
    class ASBuilder final :
        public NodeVisitor
    {
        void process(Node* ptr_node)        override;
        void process(MeshNode* ptr_node)    override;

    public:
        ASBuilder(const Context* ptr_context);

    private:
        const Context* _ptr_context;

        uint32_t _custom_index = 0;

        std::optional<Buffer> _identity_matrix;
    };
}