//
// Created by Sohal03 on 26. 11. 5..
//
#include "camera/Camera.h"
#include <glm/gtc/matrix_transform.hpp>

using namespace glm;

// 마우스 회전
void Camera::processMouse(float xoffset, float yoffset) {
    xoffset *= mouseSensitivity;
    yoffset *= mouseSensitivity;

    yaw += xoffset;
    pitch += yoffset;

    pitch = clamp(pitch, -89.0f, 89.0f);

    vec3 dir;
    dir.x = cos(radians(yaw)) * cos(radians(pitch));
    dir.y = sin(radians(pitch));
    dir.z = sin(radians(yaw)) * cos(radians(pitch));
    front = normalize(dir);
}

void Camera::processKeyboard(const CameraMovement direction, const float deltaTime) {
    const float velocity = moveSpeed * deltaTime;
    const vec3 frontXZ = normalize(vec3(front.x, 0.0f, front.z));

    switch (direction) {
    case CameraMovement::Forward:
        position += velocity * frontXZ;
        break;

    case CameraMovement::Backward:
        position -= velocity * frontXZ;
        break;

    case CameraMovement::Left:
        position -= normalize(cross(frontXZ, up)) * velocity;
        break;

    case CameraMovement::Right:
        position += normalize(cross(frontXZ, up)) * velocity;
        break;
    }
}
