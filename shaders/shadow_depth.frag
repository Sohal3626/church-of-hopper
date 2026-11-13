#version 410 core
in vec3 worldPosition;

uniform vec3 lightPos;
uniform float shadowFarPlane;

void main() {
    gl_FragDepth = length(worldPosition - lightPos) / shadowFarPlane;
}
