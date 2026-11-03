//
// Created by desktop on 26. 3. 19..
//

#ifndef TOYRENDERER2_MESH_H
#define TOYRENDERER2_MESH_H
#include <string>
#include <vector>
#include <cstdint>
#include <functional>
#include <glm/glm.hpp>

#include "../library/Geometry.h"
#include "Material.h"

struct alignas(16) Vertex {
    vec4 position;
    vec4 normal;
    vec2 texCoord;
    float padding[2];

    bool operator==(const Vertex& v) const {
        return position == v.position && texCoord == v.texCoord && normal == v.normal;
    }
};

class Mesh {
public:
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    struct MeshGroup {
        uint32_t indexStart;
        uint32_t indexCount;
        uint16_t mtlId = 65535;
        std::string mtlName;
    };
    std::vector<MeshGroup> subMeshes;

    bool loadObj(const std::string& filename);
    void linkMtl(const Material&);
    [[nodiscard]] bool hasMtl() const;
    [[nodiscard]] uint16_t getMtlId(int index) const;

private:
    void set_indexStart();
};

struct alignas(16) FaceVertex {
    int position;
    int texCoord;
    int normal;
    FaceVertex (const std::initializer_list<int> list){
        auto it = list.begin();
        while (it != list.end()) {
            if (it != list.end()) position = *it++;
            if (it != list.end()) texCoord = *it++;
            if (it != list.end()) normal = *it++;
        }
    }
    auto operator<=>(const FaceVertex&) const = default;
};

#endif //TOYRENDERER2_MESH_H