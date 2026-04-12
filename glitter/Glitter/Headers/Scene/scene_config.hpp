#pragma once

#include <glm/glm.hpp>
#include <string>
#include <vector>

struct DirLightConfig {
  glm::vec3 direction = glm::vec3(-0.3f, -1.0f, -0.5f);
  glm::vec3 color     = glm::vec3(1.0f);
  float     intensity = 1.0f;
};

struct PointLightConfig {
  glm::vec3 position  = glm::vec3(0.0f);
  glm::vec3 color     = glm::vec3(1.0f);
  float     intensity = 1.0f;
};

struct SceneConfig {
  std::string name;
  std::string complexityLevel;

  std::string floorTexture;
  std::string wallTexture;
  std::string ceilingTexture;

  float                        ambientIntensity = 0.15f;
  DirLightConfig               dirLight;
  std::vector<PointLightConfig> pointLights;
};

SceneConfig loadSceneConfig(const std::string& path);
