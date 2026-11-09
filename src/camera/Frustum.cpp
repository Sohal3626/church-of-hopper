//
// Created by Sohal03 on 26. 11. 9..
//
#include "camera/Frustum.h"
#include <glm/geometric.hpp>

using namespace glm;
using namespace std;

Frustum::Frustum(const mat4& viewProjection) { extractPlanes(viewProjection); }

void Frustum::extractPlanes(const mat4& m) {
    const vec4 row0(m[0][0], m[1][0], m[2][0], m[3][0]);
    const vec4 row1(m[0][1], m[1][1], m[2][1], m[3][1]);
    const vec4 row2(m[0][2], m[1][2], m[2][2], m[3][2]);
    const vec4 row3(m[0][3], m[1][3], m[2][3], m[3][3]);

    const array<vec4, 6> extracted = {
        row3 + row0,
        row3 - row0,
        row3 + row1,
        row3 - row1,
        row3 + row2,
        row3 - row2
    };

    for (size_t i = 0; i < planes.size(); ++i) {
        const vec3 normal(extracted[i]);
        const float length = glm::length(normal);
        planes[i].normal = normal / length;
        planes[i].distance = extracted[i].w / length;
    }
}

bool Frustum::containsPoint(const glm::vec3& point) const {
    return std::ranges::all_of(planes, [&point](const auto& plane) {
        return glm::dot(plane.normal, point) + plane.distance >= 0.0f;
    });
}