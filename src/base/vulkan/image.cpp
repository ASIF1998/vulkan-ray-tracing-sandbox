#include <base/vulkan/image.hpp>
#include <base/vulkan/memory.hpp>
#include <base/vulkan/buffer.hpp>
#include <base/vulkan/utils.hpp>

#include <base/private/stb.hpp>

#include <base/logger/logger.hpp>

#include <map>

#include <ranges>

#include <stdexcept>

namespace vrts
{
    Image::Image(const Context* ptr_context) :
        _ptr_context (ptr_context)
    { 
        if (!ptr_context)
            log::error("[Image]: ptr_context is null.");
    }
    
    Image::Image(Image&& image) :
        format          (image.format),
        level_count     (image.level_count),
        layer_count     (image.layer_count),
        _ptr_context    (image._ptr_context)
    {
        std::swap(vk_handle, image.vk_handle);
        std::swap(view_handle, image.view_handle);
        std::swap(sampler_handle, image.sampler_handle);
        std::swap(_memory_handle, image._memory_handle);
    }

    Image::~Image()
    {
        if (isInit())
        {
            if (sampler_handle != VK_NULL_HANDLE)
                vkDestroySampler(_ptr_context->device_handle, sampler_handle, nullptr);

            vkDestroyImageView(_ptr_context->device_handle, view_handle, nullptr);
            vkDestroyImage(_ptr_context->device_handle, vk_handle, nullptr);
            vkFreeMemory(_ptr_context->device_handle, _memory_handle, nullptr);
        }
    }

    Image& Image::operator = (Image&& image)
    {
        format      = image.format;
        level_count = image.level_count;
        layer_count = image.layer_count;

        std::swap(vk_handle, image.vk_handle);
        std::swap(view_handle, image.view_handle);
        std::swap(sampler_handle, image.sampler_handle);
        std::swap(_ptr_context, image._ptr_context);
        std::swap(_memory_handle, image._memory_handle);

        return *this;
    }

    bool Image::isInit() const noexcept
    {
        return 
                vk_handle != VK_NULL_HANDLE 
            &&  view_handle != VK_NULL_HANDLE 
            &&  _memory_handle != VK_NULL_HANDLE;
    }

    void Image::writeData(const ImageWriteData& write_data)
    {
        const size_t pixel_format_size    = VkUtils::getFormatSize(write_data.format);
        const size_t scanline_size        = write_data.width * pixel_format_size; 
        const size_t image_size           = write_data.height * scanline_size;

        auto temp_buffer = Buffer::Builder(_ptr_context)
            .vkSize(image_size)
            .vkUsage(VK_BUFFER_USAGE_TRANSFER_SRC_BIT)
            .isHostVisible(true)
            .name("[ASBuilder] Identity matrix for AS build")
            .build();

        {
            void* ptr_temp_buffer_memory = nullptr;

            VK_CHECK(
                vkMapMemory(
                    _ptr_context->device_handle,
                    temp_buffer.memory_handle,
                    0, image_size,
                    0,
                    &ptr_temp_buffer_memory
                )
            );

            memcpy(ptr_temp_buffer_memory, write_data.ptr_data, image_size);

            vkUnmapMemory(_ptr_context->device_handle, temp_buffer.memory_handle);
        }

        auto command_buffer = VkUtils::getCommandBuffer(_ptr_context);

        command_buffer.write([this, &temp_buffer, &write_data] (VkCommandBuffer command_buffer_handle)
        {
            constexpr VkImageSubresourceLayers subresource 
            { 
                .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel       = 0,
                .baseArrayLayer = 0,
                .layerCount     = 1
            };

            const VkBufferImageCopy2 region 
            { 
                .sType                = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2,
                .bufferOffset         = 0,
                .bufferRowLength      = static_cast<uint32_t>(write_data.width), // scanline_size
                .bufferImageHeight    = static_cast<uint32_t>(write_data.height),
                .imageSubresource     = subresource,
                .imageOffset          = { },
                .imageExtent          = {static_cast<uint32_t>(write_data.width), static_cast<uint32_t>(write_data.height), 1}
            };

            const VkCopyBufferToImageInfo2 copy_info 
            { 
                .sType             = VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2,
                .srcBuffer         = temp_buffer.vk_handle,
                .dstImage          = vk_handle,
                .dstImageLayout    = VK_IMAGE_LAYOUT_GENERAL,
                .regionCount       = 1,
                .pRegions          = &region
            };

            vkCmdCopyBufferToImage2(command_buffer_handle, &copy_info);
        }, "Write data in image", GpuMarkerColors::write_data_in_image);

        command_buffer.upload(_ptr_context);
    }

    void Image::init() noexcept
    {
        stbi_set_flip_vertically_on_load(true);
    }
}

