#pragma once

#include <memory>
#include <limits>

#include <SDL2/SDL.h>

#include <base/math.hpp>

namespace sample_vk
{
    class Camera
    {
        friend class Scene;
        friend class CameraController;

    public:
        explicit Camera(uint32_t width, uint32_t height);

        [[nodiscard]] glm::mat4 getInvViewMatrix() const;
        [[nodiscard]] glm::mat4 getInvProjection() const;

        [[nodiscard]] std::pair<float, float> getSize() const;
        void setSize(float width, float height);

        void setDepthRange(float near, float far);
        
        void lookaAt(
            const glm::vec3& pos,
            const glm::vec3& dir,
            const glm::vec3& up = glm::vec3(0, 1, 0)
        );

        [[nodiscard]] float getEyeToPixelConeSpreadAngle() const;

    private:
        float _width;
        float _height;

        glm::vec3 _pos  = glm::vec3(0);
        glm::vec3 _dir  = glm::vec3(0, 0, -1);
        glm::vec3 _up   = glm::vec3(0, 1, 0);

        float _near  = 0.01;
        float _far   = std::numeric_limits<float>::max() - 1.0f;

        float _fov = 45.0;
    };

    class CameraController
    {
    public:
        CameraController(const Camera& ptr_camera);

        void process(const SDL_Event* ptr_event);

        [[nodiscard]]
        Camera& getCamera() noexcept;

        void update();
    private:
        void processMouse(const SDL_Event* ptr_event);
        void translate(const glm::vec3& t);

    private:
        Camera _camera;

        Sint32 _prev_x = 0.0;
        Sint32 _prev_y = 0.0;

        bool _is_mouse_button_pressed = false;

        float       _move_speed = 0.0;
        glm::vec3   _move_dir;

        static constexpr float _max_move_speed = 100.0f;
    };
}