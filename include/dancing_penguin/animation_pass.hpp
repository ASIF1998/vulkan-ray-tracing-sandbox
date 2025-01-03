#pragma once

#include <base/scene/mesh.hpp>
#include <base/scene/visitors/node_visitor.hpp>

#include <base/math.hpp>

#include <base/vulkan/buffer.hpp>

#include <span>

namespace vrts
{
    struct Context;
    struct SkinnedMesh;
}

namespace vrts::dancing_penguin
{
    class AnimationPass
    {
        struct Bindings
        {
            enum : 
                size_t 
            {
                index_buffer,
                src_vertex_buffer,
                dst_vertex_buffer,
                skinning_data_buffer,
                final_bones_matrices
            };
        };
        
        explicit AnimationPass(const Context* ptr_context);

        void bindMesh(const SkinnedMesh* ptr_mesh);

        void updateMatrices(std::span<const glm::mat4> matrices);

    public:
        class Builder;

        AnimationPass(AnimationPass&& animation_pass);
        AnimationPass(const AnimationPass& animation_pass) = delete;

        ~AnimationPass();

        AnimationPass& operator = (AnimationPass&& animation_pass);
        AnimationPass& operator = (const AnimationPass& animation_pass) = delete;

        void process(std::span<const glm::mat4> final_bones_matrices);

    private:
        std::vector<SkinnedMesh*> _meshes;

        VkPipeline              _pipeline_handle        = VK_NULL_HANDLE;
        VkPipelineLayout        _pipeline_layout        = VK_NULL_HANDLE;
        VkDescriptorSetLayout   _descriptor_set_layout  = VK_NULL_HANDLE;

        VkDescriptorPool    _descriptor_pool_handle = VK_NULL_HANDLE;
        VkDescriptorSet     _descriptor_set_handle  = VK_NULL_HANDLE;

        std::optional<Buffer> _final_bones_matrices;

        const Context* _ptr_context;
    };

    class AnimationPass::Builder : 
        public NodeVisitor
    {
        void process(Node* ptr_node)            override;
        void process(MeshNode* ptr_node)        override;
        void process(SkinnedMeshNode* ptr_node) override;

        void createPipelineLayout();
        void createPipeline();

        void createDescriptorPool();
        void allocateDescriptorSet();

    public:
        Builder(const Context* ptr_context);

        Builder(Builder&& builder)      = delete;
        Builder(const Builder& builder) = delete;

        ~Builder();

        Builder& operator = (Builder&& builder)         = delete;
        Builder& operator = (const Builder& builder)    = delete;

        AnimationPass build();

    private:
        const Context* _ptr_context;

        std::vector<SkinnedMesh*> _meshes;

        VkPipeline              _pipeline_handle        = VK_NULL_HANDLE;
        VkPipelineLayout        _pipeline_layout        = VK_NULL_HANDLE;
        VkDescriptorSetLayout   _descriptor_set_layout  = VK_NULL_HANDLE;

        VkDescriptorPool    _descriptor_pool_handle = VK_NULL_HANDLE;
        VkDescriptorSet     _descriptor_set_handle  = VK_NULL_HANDLE;

        VkShaderModule _compute_shader_handle = VK_NULL_HANDLE;
    };
}