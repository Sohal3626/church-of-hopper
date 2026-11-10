#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <string>
#include <fstream>

#include "camera/Camera.h"
#include "camera/CameraControl.h"
#include "../include/resource/Mesh.h"
#include "../include/resource/Material.h"
#include "renderer/Renderer.h"
#include "renderer/Shader.h"

// 경로 맞춰주는 매크로
#ifndef PROJECT_SOURCE_DIR
#define PROJECT_SOURCE_DIR "."
#endif

inline std::string getPath(const std::string& relativePath) {
    return std::string(PROJECT_SOURCE_DIR) + "/" + relativePath;
}

// 창 크기
constexpr unsigned int SCR_WIDTH = 1280;
constexpr unsigned int SCR_HEIGHT = 720;

bool firstMouse = true;
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;

// 프레임 시간
float deltaTime = 0.0f;
float lastFrame = 0.0f;

int main() {
    // glfw 초기화
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Church of Hopper", nullptr, nullptr);
    if (window == nullptr) {
        std::cout << "창 생성 실패" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);

    // 마우스 고정
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad 초기화
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        std::cout << "GLAD 초기화 실패" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    Camera camera;
    Renderer renderer;

    Mesh gothic(getPath("data/gothic.obj"));
    Material gothic_materials(getPath("data/gothic.mtl"));

    Mesh stgl(getPath("data/rose_window.obj"));
    Material stgl_material(getPath("data/rose_window.mtl"));


    // 셰이더 불러오기
    std::string vsPath = getPath("shaders/vshader.glsl");
    std::string fsPath = getPath("shaders/fshader.glsl");

    Shader shader(vsPath, fsPath);
    Shader glass_shader(vsPath, getPath("shaders/glass.glsl"));

    glfwSetWindowUserPointer(window, &camera);

    auto model = glm::mat4(1.0f);
    auto glassModel = glm::translate(glm::mat4(1.0f), glm::vec3(-4.0f, 5.35f, -0.46f));
    glassModel = glm::rotate(glassModel, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glassModel = glm::scale(glassModel, glm::vec3(1.2f, 1.2f, 1.2f));

    // 루프 시작
    while (!glfwWindowShouldClose(window)) {
        const auto currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window, camera, deltaTime);

        glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        if (width == 0 || height == 0) {
            glfwPollEvents();
            continue;
        }

        constexpr float aspectRatio = static_cast<float>(SCR_WIDTH) / SCR_HEIGHT;

        const glm::mat4 projection = camera.getProjectionMatrix(aspectRatio);
        const glm::mat4 view = camera.getViewMatrix();

        shader.use();
        shader.setMat4("projection", projection);
        shader.setMat4("view", view);
        shader.setMat4("model", model);

        shader.setVec3("lightPos", glm::vec3(5.0f));
        shader.setVec3("viewPos", camera.getPosition());

        renderer.draw(gothic, gothic_materials, shader);

        glass_shader.use();
        glass_shader.setMat4("projection", projection);
        glass_shader.setMat4("view", view);
        glass_shader.setMat4("model", glassModel);

        renderer.draw(stgl, stgl_material, glass_shader);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}