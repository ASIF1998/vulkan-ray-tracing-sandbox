#pragma once

#include <base/vulkan/function_pointer_table.hpp>
#include <base/vulkan/command_buffer.hpp>

#include <base/logger/logger.hpp>

#include <base/math.hpp>

#include <vulkan/vulkan.h>

#include <concepts>

#include <string_view>

#include <stdexcept>

#include <map>

#include <optional>

#define VK_CHECK(fn)                            \
    do {                                        \
        if (auto res = fn; res != VK_SUCCESS)   \
            log::vkError(#fn);                  \
    } while (false)

#define SDL_CHECK(fn)                       \
    do {                                    \
        if (auto res = fn; res != SDL_TRUE) \
            log::appError(#fn);             \
    } while (false)

namespace sample_vk
{
    struct Context;
}

namespace sample_vk
{
    template<typename T>
    concept IsVulkanWrapper = requires(T t)
    {
        t.vk_handle;
    };

    template<typename T>
    concept IsVulkanHandle =
    (
        std::is_same_v<VkImage, T>                       ||
        std::is_same_v<VkImageView, T>                   ||
        std::is_same_v<VkBuffer, T>                      ||
        std::is_same_v<VkShaderModule, T>                ||
        std::is_same_v<VkAccelerationStructureKHR, T>
    );

    class VkUtils
    {
    public:
        VkUtils() = delete;

        static void init(const Context* ptr_context);

        static void setName(
            VkDevice                    device_handle, 
            const IsVulkanWrapper auto& wrapper, 
            VkObjectType                type, 
            const std::string_view      name
        );

        static void setName(
            VkDevice                device_handle, 
            IsVulkanHandle auto     handle, 
            VkObjectType            type, 
            const std::string_view  name
        );

        static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT      message_severity,
            VkDebugUtilsMessageTypeFlagsEXT             message_type,
            const VkDebugUtilsMessengerCallbackDataEXT* ptr_callback_data,
            void*                                       ptr_user_data
        );

        [[nodiscard]]
        static CommandBuffer getCommandBuffer(const Context* ptr_context);

        [[nodiscard]]
        static constexpr auto getVulkanIdentityMatrix()
        {
            constexpr VkTransformMatrixKHR matrix =
            {
                1, 0, 0, 0,
                0, 1, 0, 0,
                0, 0, 1, 0
            };

            return matrix;
        }

        [[nodiscard]]
        static auto cast(const glm::mat4& mat)
        {
            VkTransformMatrixKHR matrix = { };
            memcpy(&matrix, &mat, sizeof(VkTransformMatrixKHR));

            return matrix;
        }

        [[nodiscard]]
        static size_t getFormatSize(VkFormat format);

        [[nodiscard]]
        static uint32_t getAlignedSize(uint32_t size, uint32_t aligned);

        [[nodiscard]]
        static const VkFunctionPointerTable& getVulkanFunctionPointerTable();

    private:
        static std::optional<VkFunctionPointerTable> _vulkan_function_pointer_table;

        static const std::map<VkFormat, size_t> _vk_formats_sizes_table;
    };
}

#include "utils.inl"