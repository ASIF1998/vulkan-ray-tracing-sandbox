#include <base/configuration.hpp>
#include <base/vulkan/function_pointer_table.hpp>

#include <base/logger/logger.hpp>

namespace vrts
{  
    template<typename VulkanFunctionType>
    VulkanFunctionType loadFunction(VkDevice device_handle, const std::string_view function_name)
    {
        auto ptr_function = reinterpret_cast<VulkanFunctionType>(
            vkGetDeviceProcAddr(
                device_handle, 
                function_name.data()
            )
        );

        if (ptr_function)
            log::info("[Load function]: {}", function_name);
        else
            log::error("[Load function]: {}", function_name);

        return ptr_function;
    }

    VkFunctionPointerTable::VkFunctionPointerTable(VkDevice device_handle)
    {
        vkSetDebugUtilsObjectNameEXT                    = loadFunction<PFN_vkSetDebugUtilsObjectNameEXT>(device_handle, "vkSetDebugUtilsObjectNameEXT");
        vkDestroyAccelerationStructureKHR               = loadFunction<PFN_vkDestroyAccelerationStructureKHR>(device_handle, "vkDestroyAccelerationStructureKHR");
        vkCmdBuildAccelerationStructuresKHR             = loadFunction<PFN_vkCmdBuildAccelerationStructuresKHR>(device_handle, "vkCmdBuildAccelerationStructuresKHR");
        vkGetAccelerationStructureBuildSizesKHR         = loadFunction<PFN_vkGetAccelerationStructureBuildSizesKHR>(device_handle, "vkGetAccelerationStructureBuildSizesKHR");
        vkCreateAccelerationStructureKHR                = loadFunction<PFN_vkCreateAccelerationStructureKHR>(device_handle, "vkCreateAccelerationStructureKHR");
        vkGetAccelerationStructureDeviceAddressKHR      = loadFunction<PFN_vkGetAccelerationStructureDeviceAddressKHR>(device_handle, "vkGetAccelerationStructureDeviceAddressKHR");
        vkCreateRayTracingPipelinesKHR                  = loadFunction<PFN_vkCreateRayTracingPipelinesKHR>(device_handle, "vkCreateRayTracingPipelinesKHR");
        vkGetRayTracingShaderGroupHandlesKHR            = loadFunction<PFN_vkGetRayTracingShaderGroupHandlesKHR>(device_handle, "vkGetRayTracingShaderGroupHandlesKHR");
        vkCmdTraceRaysKHR                               = loadFunction<PFN_vkCmdTraceRaysKHR>(device_handle, "vkCmdTraceRaysKHR");
  
        if (vrts::enable_vk_debug_marker)
        {
            vkCmdDebugMarkerBeginEXT    = loadFunction<PFN_vkCmdDebugMarkerBeginEXT>(device_handle, "vkCmdDebugMarkerBeginEXT");
            vkCmdDebugMarkerEndEXT      = loadFunction<PFN_vkCmdDebugMarkerEndEXT>(device_handle, "vkCmdDebugMarkerEndEXT");
        }       
    }
}