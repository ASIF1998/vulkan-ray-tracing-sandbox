#include <base/vulkan/acceleration_structure.hpp>

namespace sample_vk
{
    AccelerationStructure::AccelerationStructure(
        const Context*              ptr_context,
        VkAccelerationStructureKHR  vk_acceleration_structure_handle,
        Buffer&&                    acceleration_structure_buffer
    ) :
        _ptr_context    (ptr_context),
        vk_handle       (vk_acceleration_structure_handle),
        buffer          (std::move(acceleration_structure_buffer))
    {
        validate();
    }

    void AccelerationStructure::validate() const
    {
        if (!_ptr_context)
            log::error("[AccelerationStructure]: Not driver.");

        if (vk_handle == VK_NULL_HANDLE)
            log::error("[AccelerationStructure]: acceleration structure handle is null.");

        if (buffer == std::nullopt)
            log::error("[AccelerationStructure]: buffer not initialize.");
    }

    AccelerationStructure::AccelerationStructure(AccelerationStructure&& acceleration_structure) :
        _ptr_context (acceleration_structure._ptr_context)
    {
        std::swap(vk_handle, acceleration_structure.vk_handle);
        std::swap(buffer, acceleration_structure.buffer);
    }

    AccelerationStructure::~AccelerationStructure()
    {
        if (isInit())
        {
            auto func_table = VkUtils::getVulkanFunctionPointerTable();

            func_table.vkDestroyAccelerationStructureKHR(
                _ptr_context->device_handle, 
                vk_handle,
                nullptr
            );
        }
    }

    AccelerationStructure& AccelerationStructure::operator = (AccelerationStructure&& acceleration_structure)
    {
        std::swap(_ptr_context, acceleration_structure._ptr_context);
        std::swap(vk_handle, acceleration_structure.vk_handle);
        std::swap(buffer, acceleration_structure.buffer);

        return *this;
    }

    bool AccelerationStructure::isInit() const
    {
        return  _ptr_context != nullptr     && 
                vk_handle != VK_NULL_HANDLE && 
                buffer != std::nullopt;
    }
}