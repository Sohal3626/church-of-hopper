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

    const array extracted = {
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

bool Frustum::containsPoint(const vec3& point) const {
    return std::ranges::all_of(planes, [&point](const auto& plane) {
        return glm::dot(plane.normal, point) + plane.distance >= 0.0f;
    });
}

bool Frustum::intersectsAABB(const AABB& box, const mat4& model) const {
    const vec3 center = (box.min + box.max) * 0.5f;
    const vec3 extent = (box.max - box.min) * 0.5f;
    const auto worldCenter = vec3(model * vec4(center, 1.0f));
    vec3 worldExtent(0.0f);

    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            worldExtent[i] += std::abs(model[j][i]) * extent[j];

    const AABB worldBox{
        worldCenter - worldExtent,
        worldCenter + worldExtent
    };

    return intersectsAABB(worldBox);
}

bool Frustum::intersectsAABB(const AABB& box) const {
    for (const auto& [normal, distance] : planes) {
        vec3 positiveVertex;

        positiveVertex.x = normal.x >= 0.0f ? box.max.x : box.min.x;
        positiveVertex.y = normal.y >= 0.0f ? box.max.y : box.min.y;
        positiveVertex.z = normal.z >= 0.0f ? box.max.z : box.min.z;

        if (dot(normal, positiveVertex) + distance < 0.0f) return false;
    }

    return true;
}