#pragma once

// Graphics headers
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

// Physics headers
#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>

// Utility headers
#include <Window.hpp>
#include <Utils.hpp>
#include <Texture.hpp>

// Standard headers
#include <vector>
#include <memory>
#include <string>
#include <unordered_map>
#include <iostream>

namespace gl {

    // Simple physics initialization
    bool InitializePhysics() {
        static bool initialized = false;
        if (initialized) return true;

        JPH::RegisterDefaultAllocator();
        JPH::Factory::sInstance = new JPH::Factory();
        JPH::RegisterTypes();

        initialized = true;
        std::cout << "Physics initialized" << std::endl;
        return true;
    }

    // ============ MESH STRUCT ============
    struct Mesh {
        struct Vertex {
            glm::vec3 position;
            glm::vec3 normal;
            glm::vec2 texCoords;
            glm::vec3 tangent;
            glm::vec3 bitangent;
        };

        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        glm::vec3 minBounds = glm::vec3(FLT_MAX);
        glm::vec3 maxBounds = glm::vec3(-FLT_MAX);

        Mesh(const std::vector<Vertex>& verts, const std::vector<unsigned int>& inds)
            : vertices(verts), indices(inds) {
            calculateBounds();
        }

        glm::vec3 getCenter() const {
            return (minBounds + maxBounds) * 0.5f;
        }

        glm::vec3 getExtents() const {
            return (maxBounds - minBounds) * 0.5f;
        }

        // Get vertices for physics shape (scaled)
        std::vector<glm::vec3> getPhysicsVertices(const glm::vec3& scale = glm::vec3(1.0f)) const {
            std::vector<glm::vec3> result;
            result.reserve(vertices.size());
            for (const auto& v : vertices) {
                result.push_back(v.position * scale);
            }
            return result;
        }

    private:
        void calculateBounds() {
            for (const auto& v : vertices) {
                minBounds = glm::min(minBounds, v.position);
                maxBounds = glm::max(maxBounds, v.position);
            }
        }
    };

    // ============ OBJECT STRUCT ============
    struct Object {
        // Rendering
        std::vector<std::shared_ptr<Mesh>> meshes;
        std::shared_ptr<shader> shader;
        std::unordered_map<std::string, GLuint> textures;

        // Transform
        glm::vec3 position = glm::vec3(0.0f);
        glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        glm::vec3 scale = glm::vec3(1.0f);

        // Physics
        JPH::Body* physicsBody = nullptr;
        JPH::BodyInterface* physicsInterface = nullptr;
        bool hasPhysics = false;

        // State
        std::string name;
        bool visible = true;

        Object() = default;

        Object(const std::string& objName) : name(objName) {}

        // Load model from file
        bool loadModel(const std::string& path) {
            Assimp::Importer importer;
            const aiScene* scene = importer.ReadFile(path,
                aiProcess_Triangulate |
                aiProcess_GenSmoothNormals |
                aiProcess_FlipUVs);

            if (!scene || !scene->mRootNode) {
                std::cerr << "Failed to load model: " << path << std::endl;
                return false;
            }

            processNode(scene->mRootNode, scene);
            return true;
        }

        // Create physics body (simple box shape from bounds)
        void createPhysicsBody(JPH::PhysicsSystem& physicsSystem, bool isStatic = true) {
            if (meshes.empty()) return;

            // Use first mesh for physics shape
            auto& mesh = meshes[0];
            glm::vec3 center = mesh->getCenter() * scale;
            glm::vec3 extents = mesh->getExtents() * scale;

            // Create box shape
            JPH::BoxShapeSettings boxSettings(JPH::Vec3(extents.x, extents.y, extents.z));
            JPH::ShapeSettings::ShapeResult result = boxSettings.Create();

            if (!result.IsValid()) return;

            JPH::Ref<JPH::Shape> shape = result.Get();

            // Create body
            JPH::EMotionType motionType = isStatic ?
                JPH::EMotionType::Static : JPH::EMotionType::Dynamic;

            auto settings = JPH::BodyCreationSettings(
                shape,
                JPH::RVec3Arg(position.x, position.y, position.z),
                JPH::QuatArg(rotation.w, rotation.x, rotation.y, rotation.z),
                motionType,
                0 // Layer
            );

            physicsBody = physicsSystem.GetBodyInterface().CreateBody(settings);
            if (physicsBody) {
                physicsSystem.GetBodyInterface().AddBody(physicsBody->GetID(), JPH::EActivation::Activate);
                physicsInterface = &physicsSystem.GetBodyInterface();
                hasPhysics = true;
            }
        }

        // Update transform from physics
        void updateFromPhysics() {
            if (!hasPhysics || !physicsBody || !physicsInterface) return;

            JPH::Vec3 pos = physicsInterface->GetPosition(physicsBody->GetID());
            JPH::Quat rot = physicsInterface->GetRotation(physicsBody->GetID());

            position = glm::vec3(pos.GetX(), pos.GetY(), pos.GetZ());
            rotation = glm::quat(rot.GetW(), rot.GetX(), rot.GetY(), rot.GetZ());
        }

        // Get model matrix for rendering
        glm::mat4 getModelMatrix() const {
            glm::mat4 mat = glm::translate(glm::mat4(1.0f), position);
            mat *= glm::mat4_cast(rotation);
            mat = glm::scale(mat, scale);
            return mat;
        }

