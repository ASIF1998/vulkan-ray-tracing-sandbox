#pragma once

#include <base/vulkan/acceleration_structure.hpp>

#include <base/scene/mesh.hpp>
#include <base/scene/material_manager.hpp>

#include <base/scene/visitors/node_visitor.hpp>

#include <vector>
#include <optional>
#include <string_view>
#include <memory>

namespace sample_vk
{
    enum class NodeType
    {
        Base,
        Mesh
    };

    class Node
    {
    public:
        Node(const std::string_view name, const glm::mat4& transform);

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

        void visit(NodeVisitor* ptr_visiter);
    
    protected:
        NodeType _type;

    public:
        std::string                             name;
        glm::mat4                               transform;
        std::vector<std::unique_ptr<Node>>      children;
        std::optional<AccelerationStructure>    acceleation_structure;
    };

    class MeshNode :
        public Node
    {
    public:
        MeshNode(const std::string_view name, const glm::mat4& transform, Mesh&& mesh);

    public:
        Mesh mesh;
    };
}