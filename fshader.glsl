#version 410 core

in vec3 FragPos;
in vec3 Normal;

uniform vec4 objColor;
uniform vec3 lightDir;   // directional light
uniform vec3 viewPos;    // camera position

out vec4 fColor;

void main() {
    // Two-sided lighting - fix for perpendicular faces
    vec3 norm = normalize(gl_FrontFacing ? Normal : -Normal);
    
    // Multi-directional lighting for better visibility from all angles
    vec3 lightDirection1 = normalize(-lightDir);
    vec3 lightDirection2 = normalize(vec3(1.0, 0.0, 0.0));  // Side light
    vec3 lightDirection3 = normalize(vec3(0.0, 1.0, 0.0));  // Top light
    
    // Strong ambient for baseline visibility
    float ambientStrength = 0.5;
    vec3 ambient = ambientStrength * vec3(objColor);
    
    // Diffuse from multiple directions
    float diff1 = max(dot(norm, lightDirection1), 0.0);
    float diff2 = max(dot(norm, lightDirection2), 0.0) * 0.7;
    float diff3 = max(dot(norm, lightDirection3), 0.0) * 0.7;
    vec3 diffuse = (diff1 + diff2 + diff3) * vec3(objColor);
    
    // Specular
    float specularStrength = 0.4;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDirection1, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16.0);
    vec3 specular = specularStrength * spec * vec3(1.0);
    
    // Final result with enhanced brightness
    vec3 result = ambient + diffuse + specular;
    
    // Ensure minimum brightness (key for perpendicular visibility)
    float minBrightness = 0.3;
    float luminance = 0.299 * result.r + 0.587 * result.g + 0.114 * result.b;
    if (luminance < minBrightness) {
        float boost = minBrightness / max(luminance, 0.001);
        result = mix(result, result * boost, 0.8);
    }
    
    fColor = vec4(result, objColor.a);
}