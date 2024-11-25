#pragma once

#include <base/scene/mesh.hpp>

#include <base/scene/visitors/node_visitor.hpp>

/// @todo void Animation::initVertexBufferReferences() должен использовать то, что будет в SkinnedMesh и обновлять данные каждый кадр

/// @todo void Animation::initScene() 
///     auto ptr_as_builder = std::make_unique<ASBuilder>(getContext());
///     auto& model = _scene->getModel();
///     model.visit(ptr_as_builder);
/// -> должно использоваться для построения то, что есть в SkinnedMesh

/// @todo может сделать skinned/animated_model и просто строить его обходом в source_model ?

namespace sample_vk
{
    struct Context;   
}

namespace sample_vk::animation
{
    class SkinnedMesh
    {
    public:
        explicit SkinnedMesh(const Context* ptr_context, const Mesh& source_mesh);

        // SkinnedMesh(SkinnedMesh&& skinned_mesh)         = delete;
        SkinnedMesh(const SkinnedMesh& skinned_mesh)    = delete;

        // SkinnedMesh& operator = (SkinnedMesh&& skinned_mesh)        = delete;
        SkinnedMesh& operator = (const SkinnedMesh& skinned_mesh)   = delete;

    private:
        const Mesh& _source_mesh;
        Mesh        _animated_mesh;
    };
}

namespace sample_vk::animation
{
    class AnimationPass
    {
    public:
        class Builder;

        AnimationPass(AnimationPass&& animation_pass);
        AnimationPass(const AnimationPass& animation_pass) = delete;

        AnimationPass& operator = (AnimationPass&& animation_pass);
        AnimationPass& operator = (const AnimationPass& animation_pass) = delete;

    private:

    };

    class AnimationPass::Builder: 
        public NodeVisitor
    {
        /// @todo implement
        void process(Node* ptr_node) override;
        void process(MeshNode* ptr_node) override;
    };
}