#include <base/shader_compiler.hpp>
#include <base/vulkan/utils.hpp>

#include <base/logger/logger.hpp>

#include <glslang/Public/resource_limits_c.h>

#include <stdexcept>

#include <fstream>
#include <codecvt>

#define CHECK(fn, log_fn, log_fn_arg)                                   \
    do                                                                  \
    {                                                                   \
        if(!fn)                                                         \
            log::error("[Shader::Compiler]: {}", log_fn(log_fn_arg));   \
    } while (false)

#define CHECK_SHADER(fn)    CHECK(fn, glslang_shader_get_info_log, ptr_shader)
#define CHECK_PROGRAM(fn)   CHECK(fn, glslang_program_get_info_log, ptr_program)

namespace vrts::shader
{
    struct IncludeProcessData
    {
        std::string                             header_data;
        std::unique_ptr<glsl_include_result_t>  ptr_include_result;
    };

    class IncludeProcessUtils
    {
    public:
        static std::string getSource(const std::filesystem::path file_path)
        {
            if (!std::filesystem::exists(file_path))
                log::error("Not find file: {}", file_path.string());

            std::ifstream file(file_path);

            if (!file)
                log::error("Failed open file: {}", file_path.string());

            std::ostringstream contents;
            contents << file.rdbuf();

            return contents.str();
        }

        static glsl_include_result_t* localInclude (
            void*       ctx, 
            const char* header_name, 
            const char* includer_name,
            size_t      include_depth
        )
        {
            log::error("[Sahder Compiler]: We not processe local include files.");
            return nullptr;
        }

        static glsl_include_result_t* systemInclude (
            void*       ctx, 
            const char* header_name, 
            const char* includer_name,
            size_t      include_depth
        )
        {
            if (!header_name)
                return nullptr;

            if (auto return_value = _includes_data.find(header_name); return_value != std::end(_includes_data))
                return return_value->second.ptr_include_result.get();

            std::string key     = header_name;
            std::string code    = getSource(project_dir / header_name);

            auto ptr_include_data = std::make_unique<glsl_include_result_t>();
            ptr_include_data->header_name   = key.data();
            ptr_include_data->header_data   = code.data();
            ptr_include_data->header_length = code.size();

            auto return_value = ptr_include_data.get();

            _includes_data.insert(
                std::make_pair(
                    std::move(key), 
                    IncludeProcessData 
                    {
                        std::move(code), 
                        std::move(ptr_include_data)
                    }
                )
            );

            return return_value;
        }

        static int freeInclude(void* ctx, glsl_include_result_t* ptr_result)
        {
            if (ptr_result)
            {
                if (auto include_data = _includes_data.find(ptr_result->header_name); include_data != std::end(_includes_data))
                    _includes_data.erase(include_data);
            }

            return 0;
        }

    private:
        static std::map<std::string, IncludeProcessData> _includes_data;
    };

    std::map<std::string, IncludeProcessData> IncludeProcessUtils::_includes_data;
}

namespace vrts::shader
{
    bool Compiler::_is_init = false;

    void Compiler::init()
    {
        if (!glslang_initialize_process())
            log::error("[Sahder Compiler]: Failed initialize glslang.");
        
        _is_init = true;
    }

    void Compiler::finalize() noexcept
    {
        if (_is_init)
            glslang_finalize_process();

        _is_init = false;
    }
}

namespace vrts::shader
{
    glslang_stage_t getStage(Type type)
    {
        constexpr auto invalid_type = GLSLANG_STAGE_COUNT;

        switch(type)
        {
            case Type::compute:
                return GLSLANG_STAGE_COMPUTE;
            case Type::raygen:
                return GLSLANG_STAGE_RAYGEN;
            case Type::intersection:
                return GLSLANG_STAGE_INTERSECT;
            case Type::anyhit:
                return GLSLANG_STAGE_ANYHIT;
            case Type::closesthit:
                return GLSLANG_STAGE_CLOSESTHIT;
            case Type::miss:
                return GLSLANG_STAGE_MISS;
            case Type::callable:
                return GLSLANG_STAGE_CALLABLE;
            default:
                log::error("[Shader::Compiler]: invalid stage type");
        };

        return invalid_type;
    }

    std::vector<uint32_t> Compiler::createIL(const std::filesystem::path& filename, Type type)
    {
        auto source = IncludeProcessUtils::getSource(filename);
        auto stage  = getStage(type);

        const glsl_include_callbacks_t includer 
        { 
            .include_system         = IncludeProcessUtils::systemInclude,
            .include_local          = IncludeProcessUtils::localInclude,
            .free_include_result    = IncludeProcessUtils::freeInclude
        };

        const glslang_input_t input 
        { 
            .language                           = GLSLANG_SOURCE_GLSL,
            .stage                              = stage,
            .client                             = GLSLANG_CLIENT_VULKAN,
            .client_version                     = GLSLANG_TARGET_VULKAN_1_3,
            .target_language                    = GLSLANG_TARGET_SPV,
            .target_language_version            = GLSLANG_TARGET_SPV_1_6,
            .code                               = source.c_str(),
            .default_version                    = 100,
            .default_profile                    = GLSLANG_NO_PROFILE,
            .force_default_version_and_profile  = false,
            .forward_compatible                 = false,
            .messages                           = GLSLANG_MSG_DEFAULT_BIT,
            .resource                           = glslang_default_resource(),
            .callbacks                          = includer
        };

        auto ptr_shader = glslang_shader_create(&input);

        CHECK_SHADER(glslang_shader_preprocess(ptr_shader, &input));
        CHECK_SHADER(glslang_shader_parse(ptr_shader, &input));

        auto ptr_program = glslang_program_create();
        glslang_program_add_shader(ptr_program, ptr_shader);

        CHECK_PROGRAM(glslang_program_link(ptr_program, GLSLANG_MSG_SPV_RULES_BIT | GLSLANG_MSG_VULKAN_RULES_BIT));

        glslang_program_SPIRV_generate(ptr_program, stage);

        if (auto spirv_message = glslang_program_SPIRV_get_messages(ptr_program))
            log::error("[Sahder Compiler]: {}", spirv_message);

        std::vector<uint32_t> il (glslang_program_SPIRV_get_size(ptr_program));

        glslang_program_SPIRV_get(ptr_program, il.data());

        glslang_program_delete(ptr_program);
        glslang_shader_delete(ptr_shader);

        return il;
    }

    VkShaderModule Compiler::createShaderModule(VkDevice device_handle, const std::filesystem::path& filename, Type type)
    {
        log::info("[Shader Compiler]: Compile shader: {}", filename.filename().string());

        auto il = createIL(filename, type);

        VkShaderModule shader_module_handle = VK_NULL_HANDLE;

        const VkShaderModuleCreateInfo shader_module_create_info
        {
            .sType      = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize   = il.size() * sizeof(uint32_t),
            .pCode      = il.data()
        };

        VK_CHECK(
            vkCreateShaderModule(
                device_handle, 
                &shader_module_create_info, 
                nullptr, 
                &shader_module_handle
            )
        );

        return shader_module_handle;
    }
}
