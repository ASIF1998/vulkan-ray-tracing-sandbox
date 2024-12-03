#include <animation/animation_pass.hpp>

#include <base/scene/node.hpp>

#include <base/logger/logger.hpp>

#include <base/shader_compiler.hpp>

namespace sample_vk::animation
{
    SkinnedMesh::SkinnedMesh(const Context* ptr_context, const Mesh* ptr_source_mesh) :
        ptr_source_mesh(ptr_source_mesh)
    {
        if (!ptr_source_mesh)
            log::vkError("[SkinnedMesh::SkinnedMesh] Pointer to source mesh is null");

        animated_mesh.index_count   = ptr_source_mesh->index_count;
        animated_mesh.vertex_count  = ptr_source_mesh->vertex_count;

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

        animated_mesh.index_buffer = Buffer::make(
            ptr_context, 
            animated_mesh.index_count, 
            memory_index,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT | shared_usage_flags, 
            std::format("[SkinnedMesh][IndexBuffer]: {}", index_for_current_buffers)
        );

        animated_mesh.vertex_buffer = Buffer::make(
            ptr_context, 
            animated_mesh.vertex_count, 
            memory_index,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | shared_usage_flags,
            std::format("[SkinnedMesh][VertexBuffer]: {}", index_for_current_buffers)
        );
    }

    SkinnedMesh::SkinnedMesh(SkinnedMesh&& skinned_mesh) :
        ptr_source_mesh (skinned_mesh.ptr_source_mesh),
        animated_mesh   (std::move(skinned_mesh.animated_mesh))
    { }

    SkinnedMesh& SkinnedMesh::operator = (SkinnedMesh&& skinned_mesh)
    {
        ptr_source_mesh = skinned_mesh.ptr_source_mesh;
        animated_mesh   = std::move(skinned_mesh.animated_mesh);
        return *this;
    }
}

namespace sample_vk::animation
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
        if (_descriptor_set_handle != VK_NULL_HANDLE && _descriptor_pool_handle != VK_NULL_HANDLE)
        {
            vkFreeDescriptorSets(_ptr_context->device_handle, _descriptor_pool_handle, 1, &_descriptor_set_handle);
            vkDestroyDescriptorPool(_ptr_context->device_handle, _descriptor_pool_handle, nullptr);
        }

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

    void AnimationPass::bindMesh(const SkinnedMesh& mesh)
    {
        std::array<VkDescriptorBufferInfo, 4> buffers_info;

        buffers_info[Bindings::src_vertex_buffer] = { };
        buffers_info[Bindings::src_vertex_buffer].buffer  = mesh.ptr_source_mesh->vertex_buffer->vk_handle;
        buffers_info[Bindings::src_vertex_buffer].range   = mesh.ptr_source_mesh->vertex_buffer->size_in_bytes;
        
        buffers_info[Bindings::dst_vertex_buffer] = { };
        buffers_info[Bindings::dst_vertex_buffer].buffer  = mesh.animated_mesh.vertex_buffer->vk_handle;
        buffers_info[Bindings::dst_vertex_buffer].range   = mesh.animated_mesh.vertex_buffer->size_in_bytes;
        
        buffers_info[Bindings::skinning_data_buffer] = { };
        buffers_info[Bindings::skinning_data_buffer].buffer  = mesh.ptr_source_mesh->skinning_buffer->vk_handle;
        buffers_info[Bindings::skinning_data_buffer].range   = mesh.ptr_source_mesh->skinning_buffer->size_in_bytes;
        
        buffers_info[Bindings::index_buffer]  = { };
        buffers_info[Bindings::index_buffer] .buffer  = mesh.ptr_source_mesh->index_buffer->vk_handle;
        buffers_info[Bindings::index_buffer] .range   = mesh.ptr_source_mesh->index_buffer->size_in_bytes;

        std::array<VkWriteDescriptorSet, 4> write_infos;

        for (size_t i = 0; i < write_infos.size(); ++i)
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

    void AnimationPass::process()
    {
        for (const auto& skinned_mesh: _meshes)
        {
            bindMesh(skinned_mesh);

            auto command_buffer = VkUtils::getCommandBuffer(_ptr_context);
            command_buffer.write([this, &skinned_mesh] (VkCommandBuffer command_buffer_handle)
            {
                const auto x_group_size = static_cast<uint32_t>(skinned_mesh.ptr_source_mesh->index_count);

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
            });

            command_buffer.upload(_ptr_context);
        }
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
            ptr_child->visit(this);
    }

    void AnimationPass::Builder::process(MeshNode* ptr_node)
    {
        _meshes.emplace_back(_ptr_context, &ptr_node->mesh);
    }

    void AnimationPass::Builder::createPipelineLayout()
    {
        log::vkInfo("[AnimationPass::Builder] Create pipeline layout");

        std::array<VkDescriptorSetLayoutBinding, 4> bindings_info;

        for (size_t i = 0; i < bindings_info.size(); ++i)
        {
            bindings_info[i] = { };
            bindings_info[i].binding            = static_cast<uint32_t>(i);
            bindings_info[i].descriptorCount    = 1;
            bindings_info[i].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
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
        log::vkInfo("[AnimationPass::Builder] Create compute pipeline");

        const auto shader_name = project_dir / "shaders/animation/animation_pass.glsl.comp";

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
        descriptor_size.descriptorCount = 4;

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
            "Descritptor set for animation pass"
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