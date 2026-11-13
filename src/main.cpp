#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <string>
#include <array>
#include <filesystem>
#include <optional>
#include <stdexcept>

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

namespace {

constexpr int kShadowMapSize = 1024;
constexpr int kShadowTextureUnit = 1;
constexpr float kShadowNearPlane = 0.1f;
constexpr float kShadowFarPlane = 30.0f;

struct CubeFace {
    glm::vec3 direction;
    glm::vec3 up;
};

// Order and orientations match GL_TEXTURE_CUBE_MAP_POSITIVE_X through NEGATIVE_Z.
const std::array<CubeFace, 6> kCubeFaces = {{
    {{ 1,  0,  0}, {0, -1,  0}},
    {{-1,  0,  0}, {0, -1,  0}},
    {{ 0,  1,  0}, {0,  0,  1}},
    {{ 0, -1,  0}, {0,  0, -1}},
    {{ 0,  0,  1}, {0, -1,  0}},
    {{ 0,  0, -1}, {0, -1,  0}},
}};

class PointShadowMap {
public:
    PointShadowMap() {
        glGenTextures(1, &depthCube);
        glBindTexture(GL_TEXTURE_CUBE_MAP, depthCube);
        for (unsigned int face = 0; face < kCubeFaces.size(); ++face) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_DEPTH_COMPONENT24,
                         kShadowMapSize, kShadowMapSize, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        }
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

        glGenFramebuffers(1, &framebuffer);
    }

    ~PointShadowMap() {
        glDeleteFramebuffers(1, &framebuffer);
        glDeleteTextures(1, &depthCube);
    }

    PointShadowMap(const PointShadowMap&) = delete;
    PointShadowMap& operator=(const PointShadowMap&) = delete;

    void render(Renderer& renderer, const Mesh& mesh, const glm::mat4& model,
                const glm::vec3& lightPos, const Shader& depthShader) const {
        const glm::mat4 projection = glm::perspective(glm::radians(90.0f), 1.0f,
                                                      kShadowNearPlane, kShadowFarPlane);
        depthShader.use();
        depthShader.setMat4("model", model);
        depthShader.setVec3("lightPos", lightPos);
        depthShader.setFloat("shadowFarPlane", kShadowFarPlane);

        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        glViewport(0, 0, kShadowMapSize, kShadowMapSize);
        for (unsigned int face = 0; face < kCubeFaces.size(); ++face) {
            const GLenum target = GL_TEXTURE_CUBE_MAP_POSITIVE_X + face;
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, target, depthCube, 0);
            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                throw std::runtime_error("Shadow map framebuffer is incomplete");
            }

            const CubeFace& orientation = kCubeFaces[face];
            const glm::mat4 view = glm::lookAt(lightPos, lightPos + orientation.direction,
                                                orientation.up);
            depthShader.setMat4("lightViewProjection", projection * view);
            glClear(GL_DEPTH_BUFFER_BIT);
            renderer.drawDepth(mesh);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void bind() const {
        glActiveTexture(GL_TEXTURE0 + kShadowTextureUnit);
        glBindTexture(GL_TEXTURE_CUBE_MAP, depthCube);
        glActiveTexture(GL_TEXTURE0);
    }

private:
    GLuint depthCube = 0;
    GLuint framebuffer = 0;
};

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

    const glm::mat4 model(1.0f);
    const glm::vec3 lightPos(5.0f);
    auto glassModel = glm::translate(glm::mat4(1.0f), glm::vec3(-4.0f, 5.35f, -0.46f));
    glassModel = glm::rotate(glassModel, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glassModel = glm::scale(glassModel, glm::vec3(1.2f, 1.2f, 1.2f));

    const Shader depthShader(getPath("shaders/shadow_depth.vert"),
                             getPath("shaders/shadow_depth.frag"));
    PointShadowMap shadowMap;
    // The light and its caster are static, so the depth pass runs once.
    shadowMap.render(renderer, gothic, model, lightPos, depthShader);
    shadowMap.bind();

    shader.use();
    shader.setInt("shadowMap", kShadowTextureUnit);
    shader.setFloat("shadowFarPlane", kShadowFarPlane);
    shader.setVec3("lightPos", lightPos);

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

        const float aspectRatio = static_cast<float>(width) / height;
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