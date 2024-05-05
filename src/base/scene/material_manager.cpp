#include <base/scene/material_manager.hpp>

#include <base/vulkan/context.hpp>

namespace sample_vk
{
    MaterialManager::MaterialManager(MaterialManager&& material_manager) :
        _materials (std::move(material_manager._materials))
    {
    }

    MaterialManager& MaterialManager::operator = (MaterialManager&& material_manager)
    {
        std::swap(_materials, material_manager._materials);
        return *this;
    }

    void MaterialManager::add(Material&& material)
    {
        _materials.push_back(std::move(material));
    }

    const std::vector<Material>& MaterialManager::getMaterials() const noexcept
    {
        return _materials;
    }
}