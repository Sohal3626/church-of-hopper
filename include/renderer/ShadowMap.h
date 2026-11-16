#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include "renderer/Renderer.h"
#include "renderer/Shader.h"
#include "resource/Mesh.h"

// 점광원용 큐브맵 그림자 맵(Point Shadow Map).
// 광원에서 6방향으로 depth 패스를 렌더링해 큐브맵에 저장하고,
// 메인 셰이더가 samplerCubeShadow로 그림자를 판정할 수 있게 한다.
class PointShadowMap {
public:
    PointShadowMap();
    ~PointShadowMap();

    PointShadowMap(const PointShadowMap&) = delete;
    PointShadowMap& operator=(const PointShadowMap&) = delete;

    // 광원 위치에서 mesh의 depth를 큐브맵 6면에 렌더링한다.
    // 호출 후 기본 프레임버퍼(0)가 바인딩되며, viewport는 호출자가 다시 설정해야 한다.
    void render(Renderer& renderer, const Mesh& mesh, const glm::mat4& model,
                const glm::vec3& lightPos, const Shader& depthShader) const;

    // depth 큐브맵을 그림자 전용 텍스처 유닛에 바인딩한다.
    void bind() const;

    // 메인 셰이더에 그림자 관련 uniform(shadowMap, shadowFarPlane, lightPos)을 설정한다.
    // 내부에서 shader.use()를 호출한다.
    void applyUniforms(const Shader& shader, const glm::vec3& lightPos) const;

private:
    GLuint depthCube = 0;
    GLuint framebuffer = 0;
};
