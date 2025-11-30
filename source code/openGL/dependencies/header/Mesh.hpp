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

#define MAX_TEXTURE_UNITS 4

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

    // Place this where your previous 'class object' was defined.
    class object {
    private:
        std::vector<vertex> vertices;
        std::vector<unsigned int> indices;
        std::vector<Light> lights;

        GLuint VAO = 0, VBO = 0, EBO = 0;

        enum class TextureType {
            BaseColor,
            Normal,
            MetallicRoughness,
            Occlusion,
            Emissive,
            ExtraDiffuse
        };

        struct TexEntry {
            std::string name;
            std::unique_ptr<gl::texture2D> text; // ownership
        };

        std::vector<TexEntry> textures;

        std::unique_ptr<Assimp::Importer> importer; // keep alive for embedded textures

    public:
        void uploadLights(GLuint shaderProgram) {
            glUseProgram(shaderProgram);

            GLint numLoc = glGetUniformLocation(shaderProgram, "numLights");
            glUniform1i(numLoc, static_cast<GLint>(lights.size()));

            for (size_t i = 0; i < lights.size(); ++i) {
                std::string posName = "lightPos[" + std::to_string(i) + "]";
                std::string colName = "lightColor[" + std::to_string(i) + "]";

                GLint posLoc = glGetUniformLocation(shaderProgram, posName.c_str());
                GLint colLoc = glGetUniformLocation(shaderProgram, colName.c_str());

                if (posLoc >= 0) glUniform3fv(posLoc, 1, glm::value_ptr(lights[i].position));
                if (colLoc >= 0) glUniform3fv(colLoc, 1, glm::value_ptr(lights[i].color));
            }
        }

        void addLight(const glm::vec3& pos, const glm::vec3& color) {
            lights.push_back({ pos, color });
        }

        object(const std::string& glbPath) {
            if (glbPath.size() < 4 || glbPath.substr(glbPath.size() - 4) != ".glb")
                throw std::runtime_error("Only .glb files are supported!");

            importer = std::make_unique<Assimp::Importer>();
            const aiScene* scene = importer->ReadFile(
                glbPath,
                aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace
            );

            if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode)
                throw std::runtime_error(std::string("ASSIMP ERROR: ") + importer->GetErrorString());

            processNode(scene->mRootNode, scene);
            setupMesh();
            loadTextures(scene, glbPath);
        }

        ~object() {
            // unique_ptr will clean up textures and importer automatically
            textures.clear();
            importer.reset();

            if (VAO) glDeleteVertexArrays(1, &VAO);
            if (VBO) glDeleteBuffers(1, &VBO);
            if (EBO) glDeleteBuffers(1, &EBO);
        }

        // Accept ownership transfer from a raw pointer
        void setTexture2D(gl::texture2D* tex, unsigned index) {
            if (index >= textures.size()) return;
            textures[index].text.reset(tex); // takes ownership
        }

        // Accept a unique_ptr directly
        void setTexture2D(std::unique_ptr<gl::texture2D> tex, unsigned index) {
            if (index >= textures.size()) return;
            textures[index].text = std::move(tex);
        }

        void draw(GLuint shaderProgram,
            const glm::vec3& pos,
            const glm::vec3& scale,
            const glm::vec3& rotation)
        {
            glUseProgram(shaderProgram);
            GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
            // keep your original compact matrix if you prefer; optionally build step-by-step
            glm::mat4 model = glm::translate(glm::mat4(1.0f), pos);
            model = glm::rotate(model, glm::radians(rotation.x), glm::vec3(1, 0, 0));
            model = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0, 1, 0));
            model = glm::rotate(model, glm::radians(rotation.z), glm::vec3(0, 0, 1));
            model = glm::scale(model, scale);
            if (modelLoc >= 0) glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

            // bind textures, upload lights, draw VAO
            for (int i = 0; i < MAX_TEXTURE_UNITS; ++i) {
                glActiveTexture(GL_TEXTURE0 + i);
                if (i < static_cast<int>(textures.size()) && textures[i].text) {
                    textures[i].text->bind(GL_TEXTURE_2D);
                    std::string uniformName = textures[i].name;
                    GLint loc = glGetUniformLocation(shaderProgram, uniformName.c_str());
                    if (loc >= 0) glUniform1i(loc, i);
                }
                else {
                    glBindTexture(GL_TEXTURE_2D, 0);
                }
            }

            uploadLights(shaderProgram);

            glBindVertexArray(VAO);
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);

            glActiveTexture(GL_TEXTURE0);
        }

        void draw(GLuint shaderProgram, const glm::mat4& model) {
            glUseProgram(shaderProgram);

            GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
            if (modelLoc >= 0) glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

            for (int i = 0; i < MAX_TEXTURE_UNITS; ++i) {
                glActiveTexture(GL_TEXTURE0 + i);
                if (i < static_cast<int>(textures.size()) && textures[i].text) {
                    textures[i].text->bind(GL_TEXTURE_2D);
                    std::string uniformName = textures[i].name;
                    GLint loc = glGetUniformLocation(shaderProgram, uniformName.c_str());
                    if (loc >= 0) glUniform1i(loc, i);
                }
                else {
                    glBindTexture(GL_TEXTURE_2D, 0);
                }
            }

            uploadLights(shaderProgram);

            glBindVertexArray(VAO);
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);

            glActiveTexture(GL_TEXTURE0);
        }

    private:
        inline std::filesystem::path getPath(const std::string& relativePath) {
            std::filesystem::path exePath = std::filesystem::current_path();
            return exePath / relativePath;
        }

        void loadModel(const std::string& path) {
            if (!importer) importer = std::make_unique<Assimp::Importer>();

            const aiScene* scene = importer->ReadFile(
                getPath(path).string(),
                aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace
            );

            if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
                throw std::runtime_error("ASSIMP ERROR: " + std::string(importer->GetErrorString()));

            processNode(scene->mRootNode, scene);
        }

        void processNode(aiNode* node, const aiScene* scene) {
            for (unsigned int i = 0; i < node->mNumMeshes; i++) {
                aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
                processMesh(mesh);
            }
            for (unsigned int i = 0; i < node->mNumChildren; i++) {
                processNode(node->mChildren[i], scene);
            }
        }

        void processMesh(aiMesh* mesh) {
            vertices.resize(mesh->mNumVertices);

            for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
                vertex& v = vertices[i];
                v.Position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);

                if (mesh->HasNormals())
                    v.Normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
                else
                    v.Normal = glm::vec3(0.0f, 0.0f, 1.0f);

                if (mesh->mTextureCoords[0])
                    v.TexCoords = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
                else
                    v.TexCoords = glm::vec2(0.0f);

                v.Tangent = glm::vec3(0.0f);
            }

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

                float denom = deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y;
                if (std::abs(denom) < 1e-8f) continue; // skip degenerate UV triangle
                float f = 1.0f / denom;

                glm::vec3 tangent;
                tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
                tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
                tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

                tangent = glm::normalize(tangent);

                v0.Tangent += tangent;
                v1.Tangent += tangent;
                v2.Tangent += tangent;
            }

            for (auto& v : vertices) {
                if (glm::length(v.Tangent) > 0.0f)
                    v.Tangent = glm::normalize(v.Tangent);
                else
                    v.Tangent = glm::vec3(1.0f, 0.0f, 0.0f);
            }

            for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
                aiFace face = mesh->mFaces[i];
                for (unsigned int j = 0; j < face.mNumIndices; j++)
                    indices.push_back(face.mIndices[j]);
            }
        }

        void loadTextures(const aiScene* scene, const std::string& modelPath) {
            for (unsigned m = 0; m < scene->mNumMaterials; ++m) {
                aiMaterial* mat = scene->mMaterials[m];

                auto process = [&](aiTextureType type, const char* logicalName) {
                    if (mat->GetTextureCount(type) == 0) return;
                    aiString str; mat->GetTexture(type, 0, &str);
                    if (str.C_Str()[0] == '*') {
                        int idx = atoi(str.C_Str() + 1);
                        aiTexture* tex = scene->mTextures[idx];
                        if (tex->mHeight == 0) {
                            int w, h, ch;
                            unsigned char* data = stbi_load_from_memory(
                                reinterpret_cast<unsigned char*>(tex->pcData),
                                tex->mWidth, &w, &h, &ch, 0
                            );
                            if (data) pushTextureFromMemory(logicalName, data, w, h, ch, true);
                        }
                        else {
                            int w = tex->mWidth, h = tex->mHeight, ch = 4;
                            unsigned char* raw = new unsigned char[w * h * 4];
                            memcpy(raw, tex->pcData, w * h * 4);
                            textures.push_back({ logicalName, std::make_unique<gl::texture2D>(raw, w, h, ch) });
                            delete[] raw;
                        }
                    }
                    else {
                        std::filesystem::path texPath = std::filesystem::path(modelPath).parent_path() / str.C_Str();
                        if (std::filesystem::exists(texPath)) {
                            textures.push_back({ logicalName, std::make_unique<gl::texture2D>(texPath.string()) });
                        }
                    }
                    };

                process(aiTextureType_DIFFUSE, "baseColor");
                process(aiTextureType_NORMALS, "normal");
                process(aiTextureType_METALNESS, "metallicRoughness");
                process(aiTextureType_AMBIENT_OCCLUSION, "occlusion");
                process(aiTextureType_EMISSIVE, "emissive");
            }
        }

        void pushTextureFromMemory(const std::string& logicalName, unsigned char* data, int w, int h, int channels, bool freeAfter = true) {
            if (!data) return;
            textures.push_back({ logicalName, std::make_unique<gl::texture2D>(data, w, h, channels) });
            if (freeAfter) stbi_image_free(data);
        }

        void setupMesh() {
            glGenVertexArrays(1, &VAO);
            glGenBuffers(1, &VBO);
            glGenBuffers(1, &EBO);

            glBindVertexArray(VAO);

            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertex), vertices.data(), GL_STATIC_DRAW);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

            // Position
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)0);

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
    }; // class object


}