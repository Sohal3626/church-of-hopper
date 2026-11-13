#version 410 core
layout (location = 0) in vec3 aPos;

out vec3 worldPosition;

uniform mat4 model;
uniform mat4 lightViewProjection;

void main() {
    vec4 position = model * vec4(aPos, 1.0);
    worldPosition = position.xyz;
    gl_Position = lightViewProjection * position;
}
