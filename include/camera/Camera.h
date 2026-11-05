//
// Created by Sohal03 on 26. 11. 5..
//

#ifndef CHURCH_OF_HOPPER_CAMERA_H
#define CHURCH_OF_HOPPER_CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

enum class CameraMovement {
    Forward,
    Backward,
    Left,
    Right
};

class Camera {
    float yaw = -90.0f;
    float pitch = 0.0f;
    float fov = 45.0f;

    glm::vec3 position{0.0f, 1.5f, -0.5f};
    glm::vec3 front{0.0f, 0.0f, -1.0f};
    glm::vec3 up{0.0f, 1.0f, 0.0f};

    float mouseSensitivity = 0.1f;
    float moveSpeed = 5.0f;

public:
    void processKeyboard(CameraMovement direction, float deltaTime);
    void processMouse(float xoffset, float yoffset);
    [[nodiscard]] glm::mat4 getViewMatrix() const { return glm::lookAt(position, position + front, up); }
    [[nodiscard]] glm::vec3 getPosition() const { return position; }
    [[nodiscard]] float getFov() const { return fov; }
};

#endif //CHURCH_OF_HOPPER_CAMERA_H