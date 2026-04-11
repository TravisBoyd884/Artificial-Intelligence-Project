#pragma once
#include <cstdio>
#include <glad/glad.h> //
                       //
#include <GLFW/glfw3.h>

class Window {
public:
  Window(int screenWidth, int screenHeight);
  ~Window();
  operator GLFWwindow *() const { return m_Window; }
  void handle_input();

private:
  GLFWwindow *m_Window;
  int m_screenHeight;
  int m_screenWidth;

  static void framebuffer_size_callback(GLFWwindow *window, int width,
                                        int height);

  void init_GLFW();
};
