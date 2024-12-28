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
        format   = image.format;
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

    void Image::writeData(
        const ImageWriteData&   write_data
    )
    {
        size_t pixel_format_size    = VkUtils::getFormatSize(write_data.format);
        size_t scanline_size        = write_data.width * pixel_format_size; 
        size_t image_size           = write_data.height * scanline_size;

        auto memory_type_index = MemoryProperties::getMemoryIndex(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
        if (!memory_type_index.has_value())
            log::error("[Image]: Not memory index for create temp buffer.");

        auto temp_buffer = Buffer::make(
            _ptr_context,
            image_size,
            *memory_type_index,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT
        );

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
            VkImageSubresourceLayers subresource = { };
            subresource.aspectMask      = VK_IMAGE_ASPECT_COLOR_BIT;
            subresource.baseArrayLayer  = 0;
            subresource.layerCount      = 1;
            subresource.mipLevel        = 0;

            VkBufferImageCopy2 region = { };
            region.sType                = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2;
            region.bufferImageHeight    = write_data.height;
            region.bufferOffset         = 0;
            region.bufferRowLength      = write_data.width; // scanline_size;
            region.imageExtent          = {static_cast<uint32_t>(write_data.width), static_cast<uint32_t>(write_data.height), 1};
            region.imageOffset          = { };
            region.imageSubresource     = subresource;

            VkCopyBufferToImageInfo2 copy_info = { };
            copy_info.sType             = VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2;
            copy_info.srcBuffer         = temp_buffer.vk_handle;
            copy_info.dstImage          = vk_handle;
            copy_info.dstImageLayout    = VK_IMAGE_LAYOUT_GENERAL;
            copy_info.regionCount       = 1;
            copy_info.pRegions          = &region;

            vkCmdCopyBufferToImage2(command_buffer_handle, &copy_info);
        });

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

                VkImageSubresourceLayers src_subresource = { };
                src_subresource.aspectMask      = VK_IMAGE_ASPECT_COLOR_BIT;
                src_subresource.baseArrayLayer  = 0;
                src_subresource.layerCount      = 1;
                src_subresource.mipLevel        = mip_level - 1;

                VkImageSubresourceLayers dst_subresource = { };
                dst_subresource.aspectMask      = VK_IMAGE_ASPECT_COLOR_BIT;
                dst_subresource.baseArrayLayer  = 0;
                dst_subresource.layerCount      = 1;
                dst_subresource.mipLevel        = mip_level;

                VkImageBlit2 region = { };
                region.sType            = VK_STRUCTURE_TYPE_IMAGE_BLIT_2;
                region.srcSubresource   = src_subresource;
                region.srcOffsets[1]    = {src_width, src_height, 1};
                region.dstSubresource   = dst_subresource;
                region.dstOffsets[1]    = {dst_width, dst_height, 1};

                VkBlitImageInfo2 blit_info = { };
                blit_info.sType             = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2;
                blit_info.srcImage          = image.vk_handle;
                blit_info.srcImageLayout    = VK_IMAGE_LAYOUT_GENERAL;
                blit_info.dstImage          = image.vk_handle;
                blit_info.dstImageLayout    = VK_IMAGE_LAYOUT_GENERAL;
                blit_info.filter            = filter;
                blit_info.pRegions          = &region;
                blit_info.regionCount       = 1;

                command_buffer.write([&blit_info] (VkCommandBuffer command_buffer_handle)
                {
                    vkCmdBlitImage2(command_buffer_handle, &blit_info);
                });

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
            VkImageSubresourceRange subresource = { };
            subresource.aspectMask      = VK_IMAGE_ASPECT_COLOR_BIT;
            subresource.baseArrayLayer  = 0;
            subresource.layerCount      = image.layer_count;
            subresource.baseMipLevel    = 0;
            subresource.levelCount      = image.level_count; 

            VkImageMemoryBarrier2 image_memory_barrier = { };
            image_memory_barrier.sType                  = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
            image_memory_barrier.srcAccessMask          = VK_ACCESS_2_NONE;
            image_memory_barrier.dstAccessMask          = VK_ACCESS_2_NONE;
            image_memory_barrier.srcQueueFamilyIndex    = _ptr_context->queue.family_index;
            image_memory_barrier.dstQueueFamilyIndex    = _ptr_context->queue.family_index;
            image_memory_barrier.srcStageMask           = VK_PIPELINE_STAGE_2_NONE;
            image_memory_barrier.dstStageMask           = VK_PIPELINE_STAGE_2_NONE;
            image_memory_barrier.image                  = image.vk_handle;
            image_memory_barrier.oldLayout              = VK_IMAGE_LAYOUT_UNDEFINED;
            image_memory_barrier.newLayout              = VK_IMAGE_LAYOUT_GENERAL;
            image_memory_barrier.subresourceRange       = subresource;

            VkDependencyInfo dependency_info = { };
            dependency_info.sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
            dependency_info.imageMemoryBarrierCount = 1;
            dependency_info.pImageMemoryBarriers    = &image_memory_barrier;

            vkCmdPipelineBarrier2(command_buffer_handle, &dependency_info);
        });

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
        
        VkImageCreateInfo image_create_info = { };
        image_create_info.sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_create_info.usage                 = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        image_create_info.extent                = {_width, _height, 1};
        image_create_info.format                = _format;
        image_create_info.samples               = VK_SAMPLE_COUNT_1_BIT;
        image_create_info.imageType             = VK_IMAGE_TYPE_2D;
        image_create_info.initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED;
        image_create_info.arrayLayers           = 1;
        image_create_info.mipLevels             = 1;    
        image_create_info.queueFamilyIndexCount = 1;
        image_create_info.pQueueFamilyIndices   = &_ptr_context->queue.family_index;
        image_create_info.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
        image_create_info.tiling                = VK_IMAGE_TILING_OPTIMAL;

        if (_generate_mipmap)
            image_create_info.mipLevels = VkUtils::getMipLevelsCount(_width, _height);

        Image image (_ptr_context);

        image.level_count = image_create_info.mipLevels;

        image.format = _format;

        image._ptr_context = _ptr_context;

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

        VkMemoryAllocateFlagsInfoKHR memory_allocation_flags = { };
        memory_allocation_flags.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR;
        memory_allocation_flags.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;

        auto aligned_size = (memory_requirements.size + (memory_requirements.alignment - 1)) & ~(memory_requirements.alignment - 1);

        if (auto memory_index = MemoryProperties::getMemoryIndex(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
        {
            VkMemoryAllocateInfo allocate_info = { };
            allocate_info.sType             = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocate_info.pNext             = &memory_allocation_flags;
            allocate_info.memoryTypeIndex   = *memory_index;
            allocate_info.allocationSize    = aligned_size;

            VK_CHECK(
                vkAllocateMemory(
                    _ptr_context->device_handle,
                    &allocate_info,
                    nullptr,
                    &image._memory_handle
                )
            );
        }

        VkBindImageMemoryInfo bind_info = { };
        bind_info.sType     = VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO;
        bind_info.image     = image.vk_handle;
        bind_info.memory    = image._memory_handle;
        VK_CHECK(
            vkBindImageMemory2(
                _ptr_context->device_handle,
                1, 
                &bind_info
            )
        );

        VkImageViewCreateInfo image_view_create_info = { };
        image_view_create_info.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        image_view_create_info.image    = image.vk_handle;
        image_view_create_info.format   = image.format;
        image_view_create_info.components   = 
        { 
            VK_COMPONENT_SWIZZLE_R, 
            VK_COMPONENT_SWIZZLE_G, 
            VK_COMPONENT_SWIZZLE_B, 
            VK_COMPONENT_SWIZZLE_A
        };
        
        auto& subresource_range = image_view_create_info.subresourceRange;
        subresource_range.aspectMask        = VK_IMAGE_ASPECT_COLOR_BIT;
        subresource_range.baseArrayLayer    = 0;
        subresource_range.baseMipLevel      = 0;
        subresource_range.layerCount        = image.layer_count;
        subresource_range.levelCount        = image.level_count;

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
            VkSamplerCreateInfo sampler_create_info = { };
            sampler_create_info.sType               = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            sampler_create_info.addressModeU        = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            sampler_create_info.addressModeV        = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            sampler_create_info.addressModeW        = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            sampler_create_info.borderColor         = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
            sampler_create_info.compareEnable       = VK_FALSE;
            sampler_create_info.minFilter           = _filter;
            sampler_create_info.magFilter           = _filter;
            sampler_create_info.anisotropyEnable    = VK_FALSE;
            sampler_create_info.mipmapMode          = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            sampler_create_info.minLod              = 0;
            sampler_create_info.maxLod              = static_cast<float>(image.level_count);

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
                VkClearColorValue clear_color = { };
                clear_color.float32[0] = _fill_color->r;
                clear_color.float32[1] = _fill_color->g;
                clear_color.float32[2] = _fill_color->b;
                clear_color.float32[3] = 1.0f;

                VkImageSubresourceRange range = { };
                range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                range.baseArrayLayer    = 0;
                range.layerCount        = 1;
                range.baseMipLevel      = 0;
                range.levelCount        = 1;

                vkCmdClearColorImage(handle, image.vk_handle, VK_IMAGE_LAYOUT_GENERAL, &clear_color, 1, &range);
            });

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