        // Simple render
        void render() {
            if (!visible || !shader || meshes.empty()) return;

            shader->useProgram();
            shader->setUniformMat4fv("model", getModelMatrix());

            // Bind textures if any
            int texUnit = 0;
            for (auto& [name, texId] : textures) {
                glActiveTexture(GL_TEXTURE0 + texUnit);
                glBindTexture(GL_TEXTURE_2D, texId);
                shader->setUniform1i(name.c_str(), texUnit);
                texUnit++;
            }

            // For now, just render first mesh
            // You'd need VAO/VBO setup for actual rendering
        }

    private:
        void processNode(aiNode* node, const aiScene* scene) {
            for (unsigned int i = 0; i < node->mNumMeshes; i++) {
                aiMesh* aiMesh = scene->mMeshes[node->mMeshes[i]];
                meshes.push_back(convertMesh(aiMesh));
            }

            for (unsigned int i = 0; i < node->mNumChildren; i++) {
                processNode(node->mChildren[i], scene);
            }
        }

        std::shared_ptr<Mesh> convertMesh(aiMesh* aiMesh) {
            std::vector<Mesh::Vertex> vertices;
            std::vector<unsigned int> indices;

            // Convert vertices
            for (unsigned int i = 0; i < aiMesh->mNumVertices; i++) {
                Mesh::Vertex vertex;

                vertex.position = glm::vec3(
                    aiMesh->mVertices[i].x,
                    aiMesh->mVertices[i].y,
                    aiMesh->mVertices[i].z
                );

                if (aiMesh->HasNormals()) {
                    vertex.normal = glm::vec3(
                        aiMesh->mNormals[i].x,
                        aiMesh->mNormals[i].y,
                        aiMesh->mNormals[i].z
                    );
                }

                if (aiMesh->mTextureCoords[0]) {
                    vertex.texCoords = glm::vec2(
                        aiMesh->mTextureCoords[0][i].x,
                        aiMesh->mTextureCoords[0][i].y
                    );
                }

                vertices.push_back(vertex);
            }

            // Convert indices
            for (unsigned int i = 0; i < aiMesh->mNumFaces; i++) {
                aiFace face = aiMesh->mFaces[i];
                for (unsigned int j = 0; j < face.mNumIndices; j++) {
                    indices.push_back(face.mIndices[j]);
                }
            }

            return std::make_shared<Mesh>(vertices, indices);
        }
    };

    // ============ SCENE CLASS ============
    class Scene {
    private:
        std::string name;
        std::vector<std::shared_ptr<Object>> objects;
        std::vector<std::shared_ptr<gl::player>> players;
        std::vector<std::shared_ptr<window>> uiWindows;
        JPH::PhysicsSystem* physicsSystem = nullptr;
        JPH::TempAllocatorImpl* tempAllocator;

    public:
        Scene(const std::string& sceneName = "Scene") 
            : name(sceneName) 
        {
            tempAllocator = new JPH::TempAllocatorImpl(10 * 1024 * 1024);
        }

        void setPhysicsSystem(JPH::PhysicsSystem* system) {
            physicsSystem = system;
        }

        // Object management
        std::shared_ptr<Object> addObject(const std::string& name) {
            auto obj = std::make_shared<Object>(name);
            objects.push_back(obj);
            return obj;
        }

        std::shared_ptr<Object> addObject(std::shared_ptr<Object> obj) {
            objects.push_back(obj);
            return obj;
        }

        void removeObject(const std::string& name) {
            objects.erase(
                std::remove_if(objects.begin(), objects.end(),
                    [&](const auto& obj) { return obj->name == name; }),
                objects.end()
            );
        }

        // Player management
        std::shared_ptr<Object> addPlayer(const std::string& name) {
            auto player = std::make_shared<Object>(name);
            players.push_back(player);
            return player;
        }

        // UI management
        void addUIWindow(std::shared_ptr<window> uiWindow) {
            uiWindows.push_back(uiWindow);
        }

        // Update everything
        void update(float deltaTime) {
            // Update physics

            if (physicsSystem) {
                physicsSystem->Update(deltaTime, 1, tempAllocator, nullptr);

                // Update objects with physics
                for (auto& obj : objects) {
                    if (obj->hasPhysics) {
                        obj->updateFromPhysics();
                    }
                }

                // Update players with physics
                for (auto& player : players) {
                    if (player->hasPhysics) {
                        player->updateFromPhysics();
                    }
                }
            }
        }

        // Render everything
        void render() {
            // Render objects
            for (auto& obj : objects) {
                obj->render();
            }

            // Render players
            for (auto& player : players) {
                player->render();
            }

            // Render UI (you'd handle this with ImGui)
            for (auto& uiWindow : uiWindows) {
                // UI rendering code here
            }
        }

        // Getters
        const std::string& getName() const { return name; }
        size_t getObjectCount() const { return objects.size(); }
        size_t getPlayerCount() const { return players.size(); }

        std::vector<std::shared_ptr<Object>> getObjects() const { return objects; }
        std::vector<std::shared_ptr<Object>> getPlayers() const { return players; }

        std::shared_ptr<Object> findObject(const std::string& name) {
            for (auto& obj : objects) {
                if (obj->name == name) return obj;
            }
            return nullptr;
        }
    };

} // namespace gl