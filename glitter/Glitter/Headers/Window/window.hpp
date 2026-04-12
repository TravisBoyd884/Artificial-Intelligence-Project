#pragma once
#include <cstdio>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

class Window {
public:
  Window(int screenWidth, int screenHeight);
  ~Window();

  operator GLFWwindow *() const { return m_Window; }

  void handle_input();

  // Enable cursor capture and start tracking mouse movement.
  void captureMouse();

  // Returns the mouse offset since the last call, then resets it to zero.
  // X is positive right, Y is positive up (already inverted from GLFW).
  glm::vec2 getMouseDelta();

private:
  GLFWwindow *m_Window;
  int m_screenHeight;
  int m_screenWidth;

  double m_lastMouseX = 0.0;
  double m_lastMouseY = 0.0;
  float  m_mouseOffsetX = 0.0f;
  float  m_mouseOffsetY = 0.0f;
  bool   m_firstMouse = true;

  static void framebuffer_size_callback(GLFWwindow *window, int width, int height);
  static void mouse_callback(GLFWwindow *window, double xpos, double ypos);

  void init_GLFW();
};
