#pragma once

#include <base/math.hpp>

namespace vrts::utils
{
    [[nodiscard]] glm::vec3 generateColor(uint32_t marker_id);
}

namespace vrts
{
    struct GpuMarkerColors final
    {
        inline static const glm::vec3 write_data_in_buffer  = utils::generateColor(1);
        inline static const glm::vec3 write_data_in_image   = utils::generateColor(2);

        inline static const glm::vec3 build_tlas = utils::generateColor(3);
        inline static const glm::vec3 build_blas = utils::generateColor(4);

        inline static const glm::vec3 create_mipmap = utils::generateColor(5);

        inline static const glm::vec3 change_image_layout = utils::generateColor(6);

        inline static const glm::vec3 run_compute_pipeline      = utils::generateColor(7);
        inline static const glm::vec3 run_ray_tracing_pipeline  = utils::generateColor(8);
    };
}