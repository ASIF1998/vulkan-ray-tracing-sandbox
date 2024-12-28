#include <base/configuration.hpp>

namespace vrts
{
    void VkUtils::setName(
        VkDevice                    device_handle, 
        const IsVulkanWrapper auto& wrapper, 
        VkObjectType                type, 
        const std::string_view      name
    )
    {
        setName(device_handle, wrapper.vk_handle, type, name);
    }

    void VkUtils::setName(
        VkDevice                device_handle, 
        IsVulkanHandle auto     handle,
        VkObjectType            type, 
        const std::string_view  name
    )
    {
        if constexpr (enable_vk_objects_naming)
        {
            VkDebugUtilsObjectNameInfoEXT name_info = { };
            name_info.sType          = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
            name_info.objectHandle   = reinterpret_cast<uint64_t>(handle);
            name_info.objectType     = type;
            name_info.pObjectName    = name.data();

            VK_CHECK(_vulkan_function_pointer_table->vkSetDebugUtilsObjectNameEXT(device_handle, &name_info));
        }
    }
}