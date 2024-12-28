#pragma once 

#include <vulkan/vulkan.h>

#include <vector>

#include <optional>

namespace vrts
{
    class MemoryProperties
    {
    public:
        MemoryProperties(MemoryProperties&& memory_properties)      = delete;
        MemoryProperties(const MemoryProperties& memory_properties) = delete;

        MemoryProperties& operator = (MemoryProperties&& memory_properties)      = delete;
        MemoryProperties& operator = (const MemoryProperties& memory_properties) = delete;

        static void init(VkPhysicalDevice physical_device_handle);

        [[nodiscard]]
        static std::optional<uint32_t> getMemoryIndex(VkMemoryPropertyFlags flags);

    private:
        static std::vector<VkMemoryPropertyFlags> _memory_properties;
    };
}