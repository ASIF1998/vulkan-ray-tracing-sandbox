#pragma once

#include <vulkan/vulkan.h>

#include <functional>

namespace sample_vk
{
    struct Context;
}

namespace sample_vk
{
    class CommandBuffer
    {
    public:
        CommandBuffer(const Context* ptr_context, VkCommandBuffer command_buffer_handle);

        CommandBuffer(CommandBuffer&& command_buffer);
        CommandBuffer(const CommandBuffer& command_buffer) = delete;

        ~CommandBuffer();

        CommandBuffer& operator = (CommandBuffer&& command_buffer);
        CommandBuffer& operator = (const CommandBuffer& command_buffer) = delete;

        void write(const std::function<void (VkCommandBuffer command_buffer_handle)>& writer);

        void execute(const Context* ptr_context);

    private:
        VkCommandBuffer _vk_handle      = VK_NULL_HANDLE;
        const Context*  _ptr_context    = nullptr;
    };
}