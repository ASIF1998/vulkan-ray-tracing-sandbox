#include <base/vulkan/utils.hpp>

#include <base/logger/logger.hpp>

#include <base/vulkan/context.hpp>

namespace sample_vk
{
    const std::map<VkFormat, size_t> VkUtils::_vk_formats_sizes_table
    {
        {VK_FORMAT_UNDEFINED,                   0},
        {VK_FORMAT_R4G4_UNORM_PACK8,            1},
        {VK_FORMAT_R4G4B4A4_UNORM_PACK16,       2},
        {VK_FORMAT_B4G4R4A4_UNORM_PACK16,       2},
        {VK_FORMAT_R5G6B5_UNORM_PACK16,         2},
        {VK_FORMAT_B5G6R5_UNORM_PACK16,         2},
        {VK_FORMAT_R5G5B5A1_UNORM_PACK16,       2},
        {VK_FORMAT_B5G5R5A1_UNORM_PACK16,       2},
        {VK_FORMAT_A1R5G5B5_UNORM_PACK16,       2},
        {VK_FORMAT_R8_UNORM,                    1},
        {VK_FORMAT_R8_SNORM,                    1},
        {VK_FORMAT_R8_USCALED,                  1},
        {VK_FORMAT_R8_SSCALED,                  1},
        {VK_FORMAT_R8_UINT,                     1},
        {VK_FORMAT_R8_SINT,                     1},
        {VK_FORMAT_R8_SRGB,                     1},
        {VK_FORMAT_R8G8_UNORM,                  2},
        {VK_FORMAT_R8G8_SNORM,                  2},
        {VK_FORMAT_R8G8_USCALED,                2},
        {VK_FORMAT_R8G8_SSCALED,                2},
        {VK_FORMAT_R8G8_UINT,                   2},
        {VK_FORMAT_R8G8_SINT,                   2},
        {VK_FORMAT_R8G8_SRGB,                   2},
        {VK_FORMAT_R8G8B8_UNORM,                3},
        {VK_FORMAT_R8G8B8_SNORM,                3},
        {VK_FORMAT_R8G8B8_USCALED,              3},
        {VK_FORMAT_R8G8B8_SSCALED,              3},
        {VK_FORMAT_R8G8B8_UINT,                 3},
        {VK_FORMAT_R8G8B8_SINT,                 3},
        {VK_FORMAT_R8G8B8_SRGB,                 3},
        {VK_FORMAT_B8G8R8_UNORM,                3},
        {VK_FORMAT_B8G8R8_SNORM,                3},
        {VK_FORMAT_B8G8R8_USCALED,              3},
        {VK_FORMAT_B8G8R8_SSCALED,              3},
        {VK_FORMAT_B8G8R8_UINT,                 3},
        {VK_FORMAT_B8G8R8_SINT,                 3},
        {VK_FORMAT_B8G8R8_SRGB,                 3},
        {VK_FORMAT_R8G8B8A8_UNORM,              4},
        {VK_FORMAT_R8G8B8A8_SNORM,              4},
        {VK_FORMAT_R8G8B8A8_USCALED,            4},
        {VK_FORMAT_R8G8B8A8_SSCALED,            4},
        {VK_FORMAT_R8G8B8A8_UINT,               4},
        {VK_FORMAT_R8G8B8A8_SINT,               4},
        {VK_FORMAT_R8G8B8A8_SRGB,               4},
        {VK_FORMAT_B8G8R8A8_UNORM,              4},
        {VK_FORMAT_B8G8R8A8_SNORM,              4},
        {VK_FORMAT_B8G8R8A8_USCALED,            4},
        {VK_FORMAT_B8G8R8A8_SSCALED,            4},
        {VK_FORMAT_B8G8R8A8_UINT,               4},
        {VK_FORMAT_B8G8R8A8_SINT,               4},
        {VK_FORMAT_B8G8R8A8_SRGB,               4},
        {VK_FORMAT_A8B8G8R8_UNORM_PACK32,       4},
        {VK_FORMAT_A8B8G8R8_SNORM_PACK32,       4},
        {VK_FORMAT_A8B8G8R8_USCALED_PACK32,     4},
        {VK_FORMAT_A8B8G8R8_SSCALED_PACK32,     4},
        {VK_FORMAT_A8B8G8R8_UINT_PACK32,        4},
        {VK_FORMAT_A8B8G8R8_SINT_PACK32,        4},
        {VK_FORMAT_A8B8G8R8_SRGB_PACK32,        4},
        {VK_FORMAT_A2R10G10B10_UNORM_PACK32,    4},
        {VK_FORMAT_A2R10G10B10_SNORM_PACK32,    4},
        {VK_FORMAT_A2R10G10B10_USCALED_PACK32,  4},
        {VK_FORMAT_A2R10G10B10_SSCALED_PACK32,  4},
        {VK_FORMAT_A2R10G10B10_UINT_PACK32,     4},
        {VK_FORMAT_A2R10G10B10_SINT_PACK32,     4},
        {VK_FORMAT_A2B10G10R10_UNORM_PACK32,    4},
        {VK_FORMAT_A2B10G10R10_SNORM_PACK32,    4},
        {VK_FORMAT_A2B10G10R10_USCALED_PACK32,  4},
        {VK_FORMAT_A2B10G10R10_SSCALED_PACK32,  4},
        {VK_FORMAT_A2B10G10R10_UINT_PACK32,     4},
        {VK_FORMAT_A2B10G10R10_SINT_PACK32,     4},
        {VK_FORMAT_R16_UNORM,                   2},
        {VK_FORMAT_R16_SNORM,                   2},
        {VK_FORMAT_R16_USCALED,                 2},
        {VK_FORMAT_R16_SSCALED,                 2},
        {VK_FORMAT_R16_UINT,                    2},
        {VK_FORMAT_R16_SINT,                    2},
        {VK_FORMAT_R16_SFLOAT,                  2},
        {VK_FORMAT_R16G16_UNORM,                4},
        {VK_FORMAT_R16G16_SNORM,                4},
        {VK_FORMAT_R16G16_USCALED,              4},
        {VK_FORMAT_R16G16_SSCALED,              4},
        {VK_FORMAT_R16G16_UINT,                 4},
        {VK_FORMAT_R16G16_SINT,                 4},
        {VK_FORMAT_R16G16_SFLOAT,               4},
        {VK_FORMAT_R16G16B16_UNORM,             6},
        {VK_FORMAT_R16G16B16_SNORM,             6},
        {VK_FORMAT_R16G16B16_USCALED,           6},
        {VK_FORMAT_R16G16B16_SSCALED,           6},
        {VK_FORMAT_R16G16B16_UINT,              6},
        {VK_FORMAT_R16G16B16_SINT,              6},
        {VK_FORMAT_R16G16B16_SFLOAT,            6},
        {VK_FORMAT_R16G16B16A16_UNORM,          8},
        {VK_FORMAT_R16G16B16A16_SNORM,          8},
        {VK_FORMAT_R16G16B16A16_USCALED,        8},
        {VK_FORMAT_R16G16B16A16_SSCALED,        8},
        {VK_FORMAT_R16G16B16A16_UINT,           8},
        {VK_FORMAT_R16G16B16A16_SINT,           8},
        {VK_FORMAT_R16G16B16A16_SFLOAT,         8},
        {VK_FORMAT_R32_UINT,                    4},
        {VK_FORMAT_R32_SINT,                    4},
        {VK_FORMAT_R32_SFLOAT,                  4},
        {VK_FORMAT_R32G32_UINT,                 8},
        {VK_FORMAT_R32G32_SINT,                 8},
        {VK_FORMAT_R32G32_SFLOAT,               8},
        {VK_FORMAT_R32G32B32_UINT,              12},
        {VK_FORMAT_R32G32B32_SINT,              12},
        {VK_FORMAT_R32G32B32_SFLOAT,            12},
        {VK_FORMAT_R32G32B32A32_UINT,           16},
        {VK_FORMAT_R32G32B32A32_SINT,           16},
        {VK_FORMAT_R32G32B32A32_SFLOAT,         16},
        {VK_FORMAT_R64_UINT,                    8},
        {VK_FORMAT_R64_SINT,                    8},
        {VK_FORMAT_R64_SFLOAT,                  8},
        {VK_FORMAT_R64G64_UINT,                 16},
        {VK_FORMAT_R64G64_SINT,                 16},
        {VK_FORMAT_R64G64_SFLOAT,               16},
        {VK_FORMAT_R64G64B64_UINT,              24},
        {VK_FORMAT_R64G64B64_SINT,              24},
        {VK_FORMAT_R64G64B64_SFLOAT,            24},
        {VK_FORMAT_R64G64B64A64_UINT,           32},
        {VK_FORMAT_R64G64B64A64_SINT,           32},
        {VK_FORMAT_R64G64B64A64_SFLOAT,         32},
        {VK_FORMAT_B10G11R11_UFLOAT_PACK32,     4},
        {VK_FORMAT_E5B9G9R9_UFLOAT_PACK32,      4},
        {VK_FORMAT_D16_UNORM,                   2},
        {VK_FORMAT_X8_D24_UNORM_PACK32,         4},
        {VK_FORMAT_D32_SFLOAT,                  4},
        {VK_FORMAT_S8_UINT,                     1},
        {VK_FORMAT_D16_UNORM_S8_UINT,           3},
        {VK_FORMAT_D24_UNORM_S8_UINT,           4},
        {VK_FORMAT_D32_SFLOAT_S8_UINT,          8},
        {VK_FORMAT_BC1_RGB_UNORM_BLOCK,         8},
        {VK_FORMAT_BC1_RGB_SRGB_BLOCK,          8},
        {VK_FORMAT_BC1_RGBA_UNORM_BLOCK,        8},
        {VK_FORMAT_BC1_RGBA_SRGB_BLOCK,         8},
        {VK_FORMAT_BC2_UNORM_BLOCK,             16},
        {VK_FORMAT_BC2_SRGB_BLOCK,              16},
        {VK_FORMAT_BC3_UNORM_BLOCK,             16},
        {VK_FORMAT_BC3_SRGB_BLOCK,              16},
        {VK_FORMAT_BC4_UNORM_BLOCK,             8},
        {VK_FORMAT_BC4_SNORM_BLOCK,             8},
        {VK_FORMAT_BC5_UNORM_BLOCK,             16},
        {VK_FORMAT_BC5_SNORM_BLOCK,             16},
        {VK_FORMAT_BC6H_UFLOAT_BLOCK,           16},
        {VK_FORMAT_BC6H_SFLOAT_BLOCK,           16},
        {VK_FORMAT_BC7_UNORM_BLOCK,             16},
        {VK_FORMAT_BC7_SRGB_BLOCK,              16},
        {VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK,     8},
        {VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK,      8},
        {VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK,   8},
        {VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK,    8},
        {VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK,   16},
        {VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK,    16},
        {VK_FORMAT_EAC_R11_UNORM_BLOCK,         8},
        {VK_FORMAT_EAC_R11_SNORM_BLOCK,         8},
        {VK_FORMAT_EAC_R11G11_UNORM_BLOCK,      16},
        {VK_FORMAT_EAC_R11G11_SNORM_BLOCK,      16},
        {VK_FORMAT_ASTC_4x4_UNORM_BLOCK,        16},
        {VK_FORMAT_ASTC_4x4_SRGB_BLOCK,         16},
        {VK_FORMAT_ASTC_5x4_UNORM_BLOCK,        16},
        {VK_FORMAT_ASTC_5x4_SRGB_BLOCK,         16},
        {VK_FORMAT_ASTC_5x5_UNORM_BLOCK,        16},
        {VK_FORMAT_ASTC_5x5_SRGB_BLOCK,         16},
        {VK_FORMAT_ASTC_6x5_UNORM_BLOCK,        16},
        {VK_FORMAT_ASTC_6x5_SRGB_BLOCK,         16},
        {VK_FORMAT_ASTC_6x6_UNORM_BLOCK,        16},
        {VK_FORMAT_ASTC_6x6_SRGB_BLOCK,         16},
        {VK_FORMAT_ASTC_8x5_UNORM_BLOCK,        16},
        {VK_FORMAT_ASTC_8x5_SRGB_BLOCK,         16},
        {VK_FORMAT_ASTC_8x6_UNORM_BLOCK,        16},
        {VK_FORMAT_ASTC_8x6_SRGB_BLOCK,         16},
        {VK_FORMAT_ASTC_8x8_UNORM_BLOCK,        16},
        {VK_FORMAT_ASTC_8x8_SRGB_BLOCK,         16},
        {VK_FORMAT_ASTC_10x5_UNORM_BLOCK,       16},
        {VK_FORMAT_ASTC_10x5_SRGB_BLOCK,        16},
        {VK_FORMAT_ASTC_10x6_UNORM_BLOCK,       16},
        {VK_FORMAT_ASTC_10x6_SRGB_BLOCK,        16},
        {VK_FORMAT_ASTC_10x8_UNORM_BLOCK,       16},
        {VK_FORMAT_ASTC_10x8_SRGB_BLOCK,        16},
        {VK_FORMAT_ASTC_10x10_UNORM_BLOCK,      16},
        {VK_FORMAT_ASTC_10x10_SRGB_BLOCK,       16},
        {VK_FORMAT_ASTC_12x10_UNORM_BLOCK,      16},
        {VK_FORMAT_ASTC_12x10_SRGB_BLOCK,       16},
        {VK_FORMAT_ASTC_12x12_UNORM_BLOCK,      16},
        {VK_FORMAT_ASTC_12x12_SRGB_BLOCK,       16},
        {VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG, 8},
        {VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG, 8},
        {VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG, 8},
        {VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG, 8},
        {VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG,  8},
        {VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG,  8},
        {VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG,  8},
        {VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG,  8},

        // KHR_sampler_YCbCr_conversion extension - single-plane variants
        // 'PACK' formats are normal, uncompressed
        {VK_FORMAT_R10X6_UNORM_PACK16,                          2},
        {VK_FORMAT_R10X6G10X6_UNORM_2PACK16,                    4},
        {VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16,          8},
        {VK_FORMAT_R12X4_UNORM_PACK16,                          2},
        {VK_FORMAT_R12X4G12X4_UNORM_2PACK16,                    4},
        {VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16,          8},
        // _422 formats encode 2 texels per entry with B, R components shared - treated as compressed w/ 2x1 block size
        {VK_FORMAT_G8B8G8R8_422_UNORM,                          4},
        {VK_FORMAT_B8G8R8G8_422_UNORM,                          4},
        {VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16,      8},
        {VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16,      8},
        {VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16,      8},
        {VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16,      8},
        {VK_FORMAT_G16B16G16R16_422_UNORM,                      8},
        {VK_FORMAT_B16G16R16G16_422_UNORM,                      8},
        // KHR_sampler_YCbCr_conversion extension - multi-plane variants
        // Formats that 'share' components among texels (_420 and _422), size represents total bytes for the smallest possible texel block
        // _420 share B, R components within a 2x2 texel block
        {VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM,                   6},
        {VK_FORMAT_G8_B8R8_2PLANE_420_UNORM,                    6},
        {VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16,  12},
        {VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16,   12},
        {VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16,  12},
        {VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16,   12},
        {VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM,                12},
        {VK_FORMAT_G16_B16R16_2PLANE_420_UNORM,                 12},
        // _422 share B, R components within a 2x1 texel block
        {VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM,                   4},
        {VK_FORMAT_G8_B8R8_2PLANE_422_UNORM,                    4},
        {VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16,  8},
        {VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16,   8},
        {VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16,  8},
        {VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16,   8},
        {VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM,                8},
        {VK_FORMAT_G16_B16R16_2PLANE_422_UNORM,                 8},
        // _444 do not share
        {VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM,                   3},
        {VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16,  6},
        {VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16,  6},
        {VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM,                6}
    };
}

