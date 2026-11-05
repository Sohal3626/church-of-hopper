//
// Created by desktop on 26. 3. 19..
//

#ifndef TOYRENDERER2_MATERIAL_H
#define TOYRENDERER2_MATERIAL_H
#include <cstdint>
#include <map>
#include <string>
#include <vector>
#include <glm/glm.hpp>

#include "../../src/dataLoader/Texture.h"

struct material {
    int16_t illumination = 0;
    float dissolve = 1.f;
    float shininess = 0.f;
    float opticalDensity = 1.f;

    glm::vec3 ambient = {0.2f, 0.2f, 0.2f};
    glm::vec3 diffuse = {0.8f, 0.8f, 0.8f};
    glm::vec3 specular = {0.f, 0.f, 0.f};
    glm::vec3 emission = {0.f, 0.f, 0.f};

    const Texture* texture = nullptr;
};

class Material {
public:
    bool loadMtl(const std::string &filename);

    [[nodiscard]] const material& get(const std::string &name) const {
        if (const material* m = find(name)) return *m;
        static constexpr material default_material{};
        return default_material;
    }

    [[nodiscard]] const material* find(const std::string& name) const {
        const auto it = mtls.find(name);
        if (it == mtls.end()) return nullptr;
        return &it->second;
    }

private:
    std::map<std::string,material> mtls;
    std::map<std::string, Texture> textures;
};


#endif //TOYRENDERER2_MATERIAL_H