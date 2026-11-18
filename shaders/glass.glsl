#version 410 core

struct Material {
    vec3 diffuse;
    vec3 emission;
};

uniform Material material;

out vec4 FragColor;

void main() {
    vec3 color = 0.55 * material.diffuse + material.emission;
    FragColor = vec4(color, 1.0);
}