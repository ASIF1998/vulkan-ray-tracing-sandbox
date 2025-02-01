#pragma once

#include <base/math.hpp>

#include <vulkan/vulkan.h>

#include <functional>

#include <string_view>

namespace vrts
{
    struct Context;
}

namespace vrts
{
    class CommandBuffer
    {
        using WriteFunctionType = std::function<void (VkCommandBuffer command_buffer_handle)>;

    public:
        CommandBuffer(const Context* ptr_context, VkCommandBuffer command_buffer_handle);

        CommandBuffer(CommandBuffer&& command_buffer);
        CommandBuffer(const CommandBuffer& command_buffer) = delete;

        ~CommandBuffer();

        CommandBuffer& operator = (CommandBuffer&& command_buffer);
        CommandBuffer& operator = (const CommandBuffer& command_buffer) = delete;

        void write(
            const WriteFunctionType&    writer, 
            std::string_view            name = "",
            const glm::vec3&            col = glm::vec3(0)
        );

        void upload(const Context* ptr_context);

    private:
        VkCommandBuffer _vk_handle      = VK_NULL_HANDLE;
        const Context*  _ptr_context    = nullptr;
    };
}