namespace vrts
{
    struct ImageUtils
    {
        static void generateMipmap(
            Image&              image, 
            const Context*      ptr_context,
            const glm::ivec2&   size, 
            VkFilter            filter
        )
        {
            for (auto mip_level: std::views::iota(1u, image.level_count))
            {
                auto command_buffer = VkUtils::getCommandBuffer(ptr_context);

                auto src_width  = std::max(size.x >> (mip_level - 1), 1);
                auto src_height = std::max(size.y >> (mip_level - 1), 1);
                
                auto dst_width  = std::max(size.x >> mip_level, 1);
                auto dst_height = std::max(size.y >> mip_level, 1);

                const VkImageSubresourceLayers src_subresource 
                { 
                    .aspectMask      = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel        = mip_level - 1,
                    .baseArrayLayer  = 0,
                    .layerCount      = 1
                };

                const VkImageSubresourceLayers dst_subresource 
                { 
                    .aspectMask      = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel        = mip_level,
                    .baseArrayLayer  = 0,
                    .layerCount      = 1
                };

                const VkImageBlit2 region 
                { 
                    .sType          = VK_STRUCTURE_TYPE_IMAGE_BLIT_2,
                    .srcSubresource = src_subresource,
                    .srcOffsets     = {{}, {src_width, src_height, 1}},
                    .dstSubresource = dst_subresource,
                    .dstOffsets     = {{}, {dst_width, dst_height, 1}}
                };

                const VkBlitImageInfo2 blit_info 
                { 
                    .sType             = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2,
                    .srcImage          = image.vk_handle,
                    .srcImageLayout    = VK_IMAGE_LAYOUT_GENERAL,
                    .dstImage          = image.vk_handle,
                    .dstImageLayout    = VK_IMAGE_LAYOUT_GENERAL,
                    .regionCount       = 1,
                    .pRegions          = &region,
                    .filter            = filter
                };

                command_buffer.write([&blit_info] (VkCommandBuffer command_buffer_handle)
                {
                    vkCmdBlitImage2(command_buffer_handle, &blit_info);
                }, std::format("Create mipmap for level = {}", mip_level - 1), GpuMarkerColors::create_mipmap);

                command_buffer.upload(ptr_context);
            }
        }
    };
}

namespace vrts
{
    Image::Builder::Builder(const Context* ptr_context) noexcept :
        _ptr_context (ptr_context)
    { }

    Image::Builder& Image::Builder::vkFormat(VkFormat format)
    {
        _format = format;

        return *this;
    }

    Image::Builder& Image::Builder::size(uint32_t width, uint32_t height)
    {
        _width  = width;
        _height = height;

        return *this;
    }

    Image::Builder& Image::Builder::generateMipmap(bool is_generate)
    {
        _generate_mipmap = is_generate;
        return *this;
    }

    Image::Builder& Image::Builder::vkFilter(VkFilter filter)
    {
        _filter = filter;

        return *this;
    }

    Image::Builder& Image::Builder::fillColor(const glm::vec3& fill_color)
    {
        _fill_color = std::make_optional(fill_color);
        return *this;
    }

    void Image::Builder::changeUndefinedToGeneralLayout(
        Image&          image, 
        VkCommandPool   command_pool_handle, 
        VkQueue         queue_handle
    )
    {
        auto command_buffer = VkUtils::getCommandBuffer(_ptr_context);

        command_buffer.write([this, &image] (VkCommandBuffer command_buffer_handle)
        {
            const VkImageSubresourceRange subresource 
            { 
                .aspectMask      = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel    = 0,
                .levelCount      = image.level_count,
                .baseArrayLayer  = 0,
                .layerCount      = image.layer_count
            };

            const VkImageMemoryBarrier2 image_memory_barrier 
            { 
                .sType                  = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .srcStageMask           = VK_PIPELINE_STAGE_2_NONE,
                .srcAccessMask          = VK_ACCESS_2_NONE,
                .dstStageMask           = VK_PIPELINE_STAGE_2_NONE,
                .dstAccessMask          = VK_ACCESS_2_NONE,
                .oldLayout              = VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout              = VK_IMAGE_LAYOUT_GENERAL,
                .srcQueueFamilyIndex    = _ptr_context->queue.family_index,
                .dstQueueFamilyIndex    = _ptr_context->queue.family_index,
                .image                  = image.vk_handle,
                .subresourceRange       = subresource
            };

            const VkDependencyInfo dependency_info 
            { 
                .sType                      = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                .imageMemoryBarrierCount    = 1,
                .pImageMemoryBarriers       = &image_memory_barrier
            };

            vkCmdPipelineBarrier2(command_buffer_handle, &dependency_info);
        }, "Change image layout from undefined to general");

        command_buffer.upload(_ptr_context);
    }

