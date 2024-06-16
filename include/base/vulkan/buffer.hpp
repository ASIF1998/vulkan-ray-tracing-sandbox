#pragma once

#include <vulkan/vulkan.h>

#include <vector>
#include <span>

#include <string_view>

#include <base/vulkan/command_buffer.hpp>

namespace sample_vk
{
    struct Context;
}

namespace sample_vk
{
    struct Buffer
    {
    public:
        Buffer(const Context* ptr_context);

        Buffer(Buffer&& buffer);
        Buffer(const Buffer& buffer) = delete;

        ~Buffer();

        Buffer& operator = (Buffer&& buffer);
        Buffer& operator = (const Buffer& buffer) = delete;

        [[nodiscard]] VkMemoryRequirements  getMemoryRequirements() const noexcept;
        [[nodiscard]] VkDeviceAddress       getAddress()            const noexcept;

        [[nodiscard]] bool isInit() const noexcept;

        [[nodiscard]]
        static Buffer make(
            const Context*          ptr_context,
            VkDeviceSize            size, 
            uint32_t                type_index,
            VkBufferUsageFlags      usage_flags,
            const std::string_view  buffer_name = "Temp buffer"
        );

        template<typename T>
        static void writeData(
            Buffer&             buffer, 
            const std::span<T>  data, 
            CommandBuffer&      command_buffer
        );

        template<typename T>
        static void writeData(
            Buffer&         buffer, 
            const T&        obj, 
            CommandBuffer&  command_buffer
        );
        
    public:
        VkBuffer        vk_handle       = VK_NULL_HANDLE;
        VkDeviceMemory  memory_handle   = VK_NULL_HANDLE;

        VkDeviceSize size_in_bytes = 0;

    private:
        const Context* _ptr_context = nullptr;
    };
}

#include <base/vulkan/buffer.inl>