#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <string>
#include <filesystem>
#include <optional>
#include <stdexcept>

#include "camera/Camera.h"
#include "camera/CameraControl.h"
#include "../include/resource/Mesh.h"
#include "../include/resource/Material.h"
#include "renderer/Renderer.h"
#include "renderer/Shader.h"
#include "renderer/ShadowMap.h"

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

namespace {

void runScene(GLFWwindow* window) {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    Camera camera;
    Renderer renderer;

    Mesh gothic(getPath("data/gothic.obj"));
    Material gothic_materials(getPath("data/gothic.mtl"));

    const std::string vsPath = getPath("shaders/vshader.glsl");
    Shader shader(vsPath, getPath("shaders/fshader.glsl"));

    const std::string glassShaderPath = getPath("shaders/glass.glsl");
    std::optional<Shader> glass_shader;
    std::optional<Mesh> stgl;
    std::optional<Material> stgl_material;
    if (std::filesystem::exists(glassShaderPath)) {
        glass_shader.emplace(vsPath, glassShaderPath);
        stgl.emplace(getPath("data/rose_window.obj"));
        stgl_material.emplace(getPath("data/rose_window.mtl"));
    } else {
        std::cerr << "Skipping stained glass: missing " << glassShaderPath << '\n';
    }

    glfwSetWindowUserPointer(window, &camera);

    constexpr glm::mat4 model(1.0f);
    constexpr glm::vec3 lightPos{-4.0f, 5.35f, -0.46f};
    auto glassModel = glm::translate(glm::mat4(1.0f), glm::vec3(-4.0f, 5.35f, -0.46f));
    glassModel = glm::rotate(glassModel, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glassModel = glm::scale(glassModel, glm::vec3(1.2f, 1.2f, 1.2f));

    const Shader depthShader(getPath("shaders/shadow_depth.vert"),
                             getPath("shaders/shadow_depth.frag"));
    PointShadowMap shadowMap;
    // The light and its caster are static, so the depth pass runs once.
    shadowMap.render(renderer, gothic, model, lightPos, depthShader);
    shadowMap.bind();

    shadowMap.applyUniforms(shader, lightPos);

    while (!glfwWindowShouldClose(window)) {
        const auto currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window, camera, deltaTime);

        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        if (width == 0 || height == 0) {
            glfwPollEvents();
            continue;
        }

        glViewport(0, 0, width, height);
        glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const float aspectRatio = static_cast<float>(width) / static_cast<float>(height);
        const glm::mat4 projection = camera.getProjectionMatrix(aspectRatio);
        const glm::mat4 view = camera.getViewMatrix();

        shader.use();
        shader.setMat4("projection", projection);
        shader.setMat4("view", view);
        shader.setMat4("model", model);
        shader.setVec3("viewPos", camera.getPosition());
        renderer.draw(gothic, gothic_materials, shader);

        if (glass_shader) {
            glass_shader->use();
            glass_shader->setMat4("projection", projection);
            glass_shader->setMat4("view", view);
            glass_shader->setMat4("model", glassModel);
            renderer.draw(*stgl, *stgl_material, *glass_shader);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwSetWindowUserPointer(window, nullptr);
}

} // namespace

int main() {
    // glfw 초기화
    if (!glfwInit()) {
        std::cerr << "GLFW initialization failed\n";
        return -1;
    }
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
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    int exitCode = 0;
    try {
        runScene(window);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        exitCode = 1;
    }
    glfwDestroyWindow(window);
    glfwTerminate();
    return exitCode;
}