#include <base/scene/node.hpp>

#include <base/raytracing_base.hpp>

namespace vrts
{
    Node::Node(const std::string_view name, const glm::mat4& transform) :
        _type       (NodeType::Base),
        name        (name),
        transform   (transform)
    { }

    void Node::visit(NodeVisitor* ptr_visitor)
    {
        switch(_type)
        {
            case NodeType::Base:
                ptr_visitor->process(this);
                break;
            case NodeType::Mesh:
                ptr_visitor->process(static_cast<MeshNode*>(this));
                break;
            case NodeType::SkinnedMesh:
                ptr_visitor->process(static_cast<SkinnedMeshNode*>(this));
                break;
            default:
                log::error("Undefined node type");
                break;
        };
    }
}

namespace vrts
{
    MeshNode::MeshNode(const std::string_view name, const glm::mat4& transform, Mesh&& mesh) :
        Node    (name, transform),
        mesh    (std::move(mesh))
    {
        _type = NodeType::Mesh;
    }
}

namespace vrts
{
    SkinnedMeshNode::SkinnedMeshNode(const std::string_view name, const glm::mat4& transform, SkinnedMesh&& mesh) : 
        Node(name, transform),
        mesh(std::move(mesh))
    {
        _type = NodeType::SkinnedMesh;
    }
}