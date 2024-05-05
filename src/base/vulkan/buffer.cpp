#include <base/vulkan/buffer.hpp>

#include <base/vulkan/utils.hpp>

#include <stdexcept>

namespace sample_vk
{
    Buffer::Buffer(const Context* ptr_context):
        _ptr_context (ptr_context)
    { 
        if (!ptr_context)
            log::vkError("[Buffer]: ptr_context is null.");
    }

    Buffer::Buffer(Buffer&& buffer)
    {
        std::swap(_ptr_context, buffer._ptr_context);
        std::swap(memory_handle, buffer.memory_handle);
        std::swap(vk_handle, buffer.vk_handle);
        std::swap(size_in_bytes, buffer.size_in_bytes);
    }

    Buffer::~Buffer()
    {
        if (isInit())
        {
            vkDestroyBuffer(_ptr_context->device_handle, vk_handle, nullptr);
            vkFreeMemory(_ptr_context->device_handle, memory_handle, nullptr);
        }
    }

    Buffer& Buffer::operator = (Buffer&& buffer)
    {
        std::swap(_ptr_context, buffer._ptr_context);
        std::swap(memory_handle, buffer.memory_handle);
        std::swap(vk_handle, buffer.vk_handle);
        std::swap(size_in_bytes, buffer.size_in_bytes);

        return *this;
    }

    VkMemoryRequirements Buffer::getMemoryRequirements() const noexcept
    {
        VkMemoryRequirements memory_requirements  = { };

        vkGetBufferMemoryRequirements(_ptr_context->device_handle, vk_handle, &memory_requirements);
        
        return memory_requirements;
    }

    VkDeviceAddress Buffer::getAddress() const noexcept
    {
        VkBufferDeviceAddressInfo buffer_device_address_info = { };
        buffer_device_address_info.sType    = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        buffer_device_address_info.buffer   = vk_handle;

        return vkGetBufferDeviceAddress(_ptr_context->device_handle, &buffer_device_address_info);
    }

    bool Buffer::isInit() const noexcept
    {
        return  memory_handle != VK_NULL_HANDLE  && vk_handle != VK_NULL_HANDLE;
    }

    Buffer Buffer::make(
        const Context*          ptr_context,
        VkDeviceSize            size, 
        uint32_t                type_index,
        VkBufferUsageFlags      usage_flags,
        const std::string_view  buffer_name
    )
    {
        if (!ptr_context)
            log::vkError("[Buffer]: ptr_context is null.");

        if (!size)
            log::vkError("[Buffer]: size is 0.");

        Buffer buffer (ptr_context);

        buffer.size_in_bytes    = size;
        buffer._ptr_context     = ptr_context;

        VkBufferCreateInfo buffer_create_info = { };
        buffer_create_info.sType                    = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_create_info.usage                    = usage_flags;
        buffer_create_info.size                     = size;
        buffer_create_info.sharingMode              = VK_SHARING_MODE_EXCLUSIVE;
        buffer_create_info.queueFamilyIndexCount    = static_cast<uint32_t>(1);
        buffer_create_info.pQueueFamilyIndices      = &ptr_context->queue.family_index;

        VK_CHECK(
            vkCreateBuffer(
                ptr_context->device_handle, 
                &buffer_create_info, 
                nullptr, 
                &buffer.vk_handle
            )
        );

        auto memory_requirements = buffer.getMemoryRequirements();

        VkMemoryAllocateFlagsInfoKHR memory_allocation_flags = { };
        memory_allocation_flags.sType       = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR;
        memory_allocation_flags.flags       = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;

        VkMemoryAllocateInfo memory_allocate_info = { };
        memory_allocate_info.sType              = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memory_allocate_info.pNext              = &memory_allocation_flags;
        memory_allocate_info.allocationSize     = VkUtils::getAlignedSize(memory_requirements.size, memory_requirements.alignment);
        memory_allocate_info.memoryTypeIndex    = type_index;

        VK_CHECK(
            vkAllocateMemory(
                ptr_context->device_handle, 
                &memory_allocate_info, 
                nullptr, 
                &buffer.memory_handle
            )
        );

        VK_CHECK(
            vkBindBufferMemory(
                ptr_context->device_handle, 
                buffer.vk_handle, 
                buffer.memory_handle, 
                0
            )
        );

        VkUtils::setName(ptr_context->device_handle, buffer, VK_OBJECT_TYPE_BUFFER, buffer_name);

        return buffer; 
    }
}