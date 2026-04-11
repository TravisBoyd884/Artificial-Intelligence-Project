// Local Headers
#include "GLFW/glfw3.h"
#include "Rendering/shader.hpp"
#include "glitter.hpp"

int main() {

  Window window(800, 600);

  Shader ourShader(PROJECT_SOURCE_DIR "/Glitter/Shaders/shader.vs",
                   PROJECT_SOURCE_DIR "/Glitter/Shaders/shader.fs");

  float vertices[] = {
      -0.5f, -0.5f, 0.0f, // 0 bottom left
      -0.5f, 0.5f,  0.0f, // 1 top left
      0.5f,  0.5f,  0.0f, // 2 top right
      0.5f,  -0.5f, 0.0f, // 3 bottom right
  };
  unsigned int indices[] = {
      1, 0, 3, //
      1, 2, 3  //
  };

  unsigned int VAO;
  glGenVertexArrays(1, &VAO);
  glBindVertexArray(VAO);

  unsigned int VBO;
  glGenBuffers(1, &VBO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  unsigned int EBO;
  glGenBuffers(1, &EBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices,
               GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);

  glEnableVertexAttribArray(0);

  // Rendering Loop
  while (glfwWindowShouldClose(window) == false) {
    window.handle_input();

    glClearColor(0.25f, 0.25f, 0.25f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    float time = glfwGetTime();
    float timeValue = (sin(time * 3) / 4.0f) + 0.75f;

    // Background Fill Color
    ourShader.use();
    ourShader.setFloat("sineTime", timeValue);
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    // glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);

    // Flip Buffers and Draw
    glfwSwapBuffers(window);
    glfwPollEvents();
  }
  return EXIT_SUCCESS;
}
