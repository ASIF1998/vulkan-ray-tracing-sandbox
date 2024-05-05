#pragma once

#include <base/vulkan/image.hpp>

#include <vector>
#include <numeric>

#include <base/vulkan/utils.hpp>

namespace sample_vk
{
    struct Material
    {
        std::optional<Image> albedo;
        std::optional<Image> normal_map;
        std::optional<Image> metallic;
        std::optional<Image> roughness;
        std::optional<Image> emissive;
    };

    class MaterialManager
    {
    public:
        MaterialManager() = default;

        MaterialManager(MaterialManager&& material_manager);
        MaterialManager(const MaterialManager& material_manager) = delete;

        MaterialManager& operator = (MaterialManager&& material_manager);
        MaterialManager& operator = (const MaterialManager& material_manager)   = delete;

        [[nodiscard]] const std::vector<Material>& getMaterials() const noexcept;

        void add(Material&& material);

    private:
        std::vector<Material> _materials;
    };    
}