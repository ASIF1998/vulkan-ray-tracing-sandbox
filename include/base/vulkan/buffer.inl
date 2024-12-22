#include <base/raytracing_base.hpp>
#include <base/vulkan/memory.hpp>

#include <base/vulkan/utils.hpp>

namespace sample_vk
{
    template<typename T>
    static void Buffer::writeData(
        Buffer&             buffer, 
        const std::span<T>  data, 
        CommandBuffer&      command_buffer
    )
    {
        Buffer temp_buffer (buffer._ptr_context);

        if (auto memory_index = MemoryProperties::getMemoryIndex(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))
        {
            temp_buffer = Buffer::make(
                buffer._ptr_context,
                data.size_bytes(),
                *memory_index,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT
            );
        }
        else 
            log::error("[Buffer]: Not memory index for create temp buffer.");

        uint8_t* ptr_src = nullptr;

        VK_CHECK(
            vkMapMemory(
                temp_buffer._ptr_context->device_handle, 
                temp_buffer.memory_handle, 
                0, data.size_bytes(), 
                0,
                reinterpret_cast<void**>(&ptr_src)
            )
        );

        memcpy(ptr_src, data.data(), data.size_bytes());

        vkUnmapMemory(temp_buffer._ptr_context->device_handle, temp_buffer.memory_handle);

        command_buffer.write([&data, &temp_buffer, &buffer] (VkCommandBuffer vk_handle)
        {
            VkBufferCopy copy = { };
            copy.dstOffset  = 0;
            copy.srcOffset  = 0;
            copy.size       = data.size_bytes();

            vkCmdCopyBuffer(vk_handle, temp_buffer.vk_handle, buffer.vk_handle, 1, &copy);
        });

        command_buffer.upload(buffer._ptr_context);
    }

    template<typename T>
    static void Buffer::writeData(
        Buffer&         buffer, 
        const T&        obj, 
        CommandBuffer&  command_buffer
    )
    {
        writeData(
            buffer, 
            std::span(reinterpret_cast<const uint8_t*>(&obj), sizeof(T)), 
            command_buffer
        );
    }
}