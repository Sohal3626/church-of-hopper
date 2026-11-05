//
// Created by desktop on 26. 3. 19..
//

#ifndef TOYRENDERER2_MESH_H
#define TOYRENDERER2_MESH_H
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "Material.h"

struct alignas(16) Vertex {
    glm::vec4 position;
    glm::vec4 normal;
    glm::vec2 texCoord;
    float padding[2];

    bool operator==(const Vertex& v) const {
        return position == v.position && texCoord == v.texCoord && normal == v.normal;
    }
};

class Mesh {
public:
    Mesh() = default;
    explicit Mesh(const std::string& filename);
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    struct MeshGroup {
        uint32_t indexStart;
        uint32_t indexCount;
        std::string mtlName;
    };
    std::vector<MeshGroup> subMeshes;

    bool loadObj(const std::string& filename);

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