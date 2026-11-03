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

#include "Texture.h"

struct material {
    uint16_t materialId = 65535;
    uint16_t textureId = 65535;
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
        if (const auto it = mtls.find(name); it != mtls.end()) return it->second;
        static const material default_material;
        std::cout << "using default material" << std::endl;
        return default_material;
    }
private:
    std::map<std::string,material> mtls;
    std::map<std::string, Texture> textures;
};


#endif //TOYRENDERER2_MATERIAL_H