#pragma once

#include <base/vulkan/buffer.hpp>

#include <base/math.hpp>

#include <optional>

namespace vrts
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

        struct SkinningData
        {
            glm::ivec4  bone_ids    = glm::ivec4(-1);
            glm::vec4   weights     = glm::vec4(-1.0f);
        };

    public:
        Mesh() = default;

        Mesh(Mesh&& mesh)       = default; 
        Mesh(const Mesh& mesh)  = delete;

        Mesh& operator = (Mesh&& mesh)      = default;
        Mesh& operator = (const Mesh& mesh) = delete;

    public:
        std::optional<Buffer> index_buffer;
        size_t index_count = 0;

        std::optional<Buffer> vertex_buffer;
        size_t vertex_count = 0;

        std::optional<Buffer> skinning_buffer;
    };

    struct SkinnedMesh
    {
        std::optional<Buffer> source_vertex_buffer;
        std::optional<Buffer> processed_vertex_buffer;
        std::optional<Buffer> skinning_buffer;
        size_t vertex_count = 0;
        
        std::optional<Buffer> index_buffer;
        size_t index_count = 0;
    };
}