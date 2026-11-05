//
// Created by Sohal03 on 26. 11. 5..
//

#ifndef CHURCH_OF_HOPPER_CAMINPUT_H
#define CHURCH_OF_HOPPER_CAMINPUT_H

#include <GLFW/glfw3.h>

#include "camera/Camera.h"

void processInput(GLFWwindow* window, Camera& camera, float delta);
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);

#endif //CHURCH_OF_HOPPER_CAMINPUT_H