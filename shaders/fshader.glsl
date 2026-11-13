#version 410 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform sampler2D texture_diffuse;
uniform samplerCubeShadow shadowMap;
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform float shadowFarPlane;

float lightVisibility(vec3 fromLight, float distanceToLight, float diffuseFactor) {
    if (distanceToLight >= shadowFarPlane || diffuseFactor <= 0.0)
        return 1.0;

    float bias = max(0.045 * (1.0 - diffuseFactor), 0.012);
    float referenceDepth = (distanceToLight - bias) / shadowFarPlane;
    return texture(shadowMap, vec4(fromLight, referenceDepth));
}

void main() {
    // Ambient
    float ambientStrength = 0.3;
    vec3 ambient = ambientStrength * vec3(1.0);
  
    // Diffuse 
    vec3 norm = normalize(Normal);
    vec3 fromLight = FragPos - lightPos;
    float distanceToLight = length(fromLight);
    vec3 lightDir = -fromLight / max(distanceToLight, 0.0001);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * vec3(1.0);
    
    vec4 texColor = texture(texture_diffuse, TexCoords);
    float visibility = lightVisibility(fromLight, distanceToLight, diff);
    vec3 result = (ambient + visibility * diffuse) * texColor.rgb;
    FragColor = vec4(result, texColor.a);
}