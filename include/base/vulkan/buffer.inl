#include <base/raytracing_base.hpp>
#include <base/vulkan/memory.hpp>

#include <base/vulkan/utils.hpp>
#include <base/vulkan/gpu_marker_colors.hpp>

namespace vrts
{
    template<typename T>
    static void Buffer::writeData(Buffer& buffer, const std::span<T> data)
    {
        auto temp_buffer = Buffer::Builder(buffer._ptr_context)
            .vkSize(data.size_bytes())
            .vkUsage(VK_BUFFER_USAGE_TRANSFER_SRC_BIT)
            .isHostVisible(true)
            .build();

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

        auto command_buffer = VkUtils::getCommandBuffer(buffer._ptr_context);

        command_buffer.write([&data, &temp_buffer, &buffer] (VkCommandBuffer vk_handle)
        {
            VkBufferCopy copy = { };
            copy.dstOffset  = 0;
            copy.srcOffset  = 0;
            copy.size       = data.size_bytes();

            vkCmdCopyBuffer(vk_handle, temp_buffer.vk_handle, buffer.vk_handle, 1, &copy);
        }, "Write data in buffer", GpuMarkerColors::write_data_in_buffer);

        command_buffer.upload(buffer._ptr_context);
    }

    template<typename T>
    static void Buffer::writeData(Buffer& buffer, const T& obj)
    {
        writeData(buffer, std::span(reinterpret_cast<const uint8_t*>(&obj), sizeof(T)));
    }
}