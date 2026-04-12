#version 330 core

#define MAX_POINT_LIGHTS 4

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

struct DirLight {
  vec3  direction;
  vec3  color;
  float intensity;
};

struct PointLight {
  vec3  position;
  vec3  color;
  float intensity;
  float constant;
  float linear;
  float quadratic;
};

uniform sampler2D texture_diffuse0;
uniform float     ambientIntensity;
uniform vec3      viewPos;
uniform DirLight  dirLight;
uniform PointLight pointLights[MAX_POINT_LIGHTS];
uniform int        numPointLights;

vec3 calcDirLight(DirLight light, vec3 norm, vec3 viewDir, vec3 albedo) {
  vec3  lightDir   = normalize(-light.direction);
  float diff       = max(dot(norm, lightDir), 0.0);
  vec3  halfwayDir = normalize(lightDir + viewDir);
  float spec       = pow(max(dot(norm, halfwayDir), 0.0), 32.0);
  return diff * light.color * light.intensity * albedo
       + 0.3 * spec * light.color * light.intensity;
}

vec3 calcPointLight(PointLight light, vec3 norm, vec3 fragPos, vec3 viewDir, vec3 albedo) {
  vec3  lightDir   = normalize(light.position - fragPos);
  float diff       = max(dot(norm, lightDir), 0.0);
  vec3  halfwayDir = normalize(lightDir + viewDir);
  float spec       = pow(max(dot(norm, halfwayDir), 0.0), 32.0);
  float dist        = length(light.position - fragPos);
  float attenuation = 1.0 / (light.constant + light.linear * dist + light.quadratic * dist * dist);
  return (diff * light.color * light.intensity * albedo
        + 0.3 * spec * light.color * light.intensity) * attenuation;
}

void main() {
  vec4 texColor = texture(texture_diffuse0, TexCoords);
  vec3 norm     = normalize(Normal);
  vec3 viewDir  = normalize(viewPos - FragPos);
  vec3 result   = ambientIntensity * texColor.rgb;

  result += calcDirLight(dirLight, norm, viewDir, texColor.rgb);

  for (int i = 0; i < numPointLights; i++)
    result += calcPointLight(pointLights[i], norm, FragPos, viewDir, texColor.rgb);

  FragColor = vec4(result, texColor.a);
}
