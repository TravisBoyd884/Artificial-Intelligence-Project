#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

enum class CameraMovement { FORWARD, BACKWARD, LEFT, RIGHT };

class Camera {
public:
  // Sensible defaults for a first-person camera.
  static constexpr float DEFAULT_SPEED       = 2.5f;
  static constexpr float DEFAULT_SENSITIVITY = 0.1f;
  static constexpr float DEFAULT_ZOOM        = 45.0f;

  // position   — starting world position
  // worldUp    — the scene's up axis (almost always +Y)
  // yaw        — horizontal rotation in degrees (-90 faces -Z, the OpenGL default)
  // pitch      — vertical rotation in degrees
  Camera(glm::vec3 position  = glm::vec3(0.0f, 0.0f, 3.0f),
         glm::vec3 worldUp   = glm::vec3(0.0f, 1.0f, 0.0f),
         float     yaw       = -90.0f,
         float     pitch     = 0.0f);

  // Returns the view matrix computed from the current position and orientation.
  glm::mat4 getViewMatrix() const;

  glm::vec3 getPosition() const { return m_position; }
  float     getZoom()     const { return m_zoom; }

  // direction  — which way to move
  // deltaTime  — seconds since last frame (keeps speed frame-rate independent)
  void processKeyboard(CameraMovement direction, float deltaTime);

  // xoffset / yoffset — raw mouse deltas in screen pixels
  // constrainPitch    — clamp pitch to ±89° so the view never flips
  void processMouseMovement(float xoffset, float yoffset,
                            bool constrainPitch = true);

private:
  glm::vec3 m_position;
  glm::vec3 m_front;
  glm::vec3 m_up;
  glm::vec3 m_right;
  glm::vec3 m_worldUp;

  float m_yaw;
  float m_pitch;
  float m_movementSpeed;
  float m_mouseSensitivity;
  float m_zoom;

  // Recomputes m_front, m_right, and m_up from the current yaw and pitch.
  void updateVectors();
};
