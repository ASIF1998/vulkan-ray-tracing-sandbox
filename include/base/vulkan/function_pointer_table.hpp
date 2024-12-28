#pragma once

#include <vulkan/vulkan.h>

namespace vrts
{
    struct VkFunctionPointerTable
    {
        VkFunctionPointerTable(VkDevice device_handle);

        PFN_vkSetDebugUtilsObjectNameEXT                vkSetDebugUtilsObjectNameEXT                = VK_NULL_HANDLE;
        PFN_vkDestroyAccelerationStructureKHR           vkDestroyAccelerationStructureKHR           = VK_NULL_HANDLE;
        PFN_vkCmdBuildAccelerationStructuresKHR         vkCmdBuildAccelerationStructuresKHR         = VK_NULL_HANDLE;
        PFN_vkGetAccelerationStructureBuildSizesKHR     vkGetAccelerationStructureBuildSizesKHR     = VK_NULL_HANDLE;
        PFN_vkCreateAccelerationStructureKHR            vkCreateAccelerationStructureKHR            = VK_NULL_HANDLE;
        PFN_vkGetAccelerationStructureDeviceAddressKHR  vkGetAccelerationStructureDeviceAddressKHR  = VK_NULL_HANDLE;
        PFN_vkCreateRayTracingPipelinesKHR              vkCreateRayTracingPipelinesKHR              = VK_NULL_HANDLE;
        PFN_vkGetRayTracingShaderGroupHandlesKHR        vkGetRayTracingShaderGroupHandlesKHR        = VK_NULL_HANDLE;
        PFN_vkCmdTraceRaysKHR                           vkCmdTraceRaysKHR                           = VK_NULL_HANDLE;
    };
}