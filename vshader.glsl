#version 410 core

in vec4 vPosition;
in vec3 vNormal;

uniform mat4 model;
uniform mat4 projection;

out vec3 FragPos;
out vec3 Normal;

void main() {
    vec4 worldPos = model * vPosition;
    FragPos = worldPos.xyz;
    // Transform normals using the inverse transpose of the model matrix
    Normal = mat3(transpose(inverse(model))) * vNormal;
    gl_Position = projection * worldPos;
}
