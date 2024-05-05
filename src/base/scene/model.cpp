#include <base/scene/model.hpp>
#include <base/logger/logger.hpp>

#include <stdexcept>

namespace sample_vk
{
    Model::Model(std::unique_ptr<Node>&& ptr_node, MaterialManager&& material_manager) :
        _ptr_node           (std::move(ptr_node)),
        _material_manager   (std::move(material_manager))
    { }

    std::optional<VkAccelerationStructureKHR> Model::getRootTLAS() const noexcept
    {
        if (_ptr_node->acceleation_structure)
            return _ptr_node->acceleation_structure->vk_handle;

        return std::nullopt;
    }

    MaterialManager& Model::getMaterialManager() noexcept
    {
        return _material_manager;
    }

    const MaterialManager& Model::getMaterialManager() const noexcept
    {
        return _material_manager;
    }

    void Model::applyTransform(const  glm::mat4& transform)
    {
        if (_ptr_node)
            _ptr_node->transform *= transform;
    }

    void Model::addNode(std::unique_ptr<Node>&& ptr_child)
    {
        _ptr_node->children.push_back(std::move(ptr_child));
    }
}