#pragma once

#include <base/scene/node.hpp>
#include <base/scene/material_manager.hpp>
#include <base/scene/animator.hpp>

#include <base/math.hpp>

#include <memory>

#include <optional>

#include <concepts>

namespace vrts
{
    class Model
    {
        friend class Scene;

    private:
        explicit Model(std::unique_ptr<Node>&& ptr_node, MaterialManager&& material_manager);

    public:
        template<IsNodeVisitor T>
        void visit(const std::unique_ptr<T>& ptr_visitor)
        {
            _ptr_node->visit(ptr_visitor);
        }

        [[nodiscard]] 
        std::optional<VkAccelerationStructureKHR> getRootTLAS() const noexcept;

        [[nodiscard]] 
        MaterialManager& getMaterialManager() noexcept;

        [[nodiscard]] 
        const MaterialManager& getMaterialManager() const noexcept;

        void applyTransform(const glm::mat4& transform);

        void addNode(std::unique_ptr<Node>&& ptr_child);

    private:
        std::unique_ptr<Node>   _ptr_node;
        MaterialManager         _material_manager;
    };
}