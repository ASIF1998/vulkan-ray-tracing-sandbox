#include <dancing_penguin/animation_pass.hpp>

#include <base/scene/node.hpp>
#include <base/scene/mesh.hpp>

#include <base/logger/logger.hpp>

#include <base/shader_compiler.hpp>

#include <ranges>

namespace vrts::dancing_penguin
{
    AnimationPass::AnimationPass(const Context* ptr_context) :
        _ptr_context        (ptr_context)
    { }

    AnimationPass::AnimationPass(AnimationPass&& animation_pass) :
        _meshes         (std::move(animation_pass._meshes)),
        _ptr_context    (animation_pass._ptr_context)
    { 
        std::swap(_pipeline_handle, animation_pass._pipeline_handle);
        std::swap(_pipeline_layout, animation_pass._pipeline_layout);
        std::swap(_descriptor_set_layout, animation_pass._descriptor_set_layout);
        std::swap(_descriptor_pool_handle, animation_pass._descriptor_pool_handle);
        std::swap(_descriptor_set_handle, animation_pass._descriptor_set_handle);
    }

    AnimationPass::~AnimationPass()
    {
        if (_descriptor_pool_handle != VK_NULL_HANDLE)
            vkDestroyDescriptorPool(_ptr_context->device_handle, _descriptor_pool_handle, nullptr);

        if (_descriptor_set_layout != VK_NULL_HANDLE)
            vkDestroyDescriptorSetLayout(_ptr_context->device_handle, _descriptor_set_layout, nullptr);

        if (_pipeline_layout != VK_NULL_HANDLE)
            vkDestroyPipelineLayout(_ptr_context->device_handle, _pipeline_layout, nullptr);

        if (_pipeline_handle != VK_NULL_HANDLE)
            vkDestroyPipeline(_ptr_context->device_handle, _pipeline_handle, nullptr);

    }

    AnimationPass& AnimationPass::operator = (AnimationPass&& animation_pass)
    {
        _meshes = std::move(animation_pass._meshes);

        std::swap(_pipeline_handle, animation_pass._pipeline_handle);
        std::swap(_pipeline_layout, animation_pass._pipeline_layout);
        std::swap(_descriptor_set_layout, animation_pass._descriptor_set_layout);
        std::swap(_descriptor_pool_handle, animation_pass._descriptor_pool_handle);
        std::swap(_descriptor_set_handle, animation_pass._descriptor_set_handle);

        return *this;
    }

    void AnimationPass::bindMesh(const SkinnedMesh* ptr_mesh)
    {
        std::array<VkDescriptorBufferInfo, 4> buffers_info;

        buffers_info[Bindings::src_vertex_buffer] = { };
        buffers_info[Bindings::src_vertex_buffer].buffer  = ptr_mesh->source_vertex_buffer->vk_handle;
        buffers_info[Bindings::src_vertex_buffer].range   = ptr_mesh->source_vertex_buffer->size_in_bytes;
        
        buffers_info[Bindings::dst_vertex_buffer] = { };
        buffers_info[Bindings::dst_vertex_buffer].buffer  = ptr_mesh->processed_vertex_buffer->vk_handle;
        buffers_info[Bindings::dst_vertex_buffer].range   = ptr_mesh->processed_vertex_buffer->size_in_bytes;
        
        buffers_info[Bindings::skinning_data_buffer] = { };
        buffers_info[Bindings::skinning_data_buffer].buffer  = ptr_mesh->skinning_buffer->vk_handle;
        buffers_info[Bindings::skinning_data_buffer].range   = ptr_mesh->skinning_buffer->size_in_bytes;
        
        buffers_info[Bindings::index_buffer]  = { };
        buffers_info[Bindings::index_buffer] .buffer  = ptr_mesh->index_buffer->vk_handle;
        buffers_info[Bindings::index_buffer] .range   = ptr_mesh->index_buffer->size_in_bytes;

        std::array<VkWriteDescriptorSet, 4> write_infos;

        for (auto i: std::views::iota(0u, write_infos.size()))
        {
            write_infos[i] = { };
            write_infos[i].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write_infos[i].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            write_infos[i].dstArrayElement  = 0;
            write_infos[i].dstBinding       = static_cast<uint32_t>(i);
            write_infos[i].dstSet           = _descriptor_set_handle;
            write_infos[i].descriptorCount  = 1;
            write_infos[i].pBufferInfo      = &buffers_info[i];
        }

        vkUpdateDescriptorSets(
            _ptr_context->device_handle,
            static_cast<uint32_t>(write_infos.size()), write_infos.data(),
            0, nullptr
        );
    }