    void Image::Builder::validate() const
    {
        if (!_ptr_context)
            log::error("[Image::Builder] not driver.");

        if (_format == VK_FORMAT_UNDEFINED)
            log::error("[Image::Builder] format is undefined.");

        if (!_width || !_height)
            log::error("[Image::Builder] image size is 0.");
    }

    Image Image::Builder::build()
    {
        validate();
        
        const VkImageCreateInfo image_create_info 
        { 
            .sType                  = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType              = VK_IMAGE_TYPE_2D,
            .format                 = _format,
            .extent                 = {_width, _height, 1},
            .mipLevels              = _generate_mipmap ? VkUtils::getMipLevelsCount(_width, _height) : 1,
            .arrayLayers            = 1,
            .samples                = VK_SAMPLE_COUNT_1_BIT,
            .tiling                 = VK_IMAGE_TILING_OPTIMAL,
            .usage                  = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            .sharingMode            = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount  = 1,
            .pQueueFamilyIndices    = &_ptr_context->queue.family_index,
            .initialLayout          = VK_IMAGE_LAYOUT_UNDEFINED
        };

        Image image (_ptr_context);
        image.level_count   = image_create_info.mipLevels;
        image.format        = _format;
        image._ptr_context  = _ptr_context;

        VK_CHECK(
            vkCreateImage(
                _ptr_context->device_handle,
                &image_create_info,
                nullptr,
                &image.vk_handle
            )
        );

        VkMemoryRequirements memory_requirements = { };

        vkGetImageMemoryRequirements(_ptr_context->device_handle, image.vk_handle, &memory_requirements);

        constexpr VkMemoryAllocateFlagsInfoKHR memory_allocation_flags 
        { 
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR,
            .flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR
        };

        const auto aligned_size = (memory_requirements.size + (memory_requirements.alignment - 1)) & ~(memory_requirements.alignment - 1);

        const VkMemoryAllocateInfo allocate_info 
        { 
            .sType              = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .pNext              = &memory_allocation_flags,
            .allocationSize     = aligned_size,
            .memoryTypeIndex    = MemoryProperties::getMemoryIndex(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
        };

        VK_CHECK(
            vkAllocateMemory(
                _ptr_context->device_handle,
                &allocate_info,
                nullptr,
                &image._memory_handle
            )
        );

        const VkBindImageMemoryInfo bind_info 
        { 
            .sType  = VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO,
            .image  = image.vk_handle,
            .memory = image._memory_handle
        };

        VK_CHECK(
            vkBindImageMemory2(
                _ptr_context->device_handle,
                1, 
                &bind_info
            )
        );

        const VkImageViewCreateInfo image_view_create_info 
        { 
            .sType              = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image              = image.vk_handle,
            .viewType           = VK_IMAGE_VIEW_TYPE_2D,
            .format             = image.format,
            .components         = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A },
            .subresourceRange   = 
            {
                .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel   = 0,
                .levelCount     = image.level_count,
                .baseArrayLayer = 0,
                .layerCount     = image.layer_count
            }
        };
        
        VK_CHECK(
            vkCreateImageView(
                _ptr_context->device_handle,
                &image_view_create_info,
                nullptr,
                &image.view_handle
            )
        );

        changeUndefinedToGeneralLayout(image, _ptr_context->command_pool_handle, _ptr_context->queue.handle);

        if (_filter != VK_FILTER_MAX_ENUM)
        {
            const VkSamplerCreateInfo sampler_create_info 
            { 
                .sType              = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
                .magFilter          = _filter,
                .minFilter          = _filter,
                .mipmapMode         = VK_SAMPLER_MIPMAP_MODE_LINEAR,
                .addressModeU       = VK_SAMPLER_ADDRESS_MODE_REPEAT,
                .addressModeV       = VK_SAMPLER_ADDRESS_MODE_REPEAT,
                .addressModeW       = VK_SAMPLER_ADDRESS_MODE_REPEAT,
                .anisotropyEnable   = VK_FALSE,
                .compareEnable      = VK_FALSE,
                .minLod             = 0,
                .maxLod             = static_cast<float>(image.level_count),
                .borderColor        = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK
            };

            VK_CHECK(
                vkCreateSampler(
                    _ptr_context->device_handle,
                    &sampler_create_info, 
                    nullptr,
                    &image.sampler_handle
                )
            );
        }

        if (_fill_color)
        {
            auto command_buffer = VkUtils::getCommandBuffer(_ptr_context);
            
            command_buffer.write([&image, this](VkCommandBuffer handle)
            {
                const VkClearColorValue clear_color 
                { 
                    .float32 = {_fill_color->r, _fill_color->g, _fill_color->b, 1.0}
                };

                const VkImageSubresourceRange range 
                { 
                    .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel   = 0,
                    .levelCount     = 1,
                    .baseArrayLayer = 0,
                    .layerCount     = 1
                };

                vkCmdClearColorImage(handle, image.vk_handle, VK_IMAGE_LAYOUT_GENERAL, &clear_color, 1, &range);
            }, "Fill image", GpuMarkerColors::write_data_in_image);

            command_buffer.upload(_ptr_context);
        }

        return image;
    }
}

