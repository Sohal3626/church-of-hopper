#include "renderer/ShadowMap.h"

#include <glm/gtc/matrix_transform.hpp>

#include <array>
#include <stdexcept>

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

} // namespace

PointShadowMap::PointShadowMap() {
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

PointShadowMap::~PointShadowMap() {
    glDeleteFramebuffers(1, &framebuffer);
    glDeleteTextures(1, &depthCube);
}

void PointShadowMap::render(Renderer& renderer, const Mesh& mesh, const glm::mat4& model,
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

void PointShadowMap::bind() const {
    glActiveTexture(GL_TEXTURE0 + kShadowTextureUnit);
    glBindTexture(GL_TEXTURE_CUBE_MAP, depthCube);
    glActiveTexture(GL_TEXTURE0);
}

void PointShadowMap::applyUniforms(const Shader& shader, const glm::vec3& lightPos) const {
    shader.use();
    shader.setInt("shadowMap", kShadowTextureUnit);
    shader.setFloat("shadowFarPlane", kShadowFarPlane);
    shader.setVec3("lightPos", lightPos);
}
