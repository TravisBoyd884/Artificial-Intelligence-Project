#include "glitter.hpp"
#include "Camera/camera.hpp"
#include "Rendering/shader.hpp"
#include "Scene/scene_config.hpp"
#include "Scene/primitive_builder.hpp"
#include "Game/game.hpp"

#include <algorithm>

static void applyLighting(const Shader& shader, const SceneConfig& cfg) {
  shader.setFloat("ambientIntensity", cfg.ambientIntensity);
  shader.setVec3 ("dirLight.direction", cfg.dirLight.direction);
  shader.setVec3 ("dirLight.color",     cfg.dirLight.color);
  shader.setFloat("dirLight.intensity", cfg.dirLight.intensity);

  int count = static_cast<int>(
    std::min(cfg.pointLights.size(), static_cast<size_t>(4)));
  shader.setInt("numPointLights", count);

  for (int i = 0; i < count; i++) {
    std::string base = "pointLights[" + std::to_string(i) + "].";
    shader.setVec3 (base + "position",  cfg.pointLights[i].position);
    shader.setVec3 (base + "color",     cfg.pointLights[i].color);
    shader.setFloat(base + "intensity", cfg.pointLights[i].intensity);
    shader.setFloat(base + "constant",  1.0f);
    shader.setFloat(base + "linear",    0.09f);
    shader.setFloat(base + "quadratic", 0.032f);
  }
}

static void drawMesh(const Shader& shader, const Mesh& mesh,
                     const glm::mat4& modelMat) {
  shader.setMat4("model",        modelMat);
  shader.setMat3("normalMatrix", glm::mat3(glm::transpose(glm::inverse(modelMat))));
  mesh.draw(shader);
}

int main(int argc, char** argv) {
  std::string configName = (argc > 1) ? argv[1] : "low";
  std::string configPath = PROJECT_SOURCE_DIR
                           "/Glitter/Assets/Configs/" + configName + ".json";

  SceneConfig cfg;
  try {
    cfg = loadSceneConfig(configPath);
  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return EXIT_FAILURE;
  }

  const int kWidth  = 1280;
  const int kHeight = 720;

  Window window(kWidth, kHeight);
  window.captureMouse();

  Camera camera(glm::vec3(0.0f, 1.5f, 4.5f),
                glm::vec3(0.0f, 1.0f, 0.0f),
                -90.0f, 0.0f);

  Shader sceneShader(PROJECT_SOURCE_DIR "/Glitter/Shaders/scene.vs",
                     PROJECT_SOURCE_DIR "/Glitter/Shaders/scene.fs");

  std::string texDir = std::string(PROJECT_SOURCE_DIR) + "/Glitter/Assets/Textures/";

  auto resolveTexture = [&](const std::string& filename,
                            unsigned char r, unsigned char g, unsigned char b) {
    if (filename.empty())
      return PrimitiveBuilder::solidColorTexture(r, g, b);
    return PrimitiveBuilder::loadTexture(texDir + filename);
  };

  unsigned int floorTex  = resolveTexture(cfg.floorTexture,   160, 150, 130);
  unsigned int wallTex   = resolveTexture(cfg.wallTexture,    180, 180, 180);
  unsigned int ceilTex   = resolveTexture(cfg.ceilingTexture, 200, 200, 200);
  unsigned int hoopTex   = PrimitiveBuilder::solidColorTexture(220,  80,  20);
  unsigned int boardTex  = PrimitiveBuilder::solidColorTexture(230, 230, 230);
  unsigned int ballTex   = PrimitiveBuilder::solidColorTexture(210, 100,  30);

  Mesh floor     = PrimitiveBuilder::buildFloor    (6.0f, 4.0f, floorTex);
  Mesh walls     = PrimitiveBuilder::buildWalls    (12.0f, 12.0f, 6.0f, wallTex);
  Mesh ceiling   = PrimitiveBuilder::buildCeiling  (6.0f, 6.0f, ceilTex);
  Mesh backboard = PrimitiveBuilder::buildBackboard(glm::vec3(0.0f, 3.4f, -5.1f), 1.83f, 1.07f, boardTex);
  Mesh hoop      = PrimitiveBuilder::buildHoop     (glm::vec3(0.0f, 3.0f, -5.0f), 0.23f, 0.025f, 24, hoopTex);
  Mesh ball      = PrimitiveBuilder::buildSphere   (0.12f, 16, 16, ballTex);

  Game game;

  glEnable(GL_DEPTH_TEST);

  const glm::mat4 projection = glm::perspective(
      glm::radians(camera.getZoom()),
      static_cast<float>(kWidth) / static_cast<float>(kHeight),
      0.1f, 100.0f);

  float lastTime   = 0.0f;
  bool  spacePrev  = false;
  bool  rPrev      = false;

  while (glfwWindowShouldClose(window) == false) {
    float currentTime = static_cast<float>(glfwGetTime());
    float deltaTime   = std::min(currentTime - lastTime, 0.1f);
    lastTime          = currentTime;

    window.handle_input();

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
      camera.processKeyboard(CameraMovement::FORWARD,  deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
      camera.processKeyboard(CameraMovement::BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
      camera.processKeyboard(CameraMovement::LEFT,     deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
      camera.processKeyboard(CameraMovement::RIGHT,    deltaTime);

    glm::vec2 mouseDelta = window.getMouseDelta();
    camera.processMouseMovement(mouseDelta.x, mouseDelta.y);

    bool spaceNow = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
    bool rNow     = glfwGetKey(window, GLFW_KEY_R)     == GLFW_PRESS;

    if (spaceNow && !spacePrev && game.getState() == GameState::IDLE)
      game.shoot(camera.getYaw(), 25.0f, 15.0f);

    if (rNow && !rPrev)
      game.reset();

    spacePrev = spaceNow;
    rPrev     = rNow;

    game.update(deltaTime);

    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    sceneShader.use();
    sceneShader.setMat4("projection", projection);
    sceneShader.setMat4("view",       camera.getViewMatrix());
    sceneShader.setVec3("viewPos",    camera.getPosition());
    applyLighting(sceneShader, cfg);

    glm::mat4 identity(1.0f);
    drawMesh(sceneShader, floor,     identity);
    drawMesh(sceneShader, walls,     identity);
    drawMesh(sceneShader, ceiling,   identity);
    drawMesh(sceneShader, backboard, identity);
    drawMesh(sceneShader, hoop,      identity);

    glm::mat4 ballTransform = glm::translate(identity, game.getBallPosition());
    drawMesh(sceneShader, ball, ballTransform);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  return EXIT_SUCCESS;
}
