#pragma once
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <GLFW/glfw3.h>
#include <GL/glew.h>

#include <glm.hpp>            
#include <gtc/matrix_transform.hpp> 
#include <gtc/type_ptr.hpp> 

#include <stb_image.h>

#include <Window.hpp>
#include <Utils.hpp>

#include <fstream>
#include <filesystem>
#include <string>
#include <iostream>
#include <vector>
#include <stdexcept>
#include <unordered_map>

#ifndef MODEL_PATH

#define MODEL_PATH "resource/model"

#endif // MODEL_PATH


namespace gl {

    static inline unsigned textureIndex = 0;

    struct vertex {
        glm::vec3 Position;
        glm::vec3 Normal;
        glm::vec2 TexCoords;
        glm::vec3 Tangent;
    };

#define VERT_SIZE sizeof(vertex)

    struct Light {
        glm::vec3 position;
        glm::vec3 color;
    };

    class object {
    private:
        struct TexEntry {
            std::string name;          // logical name (e.g., "baseColor")
            gl::texture2D* text;       // raw pointer (ownership handled elsewhere)
        };

        std::vector<vertex> vertices;
        std::vector<unsigned int> indices;
        std::vector<Light> lights;
        GLuint VAO = 0, VBO = 0, EBO = 0;
        std::vector<TexEntry> textures;
        std::shared_ptr<gl::shader> m_Shader;
        Assimp::Importer* importer{ nullptr }; // keep alive for embedded textures

    public:
        object(const std::string& glbPath, const std::shared_ptr<gl::shader>& shader)
            : m_Shader(shader)
        {
            importer = new Assimp::Importer();
            const aiScene* scene = importer->ReadFile(
                glbPath,
                aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace
            );

            if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
                throw std::runtime_error("ASSIMP ERROR: " + std::string(importer->GetErrorString()));

            processNode(scene->mRootNode, scene);
            setupMesh();
            loadTextures(scene, glbPath);
        }

        ~object() {
            for (auto& t : textures) delete t.text;
            if (importer) delete importer;

            if (VAO) glDeleteVertexArrays(1, &VAO);
            if (VBO) glDeleteBuffers(1, &VBO);
            if (EBO) glDeleteBuffers(1, &EBO);
        }

        void addLight(const glm::vec3& pos, const glm::vec3& color) {
            lights.push_back({ pos, color });
        }

        void uploadLights() {
            m_Shader->useProgram();
            glUniform1i(m_Shader->getUniformLoc("numLights"), (GLint)lights.size());

            for (size_t i = 0; i < lights.size(); ++i) {                   
                glUniform3fv(m_Shader->getUniformLoc("lightPos", i), 1, glm::value_ptr(lights[i].position));
                glUniform3fv(m_Shader->getUniformLoc("lightColor", i), 1, glm::value_ptr(lights[i].color));
            }
        }

        void setTexture2D(gl::texture2D* tex, unsigned index) {
            if (index < textures.size())
                textures[index].text = tex;
        }

        void draw(const glm::mat4& model) {
            m_Shader->useProgram();
            m_Shader->setUniformMat4fv("model", model);

            for (int i = 0; i < textures.size(); ++i) {
                glActiveTexture(GL_TEXTURE0 + i);
                textures[i].text->bind(GL_TEXTURE_2D);
                glUniform1i(m_Shader->getUniformLoc(textures[i].name), i);
            }

            uploadLights();

            glBindVertexArray(VAO);
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);

            glActiveTexture(GL_TEXTURE0);
        }

    private:
        void processNode(aiNode* node, const aiScene* scene) {
            for (unsigned int i = 0; i < node->mNumMeshes; i++)
                processMesh(scene->mMeshes[node->mMeshes[i]]);
            for (unsigned int i = 0; i < node->mNumChildren; i++)
                processNode(node->mChildren[i], scene);
        }

