#version 330 core

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

// Single directional light — sunlight-style, no attenuation.
struct DirLight {
    vec3 direction; // points FROM the light TOWARD the scene
    vec3 color;
};

uniform sampler2D texture_diffuse0;
uniform DirLight  light;
uniform vec3      viewPos;

void main() {
    vec4 texColor = texture(texture_diffuse0, TexCoords);

    vec3 norm     = normalize(Normal);
    vec3 lightDir = normalize(-light.direction);  // flip to point toward light
    vec3 viewDir  = normalize(viewPos - FragPos);

    // Ambient — low constant fill so shadowed areas are not completely black.
    vec3 ambient  = 0.15 * texColor.rgb;

    // Diffuse — Lambert
    float diff    = max(dot(norm, lightDir), 0.0);
    vec3 diffuse  = diff * light.color * texColor.rgb;

    // Specular — Blinn-Phong with a fixed shininess.
    vec3  halfwayDir = normalize(lightDir + viewDir);
    float spec       = pow(max(dot(norm, halfwayDir), 0.0), 32.0);
    vec3  specular   = 0.3 * spec * light.color;

    FragColor = vec4(ambient + diffuse + specular, texColor.a);
}
