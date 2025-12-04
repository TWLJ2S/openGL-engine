//#pragma once
//#include <assimp/Importer.hpp>
//#include <assimp/scene.h>
//#include <assimp/postprocess.h>
//
//#include <GLFW/glfw3.h>
//#include <GL/glew.h>
//
//#include <glm.hpp>            
//#include <gtc/matrix_transform.hpp> 
//#include <gtc/type_ptr.hpp> 
//
//#include <stb_image.h>
//
//#include <Window.hpp>
//#include <Utils.hpp>
//
//#include <fstream>
//#include <filesystem>
//#include <string>
//#include <iostream>
//#include <vector>
//#include <stdexcept>
//#include <unordered_map>
//
//#define MAX_TEXTURE_UNITS 5
//
//#ifndef MODEL_PATH
//
//    #define MODEL_PATH "resource/model"
//
//#endif // MODEL_PATH
//
//
//namespace gl {
//
//    static inline unsigned textureIndex = 0;
//
//    struct vertex {
//        glm::vec3 Position;
//        glm::vec3 Normal;
//        glm::vec2 TexCoords;
//        glm::vec3 Tangent;
//        glm::vec3 BiTangent;
//    };
//
//#define VERT_SIZE sizeof(vertex)
//
//    struct Light {
//        glm::vec3 position;
//        glm::vec3 color;
//    };
//
//    // Place this where your previous 'class object' was defined.
//    class object {
//    private:
//        std::vector<vertex> vertices;
//        std::vector<unsigned int> indices;
//        std::vector<Light> lights;
//        gl::shader m_Shader;
//
//
//        GLuint VAO = 0, VBO = 0, EBO = 0;
//
//        enum class TextureType {
//            BaseColor,
//            Normal,
//            MetallicRoughness,
//            Occlusion,
//            Emissive,
//            ExtraDiffuse
//        };
//
//        struct TexEntry {
//            std::string name;
//            std::unique_ptr<gl::texture2D> text; // ownership
//        };
//
//        std::vector<TexEntry> textures;
//
//        std::unique_ptr<Assimp::Importer> m_Importer; // keep alive for embedded textures
//
//    public:
//
//        /*object(gl::shader& shader, const std::string& glbPath)
//            : m_Shader(shader)
//        {
//            if (glbPath.size() < 4 || glbPath.substr(glbPath.size() - 4) != ".glb")
//                throw std::runtime_error("Only .glb files are supported!");
//
//            importer = std::make_unique<Assimp::Importer>();
//            const aiScene* scene = importer->ReadFile(
//                glbPath,
//                aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace
//            );
//
//            if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode)
//                throw std::runtime_error(std::string("ASSIMP ERROR: ") + importer->GetErrorString());
//
//            processNode(scene->mRootNode, scene);
//            setupMesh();
//            loadTextures(scene, glbPath);
//        }*/
//
//        object(const gl::shader& shader, const std::string& glbPath) 
//            : m_Shader(shader), m_Importer(std::make_unique<Assimp::Importer>())
//        {
//            const aiScene* scene = m_Importer->ReadFile(glbPath,
//                aiProcess_Triangulate |
//                aiProcess_CalcTangentSpace |
//                aiProcess_GenSmoothNormals |
//                aiProcess_JoinIdenticalVertices |
//                aiProcess_ImproveCacheLocality);
//
//            if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
//                throw std::runtime_error("ASSIMP ERROR: " + std::string(m_Importer->GetErrorString()));
//            }
//
//            // Process all nodes & meshes
//            processNode(scene->mRootNode, scene);
//            setupMesh();
//
//            // Load all PBR textures (embedded or external)
//            for (unsigned m = 0; m < scene->mNumMaterials; ++m) {
//                aiMaterial* mat = scene->mMaterials[m];
//
//                auto loadTexture = [&](aiTextureType type, const std::string& logicalName) {
//                    if (mat->GetTextureCount(type) == 0) return;
//
//                    aiString str;
//                    mat->GetTexture(type, 0, &str);
//
//                    if (str.C_Str()[0] == '*') {
//                        // Embedded texture
//                        int texIndex = atoi(str.C_Str() + 1);
//                        aiTexture* tex = scene->mTextures[texIndex];
//
//                        int width = 0, height = 0, channels = 0;
//                        unsigned char* data = nullptr;
//
//                        if (tex->mHeight == 0) {
//                            // Compressed (PNG/JPG) in memory
//                            data = stbi_load_from_memory(
//                                reinterpret_cast<unsigned char*>(tex->pcData),
//                                tex->mWidth,
//                                &width, &height, &channels, 0
//                            );
//                            if (!data) {
//                                std::cerr << "Failed to decode embedded texture: " << logicalName << "\n";
//                                return;
//                            }
//                            textures.push_back({ logicalName, std::make_unique<gl::texture2D>(data, width, height, channels) });
//                            stbi_image_free(data);
//                        }
//                        else {
//                            // Raw RGBA
//                            width = tex->mWidth;
//                            height = tex->mHeight;
//                            channels = 4;
//                            unsigned char* raw = new unsigned char[width * height * 4];
//                            memcpy(raw, tex->pcData, width * height * 4);
//                            textures.push_back({ logicalName, std::make_unique<gl::texture2D>(raw, width, height, channels) });
//                            delete[] raw;
//                        }
//
//                    }
//                    else {
//                        // External file fallback
//                        std::filesystem::path texPath = std::filesystem::path(glbPath).parent_path() / str.C_Str();
//                        if (std::filesystem::exists(texPath))
//                            textures.push_back({ logicalName, std::make_unique<gl::texture2D>(texPath.string()) });
//                        else
//                            std::cerr << "Texture not found: " << texPath << "\n";
//                    }
//                };
//
//                // PBR bindings
//                loadTexture(aiTextureType_DIFFUSE, "baseColor");
//                loadTexture(aiTextureType_NORMALS, "normal");
//                loadTexture(aiTextureType_METALNESS, "metallicRoughness");
//                loadTexture(aiTextureType_AMBIENT_OCCLUSION, "occlusion");
//                loadTexture(aiTextureType_EMISSIVE, "emissive");
//            }
//        }
//
//        ~object() {
//            // unique_ptr will clean up textures and importer automatically
//            textures.clear();
//            m_Importer.reset();
//
//            if (VAO) glDeleteVertexArrays(1, &VAO);
//            if (VBO) glDeleteBuffers(1, &VBO);
//            if (EBO) glDeleteBuffers(1, &EBO);
//        }
//
//        void uploadLights() {
//            m_Shader.useProgram();
//            GLint numLoc = m_Shader.getUniformLoc("numLights");
//            glUniform1i(numLoc, (GLint)lights.size());
//
//            for (size_t i = 0; i < lights.size(); ++i) {
//                GLint posLoc = m_Shader.getUniformLoc("lightPos[" + std::to_string(i) + "]");
//                GLint colLoc = m_Shader.getUniformLoc("lightColor[" + std::to_string(i) + "]");
//
//                if (posLoc >= 0) glUniform3fv(posLoc, 1, glm::value_ptr(lights[i].position));
//                if (colLoc >= 0) glUniform3fv(colLoc, 1, glm::value_ptr(lights[i].color));
//            }
//        }
//
//        void addLight(const glm::vec3& pos, const glm::vec3& color) {
//            lights.push_back({ pos, color });
//        }
//
//        // Accept ownership transfer from a raw pointer
//        void setTexture2D(gl::texture2D* tex, unsigned index) {
//            if (index >= textures.size()) return;
//            textures[index].text.reset(tex); // takes ownership
//        }
//
//        // Accept a unique_ptr directly
//        void setTexture2D(std::unique_ptr<gl::texture2D> tex, unsigned index) {
//            if (index >= textures.size()) return;
//            textures[index].text = std::move(tex);
//        }
//
//        void draw(const glm::vec3& pos,
//            const glm::vec3& scale,
//            const glm::vec3& rotation)
//        {
//            m_Shader.useProgram();
//
//            // Build model matrix: scale -> rotate -> translate
//            glm::mat4 model = glm::translate(glm::mat4(1.0f), pos) *
//                glm::rotate(glm::mat4(1.0f), glm::radians(rotation.x), glm::vec3(1, 0, 0)) *
//                glm::rotate(glm::mat4(1.0f), glm::radians(rotation.y), glm::vec3(0, 1, 0)) *
//                glm::rotate(glm::mat4(1.0f), glm::radians(rotation.z), glm::vec3(0, 0, 1)) *
//                glm::scale(glm::mat4(1.0f), scale);
//
//            GLint modelLoc = m_Shader.getUniformLoc("model");
//            if (modelLoc >= 0)
//                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
//
//            // Bind textures
//            for (int i = 0; i < MAX_TEXTURE_UNITS; ++i) {
//                glActiveTexture(GL_TEXTURE0 + i);
//                if (i < static_cast<int>(textures.size()) && textures[i].text) {
//                    textures[i].text->bind(GL_TEXTURE_2D);
//
//                    GLint loc = m_Shader.getUniformLoc(textures[i].name);
//                    if (loc >= 0)
//                        glUniform1i(loc, i);
//                    else
//                        std::cerr << "Warning: shader uniform '" << textures[i].name << "' not found\n";
//                }
//                else {
//                    glBindTexture(GL_TEXTURE_2D, 0);
//                }
//            }
//
//            uploadLights();
//
//            glBindVertexArray(VAO);
//            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
//            glBindVertexArray(0);
//
//            glActiveTexture(GL_TEXTURE0);
//        }
//
//        void draw(const glm::mat4& model)
//        {
//            m_Shader.useProgram();
//
//            GLint modelLoc = m_Shader.getUniformLoc("model");
//            if (modelLoc >= 0)
//                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
//            else
//                std::cerr << "Warning: shader uniform 'model' not found\n";
//
//            // Bind textures
//            for (int i = 0; i < MAX_TEXTURE_UNITS; ++i) {
//                glActiveTexture(GL_TEXTURE0 + i);
//                if (i < static_cast<int>(textures.size()) && textures[i].text) {
//                    textures[i].text->bind(GL_TEXTURE_2D);
//
//                    GLint loc = m_Shader.getUniformLoc(textures[i].name.c_str());
//                    if (loc >= 0)
//                        glUniform1i(loc, i);
//                    else
//                        std::cerr << "Warning: shader uniform '" << textures[i].name << "' not found\n";
//                }
//                else {
//                    glBindTexture(GL_TEXTURE_2D, 0);
//                }
//            }
//
//            // Upload lights (only if uniforms exist)
//            GLint numLightsLoc = m_Shader.getUniformLoc("numLights");
//            if (numLightsLoc >= 0)
//                glUniform1i(numLightsLoc, (GLint)lights.size());
//
//            for (size_t i = 0; i < lights.size(); ++i) {
//                GLint posLoc = m_Shader.getUniformLoc("lightPos[" + std::to_string(i) + "]");
//                GLint colLoc = m_Shader.getUniformLoc("lightColor[" + std::to_string(i) + "]");
//
//                if (posLoc >= 0) glUniform3fv(posLoc, 1, glm::value_ptr(lights[i].position));
//                if (colLoc >= 0) glUniform3fv(colLoc, 1, glm::value_ptr(lights[i].color));
//            }
//
//            glBindVertexArray(VAO);
//            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
//            glBindVertexArray(0);
//
//            glActiveTexture(GL_TEXTURE0);
//        }
//
//    private:
//        inline std::filesystem::path getPath(const std::string& relativePath) {
//            std::filesystem::path exePath = std::filesystem::current_path();
//            return exePath / relativePath;
//        }
//
//        void loadModel(const std::string& path) {
//            if (!m_Importer) m_Importer = std::make_unique<Assimp::Importer>();
//
//            const aiScene* scene = m_Importer->ReadFile(
//                getPath(path).string(),
//                aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace
//            );
//
//            if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
//                throw std::runtime_error("ASSIMP ERROR: " + std::string(m_Importer->GetErrorString()));
//
//            processNode(scene->mRootNode, scene);
//        }
//
//        void processNode(aiNode* node, const aiScene* scene) {
//            for (unsigned int i = 0; i < node->mNumMeshes; i++) {
//                aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
//                processMesh(mesh);
//            }
//            for (unsigned int i = 0; i < node->mNumChildren; i++) {
//                processNode(node->mChildren[i], scene);
//            }
//        }
//
//        void processMesh(aiMesh* mesh) {
//            vertices.resize(mesh->mNumVertices);
//
//            // Initialize vertices
//            for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
//                vertex& v = vertices[i];
//                v.Position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
//                v.Normal = mesh->HasNormals() ? glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z)
//                    : glm::vec3(0.0f, 0.0f, 1.0f);
//                v.TexCoords = mesh->mTextureCoords[0] ? glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y)
//                    : glm::vec2(0.0f);
//                v.Tangent = glm::vec3(0.0f);
//                v.BiTangent = glm::vec3(0.0f);
//            }
//
//            // Compute tangents & bitangents per triangle
//            for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
//                aiFace face = mesh->mFaces[i];
//                if (face.mNumIndices != 3) continue;
//
//                vertex& v0 = vertices[face.mIndices[0]];
//                vertex& v1 = vertices[face.mIndices[1]];
//                vertex& v2 = vertices[face.mIndices[2]];
//
//                glm::vec3 edge1 = v1.Position - v0.Position;
//                glm::vec3 edge2 = v2.Position - v0.Position;
//                glm::vec2 deltaUV1 = v1.TexCoords - v0.TexCoords;
//                glm::vec2 deltaUV2 = v2.TexCoords - v0.TexCoords;
//
//                float f = deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y;
//                if (std::abs(f) < 1e-8f) continue; // degenerate UV triangle
//                f = 1.0f / f;
//
//                glm::vec3 tangent, bitangent;
//                tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
//                tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
//                tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
//
//                bitangent.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
//                bitangent.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
//                bitangent.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
//
//                // Accumulate per vertex
//                v0.Tangent += tangent; v1.Tangent += tangent; v2.Tangent += tangent;
//                v0.BiTangent += bitangent; v1.BiTangent += bitangent; v2.BiTangent += bitangent;
//            }
//
//            // Normalize tangents & bitangents
//            for (auto& v : vertices) {
//                if (glm::length(v.Tangent) > 0.0f) v.Tangent = glm::normalize(v.Tangent);
//                if (glm::length(v.BiTangent) > 0.0f) v.BiTangent = glm::normalize(v.BiTangent);
//
//                // Orthogonalize tangent with normal
//                v.Tangent = glm::normalize(v.Tangent - v.Normal * glm::dot(v.Normal, v.Tangent));
//                v.BiTangent = glm::normalize(glm::cross(v.Normal, v.Tangent));
//            }
//
//            // Fill index buffer
//            for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
//                aiFace face = mesh->mFaces[i];
//                for (unsigned int j = 0; j < face.mNumIndices; j++)
//                    indices.push_back(face.mIndices[j]);
//            }
//        }
//
//        void process(const aiScene* scene, const std::string& modelPath, aiMaterial* mat, aiTextureType type, const char* logicalName) {
//            if (mat->GetTextureCount(type) == 0) return;
//            aiString str; mat->GetTexture(type, 0, &str);
//            if (str.C_Str()[0] == '*') {
//                int idx = atoi(str.C_Str() + 1);
//                aiTexture* tex = scene->mTextures[idx];
//                if (tex->mHeight == 0) {
//                    int w, h, ch;
//                    unsigned char* data = stbi_load_from_memory(
//                        reinterpret_cast<unsigned char*>(tex->pcData),
//                        tex->mWidth, &w, &h, &ch, 0
//                    );
//                    if (data) pushTextureFromMemory(logicalName, data, w, h, ch, true);
//                }
//                else {
//                    int w = tex->mWidth, h = tex->mHeight, ch = 4;
//                    unsigned char* raw = new unsigned char[w * h * 4];
//                    memcpy(raw, tex->pcData, w * h * 4);
//                    textures.push_back({ logicalName, std::make_unique<gl::texture2D>(raw, w, h, ch) });
//                    delete[] raw;
//                }
//            }
//            else {
//                std::filesystem::path texPath = std::filesystem::path(modelPath).parent_path() / str.C_Str();
//                if (std::filesystem::exists(texPath)) {
//                    textures.push_back({ logicalName, std::make_unique<gl::texture2D>(texPath.string()) });
//                }
//            }
//        }
//
//        void loadTextures(const aiScene* scene, const std::string& modelPath) {
//            for (unsigned m = 0; m < scene->mNumMaterials; ++m) {
//                process(scene, modelPath, scene->mMaterials[m], aiTextureType_DIFFUSE, "baseColor");
//                process(scene, modelPath, scene->mMaterials[m], aiTextureType_NORMALS, "normal");
//                process(scene, modelPath, scene->mMaterials[m], aiTextureType_METALNESS, "metallicRoughness");
//                process(scene, modelPath, scene->mMaterials[m], aiTextureType_AMBIENT_OCCLUSION, "occlusion");
//                process(scene, modelPath, scene->mMaterials[m], aiTextureType_EMISSIVE, "emissive");
//            }
//        }
//
//        void pushTextureFromMemory(const std::string& logicalName, unsigned char* data, int w, int h, int channels, bool freeAfter = true) {
//            if (!data) return;
//            textures.push_back({ logicalName, std::make_unique<gl::texture2D>(data, w, h, channels) });
//            if (freeAfter) stbi_image_free(data);
//        }
//
//        void setupMesh() {
//            glGenVertexArrays(1, &VAO);
//            glGenBuffers(1, &VBO);
//            glGenBuffers(1, &EBO);
//
//            glBindVertexArray(VAO);
//
//            glBindBuffer(GL_ARRAY_BUFFER, VBO);
//            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertex), vertices.data(), GL_STATIC_DRAW);
//
//            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
//            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
//
//            // Position
//            glEnableVertexAttribArray(0);
//            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)0);
//
//            // Normal
//            glEnableVertexAttribArray(1);
//            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, Normal));
//
//            // TexCoords
//            glEnableVertexAttribArray(2);
//            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, TexCoords));
//
//            // Tangent
//            glEnableVertexAttribArray(3);
//            glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, Tangent));
//
//            glBindVertexArray(0);
//        }
//    }; // class object
//
//}

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

        GLuint VAO = 0, VBO = 0, EBO = 0;

        struct TexEntry {
            std::string name;          // logical name (e.g., "baseColor")
            gl::texture2D* text;       // raw pointer (ownership handled elsewhere)
        };
        std::vector<TexEntry> textures;

        std::shared_ptr<gl::shader> m_Shader;
        Assimp::Importer* importer{ nullptr }; // keep alive for embedded textures

    public:
        object(const std::string& glbPath, gl::shader* shader)
            : m_Shader(std::shared_ptr<gl::shader>(shader))
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

        void draw(const glm::vec3& pos, const glm::vec3& scale, const glm::vec3& rotation) {
            m_Shader->useProgram();

            glm::mat4 model = glm::translate(glm::mat4(1.0f), pos) *
                glm::rotate(glm::mat4(1.0f), glm::radians(rotation.x), glm::vec3(1, 0, 0)) *
                glm::rotate(glm::mat4(1.0f), glm::radians(rotation.y), glm::vec3(0, 1, 0)) *
                glm::rotate(glm::mat4(1.0f), glm::radians(rotation.z), glm::vec3(0, 0, 1)) *
                glm::scale(glm::mat4(1.0f), scale);

            m_Shader->setUniformMatrix4fv("model", model);

            for (unsigned char i = 0; i < MAX_TEXTURE_UNITS; ++i) {
                glActiveTexture(GL_TEXTURE0 + i);
                if (i < textures.size() && textures[i].text) {
                    textures[i].text->bind(GL_TEXTURE_2D);
                    GLint loc = m_Shader->getUniformLoc(textures[i].name);
                    if (loc >= 0) glUniform1i(loc, i);
                }
                else glBindTexture(GL_TEXTURE_2D, 0);
            }

            uploadLights();

            glBindVertexArray(VAO);
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);

            glActiveTexture(GL_TEXTURE0);
        }

        void draw(const glm::mat4& model) {
            m_Shader->useProgram();
            m_Shader->setUniformMatrix4fv("model", model);

            for (unsigned char i = 0; i < MAX_TEXTURE_UNITS; ++i) {
                glActiveTexture(GL_TEXTURE0 + i);
                if (i < textures.size() && textures[i].text) {
                    textures[i].text->bind(GL_TEXTURE_2D);
                    GLint loc = m_Shader->getUniformLoc(textures[i].name);
                    if (loc >= 0) glUniform1i(loc, i);
                }
                else glBindTexture(GL_TEXTURE_2D, 0);
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
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertex), vertices.data(), GL_STATIC_DRAW);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)0);

            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, Normal));

            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, TexCoords));

            glEnableVertexAttribArray(3);
            glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, Tangent));

            glBindVertexArray(0);
        }
    };

}