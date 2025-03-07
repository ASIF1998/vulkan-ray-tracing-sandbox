#include <base/vulkan/gpu_marker_colors.hpp>

namespace vrts::utils
{
    glm::vec3 generateColor(uint32_t marker_id)
    {
        const auto hash = std::hash<uint32_t>()(marker_id);

        const auto r = static_cast<float>((hash >> 22) & 0x3ff) * 0.000977517f;
        const auto g = static_cast<float>((hash >> 12) & 0x3ff) * 0.000977517f;
        const auto b = static_cast<float>(hash & 0xfff) * 0.0002442002f;

        return glm::vec3(r, g, b);
    }
}