namespace vrts
{
    Image::Loader::Loader(const Context* ptr_context) noexcept :
        _ptr_context (ptr_context)
    { }

    Image::Loader& Image::Loader::filename(const std::filesystem::path& filename)
    {
        _filename = filename;
        return *this;
    }

    void Image::Loader::validate() const
    {
        if (!std::filesystem::exists(_filename))
            log::error("[Image::Loader] image not found: {}.", _filename.string());

        if (!_ptr_context)
            log::error("[Image::Loader] not driver.");
    }

    Image Image::Loader::load()
    {
        validate();

        int num_channels = 0;

        ImageWriteData write_data;

        write_data.ptr_data = stbi_load(
            _filename.string().c_str(), 
            &write_data.width, &write_data.height, 
            &num_channels, 
            STBI_rgb
        );

        write_data.format = VK_FORMAT_R8G8B8_UNORM;

        if (!write_data.ptr_data)
            log::error("[Image::Loader] failed load image: {}.", _filename.string());

        auto image = Image::Builder(_ptr_context)
            .vkFormat(write_data.format)
            .size(write_data.width, write_data.height)
            .generateMipmap(_filter_for_mipmap != VK_FILTER_MAX_ENUM)
            .build();

        image.writeData(write_data);

        stbi_image_free(write_data.ptr_data); 

        if (_filter_for_mipmap != VK_FILTER_MAX_ENUM)
        {
            ImageUtils::generateMipmap(
                image, 
                _ptr_context,
                glm::ivec2(write_data.width, write_data.height),
                _filter_for_mipmap
            );
        }

        VkUtils::setName(
            _ptr_context->device_handle, 
            image, 
            VK_OBJECT_TYPE_IMAGE, 
            std::format("[Image]: {}", _filename.filename().string())
        );

        return image;
    }
}

namespace vrts
{
    Image::Decoder::Decoder(const Context* ptr_context) noexcept :
        _ptr_context (ptr_context)
    { }

    Image::Decoder& Image::Decoder::compressedImage(const uint8_t* ptr_data, size_t size)
    {
        _compressed_image.ptr_data  = ptr_data;
        _compressed_image.size      = size;
        return *this;
    }

    Image::Decoder& Image::Decoder::vkFilter(VkFilter filter) noexcept
    {
        _filter_for_mipmap = filter;
        return *this;
    }

    Image::Decoder& Image::Decoder::channelsPerPixel(int32_t channels_per_pixel)
    {
        assert(channels_per_pixel > 0 && channels_per_pixel < 5);

        _channels_per_pixel = channels_per_pixel;
        return *this; 
    }

    void Image::Decoder::validate() const
    {
        if (!_ptr_context)
            log::error("[Image::Decoder] not driver.");

        if (!_compressed_image.ptr_data || !_compressed_image.size)
            log::error("[Image::Decoder] not compressed image.");
    }

    Image Image::Decoder::decode()
    {
        validate();

        ImageWriteData write_data;

        constexpr std::array formats
        {
            VK_FORMAT_R8_UNORM,
            VK_FORMAT_R8G8_UNORM,
            VK_FORMAT_R8G8B8_UNORM,
            VK_FORMAT_R8G8B8A8_UNORM
        };

        write_data.format = formats[_channels_per_pixel - 1];

        write_data.ptr_data = stbi_load_from_memory(
            _compressed_image.ptr_data, 
            static_cast<int>(_compressed_image.size), 
            &write_data.width, &write_data.height,
            &_channels_per_pixel,
            _channels_per_pixel
        );

        auto image = Image::Builder(_ptr_context)
            .vkFormat(write_data.format)
            .size(write_data.width, write_data.height)
            .generateMipmap(_filter_for_mipmap != VK_FILTER_MAX_ENUM)
            .vkFilter(_filter_for_mipmap == VK_FILTER_MAX_ENUM ? VK_FILTER_NEAREST : _filter_for_mipmap)
            .build();

        image.writeData(write_data);

        stbi_image_free(write_data.ptr_data);

        if (_filter_for_mipmap != VK_FILTER_MAX_ENUM)
        {
            ImageUtils::generateMipmap(
                image, 
                _ptr_context, 
                glm::ivec2(write_data.width, write_data.height),
                _filter_for_mipmap
            );
        }

        return image;
    }
}