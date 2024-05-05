#pragma once

namespace sample_vk
{
    class Node;
    class MeshNode;
}

namespace sample_vk
{
    class NodeVisitor
    {
    public:
        NodeVisitor() = default;

        NodeVisitor(NodeVisitor&& visitor)      = delete;
        NodeVisitor(const NodeVisitor& visitor) = delete;

        virtual ~NodeVisitor() = default;

        NodeVisitor& operator = (NodeVisitor&& visitor)         = delete;
        NodeVisitor& operator = (const NodeVisitor& visitor)    = delete;

        virtual void process(Node* ptr_node)        = 0;
        virtual void process(MeshNode* ptr_node)    = 0;
    };

    template<typename T>
    concept IsNodeVisitor = requires(T visitor)
    {
        dynamic_cast<NodeVisitor*>(&visitor);
    };
}