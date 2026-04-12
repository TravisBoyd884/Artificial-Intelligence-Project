#include "glitter.hpp"
#include "Camera/camera.hpp"
#include "Rendering/shader.hpp"
#include "Rendering/model.hpp"

// ---------------------------------------------------------------------------
// Test scene geometry — a simple lit floor plane.
// Replace (or supplement) this with Model objects once you have .glb exports
// from Blender.
//
// Layout: position(3) | normal(3) | texCoords(2)
// ---------------------------------------------------------------------------
static const float kFloorVertices[] = {
  // position              normal             texcoords
  -5.0f, 0.0f, -5.0f,   0.0f, 1.0f, 0.0f,  0.0f, 5.0f,
   5.0f, 0.0f, -5.0f,   0.0f, 1.0f, 0.0f,  5.0f, 5.0f,
   5.0f, 0.0f,  5.0f,   0.0f, 1.0f, 0.0f,  5.0f, 0.0f,
  -5.0f, 0.0f,  5.0f,   0.0f, 1.0f, 0.0f,  0.0f, 0.0f,
};
static const unsigned int kFloorIndices[] = { 0, 1, 2,  2, 3, 0 };

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static unsigned int buildFloorVAO() {
  unsigned int VAO, VBO, EBO;
  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);
  glGenBuffers(1, &EBO);

  glBindVertexArray(VAO);

  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(kFloorVertices), kFloorVertices, GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(kFloorIndices), kFloorIndices, GL_STATIC_DRAW);

  // position
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void *>(0));
  // normal
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void *>(3 * sizeof(float)));
  // texcoords
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void *>(6 * sizeof(float)));

  glBindVertexArray(0);
  return VAO;
}

// A 1×1 solid-color texture so the floor renders even without an image file.
static unsigned int buildSolidTexture(unsigned char r, unsigned char g, unsigned char b) {
  unsigned char pixel[] = {r, g, b};
  unsigned int texID;
  glGenTextures(1, &texID);
  glBindTexture(GL_TEXTURE_2D, texID);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, pixel);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  return texID;
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main() {
  const int kWidth  = 1280;
  const int kHeight = 720;

  Window window(kWidth, kHeight);
  window.captureMouse();

  Camera camera(glm::vec3(0.0f, 1.5f, 5.0f));

  Shader sceneShader(PROJECT_SOURCE_DIR "/Glitter/Shaders/scene.vs",
                     PROJECT_SOURCE_DIR "/Glitter/Shaders/scene.fs");

  unsigned int floorVAO    = buildFloorVAO();
  unsigned int floorTex    = buildSolidTexture(180, 170, 160); // warm grey

  // --- To load a Blender model, uncomment and adjust the path: ---
  // Model sceneModel(PROJECT_SOURCE_DIR "/Glitter/Assets/scene.glb");
  // Model agentModel(PROJECT_SOURCE_DIR "/Glitter/Assets/agent.glb");

  glEnable(GL_DEPTH_TEST);

  const glm::mat4 projection = glm::perspective(
    glm::radians(camera.getZoom()),
    static_cast<float>(kWidth) / static_cast<float>(kHeight),
    0.1f, 100.0f);

  float lastTime = 0.0f;

  while (glfwWindowShouldClose(window) == false) {
    float currentTime = static_cast<float>(glfwGetTime());
    float deltaTime   = currentTime - lastTime;
    lastTime          = currentTime;

    // --- Input ---
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

    // --- Render ---
    glClearColor(0.15f, 0.15f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    sceneShader.use();

    glm::mat4 view      = camera.getViewMatrix();
    glm::mat4 modelMat  = glm::mat4(1.0f);
    glm::mat3 normalMat = glm::mat3(glm::transpose(glm::inverse(modelMat)));

    sceneShader.setMat4("projection",  projection);
    sceneShader.setMat4("view",        view);
    sceneShader.setMat4("model",       modelMat);
    sceneShader.setMat3("normalMatrix",normalMat);
    sceneShader.setVec3("viewPos",     camera.getPosition());
    sceneShader.setVec3("light.direction", glm::vec3(-0.3f, -1.0f, -0.5f));
    sceneShader.setVec3("light.color",     glm::vec3(1.0f,  1.0f,  1.0f));

    // Draw floor
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, floorTex);
    sceneShader.setInt("texture_diffuse0", 0);
    glBindVertexArray(floorVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

    // --- Draw models (once assets are available) ---
    // sceneShader.setMat4("model", glm::mat4(1.0f));
    // sceneShader.setMat3("normalMatrix", glm::mat3(1.0f));
    // sceneModel.draw(sceneShader);
    //
    // glm::mat4 agentTransform = glm::translate(glm::mat4(1.0f), agentPosition);
    // sceneShader.setMat4("model", agentTransform);
    // sceneShader.setMat3("normalMatrix", glm::mat3(glm::transpose(glm::inverse(agentTransform))));
    // agentModel.draw(sceneShader);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  return EXIT_SUCCESS;
}
