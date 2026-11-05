//
// Created by Sohal03 on 26. 11. 5..
//

#include "renderer/Renderer.h"

#include <cstddef>
#include <ranges>

using namespace std;

void Renderer::upload(const Mesh& mesh) {
    if (meshBuffers.contains(&mesh))
        return;

    MeshBuffer buffer;

    glGenVertexArrays(1, &buffer.VAO);
    glGenBuffers(1, &buffer.VBO);
    glGenBuffers(1, &buffer.EBO);

    glBindVertexArray(buffer.VAO);

    glBindBuffer(GL_ARRAY_BUFFER, buffer.VBO);
    glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(Vertex), mesh.vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size() * sizeof(uint32_t), mesh.indices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, position)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, normal)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, texCoord)));

    glBindVertexArray(0);

    meshBuffers.emplace(&mesh, buffer);
}

void Renderer::draw(const Mesh& mesh, Material& materials, const Shader& shader) {
    if (!meshBuffers.contains(&mesh))
        upload(mesh);

    if (!materials.uploadTextures())
        throw runtime_error("Failed to upload material textures");

    shader.use();
    shader.setInt("texture_diffuse", 0);

    glBindVertexArray(meshBuffers.at(&mesh).VAO);

    for (const auto& [indexStart, indexCount, mtlName] : mesh.subMeshes) {
        const material& mat = materials.get(mtlName);

        if (mat.texture) {
            mat.texture->bind(0);
            shader.setBool("useTexture", true);
        } else {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, 0);
            shader.setBool("useTexture", false);
        }

        shader.setVec3("material.diffuse", mat.diffuse);

        const auto offset = static_cast<uintptr_t>(indexStart) * sizeof(uint32_t);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indexCount), GL_UNSIGNED_INT,
                       reinterpret_cast<const void*>(offset));
    }

    glBindVertexArray(0);
}

Renderer::~Renderer() {
    for (const auto& [VAO, VBO, EBO] : meshBuffers | views::values) {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
    }
}