        void processMesh(aiMesh* mesh) {
            vertices.resize(mesh->mNumVertices);

            // copy positions, normals, UVs
            for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
                vertex& v = vertices[i];
                v.Position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
                v.Normal = mesh->HasNormals() ? glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z) : glm::vec3(0.0f, 0.0f, 1.0f);
                v.TexCoords = mesh->mTextureCoords[0] ? glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y) : glm::vec2(0.0f);
                v.Tangent = glm::vec3(0.0f);
            }

            // compute tangents per triangle
            for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
                aiFace face = mesh->mFaces[i];
                if (face.mNumIndices != 3) continue;

                vertex& v0 = vertices[face.mIndices[0]];
                vertex& v1 = vertices[face.mIndices[1]];
                vertex& v2 = vertices[face.mIndices[2]];

                glm::vec3 edge1 = v1.Position - v0.Position;
                glm::vec3 edge2 = v2.Position - v0.Position;
                glm::vec2 deltaUV1 = v1.TexCoords - v0.TexCoords;
                glm::vec2 deltaUV2 = v2.TexCoords - v0.TexCoords;

                float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y + 1e-8f);
                glm::vec3 tangent(f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x),
                    f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y),
                    f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z));

                v0.Tangent += tangent;
                v1.Tangent += tangent;
                v2.Tangent += tangent;
            }

            // normalize tangents
            for (auto& v : vertices) v.Tangent = glm::normalize(v.Tangent);

            // store indices
            for (unsigned int i = 0; i < mesh->mNumFaces; i++)
                for (unsigned int j = 0; j < mesh->mFaces[i].mNumIndices; j++)
                    indices.emplace_back(mesh->mFaces[i].mIndices[j]);
        }

        void loadTextures(const aiScene* scene, const std::string& modelPath) {
            for (unsigned m = 0; m < scene->mNumMaterials; ++m) {
                aiMaterial* mat = scene->mMaterials[m];

                auto loadTex = [&](aiTextureType type, const std::string& name) {
                    if (mat->GetTextureCount(type) == 0) return;

                    aiString str; mat->GetTexture(type, 0, &str);
                    if (str.C_Str()[0] == '*') {
                        int texIndex = atoi(str.C_Str() + 1);
                        aiTexture* tex = scene->mTextures[texIndex];
                        int w, h, ch;
                        unsigned char* data = nullptr;

                        if (tex->mHeight == 0) {
                            data = stbi_load_from_memory(reinterpret_cast<unsigned char*>(tex->pcData), tex->mWidth, &w, &h, &ch, 0);
                            if (!data) return;
                            textures.push_back({ name, new gl::texture2D(data, w, h, ch) });
                            stbi_image_free(data);
                        }
                        else {
                            w = tex->mWidth; h = tex->mHeight; ch = 4;
                            unsigned char* raw = new unsigned char[w * h * 4];
                            memcpy(raw, tex->pcData, w * h * 4);
                            textures.push_back({ name, new gl::texture2D(raw, w, h, ch) });
                            delete[] raw;
                        }
                    }
                    else {
                        std::filesystem::path texPath = std::filesystem::path(modelPath).parent_path() / str.C_Str();
                        if (std::filesystem::exists(texPath)) textures.push_back({ name, new gl::texture2D(texPath.string()) });
                    }
                    };

                loadTex(aiTextureType_DIFFUSE, "baseColor");
                loadTex(aiTextureType_NORMALS, "normal");
                loadTex(aiTextureType_METALNESS, "metallicRoughness");
                loadTex(aiTextureType_AMBIENT_OCCLUSION, "occlusion");
                loadTex(aiTextureType_EMISSIVE, "emissive");
            }
        }

        void setupMesh() {
            glGenVertexArrays(1, &VAO);
            glGenBuffers(1, &VBO);
            glGenBuffers(1, &EBO);

            glBindVertexArray(VAO);

            // Use STATIC_DRAW since vertices and indices are usually not updated per frame
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertex), vertices.data(), GL_STATIC_DRAW);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

            // Position
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, Position));

            // Normal
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, Normal));

            // TexCoords
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, TexCoords));

            // Tangent
            glEnableVertexAttribArray(3);
            glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, Tangent));

            glBindVertexArray(0);
        }

    };

}