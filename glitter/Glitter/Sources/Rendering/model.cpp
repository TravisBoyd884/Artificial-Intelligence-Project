// stb_image implementation must be compiled in exactly one translation unit.
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "Rendering/model.hpp"

#include <glad/glad.h>

#include <cstdio>
#include <iostream>

Model::Model(const std::string &path) {
  loadModel(path);
}

Model::~Model() {
  for (auto &t : m_loadedTextures)
    glDeleteTextures(1, &t.id);
}

void Model::draw(const Shader &shader) const {
  for (const auto &mesh : m_meshes)
    mesh.draw(shader);
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

void Model::loadModel(const std::string &path) {
  Assimp::Importer importer;

  // aiProcess_Triangulate  — convert quads/polygons to triangles
  // aiProcess_FlipUVs      — OpenGL expects UV origin at bottom-left
  // aiProcess_GenNormals   — create normals if the file doesn't have them
  // aiProcess_CalcTangentSpace — useful later for normal mapping
  const aiScene *scene = importer.ReadFile(
      path,
      aiProcess_Triangulate |
      aiProcess_FlipUVs     |
      aiProcess_GenNormals  |
      aiProcess_CalcTangentSpace);

  if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode) {
    std::cerr << "ERROR::ASSIMP: " << importer.GetErrorString() << '\n';
    return;
  }

  // Store the directory so relative texture paths can be resolved.
  m_directory = path.substr(0, path.find_last_of('/'));

  processNode(scene->mRootNode, scene);
}

void Model::processNode(aiNode *node, const aiScene *scene) {
  for (unsigned int i = 0; i < node->mNumMeshes; i++) {
    aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
    m_meshes.push_back(processMesh(mesh, scene));
  }
  for (unsigned int i = 0; i < node->mNumChildren; i++)
    processNode(node->mChildren[i], scene);
}

Mesh Model::processMesh(aiMesh *mesh, const aiScene *scene) {
  std::vector<Vertex>       vertices;
  std::vector<unsigned int> indices;
  std::vector<MeshTexture>  textures;

  // --- Vertices ---
  for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
    Vertex vertex;

    vertex.position = {
      mesh->mVertices[i].x,
      mesh->mVertices[i].y,
      mesh->mVertices[i].z
    };

    if (mesh->HasNormals()) {
      vertex.normal = {
        mesh->mNormals[i].x,
        mesh->mNormals[i].y,
        mesh->mNormals[i].z
      };
    }

    // Assimp supports up to 8 UV channels; we only need the first.
    if (mesh->mTextureCoords[0]) {
      vertex.texCoords = {
        mesh->mTextureCoords[0][i].x,
        mesh->mTextureCoords[0][i].y
      };
    } else {
      vertex.texCoords = {0.0f, 0.0f};
    }

    vertices.push_back(vertex);
  }

  // --- Indices ---
  for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
    const aiFace &face = mesh->mFaces[i];
    for (unsigned int j = 0; j < face.mNumIndices; j++)
      indices.push_back(face.mIndices[j]);
  }

  // --- Textures ---
  if (mesh->mMaterialIndex < scene->mNumMaterials) {
    aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];

    auto diffuse  = loadMaterialTextures(material, aiTextureType_DIFFUSE,  "texture_diffuse");
    auto specular = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");

    textures.insert(textures.end(), diffuse.begin(),  diffuse.end());
    textures.insert(textures.end(), specular.begin(), specular.end());
  }

  return Mesh(std::move(vertices), std::move(indices), std::move(textures));
}

std::vector<MeshTexture> Model::loadMaterialTextures(aiMaterial   *material,
                                                     aiTextureType type,
                                                     const std::string &typeName) {
  std::vector<MeshTexture> textures;

  for (unsigned int i = 0; i < material->GetTextureCount(type); i++) {
    aiString str;
    material->GetTexture(type, i, &str);
    std::string texPath = m_directory + '/' + str.C_Str();

    // Check the cache first.
    bool alreadyLoaded = false;
    for (const auto &loaded : m_loadedTextures) {
      if (loaded.path == texPath) {
        textures.push_back(loaded);
        alreadyLoaded = true;
        break;
      }
    }

    if (!alreadyLoaded) {
      MeshTexture tex;
      tex.id   = textureFromFile(texPath);
      tex.type = typeName;
      tex.path = texPath;
      textures.push_back(tex);
      m_loadedTextures.push_back(tex); // add to cache
    }
  }

  return textures;
}

unsigned int Model::textureFromFile(const std::string &filename) {
  unsigned int textureID;
  glGenTextures(1, &textureID);

  int width, height, channels;
  unsigned char *data = stbi_load(filename.c_str(), &width, &height, &channels, 0);

  if (data) {
    GLenum format = GL_RGB;
    if      (channels == 1) format = GL_RED;
    else if (channels == 3) format = GL_RGB;
    else if (channels == 4) format = GL_RGBA;

    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(format),
                 width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
  } else {
    std::cerr << "ERROR::TEXTURE: Failed to load " << filename << '\n';
    stbi_image_free(data);
  }

  return textureID;
}
