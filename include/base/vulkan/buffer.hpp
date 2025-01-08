#pragma once

#include <vulkan/vulkan.h>

#include <vector>
#include <span>

#include <string_view>

#include <base/vulkan/command_buffer.hpp>

namespace vrts
{
    struct Context;
}

namespace vrts
{
    struct Buffer
    {
    private:
        Buffer(const Context* ptr_context);

    public:
        class Builder;

    public:
        Buffer(Buffer&& buffer);
        Buffer(const Buffer& buffer) = delete;
        ~Buffer();

        Buffer& operator = (Buffer&& buffer);
        Buffer& operator = (const Buffer& buffer) = delete;

        [[nodiscard]] VkMemoryRequirements  getMemoryRequirements() const noexcept;
        [[nodiscard]] VkDeviceAddress       getAddress()            const noexcept;

        template<typename T>
        static void writeData(Buffer& buffer, const std::span<T>  data);

        template<typename T>
        static void writeData(Buffer& buffer, const T& obj);
        
    public:
        VkBuffer        vk_handle       = VK_NULL_HANDLE;
        VkDeviceMemory  memory_handle   = VK_NULL_HANDLE;

        VkDeviceSize size_in_bytes = 0;

    private:
        const Context* _ptr_context = nullptr;
    };

    class Buffer::Builder
    {
        void validate() const;

    public:
        Builder(const Context* ptr_context);

        Builder(Builder&& builder)      = delete;
        Builder(const Builder& builder) = delete;

        Builder& operator = (Builder&& builder)         = delete;
        Builder& operator = (const Builder& builder)    = delete;

        Builder& vkSize(VkDeviceSize size);
        Builder& vkUsage(VkBufferUsageFlags usage_flags);
        Builder& isHostVisible(bool is_host_visible);
        Builder& name(std::string_view name);

        [[nodiscard]] Buffer build();

    private:
        const Context* _ptr_context;
        
        VkDeviceSize    _size               = 0;
        bool            _is_host_visible    = false;

        VkBufferUsageFlags _usage_flags = VK_BUFFER_USAGE_FLAG_BITS_MAX_ENUM;

        std::string_view  _name = "Buffer";
    };
}

#include <base/vulkan/buffer.inl>