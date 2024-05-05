#include <base/scene/node.hpp>

#include <base/raytracing_base.hpp>

namespace sample_vk
{
    Node::Node(const std::string_view name, const glm::mat4& transform) :
        _type       (NodeType::Base),
        name        (name),
        transform   (transform)
    { }

    void Node::visit(NodeVisitor* ptr_visiter)
    {
        if (_type == NodeType::Mesh)
            ptr_visiter->process(static_cast<MeshNode*>(this));
        else
            ptr_visiter->process(this);
    }
}

namespace sample_vk
{
    MeshNode::MeshNode(const std::string_view name, const glm::mat4& transform, Mesh&& mesh) :
        Node    (name, transform),
        mesh    (std::move(mesh))
    {
        _type = NodeType::Mesh;
    }
}