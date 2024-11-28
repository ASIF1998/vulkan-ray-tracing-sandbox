#pragma once

#include <base/scene/mesh.hpp>

#include <base/scene/visitors/node_visitor.hpp>

namespace sample_vk
{
    struct Context;   
}

namespace sample_vk::animation
{
    class SkinnedMesh
    {
    public:
        explicit SkinnedMesh(const Context* ptr_context, const Mesh* ptr_source_mesh);

        SkinnedMesh(SkinnedMesh&& skinned_mesh);
        SkinnedMesh(const SkinnedMesh& skinned_mesh) = delete;

        SkinnedMesh& operator = (SkinnedMesh&& skinned_mesh);
        SkinnedMesh& operator = (const SkinnedMesh& skinned_mesh)   = delete;

    private:
        const Mesh* _ptr_source_mesh;
        Mesh        _animated_mesh;
    };
}

namespace sample_vk::animation
{
    class AnimationPass
    {
        explicit AnimationPass(std::vector<SkinnedMesh>&& meshes);

    public:
        class Builder;

        AnimationPass(AnimationPass&& animation_pass);
        AnimationPass(const AnimationPass& animation_pass) = delete;

        AnimationPass& operator = (AnimationPass&& animation_pass);
        AnimationPass& operator = (const AnimationPass& animation_pass) = delete;

        void process();

    private:
        std::vector<SkinnedMesh> _meshes;
    };

    class AnimationPass::Builder : 
        public NodeVisitor
    {
        void process(Node* ptr_node) override;
        void process(MeshNode* ptr_node) override;
    
    public:
        Builder(const Context* ptr_context);

        AnimationPass build();

    private:
        std::vector<SkinnedMesh>    _meshes;
        const Context*               _ptr_context;
    };
}