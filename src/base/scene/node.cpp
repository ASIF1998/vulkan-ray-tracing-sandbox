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
        switch(_type)
        {
            case NodeType::Base:
                ptr_visiter->process(this);
                break;
            case NodeType::Mesh:
                ptr_visiter->process(static_cast<MeshNode*>(this));
                break;
            case NodeType::SkinnedMesh:
                ptr_visiter->process(static_cast<SkinnedMeshNode*>(this));
                break;
            default:
                log::appError("Undefined node type");
                break;
        };
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

namespace sample_vk
{
    SkinnedMeshNode::SkinnedMeshNode(const std::string_view name, const glm::mat4& transform, SkinnedMesh&& mesh) : 
        Node(name, transform),
        mesh(std::move(mesh))
    {
        _type = NodeType::SkinnedMesh;
    }
}