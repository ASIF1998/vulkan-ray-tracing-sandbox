#pragma once

#include <base/vulkan/buffer.hpp>

namespace sample_vk
{
    struct Context;
}

namespace sample_vk
{
    struct AccelerationStructure
    {
    private:
        void validate() const;

    public:
        AccelerationStructure(
            const Context*              ptr_context,
            VkAccelerationStructureKHR  acceleration_structure_handle,
            Buffer&&                    acceleration_structure_buffer
        );

        AccelerationStructure(AccelerationStructure&& acceleration_structure);
        AccelerationStructure(const AccelerationStructure& acceleration_structure) = delete;

        ~AccelerationStructure();

        AccelerationStructure& operator = (AccelerationStructure&& acceleration_structure);
        AccelerationStructure& operator = (const AccelerationStructure& acceleration_structure) = delete;

        [[nodiscard]] bool isInit() const;

    public:
        VkAccelerationStructureKHR  vk_handle = VK_NULL_HANDLE;
        std::optional<Buffer>       buffer;
    
    private:
        const Context* _ptr_context = nullptr;
    };
}