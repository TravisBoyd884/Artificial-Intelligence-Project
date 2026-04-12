#pragma once

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <string>
#include <vector>

#include "Rendering/mesh.hpp"
#include "Rendering/shader.hpp"

// ---------------------------------------------------------------------------
// Model
//
// Loads a 3D model from disk using Assimp and stores it as a flat list of
// Mesh objects.  Textures are cached so the same image is never loaded twice.
//
// Usage:
//   Model myModel("path/to/model.glb");   // or .obj, .fbx, etc.
//   myModel.draw(shader);
//
// The calling code is responsible for setting the MVP uniforms on the shader
// before calling draw().
// ---------------------------------------------------------------------------

class Model {
public:
  explicit Model(const std::string &path);
  ~Model();

  // Non-copyable.
  Model(const Model &) = delete;
  Model &operator=(const Model &) = delete;

  // Draws all meshes using the given shader.
  void draw(const Shader &shader) const;

private:
  std::vector<Mesh>        m_meshes;
  std::string              m_directory;  // directory of the model file
  std::vector<MeshTexture> m_loadedTextures; // cache — avoids duplicate loads

  void loadModel(const std::string &path);
  void processNode(aiNode *node, const aiScene *scene);
  Mesh processMesh(aiMesh *mesh, const aiScene *scene);

  // Loads all textures of a given type from a material.
  // Returns only the textures that were not already in the cache.
  std::vector<MeshTexture> loadMaterialTextures(aiMaterial   *material,
                                                aiTextureType type,
                                                const std::string &typeName);

  // Uploads one image to the GPU and returns its OpenGL texture ID.
  unsigned int textureFromFile(const std::string &filename);
};
