//
// Created by Sohal03 on 26. 11. 5..
//

#ifndef CHURCH_OF_HOPPER_RENDERER_H
#define CHURCH_OF_HOPPER_RENDERER_H
#include <glad/glad.h>

#include "Shader.h"
#include "resource/Material.h"
#include "resource/Mesh.h"

#pragma once

#include <unordered_map>

class Renderer {
public:
    Renderer() = default;
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    void draw(const Mesh& mesh, Material& materials, const Shader& shader);
    void drawDepth(const Mesh& mesh);

    Renderer(Renderer&&) = delete;
    Renderer& operator=(Renderer&&) = delete;
private:
    struct MeshBuffer {
        GLuint VAO = 0;
        GLuint VBO = 0;
        GLuint EBO = 0;
    };

    std::unordered_map<const Mesh*, MeshBuffer> meshBuffers;
    void upload(const Mesh& mesh);
};

#endif //CHURCH_OF_HOPPER_RENDERER_H