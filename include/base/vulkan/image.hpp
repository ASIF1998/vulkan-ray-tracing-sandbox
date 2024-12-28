#pragma once

#include <vulkan/vulkan.h>

#include <numeric>

#include <filesystem>

#include <base/math.hpp>

namespace vrts
{
    struct Context;
}

namespace vrts
{
    struct ImageWriteData
    {
        uint8_t*        ptr_data    = nullptr;
        int             width       = 0;
        int             height      = 0;
        VkFormat        format      = VK_FORMAT_UNDEFINED;
    };

    class Image
    {
    public:
        class Builder;
        class Loader;
        class Decoder;

    public:
        Image(const Context* ptr_context);

        Image(Image&& image);
        Image(const Image& image) = delete;

        ~Image();

        Image& operator  = (Image&& image);
        Image& operator  = (const Image& image) = delete;

        [[nodiscard]] bool isInit() const noexcept;

        void writeData(const ImageWriteData& write_data);

        static void init() noexcept;

    public:
        VkImage     vk_handle   = VK_NULL_HANDLE;
        VkImageView view_handle = VK_NULL_HANDLE;
        VkFormat    format      = VK_FORMAT_UNDEFINED;

        VkSampler sampler_handle = VK_NULL_HANDLE;

        uint32_t level_count = 1;
        uint32_t layer_count = 1;

    private:
        const Context*  _ptr_context    = nullptr;
        VkDeviceMemory  _memory_handle   = VK_NULL_HANDLE; 
    };

    class Image::Builder
    {
        void changeUndefinedToGeneralLayout(
            Image&          image, 
            VkCommandPool   command_pool_handle, 
            VkQueue         queue_handle
        );

        void validate() const;

    public:
        Builder(const Context* ptr_context) noexcept;

        Builder(Builder&& builder)      = delete;
        Builder(const Builder& builder) = delete;

        Builder& operator = (Builder&& builder)         = delete;
        Builder& operator = (const Builder& builder)    = delete;

        Builder& vkFormat(VkFormat format);
        Builder& vkFilter(VkFilter filter);
        Builder& size(uint32_t width, uint32_t height);
        Builder& generateMipmap(bool is_generate);
        Builder& fillColor(const glm::vec3& fill_color);

        Image build();

    private:
        const Context* _ptr_context = nullptr;

        VkFormat _format    = VK_FORMAT_UNDEFINED;
        VkFilter _filter    = VK_FILTER_MAX_ENUM;

        uint32_t _width   = 0;
        uint32_t _height  = 0;

        bool _generate_mipmap = false;

        std::optional<glm::vec3> _fill_color;
    };

    class Image::Loader
    {
        void validate() const;

    public:
        Loader(const Context* ptr_context) noexcept;

        Loader(Loader&& loader)         = delete;
        Loader(const Loader& loader)    = delete;

        Loader& operator = (Loader&& loader)        = delete;
        Loader& operator = (const Loader& loader)   = delete;

        Loader& filename(const std::filesystem::path& filename);

        Image load();

    private:
        std::filesystem::path _filename;

        const Context* _ptr_context = nullptr;

        VkFilter _filter_for_mipmap = VK_FILTER_MAX_ENUM;
    };

    class Image::Decoder
    {
        void validate() const;

    public:
        Decoder(const Context* ptr_context) noexcept;

        Decoder(Decoder&& decoder)        = delete;
        Decoder(const Decoder& decoder)   = delete;

        Decoder& operator = (Decoder&& decoder)       = delete;
        Decoder& operator = (const Decoder& decoder)  = delete;

        Decoder& vkFilter(VkFilter filter) noexcept;        
        Decoder& channelsPerPixel(int32_t channels_per_pixel);        
        Decoder& compressedImage(const uint8_t* ptr_data, size_t size);

        Image decode();

    private:
        const Context* _ptr_context = nullptr;

        VkFilter _filter_for_mipmap = VK_FILTER_MAX_ENUM;

        int32_t _channels_per_pixel = 4;

        struct
        {
            const uint8_t* ptr_data = nullptr;
            size_t size             = 0;
        } _compressed_image;
         
    };
}