    void AnimationPass::updateMatrices(std::span<const glm::mat4> matrices)
    {
        if (matrices.empty())
            return ;

        const auto size = static_cast<VkDeviceSize>(sizeof(glm::mat4) * matrices.size());

        constexpr auto buffer_usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

        constexpr std::string_view buffer_name = "[AnimationPass] Final bone matrices";

        if (!_final_bones_matrices || _final_bones_matrices->size_in_bytes < size)
        {
            _final_bones_matrices = Buffer::Builder(_ptr_context)
                .vkSize(size)
                .vkUsage(buffer_usage)
                .name(buffer_name)
                .build();

            VkDescriptorBufferInfo buffer_info = { };
            buffer_info.buffer  = _final_bones_matrices->vk_handle;
            buffer_info.range   = _final_bones_matrices->size_in_bytes;

            VkWriteDescriptorSet write_info = { };
            write_info.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write_info.descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            write_info.dstArrayElement  = 0;
            write_info.dstBinding       = static_cast<uint32_t>(Bindings::final_bones_matrices);
            write_info.dstSet           = _descriptor_set_handle;
            write_info.descriptorCount  = 1;
            write_info.pBufferInfo      = &buffer_info;

            vkUpdateDescriptorSets(_ptr_context->device_handle, 1, &write_info, 0, nullptr);
        }

        Buffer::writeData(*_final_bones_matrices, matrices);
    }

    void AnimationPass::process(std::span<const glm::mat4> final_bones_matrices)
    {
        updateMatrices(final_bones_matrices);

        for (const auto ptr_skinned_mesh: _meshes)
        {
            bindMesh(ptr_skinned_mesh);

            auto command_buffer = VkUtils::getCommandBuffer(_ptr_context);
            command_buffer.write([this, ptr_skinned_mesh] (VkCommandBuffer command_buffer_handle)
            {
                const auto x_group_size = static_cast<uint32_t>(ptr_skinned_mesh->index_count);

                vkCmdBindPipeline(command_buffer_handle, VK_PIPELINE_BIND_POINT_COMPUTE, _pipeline_handle);
                
                vkCmdBindDescriptorSets(
                    command_buffer_handle, 
                    VK_PIPELINE_BIND_POINT_COMPUTE, 
                    _pipeline_layout, 
                    0, 
                    1, &_descriptor_set_handle, 
                    0, nullptr
                );
                
                vkCmdDispatch(command_buffer_handle, x_group_size, 1, 1); 
            }, "Run animation subpass", GpuMarkerColors::run_compute_pipeline);

            command_buffer.upload(_ptr_context);
        }
    }
}

namespace vrts::dancing_penguin
{
    AnimationPass::Builder::~Builder()
    {
        vkDestroyShaderModule(_ptr_context->device_handle, _compute_shader_handle, nullptr);
    }

    AnimationPass::Builder::Builder(const Context* ptr_context) :
        _ptr_context(ptr_context)
    {
        if (!ptr_context)
            log::error("[AnimationPass::Builder] Vulkan context is null");
    }

    void AnimationPass::Builder::process(Node* ptr_node)
    {
        for (const auto& ptr_child: ptr_node->children)
            ptr_child->visit(this);
    }

    void AnimationPass::Builder::process(MeshNode* ptr_node)
    {
    }
    
    void AnimationPass::Builder::process(SkinnedMeshNode* ptr_node)
    {
        _meshes.push_back(&ptr_node->mesh);
    }

