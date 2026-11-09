//
// Created by Sohal03 on 26. 11. 9..
//

#ifndef CHURCH_OF_HOPPER_FRUSTUM_H
#define CHURCH_OF_HOPPER_FRUSTUM_H

#include <array>
#include <glm/glm.hpp>

#include "resource/Mesh.h"

class Frustum {
public:
    explicit Frustum(const glm::mat4& viewProjection);
    [[nodiscard]] bool containsPoint(const glm::vec3& point) const;
    [[nodiscard]] bool intersectsAABB(const AABB& box, const glm::mat4& model) const;
    [[nodiscard]] bool intersectsAABB(const AABB& box) const;

private:
    struct Plane {
        glm::vec3 normal;
        float distance;
    };

    std::array<Plane, 6> planes{};
    void extractPlanes(const glm::mat4& m);
};

#endif //CHURCH_OF_HOPPER_FRUSTUM_H