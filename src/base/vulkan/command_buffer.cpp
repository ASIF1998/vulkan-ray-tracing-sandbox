#include <base/vulkan/command_buffer.hpp>
#include <base/vulkan/utils.hpp>

#include <base/vulkan/context.hpp>

namespace sample_vk
{
    CommandBuffer::CommandBuffer(const Context* ptr_context, VkCommandBuffer command_buffer_handle) :
        _vk_handle      (command_buffer_handle),
        _ptr_context    (ptr_context)
    {
        assert(command_buffer_handle != VK_NULL_HANDLE);
        assert(ptr_context != nullptr);
    }

    CommandBuffer::CommandBuffer(CommandBuffer&& command_buffer) 
    {
        std::swap(_vk_handle, command_buffer._vk_handle);
    }

    CommandBuffer::~CommandBuffer()
    {
        vkFreeCommandBuffers(_ptr_context->device_handle, _ptr_context->command_pool_handle, 1, &_vk_handle);
    }

    CommandBuffer& CommandBuffer::operator = (CommandBuffer&& command_buffer)
    {
        std::swap(_vk_handle, command_buffer._vk_handle);
        return *this;
    }

    void CommandBuffer::write(const std::function<void (VkCommandBuffer command_buffer_handle)>& writer)
    {
        VkCommandBufferBeginInfo begin_info = { };
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        vkBeginCommandBuffer(_vk_handle, &begin_info);
            writer(_vk_handle);
        vkEndCommandBuffer(_vk_handle);
    }

    void CommandBuffer::execute(const Context* ptr_context)
    {
        VkCommandBufferSubmitInfo command_buffer_info = { };
        command_buffer_info.sType           = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
        command_buffer_info.commandBuffer   = _vk_handle;

        VkSubmitInfo2KHR submit_info = { };
        submit_info.sType                   = VK_STRUCTURE_TYPE_SUBMIT_INFO_2_KHR;
        submit_info.pCommandBufferInfos     = &command_buffer_info;
        submit_info.commandBufferInfoCount  = 1;

        VkFence fence_handle = VK_NULL_HANDLE;

        VkFenceCreateInfo fence_create_info = { };
        fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

        VK_CHECK(vkCreateFence(ptr_context->device_handle, &fence_create_info, nullptr, &fence_handle));

        VK_CHECK(vkQueueSubmit2(ptr_context->queue.handle, 1, &submit_info, fence_handle));
        VK_CHECK(vkWaitForFences(ptr_context->device_handle, 1, &fence_handle, VK_TRUE, std::numeric_limits<uint64_t>::max()));

        vkDestroyFence(ptr_context->device_handle, fence_handle, nullptr);
    }
}