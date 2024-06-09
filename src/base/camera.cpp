#include <base/camera.hpp>
#include <base/math.hpp>

namespace sample_vk
{
    Camera::Camera(uint32_t width, uint32_t height) :
        _width  (width),
        _height (height)
    { }

    void Camera::setDepthRange(float near, float far)
    {
        _near   = near;
        _far    = far;
    }

    void Camera::lookaAt(
            const glm::vec3& pos,
            const glm::vec3& dir,
            const glm::vec3& up
    )
    {
        _pos    = pos;
        _dir    = dir;
        _up     = up;
    }

    float Camera::getEyeToPixelConeSpreadAngle() const
    {
        auto fov_in_radians = _fov * (M_PI / 180.0);
        return std::atan((2.0f * std::tan(fov_in_radians * 0.5f)) / _height);
    }

    glm::mat4 Camera::getInvViewMatrix() const
    {
        return glm::inverse(glm::lookAt(_pos + _dir, _pos, _up));
    }

    glm::mat4 Camera::getInvProjection() const
    {
        return glm::inverse(glm::perspectiveFov(_fov, _width, _height, _near, _far));
    }

    std::pair<float, float> Camera::getSize() const
    {
        return std::make_pair(_width, _height);
    }

    void Camera::setSize(float width, float height)
    {
        assert(width > 0.0 && height > 0.0);

        _width  = width;
        _height = _height;
    }
}

namespace sample_vk
{
    CameraController::CameraController(const Camera& camera) :
        _camera(camera)
    { }

    Camera& CameraController::getCamera() noexcept
    {
        return _camera;
    }

    void CameraController::processMouse(const SDL_Event* ptr_event)
    {
        if (ptr_event->type == SDL_MOUSEBUTTONUP && _is_mouse_button_pressed)
            _is_mouse_button_pressed = false;

        if (ptr_event->type == SDL_MOUSEBUTTONDOWN && !_is_mouse_button_pressed)
        {
            _is_mouse_button_pressed = true;

            _prev_x = ptr_event->motion.x;
            _prev_y = ptr_event->motion.y;
        }

        if (!_is_mouse_button_pressed)
            return ;

        auto diff_x = static_cast<float>(ptr_event->motion.x - _prev_x) / _camera._width;
        auto diff_y = static_cast<float>(ptr_event->motion.y - _prev_y) / _camera._height;

        auto rotate = 
            glm::angleAxis(diff_x, glm::vec3(0, 1, 0)) * 
            glm::angleAxis(diff_y, glm::vec3(1, 0, 0));

        auto dir = glm::normalize(glm::mat4_cast(rotate) * glm::vec4(_camera._dir, 0));

        _camera._dir = glm::normalize(glm::vec3(dir.x, dir.y, dir.z));

        auto right = glm::cross(_camera._dir, glm::vec3(0, 1, 0));
        _camera._up = glm::cross(right, _camera._dir);

        _prev_x = ptr_event->motion.x;
        _prev_y = ptr_event->motion.y;
    }

    void CameraController::translate(const glm::vec3& t)
    {
        auto pos = glm::translate(glm::mat4(1.0f), t + _camera._dir) * glm::vec4(_camera._pos, 1);

        _camera._pos = glm::vec3(pos.x, pos.y, pos.z) / pos.w;
    }

    void CameraController::update()
    {
        if (_move_speed > std::numeric_limits<float>::epsilon())
        {
            translate(_move_dir * _move_speed);
            _move_speed = 0.0;
        }
    }

    void CameraController::process(const SDL_Event* ptr_event)
    {
#define IS_KEY(ptr_event, scan) (ptr_event->key.keysym.scancode == SDL_SCANCODE_##scan)

        switch(ptr_event->type)
        {
            case SDL_KEYDOWN:
                if (
                    IS_KEY(ptr_event, W) || IS_KEY(ptr_event, S) ||
                    IS_KEY(ptr_event, A) || IS_KEY(ptr_event, D) ||
                    IS_KEY(ptr_event, LSHIFT) || IS_KEY(ptr_event, RSHIFT) || IS_KEY(ptr_event, SPACE)
                )
                {
                    _move_speed = _max_move_speed;

                    if (IS_KEY(ptr_event, W))
                        _move_dir = -_camera._dir;
                    else if (IS_KEY(ptr_event, S))
                        _move_dir = _camera._dir;
                    else if (IS_KEY(ptr_event, A))
                        _move_dir = glm::cross(_camera._dir, _camera._up);
                    else if (IS_KEY(ptr_event, D))
                        _move_dir = -glm::cross(_camera._dir, _camera._up);
                    else if (IS_KEY(ptr_event, LSHIFT) || IS_KEY(ptr_event, RSHIFT))
                        _move_dir = glm::vec3(0, -1, 0);
                    else if (IS_KEY(ptr_event, SPACE))
                        _move_dir = glm::vec3(0, 1, 0);
                }
                break;
            case SDL_MOUSEBUTTONUP:
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEMOTION:
                processMouse(ptr_event);
                break;
        };
    }
}