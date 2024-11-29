#include <animation/animation_pass.hpp>

#include <base/scene/node.hpp>

#include <base/logger/logger.hpp>

#include <base/shader_compiler.hpp>

namespace sample_vk::animation
{
    SkinnedMesh::SkinnedMesh(const Context* ptr_context, const Mesh* ptr_source_mesh) :
        _ptr_source_mesh(ptr_source_mesh)
    {
        if (!ptr_source_mesh)
            log::vkError("[SkinnedMesh::SkinnedMesh] Pointer to source mesh is null");

        _animated_mesh.index_count  = _ptr_source_mesh->index_count;
        _animated_mesh.vertex_count = _ptr_source_mesh->vertex_count;

        constexpr auto shared_usage_flags = 
                    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT 
                |   VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT 
                |   VK_BUFFER_USAGE_TRANSFER_DST_BIT 
                |   VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;

        const auto memory_index = MemoryProperties::getMemoryIndex(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
            .or_else([]
            {
                constexpr std::optional invalid_memory_index = std::numeric_limits<uint32_t>::infinity();
                
                log::appError("[SkinnedMesh::SkinnedMesh] Failed get memory index for create buffers");

                return invalid_memory_index;
            })
            .value();

        static int index_for_current_buffers = 0;
        ++index_for_current_buffers;

        _animated_mesh.index_buffer = Buffer::make(
            ptr_context, 
            _animated_mesh.index_count, 
            memory_index,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT | shared_usage_flags, 
            std::format("[SkinnedMesh][IndexBuffer]: {}", index_for_current_buffers)
        );

        _animated_mesh.vertex_buffer = Buffer::make(
            ptr_context, 
            _animated_mesh.vertex_count, 
            memory_index,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | shared_usage_flags,
            std::format("[SkinnedMesh][VertexBuffer]: {}", index_for_current_buffers)
        );
    }

    SkinnedMesh::SkinnedMesh(SkinnedMesh&& skinned_mesh) :
        _ptr_source_mesh    (skinned_mesh._ptr_source_mesh),
        _animated_mesh      (std::move(skinned_mesh._animated_mesh))
    { }

    SkinnedMesh& SkinnedMesh::operator = (SkinnedMesh&& skinned_mesh)
    {
        _ptr_source_mesh    = skinned_mesh._ptr_source_mesh;
        _animated_mesh      = std::move(skinned_mesh._animated_mesh);
        return *this;
    }
}

namespace sample_vk::animation
{
    AnimationPass::AnimationPass(
        const Context*              ptr_context,
        std::vector<SkinnedMesh>&&  meshes, 
        VkPipeline                  pipeline_handle, 
        VkPipelineLayout            pipeline_layout
    ) :
        _meshes             (std::move(meshes)),
        _pipeline_handle    (pipeline_handle),
        _pipeline_layout    (pipeline_layout),
        _ptr_context        (ptr_context)
    { }

    AnimationPass::AnimationPass(AnimationPass&& animation_pass) :
        _meshes(std::move(animation_pass._meshes))
    { }

    AnimationPass::~AnimationPass()
    {
        if (_pipeline_layout != VK_NULL_HANDLE)
            vkDestroyPipelineLayout(_ptr_context->device_handle, _pipeline_layout, nullptr);

        if (_pipeline_handle != VK_NULL_HANDLE)
            vkDestroyPipeline(_ptr_context->device_handle, _pipeline_handle, nullptr);
    }

    AnimationPass& AnimationPass::operator = (AnimationPass&& animation_pass)
    {
        _meshes = std::move(animation_pass._meshes);
        return *this;
    }

    void AnimationPass::process()
    {
        /// @todo
    }
}

namespace sample_vk::animation
{
    AnimationPass::Builder::~Builder()
    {
        vkDestroyShaderModule(_ptr_context->device_handle, _compute_shader_handle, nullptr);
    }

    AnimationPass::Builder::Builder(const Context* ptr_context) :
        _ptr_context(ptr_context)
    {
        if (!ptr_context)
            log::vkError("[AnimationPass::Builder] Vulkan context is null");
    }

    void AnimationPass::Builder::process(Node* ptr_node)
    {
        for (const auto& ptr_child: ptr_node->children)
            process(ptr_child.get());
    }

    void AnimationPass::Builder::process(MeshNode* ptr_node)
    {
        SkinnedMesh skinned_mesg (_ptr_context, &ptr_node->mesh);
    }

    VkPipelineLayout AnimationPass::Builder::createPipelineLayout()
    {
        log::vkInfo("[AnimationPass::Builder] Create pipeline layout");
        VkPipelineLayout layout_handle = VK_NULL_HANDLE;

        VkPipelineLayoutCreateInfo layout_info = { };
        layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        
        VK_CHECK(vkCreatePipelineLayout(
            _ptr_context->device_handle,
            &layout_info,
            nullptr,
            &layout_handle
        ));

        return layout_handle;
    }

    VkPipelineShaderStageCreateInfo AnimationPass::Builder::compileComputeShader()
    {
        log::vkInfo("[AnimationPass::Builder] Compile shader");

        const auto shader_name = project_dir / "shaders/animation/animation_pass.glsl.comp";

        _compute_shader_handle = shader::Compiler::createShaderModule(_ptr_context->device_handle, shader_name, shader::Type::compute);

        VkPipelineShaderStageCreateInfo stage = { };
        stage.sType     = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stage.module    = _compute_shader_handle;
        stage.pName     = "main";
        stage.stage     = VK_SHADER_STAGE_COMPUTE_BIT;

        return stage;
    }

    VkPipeline AnimationPass::Builder::createPipeline(VkPipelineLayout layout)
    {
        log::vkInfo("[AnimationPass::Builder] Create compute pipeline");

        VkPipeline pipeline_handle = VK_NULL_HANDLE;

        VkComputePipelineCreateInfo pipeline_info = { };
        pipeline_info.sType     = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipeline_info.layout    = layout;
        pipeline_info.stage     = compileComputeShader();
        
        VK_CHECK(vkCreateComputePipelines(
            _ptr_context->device_handle,
            VK_NULL_HANDLE,
            1, &pipeline_info,
            nullptr,
            &pipeline_handle
        ));

        return pipeline_handle;
    }

    AnimationPass AnimationPass::Builder::build()
    {
        auto layout = createPipelineLayout();
        return AnimationPass(_ptr_context, std::move(_meshes), createPipeline(layout), layout);
    }
}