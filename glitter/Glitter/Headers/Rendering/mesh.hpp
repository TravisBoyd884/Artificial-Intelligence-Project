#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <string>
#include <vector>

#include "Rendering/shader.hpp"

// ---------------------------------------------------------------------------
// Data types
// ---------------------------------------------------------------------------

struct Vertex {
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec2 texCoords;
};

struct MeshTexture {
  unsigned int id;
  std::string  type; // "texture_diffuse" or "texture_specular"
  std::string  path; // used by Model to avoid loading the same file twice
};

// ---------------------------------------------------------------------------
// Mesh
//
// Owns its GPU buffers (VAO/VBO/EBO). Textures are owned by the Model that
// created the mesh — Mesh only stores the IDs for binding at draw time.
// ---------------------------------------------------------------------------

class Mesh {
public:
  Mesh(std::vector<Vertex>      vertices,
       std::vector<unsigned int> indices,
       std::vector<MeshTexture>  textures);

  ~Mesh();

  // Non-copyable (GPU resource ownership).
  Mesh(const Mesh &) = delete;
  Mesh &operator=(const Mesh &) = delete;

  // Movable so meshes can live inside std::vector<Mesh>.
  Mesh(Mesh &&other) noexcept;
  Mesh &operator=(Mesh &&other) noexcept;

  // Binds textures, sets sampler uniforms, draws the mesh.
  // Texture uniform names follow the convention: texture_diffuse0,
  // texture_diffuse1, texture_specular0, etc.
  void draw(const Shader &shader) const;

private:
  unsigned int m_VAO = 0;
  unsigned int m_VBO = 0;
  unsigned int m_EBO = 0;

  std::vector<Vertex>       m_vertices;
  std::vector<unsigned int> m_indices;
  std::vector<MeshTexture>  m_textures;

  void setupMesh();
};
