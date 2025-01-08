#include <base/scene/material_manager.hpp>

#include <base/vulkan/context.hpp>

namespace vrts
{
    void MaterialManager::add(Material&& material)
    {
        _materials.push_back(std::move(material));
    }

    std::span<const Material> MaterialManager::getMaterials() const
    {
        return _materials;
    }
}