    void AnimationPass::Builder::createPipelineLayout()
    {
        log::info("[AnimationPass::Builder] Create pipeline layout");

        std::array<VkDescriptorSetLayoutBinding, 5> bindings_info;

        for (auto i: std::views::iota(0u, bindings_info.size()))
        {
            bindings_info[i] = { };
            bindings_info[i].binding            = static_cast<uint32_t>(i);
            bindings_info[i].descriptorCount    = 1;
            bindings_info[i].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            bindings_info[i].stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
        }

        VkDescriptorSetLayoutCreateInfo descriptor_set_info = { };
        descriptor_set_info.sType           = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptor_set_info.bindingCount    = static_cast<uint32_t>(bindings_info.size());
        descriptor_set_info.pBindings       = bindings_info.data();

        VK_CHECK(vkCreateDescriptorSetLayout(
            _ptr_context->device_handle,
            &descriptor_set_info,
            nullptr,
            &_descriptor_set_layout
        ));

        VkPipelineLayoutCreateInfo layout_info = { };
        layout_info.sType           = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layout_info.pSetLayouts     = &_descriptor_set_layout;
        layout_info.setLayoutCount  = 1;
        
        VK_CHECK(vkCreatePipelineLayout(
            _ptr_context->device_handle,
            &layout_info,
            nullptr,
            &_pipeline_layout
        ));
    }

    void AnimationPass::Builder::createPipeline()
    {
        log::info("[AnimationPass::Builder] Create compute pipeline");

        const auto shader_name = project_dir / "shaders/dancing_penguin/animation_pass.glsl.comp";

        _compute_shader_handle = shader::Compiler::createShaderModule(
            _ptr_context->device_handle, 
            shader_name, 
            shader::Type::compute
        );

        VkPipelineShaderStageCreateInfo stage = { };
        stage.sType     = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stage.module    = _compute_shader_handle;
        stage.pName     = "main";
        stage.stage     = VK_SHADER_STAGE_COMPUTE_BIT;

        VkComputePipelineCreateInfo pipeline_info = { };
        pipeline_info.sType     = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipeline_info.layout    = _pipeline_layout;
        pipeline_info.stage     = stage;
        
        VK_CHECK(vkCreateComputePipelines(
            _ptr_context->device_handle,
            VK_NULL_HANDLE,
            1, &pipeline_info,
            nullptr,
            &_pipeline_handle
        ));
    }

    void AnimationPass::Builder::createDescriptorPool()
    {
        VkDescriptorPoolSize descriptor_size = { };
        descriptor_size.type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptor_size.descriptorCount = 5;

        VkDescriptorPoolCreateInfo descriptor_pool_info = { };
        descriptor_pool_info.sType          = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptor_pool_info.maxSets        = 1;
        descriptor_pool_info.pPoolSizes     = &descriptor_size;
        descriptor_pool_info.poolSizeCount  = 1;

        VK_CHECK(vkCreateDescriptorPool(
            _ptr_context->device_handle,
            &descriptor_pool_info,
            nullptr,
            &_descriptor_pool_handle
        ));
    }

    void AnimationPass::Builder::allocateDescriptorSet()
    {
        VkDescriptorSetAllocateInfo allocate_info = { };
        allocate_info.sType                 = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocate_info.descriptorPool        = _descriptor_pool_handle;
        allocate_info.descriptorSetCount    = 1;
        allocate_info.pSetLayouts           = &_descriptor_set_layout;

        VK_CHECK(vkAllocateDescriptorSets(_ptr_context->device_handle, &allocate_info, &_descriptor_set_handle));

        VkUtils::setName(
            _ptr_context->device_handle, 
            _descriptor_set_handle, 
            VK_OBJECT_TYPE_DESCRIPTOR_SET, 
            "[AnimationPass] Descritptor set"
        );
    }

    AnimationPass AnimationPass::Builder::build()
    {
        createPipelineLayout();
        createPipeline();
        createDescriptorPool();
        allocateDescriptorSet();

        AnimationPass animation_pass (_ptr_context);

        animation_pass._meshes                  = std::move(_meshes);
        animation_pass._pipeline_handle         = _pipeline_handle;
        animation_pass._pipeline_layout         = _pipeline_layout;
        animation_pass._descriptor_set_layout   = _descriptor_set_layout;
        animation_pass._descriptor_pool_handle  = _descriptor_pool_handle;
        animation_pass._descriptor_set_handle   = _descriptor_set_handle;

        return animation_pass;
    }
}