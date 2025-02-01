#include <base/vulkan/command_buffer.hpp>
#include <base/vulkan/utils.hpp>

#include <base/vulkan/context.hpp>

namespace vrts
{
    CommandBuffer::CommandBuffer(const Context* ptr_context, VkCommandBuffer command_buffer_handle) :
        _vk_handle      (command_buffer_handle),
        _ptr_context    (ptr_context)
    {
        assert(command_buffer_handle != VK_NULL_HANDLE);
        assert(ptr_context != nullptr);
    }

    CommandBuffer::CommandBuffer(CommandBuffer&& command_buffer) :
        _ptr_context(command_buffer._ptr_context)
    {
        std::swap(_vk_handle, command_buffer._vk_handle);
    }

    CommandBuffer::~CommandBuffer()
    {
        vkFreeCommandBuffers(_ptr_context->device_handle, _ptr_context->command_pool_handle, 1, &_vk_handle);
    }

    CommandBuffer& CommandBuffer::operator = (CommandBuffer&& command_buffer)
    {
        _ptr_context = command_buffer._ptr_context;
        std::swap(_vk_handle, command_buffer._vk_handle);
        return *this;
    }

    void CommandBuffer::write(
        const WriteFunctionType&    writer, 
        std::string_view            name,
        const glm::vec3&            col
    )
    {
        VkCommandBufferBeginInfo begin_info = { };
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        VkDebugMarkerMarkerInfoEXT marker_info = { };
        marker_info.sType       = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
        marker_info.pMarkerName = name.data();

        if (col.length() > 0.0)
        {
            marker_info.color[0] = col.r;
            marker_info.color[1] = col.g;
            marker_info.color[2] = col.b;
            marker_info.color[3] = 1.0f;
        }

        const auto& functions = VkUtils::getVulkanFunctionPointerTable();

        const bool enable_marking = (!name.empty() || col.length() > 0.0) && vrts::enable_vk_debug_marker;

        vkBeginCommandBuffer(_vk_handle, &begin_info);

        if (enable_marking)
            functions.vkCmdDebugMarkerBeginEXT(_vk_handle, &marker_info);

        writer(_vk_handle);

        if (enable_marking)
            functions.vkCmdDebugMarkerEndEXT(_vk_handle);

        vkEndCommandBuffer(_vk_handle);
    }

    void CommandBuffer::upload(const Context* ptr_context)
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