namespace sample_vk
{
    void VkUtils::init(const Context* ptr_context)
    {
        if (!ptr_context)
            log::error("[VkUtils]: ptr_context is null.");

        if (!_vulkan_function_pointer_table)    
            _vulkan_function_pointer_table = std::make_optional<VkFunctionPointerTable>(ptr_context->device_handle);
    }

    VKAPI_ATTR VkBool32 VKAPI_CALL VkUtils::debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT      message_severity,
        VkDebugUtilsMessageTypeFlagsEXT             message_type,
        const VkDebugUtilsMessengerCallbackDataEXT* ptr_callback_data,
        void*                                       ptr_user_data
    )
    {
        if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
            log::warning("[Vulkan][{}]: {}", ptr_callback_data->pMessageIdName, ptr_callback_data->pMessage);
        else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
            log::warning("[Vulkan][{}]: {}", ptr_callback_data->pMessageIdName, ptr_callback_data->pMessage);
        else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT || message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
            log::info("[Vulkan][{}]: {}", ptr_callback_data->pMessageIdName, ptr_callback_data->pMessage);

        return VK_FALSE;
    }
}

namespace sample_vk
{
    CommandBuffer VkUtils::getCommandBuffer(const Context* ptr_context)
    {
        VkCommandBuffer command_buffer_handle = VK_NULL_HANDLE;

        VkCommandBufferAllocateInfo allocate_info = { };
        allocate_info.sType                 = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocate_info.level                 = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocate_info.commandBufferCount    = 1;
        allocate_info.commandPool           = ptr_context->command_pool_handle;

        VK_CHECK(
            vkAllocateCommandBuffers(
                ptr_context->device_handle, 
                &allocate_info, 
                &command_buffer_handle
            )
        );

        return CommandBuffer(ptr_context, command_buffer_handle);       
    }

    size_t VkUtils::getFormatSize(VkFormat format)
    {
        return VkUtils::_vk_formats_sizes_table.at(format);
    }
}

namespace sample_vk
{
    uint32_t VkUtils::getAlignedSize(VkDeviceSize size, VkDeviceSize aligned)
    {
        return static_cast<uint32_t>((size + (aligned - 1)) & ~(aligned - 1));
    }
}

namespace sample_vk
{
    std::optional<VkFunctionPointerTable> VkUtils::_vulkan_function_pointer_table = std::nullopt;

    const VkFunctionPointerTable& VkUtils::getVulkanFunctionPointerTable()
    {
        if (!_vulkan_function_pointer_table)
            log::error("[VkUtils]: Vulkan function pointer table not initialized.");

        return _vulkan_function_pointer_table.value();
    }
}

namespace sample_vk
{
    uint32_t VkUtils::getMipLevelsCount(uint32_t width, uint32_t height)
    {
        assert(width > 0 && height > 0);
        return static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)) + 1) + 0.5);
    }
}