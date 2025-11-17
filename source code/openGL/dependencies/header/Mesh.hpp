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

#define MAX_TEXTURE_UNITS 32

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
        std::vector<vertex> vertices;
        std::vector<unsigned int> indices;
        std::vector<Light> lights;

        GLuint VAO, VBO, EBO;

        enum class TextureType {
            BaseColor,
            Normal,
            MetallicRoughness,
            Occlusion,
            Emissive,
            ExtraDiffuse
        };

        struct TexEntry {
            std::string name;          // logical name (e.g. "baseColor", "normal")
            gl::texture2D* text;    // pointer to texture
        };

        std::vector<TexEntry> textures;

        Assimp::Importer* importer{ nullptr }; // keep alive for embedded textures

    public:
        void uploadLights(GLuint shaderProgram) {
            glUseProgram(shaderProgram);

            GLint numLoc = glGetUniformLocation(shaderProgram, "numLights");
            glUniform1i(numLoc, (GLint)lights.size());

            for (size_t i = 0; i < lights.size(); ++i) {
                std::string posName = "lightPos[" + std::to_string(i) + "]";
                std::string colName = "lightColor[" + std::to_string(i) + "]";

                GLint posLoc = glGetUniformLocation(shaderProgram, posName.c_str());
                GLint colLoc = glGetUniformLocation(shaderProgram, colName.c_str());

                glUniform3fv(posLoc, 1, glm::value_ptr(lights[i].position));
                glUniform3fv(colLoc, 1, glm::value_ptr(lights[i].color));
            }
        }

        void addLight(const glm::vec3& pos, const glm::vec3& color) {
            lights.push_back({ pos, color });
        }

        object(const std::string& glbPath) {
            // Only allow .glb
            if (glbPath.size() < 4 || glbPath.substr(glbPath.size() - 4) != ".glb")
                throw std::runtime_error("Only .glb files are supported!");

            importer = new Assimp::Importer();
            const aiScene* scene = importer->ReadFile(
                glbPath,
                aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace
            );

            if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
                throw std::runtime_error("ASSIMP ERROR: " + std::string(importer->GetErrorString()));

            // Process all nodes & meshes
            processNode(scene->mRootNode, scene);
            setupMesh();

            // Load all PBR textures (embedded or external)
            for (unsigned m = 0; m < scene->mNumMaterials; ++m) {
                aiMaterial* mat = scene->mMaterials[m];

                auto loadTexture = [&](aiTextureType type, const std::string& logicalName) {
                    if (mat->GetTextureCount(type) == 0) return;

                    aiString str;
                    mat->GetTexture(type, 0, &str);

                    if (str.C_Str()[0] == '*') {
                        // Embedded texture
                        int texIndex = atoi(str.C_Str() + 1);
                        aiTexture* tex = scene->mTextures[texIndex];

                        int width = 0, height = 0, channels = 0;
                        unsigned char* data = nullptr;

                        if (tex->mHeight == 0) {
                            // Compressed (PNG/JPG) in memory
                            data = stbi_load_from_memory(
                                reinterpret_cast<unsigned char*>(tex->pcData),
                                tex->mWidth,
                                &width, &height, &channels, 0
                            );
                            if (!data) {
                                std::cerr << "Failed to decode embedded texture: " << logicalName << "\n";
                                return;
                            }
                            textures.push_back({ logicalName, new gl::texture2D(data, width, height, channels) });
                            stbi_image_free(data);
                        }
                        else {
                            // Raw RGBA
                            width = tex->mWidth;
                            height = tex->mHeight;
                            channels = 4;
                            unsigned char* raw = new unsigned char[width * height * 4];
                            memcpy(raw, tex->pcData, width * height * 4);
                            textures.push_back({ logicalName, new gl::texture2D(raw, width, height, channels) });
                            delete[] raw;
                        }

                    }
                    else {
                        // External file fallback
                        std::filesystem::path texPath = std::filesystem::path(glbPath).parent_path() / str.C_Str();
                        if (std::filesystem::exists(texPath))
                            textures.push_back({ logicalName, new gl::texture2D(texPath.string()) });
                        else
                            std::cerr << "Texture not found: " << texPath << "\n";
                    }
                    };

                // PBR bindings
                loadTexture(aiTextureType_DIFFUSE, "baseColor");
                loadTexture(aiTextureType_NORMALS, "normal");
                loadTexture(aiTextureType_METALNESS, "metallicRoughness");
                loadTexture(aiTextureType_AMBIENT_OCCLUSION, "occlusion");
                loadTexture(aiTextureType_EMISSIVE, "emissive");
            }
        }

        ~object() {
            for (auto& t : textures) {
                delete t.text;
            }
            textures.clear();
            if (importer) delete importer;
            if (VAO) glDeleteVertexArrays(1, &VAO);
            if (VBO) glDeleteBuffers(1, &VBO);
            if (EBO) glDeleteBuffers(1, &EBO);
        }

        void setTexture2D(gl::texture2D* tex, unsigned index) {
            textures[index].text = tex;
        }

        // In gl::object
        void draw(GLuint shaderProgram,
            const glm::vec3& pos,
            const glm::vec3& scale,
            const glm::vec3& rotation) // new optional param
        {
            glUseProgram(shaderProgram);
            GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(glm::scale(glm::rotate(glm::rotate(glm::rotate(glm::translate(glm::mat4(1.0f), pos), glm::radians(rotation.x), glm::vec3(1, 0, 0)), glm::radians(rotation.y), glm::vec3(0, 1, 0)), glm::radians(rotation.z), glm::vec3(0, 0, 1)), scale)));

            // --- bind textures, upload lights, draw VAO ---
            for (int i = 0; i < MAX_TEXTURE_UNITS; ++i) {
                glActiveTexture(GL_TEXTURE0 + i);
                if (i < textures.size() && textures[i].text) {
                    textures[i].text->bind(GL_TEXTURE_2D);
                    std::string uniformName = textures[i].name;
                    GLint loc = glGetUniformLocation(shaderProgram, uniformName.c_str());
                    if (loc >= 0) glUniform1i(loc, i);
                }
                else glBindTexture(GL_TEXTURE_2D, 0);
            }

            uploadLights(shaderProgram);

            glBindVertexArray(VAO);
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);

            glActiveTexture(GL_TEXTURE0);
        }


        void draw(GLuint shaderProgram, const glm::mat4& model) {
            glUseProgram(shaderProgram);

            // Upload model matrix
            GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

            // Bind textures sequentially
            for (int i = 0; i < MAX_TEXTURE_UNITS; ++i) {
                glActiveTexture(GL_TEXTURE0 + i);
                if (i < textures.size() && textures[i].text) {
                    textures[i].text->bind(GL_TEXTURE_2D);
                    std::string uniformName = textures[i].name;
                    GLint loc = glGetUniformLocation(shaderProgram, uniformName.c_str());
                    if (loc >= 0) glUniform1i(loc, i);
                }
                else glBindTexture(GL_TEXTURE_2D, 0);
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
            if (!importer) importer = new Assimp::Importer();

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

            // Step 1: copy positions, normals, UVs
            for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
                vertex& v = vertices[i];
                v.Position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);

                // Normals
                if (mesh->HasNormals())
                    v.Normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
                else
                    v.Normal = glm::vec3(0.0f, 0.0f, 1.0f);

                // UVs (flip V)
                if (mesh->mTextureCoords[0])
                    v.TexCoords = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
                else
                    v.TexCoords = glm::vec2(0.0f);

                // Initialize tangent
                v.Tangent = glm::vec3(0.0f);
            }

            // Step 2: compute tangents per triangle
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

                glm::vec3 tangent;
                tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
                tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
                tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

                tangent = glm::normalize(tangent);

                v0.Tangent += tangent;
                v1.Tangent += tangent;
                v2.Tangent += tangent;
            }

            // Step 3: normalize tangents
            for (auto& v : vertices) {
                v.Tangent = glm::normalize(v.Tangent);
            }

            // Step 4: store indices
            for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
                aiFace face = mesh->mFaces[i];
                for (unsigned int j = 0; j < face.mNumIndices; j++)
                    indices.push_back(face.mIndices[j]);
            }
        }

        void loadTextures(const aiScene* scene, const std::string& modelPath) {
            for (unsigned m = 0; m < scene->mNumMaterials; ++m) {
                aiMaterial* mat = scene->mMaterials[m];

                auto loadTex = [&](aiTextureType type, const std::string& name) {
                    if (mat->GetTextureCount(type) == 0) return;

                    aiString str;
                    mat->GetTexture(type, 0, &str);

                    // Embedded texture
                    if (str.C_Str()[0] == '*') {
                        int texIndex = atoi(str.C_Str() + 1);
                        aiTexture* tex = scene->mTextures[texIndex];
                        int width, height, channels;
                        unsigned char* data = nullptr;

                        if (tex->mHeight == 0) {
                            data = stbi_load_from_memory(
                                reinterpret_cast<unsigned char*>(tex->pcData),
                                tex->mWidth, &width, &height, &channels, 0
                            );
                            if (!data) return;
                            textures.push_back({ name, new gl::texture2D(data, width, height, channels) });
                            stbi_image_free(data);
                        }
                        else {
                            width = tex->mWidth;
                            height = tex->mHeight;
                            channels = 4;
                            unsigned char* raw = new unsigned char[width * height * 4];
                            memcpy(raw, tex->pcData, width * height * 4);
                            textures.push_back({ name, new gl::texture2D(raw, width, height, channels) });
                            delete[] raw;
                        }
                    }
                    // External texture
                    else {
                        std::filesystem::path texPath = std::filesystem::path(modelPath).parent_path() / str.C_Str();
                        if (std::filesystem::exists(texPath))
                            textures.push_back({ name, new gl::texture2D(texPath.string()) });
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
    };

}