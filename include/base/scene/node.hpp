#pragma once

#include <base/vulkan/acceleration_structure.hpp>

#include <base/scene/mesh.hpp>
#include <base/scene/material_manager.hpp>

#include <base/scene/visitors/node_visitor.hpp>

#include <vector>
#include <optional>
#include <string_view>
#include <memory>

namespace vrts
{
    enum class NodeType
    {
        Base,
        Mesh,
        SkinnedMesh
    };

    class Node
    {
    public:
        explicit Node(const std::string_view name, const glm::mat4& transform);

        Node(Node&& node)       = delete;
        Node(const Node& node)  = delete;

        virtual ~Node() = default;

        Node& operator = (Node&& node)      = delete;
        Node& operator = (const Node& node) = delete;

        template<IsNodeVisitor T>
        void visit(const std::unique_ptr<T>& ptr_visitor)
        {
            visit(static_cast<NodeVisitor*>(ptr_visitor.get()));   
        }

        void visit(NodeVisitor* ptr_visitor);
    
    protected:
        NodeType _type;

    public:
        std::string                             name;
        glm::mat4                               transform;
        std::vector<std::unique_ptr<Node>>      children;
        std::optional<AccelerationStructure>    acceleation_structure;
    };

    struct MeshNode :
        public Node
    {
        explicit MeshNode(const std::string_view name, const glm::mat4& transform, Mesh&& mesh);

        Mesh mesh;
    };

    struct SkinnedMeshNode :
        public Node
    {
        explicit SkinnedMeshNode(const std::string_view name, const glm::mat4& transform, SkinnedMesh&& mesh);

        SkinnedMesh mesh;
    };
}