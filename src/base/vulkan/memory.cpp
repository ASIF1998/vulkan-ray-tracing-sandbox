#include <base/vulkan/memory.hpp>

#include <base/logger/logger.hpp>

#include <stdexcept>

#include <ranges>

namespace sample_vk
{
    std::vector<VkMemoryPropertyFlags> MemoryProperties::_memory_properties = { };

    void MemoryProperties::init(VkPhysicalDevice vk_physical_device_handle)
    {
        VkPhysicalDeviceMemoryProperties memory_properties = { };

        vkGetPhysicalDeviceMemoryProperties(vk_physical_device_handle, &memory_properties);

        for (auto memory_index: std::views::iota(0u, memory_properties.memoryTypeCount))
            _memory_properties.push_back(memory_properties.memoryTypes[memory_index].propertyFlags);
    }

    std::optional<uint32_t> MemoryProperties::getMemoryIndex(VkMemoryPropertyFlags flags)
    {
        for (auto memory_index: std::views::iota(0u, _memory_properties.size()))
        {
            if ((_memory_properties[memory_index] & flags) == flags)
                return memory_index;
        }

        return std::nullopt;
    }
}