#include <base/vulkan/buffer.hpp>

#include <base/vulkan/utils.hpp>

#include <stdexcept>

namespace vrts
{
    Buffer::Buffer(const Context* ptr_context):
        _ptr_context (ptr_context)
    { 
        if (!ptr_context)
            log::error("[Buffer]: ptr_context is null.");
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
        if (vk_handle != VK_NULL_HANDLE)
            vkDestroyBuffer(_ptr_context->device_handle, vk_handle, nullptr);

        if (memory_handle != VK_NULL_HANDLE) 
            vkFreeMemory(_ptr_context->device_handle, memory_handle, nullptr);
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
}

namespace vrts
{
    Buffer::Builder::Builder(const Context* ptr_context) :
        _ptr_context(ptr_context)
    { }

    void Buffer::Builder::validate() const
    {
        if (!_ptr_context)
            log::error("[Buffer::Builder] Context is null");

        if (_size == 0) 
            log::error("[Buffer::Builder] Size is 0");

        if (_usage_flags == VK_BUFFER_USAGE_FLAG_BITS_MAX_ENUM) 
            log::error("[Buffer::Builder] Didn't set usage flags");
    }

    Buffer::Builder& Buffer::Builder::vkSize(VkDeviceSize size)
    {
        _size = size;
        return *this;
    }

    Buffer::Builder& Buffer::Builder::vkUsage(VkBufferUsageFlags usage_flags)
    {
        _usage_flags = usage_flags;
        return *this;
    }

    Buffer::Builder& Buffer::Builder::isHostVisible(bool is_host_visible)
    {
        _is_host_visible = is_host_visible;
        return *this;
    }

    Buffer::Builder& Buffer::Builder::name(std::string_view name)
    {
        _name = name;
        return *this;
    }

    Buffer Buffer::Builder::build()
    {
        validate();

        Buffer buffer (_ptr_context);

        buffer.size_in_bytes    = _size;

        VkBufferCreateInfo buffer_create_info = { };
        buffer_create_info.sType                    = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_create_info.usage                    = _usage_flags;
        buffer_create_info.size                     = _size;
        buffer_create_info.sharingMode              = VK_SHARING_MODE_EXCLUSIVE;
        buffer_create_info.queueFamilyIndexCount    = static_cast<uint32_t>(1);
        buffer_create_info.pQueueFamilyIndices      = &_ptr_context->queue.family_index;

        VK_CHECK(
            vkCreateBuffer(
                _ptr_context->device_handle, 
                &buffer_create_info, 
                nullptr, 
                &buffer.vk_handle
            )
        );

        const auto type_index = _is_host_visible ?
                MemoryProperties::getMemoryIndex(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
            :   MemoryProperties::getMemoryIndex(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

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
                _ptr_context->device_handle, 
                &memory_allocate_info, 
                nullptr, 
                &buffer.memory_handle
            )
        );

        VK_CHECK(
            vkBindBufferMemory(
                _ptr_context->device_handle, 
                buffer.vk_handle, 
                buffer.memory_handle, 
                0
            )
        );

        VkUtils::setName(_ptr_context->device_handle, buffer, VK_OBJECT_TYPE_BUFFER, _name);

        return buffer; 
    }
}