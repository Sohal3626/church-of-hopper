//
// Created by Sohal03 on 26. 11. 5..
//

#include "camera/CamInput.h"

void processInput(GLFWwindow* window, Camera& camera, const float delta) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.processKeyboard(CameraMovement::Forward, delta);

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.processKeyboard(CameraMovement::Backward, delta);

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.processKeyboard(CameraMovement::Left, delta);

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.processKeyboard(CameraMovement::Right, delta);
}

void mouse_callback(GLFWwindow* window, const double xposIn, const double yposIn) {
    auto* camera = static_cast<Camera*>(glfwGetWindowUserPointer(window));

    if (!camera)
        return;

    static bool firstMouse = true;
    static float lastX = 0.0f;
    static float lastY = 0.0f;

    const auto xpos = static_cast<float>(xposIn);
    const auto ypos = static_cast<float>(yposIn);

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    const float xoffset = xpos - lastX;
    const float yoffset = lastY - ypos;

    lastX = xpos;
    lastY = ypos;

    camera->processMouse(xoffset, yoffset);
}

void framebuffer_size_callback(GLFWwindow* window, const int width, const int height) {
    glViewport(0, 0, width, height);
}