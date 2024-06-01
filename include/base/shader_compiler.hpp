#pragma once

#include <vulkan/vulkan.h>

#include <vector>
#include <memory>
#include <map>

#include <filesystem>

#include <glslang/Include/glslang_c_interface.h>

namespace sample_vk::shader
{
    enum class Type
    {
        undefined,

        compute,

        raygen,
        intersection,
        anyhit,
        closesthit,
        miss,
        callable
    };

    class Compiler final
    {
    private:
        Compiler() = default;

        Compiler(Compiler&& compiler)       = delete;
        Compiler(const Compiler& compiler)  = delete;

        Compiler& operator = (Compiler&& compiler)      = delete;
        Compiler& operator = (const Compiler& compiler) = delete;

        [[nodiscard]]
        static std::vector<uint32_t> createIL(const std::filesystem::path& filename, Type type);

    public:
        static void init();
        static void finalize() noexcept;

        [[nodiscard]]
        static VkShaderModule createShaderModule(
            VkDevice                        device_handle, 
            const std::filesystem::path&    filename, 
            Type                            type
        );

    private:
        static bool _is_init;
    };
}