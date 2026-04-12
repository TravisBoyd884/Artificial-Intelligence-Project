#include "Scene/scene_config.hpp"

#include <json.hpp>
#include <fstream>
#include <stdexcept>

using json = nlohmann::json;

static glm::vec3 vec3FromJson(const json& j) {
  return glm::vec3(j[0].get<float>(), j[1].get<float>(), j[2].get<float>());
}

SceneConfig loadSceneConfig(const std::string& path) {
  std::ifstream file(path);
  if (!file.is_open())
    throw std::runtime_error("SceneConfig: cannot open " + path);

  json j;
  file >> j;

  SceneConfig cfg;
  cfg.name            = j.value("name", "unnamed");
  cfg.complexityLevel = j.value("complexityLevel", "low");

  if (j.contains("textures")) {
    const auto& t  = j["textures"];
    cfg.floorTexture   = t.value("floor",   "");
    cfg.wallTexture    = t.value("wall",    "");
    cfg.ceilingTexture = t.value("ceiling", "");
  }

  if (j.contains("lighting")) {
    const auto& l    = j["lighting"];
    cfg.ambientIntensity = l.value("ambient", 0.15f);

    if (l.contains("dirLight")) {
      const auto& d      = l["dirLight"];
      cfg.dirLight.direction = vec3FromJson(d["direction"]);
      cfg.dirLight.color     = vec3FromJson(d["color"]);
      cfg.dirLight.intensity = d.value("intensity", 1.0f);
    }

    if (l.contains("pointLights")) {
      for (const auto& p : l["pointLights"]) {
        PointLightConfig pl;
        pl.position  = vec3FromJson(p["position"]);
        pl.color     = vec3FromJson(p["color"]);
        pl.intensity = p.value("intensity", 1.0f);
        cfg.pointLights.push_back(pl);
      }
    }
  }

  return cfg;
}
