#include "glitter.hpp"
#include "Camera/camera.hpp"
#include "Rendering/shader.hpp"
#include "Scene/scene_config.hpp"
#include "Scene/primitive_builder.hpp"
#include "Game/game.hpp"
#include "RL/fbo_capture.hpp"
#include "RL/env_server.hpp"

#include <algorithm>
#include <cmath>
#include <poll.h>

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

static bool socketReady(int fd) {
  struct pollfd pfd{};
  pfd.fd     = fd;
  pfd.events = POLLIN;
  return poll(&pfd, 1, 0) > 0;
}

int main(int argc, char** argv) {
  bool        rlMode      = false;
  bool        renderMode  = false;
  bool        fixedSpawn  = false;
  std::string configName  = "low";

  for (int i = 1; i < argc; i++) {
    if      (std::string(argv[i]) == "--rl")           rlMode     = true;
    else if (std::string(argv[i]) == "--render")       renderMode = true;
    else if (std::string(argv[i]) == "--fixed-spawn")  fixedSpawn = true;
    else                                               configName = argv[i];
  }

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

  if (rlMode && !renderMode) glfwSwapInterval(0);

  // Use the actual framebuffer pixel dimensions for viewports — they differ
  // from kWidth/kHeight on HiDPI compositors.
  int fbWidth = kWidth, fbHeight = kHeight;
  glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
  glViewport(0, 0, fbWidth, fbHeight);

  // Shared initial camera position used by both modes
  Camera camera(glm::vec3(0.0f, 1.8f, 5.5f),
                glm::vec3(0.0f, 1.0f, 0.0f),
                -90.0f, -5.0f);

  Shader sceneShader(PROJECT_SOURCE_DIR "/Glitter/Shaders/scene.vs",
                     PROJECT_SOURCE_DIR "/Glitter/Shaders/scene.fs");
  Shader lineShader (PROJECT_SOURCE_DIR "/Glitter/Shaders/line.vs",
                     PROJECT_SOURCE_DIR "/Glitter/Shaders/line.fs");

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
  game.setFixedSpawn(fixedSpawn);

  // Builds a camera sitting at the ball spawn, looking toward the hoop.
  // Called once per episode so the agent always sees from its own perspective.
  static const glm::vec3 kHoopPos(0.0f, 3.0f, -5.0f);
  auto makeAgentCam = [&]() {
    glm::vec3 start = game.getBallStart();
    glm::vec3 eye   = glm::vec3(start.x, start.y + 0.6f, start.z);
    glm::vec3 dir   = glm::normalize(kHoopPos - eye);
    float pitch = glm::degrees(std::asin(dir.y));
    float yaw   = glm::degrees(std::atan2(dir.z, dir.x));
    return Camera(eye, glm::vec3(0.0f, 1.0f, 0.0f), yaw, pitch);
  };

  // Net rendering — dynamic line geometry updated every frame.
  // Constraints: N_COLS×N_ROWS horizontal rings + 2×N_COLS×N_ROWS diagonals
  // = 3 × N_COLS × N_ROWS segments; 2 verts each.
  const int kNetMaxVerts = 2 * 3 * Net::N_COLS * Net::N_ROWS + 64; // +64 safety margin
  GLuint netVAO, netVBO;
  glGenVertexArrays(1, &netVAO);
  glGenBuffers(1, &netVBO);
  glBindVertexArray(netVAO);
  glBindBuffer(GL_ARRAY_BUFFER, netVBO);
  glBufferData(GL_ARRAY_BUFFER, kNetMaxVerts * static_cast<int>(sizeof(glm::vec3)),
               nullptr, GL_DYNAMIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);
  glEnableVertexAttribArray(0);
  glBindVertexArray(0);

  glEnable(GL_DEPTH_TEST);

  // Recomputed each frame from the live fbWidth/fbHeight (see loops below).
  auto makeWinProjection = [&]() {
      return glm::perspective(
          glm::radians(camera.getZoom()),
          fbHeight > 0 ? static_cast<float>(fbWidth) / static_cast<float>(fbHeight) : 1.0f,
          0.1f, 100.0f);
  };

  const glm::mat4 fboProjection = glm::perspective(
      glm::radians(camera.getZoom()),
      1.0f, 0.1f, 100.0f);

  // Takes an explicit camera so RL mode can separate display vs. agent views
  auto renderScene = [&](const glm::mat4& proj, const Camera& cam) {
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    sceneShader.use();
    sceneShader.setMat4("projection", proj);
    sceneShader.setMat4("view",       cam.getViewMatrix());
    sceneShader.setVec3("viewPos",    cam.getPosition());
    applyLighting(sceneShader, cfg);
    glm::mat4 identity(1.0f);
    drawMesh(sceneShader, floor,     identity);
    drawMesh(sceneShader, walls,     identity);
    drawMesh(sceneShader, ceiling,   identity);
    drawMesh(sceneShader, backboard, identity);
    drawMesh(sceneShader, hoop,      identity);
    glm::mat4 ballTransform = glm::translate(identity, game.getBallPosition());
    drawMesh(sceneShader, ball, ballTransform);
  };

  // Draws the net as GL_LINES over whatever was rendered by renderScene.
  auto drawNet = [&](const glm::mat4& proj, const Camera& cam) {
    const auto& segs = game.getNet().segments();
    std::vector<glm::vec3> verts;
    verts.reserve(segs.size() * 2);
    for (const auto& s : segs) {
      verts.push_back(s.a);
      verts.push_back(s.b);
    }
    glBindBuffer(GL_ARRAY_BUFFER, netVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0,
                    static_cast<GLsizeiptr>(verts.size() * sizeof(glm::vec3)),
                    verts.data());

    lineShader.use();
    lineShader.setMat4("projection", proj);
    lineShader.setMat4("view",       cam.getViewMatrix());
    lineShader.setVec3("lineColor",  glm::vec3(0.92f, 0.92f, 0.92f));

    glBindVertexArray(netVAO);
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(verts.size()));
    glBindVertexArray(0);
  };

  // ---------------------------------------------------------------
  // RL mode
  // ---------------------------------------------------------------
  if (rlMode) {
    FBOCapture fbo(84, 84);

    // Positioned at the ball spawn each episode, looking toward the hoop.
    // Updated on every reset so the observation reflects the new spawn point.
    Camera agentCam = makeAgentCam();

    auto captureObs = [&]() -> std::vector<uint8_t> {
      fbo.bind();
      glViewport(0, 0, fbo.width(), fbo.height());
      renderScene(fboProjection, agentCam);
      drawNet(fboProjection, agentCam);
      auto px = fbo.readPixels();
      fbo.unbind();
      glViewport(0, 0, fbWidth, fbHeight);
      return px;
    };

    EnvServer server("/tmp/glitter_env.sock");
    server.waitForClient();

    // Send the initial observation before the first action arrives
    server.sendObs(captureObs(), 0.0f, false);

    float lastTime  = static_cast<float>(glfwGetTime());
    bool  needsSend = false;   // true when an episode just finished

    while (!glfwWindowShouldClose(window)) {
      float now  = static_cast<float>(glfwGetTime());
      float dt   = std::min(now - lastTime, 0.1f);
      lastTime   = now;

      // Refresh framebuffer size every frame — the WM may have resized the window.
      glfwGetFramebufferSize(window, &fbWidth, &fbHeight);

      glfwPollEvents();
      window.handle_input();

      // Camera movement so the user can fly around and watch
      if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.processKeyboard(CameraMovement::FORWARD,  dt);
      if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.processKeyboard(CameraMovement::BACKWARD, dt);
      if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.processKeyboard(CameraMovement::LEFT,     dt);
      if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.processKeyboard(CameraMovement::RIGHT,    dt);
      glm::vec2 mouseDelta = window.getMouseDelta();
      camera.processMouseMovement(mouseDelta.x, mouseDelta.y);

      // Non-blocking: pick up an action from Python if one has arrived.
      // Accept when IDLE (step) or SCORED/MISSED (reset) — never during flight.
      if (socketReady(server.clientFd()) &&
          game.getState() != GameState::IN_FLIGHT) {
        auto action = server.recvAction();
        if (action.cmd == 0) {
          game.reset();
          agentCam  = makeAgentCam();  // reposition to new spawn, look at hoop
          needsSend = false;
          server.sendObs(captureObs(), 0.0f, false);
        } else if (game.getState() == GameState::IDLE) {
          // action.yaw is a yaw *offset* from the spawn→hoop direction.
          // agentCam is already aimed at the hoop, so its yaw IS that base.
          game.shoot(agentCam.getYaw() + action.yaw, action.pitch, action.power);
        }
      }

      // Step physics while the ball is in flight.
      // In fast mode run the whole flight in one tight CPU loop so training
      // doesn't slow to real-time; in --render mode advance one step per
      // frame so the animation plays back smoothly.
      if (game.getState() == GameState::IN_FLIGHT) {
        if (renderMode) {
          game.update(dt);
        } else {
          while (game.getState() == GameState::IN_FLIGHT)
            game.update(1.0f / 60.0f);
        }
      }

      // Episode just ended — send the terminal observation once
      if (game.isDone() && !needsSend) {
        needsSend = true;
        server.sendObs(captureObs(), game.getReward(), true);
      }
      if (!game.isDone())
        needsSend = false;

      // Always render to window every frame so it stays responsive
      glViewport(0, 0, fbWidth, fbHeight);
      renderScene(makeWinProjection(), camera);
      drawNet(makeWinProjection(), camera);
      glfwSwapBuffers(window);
    }

    return EXIT_SUCCESS;
  }

  // ---------------------------------------------------------------
  // Interactive mode
  // ---------------------------------------------------------------
  float lastTime  = 0.0f;
  bool  spacePrev = false;
  bool  rPrev     = false;

  while (glfwWindowShouldClose(window) == false) {
    float currentTime = static_cast<float>(glfwGetTime());
    float deltaTime   = std::min(currentTime - lastTime, 0.1f);
    lastTime          = currentTime;

    // Refresh framebuffer size every frame — the WM may have resized the window.
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);

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

    glViewport(0, 0, fbWidth, fbHeight);
    renderScene(makeWinProjection(), camera);
    drawNet(makeWinProjection(), camera);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  return EXIT_SUCCESS;
}
