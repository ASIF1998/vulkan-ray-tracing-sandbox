#pragma once

#include <base/vulkan/buffer.hpp>

#include <base/math.hpp>

namespace sample_vk
{
    struct Mesh
    {
    public:
        /// @todo
        struct Attributes
        {
            glm::vec4 pos;
            glm::vec4 normal;
            glm::vec4 tangent;
            glm::vec4 uv;
        };

    public:
        Mesh(const Context* ptr_context):
            index_buffer    (ptr_context),
            vertex_buffer   (ptr_context)
        { }

        Mesh(Mesh&& mesh)       = default; 
        Mesh(const Mesh& mesh)  = delete;

        Mesh& operator = (Mesh&& mesh)      = default;
        Mesh& operator = (const Mesh& mesh) = delete;

    public:
        Buffer index_buffer;
        size_t index_count = 0;

        Buffer vertex_buffer;
        size_t vertex_count = 0;
    };
}