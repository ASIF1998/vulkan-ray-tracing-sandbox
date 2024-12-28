#pragma once

#include <vulkan/vulkan.h>

#include <numeric>

namespace vrts
{
    struct Context
    {
        VkInstance          instance_handle         = VK_NULL_HANDLE;
        VkPhysicalDevice    physical_device_handle  = VK_NULL_HANDLE;
        VkDevice            device_handle           = VK_NULL_HANDLE;

        VkCommandPool command_pool_handle = VK_NULL_HANDLE;

        struct
        {
            VkQueue     handle          = VK_NULL_HANDLE;
            uint32_t    family_index    = std::numeric_limits<uint32_t>::quiet_NaN();
        } queue;
    };
}