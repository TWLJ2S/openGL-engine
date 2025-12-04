#pragma once

#include <GLFW/glfw3.h>
#include <GL/glew.h>

#include <glm.hpp>            
#include <gtc/matrix_transform.hpp> 
#include <gtc/type_ptr.hpp> 

#include <Window.hpp>

#include <fstream>
#include <filesystem>
#include <string>
#include <iostream>
#include <stdexcept>
#include <stb_image.h>

#define FLOAT_SIZE sizeof(float)
#define UINT_SIZE sizeof(unsigned)

#ifndef GL_VIEW_DISTANCE
#define GL_VIEW_DISTANCE 10000.0f
#endif // 


namespace gl {

    void terminate(GLuint VAO, GLuint VBO, GLuint shaderProgram) {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteProgram(shaderProgram);

        glfwTerminate();
    }

    void bindVertexArray(GLuint& VAO) {
        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);
    }

    void bindVertexBuffer(GLuint& VBO, const GLfloat* vertices, size_t size) {
        glGenBuffers(1, &VBO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);
    }

    void bindIndexBuffer(GLuint& IBO, const GLvoid* indices, GLsizeiptr size) {
        glGenBuffers(1, &IBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, indices, GL_STATIC_DRAW);
    }

    void positionAttribute(GLuint index, GLint size, GLsizei stride, const void* pointer) {
        glVertexAttribPointer(index, size, GL_FLOAT, GL_FALSE, stride, pointer);
        glEnableVertexAttribArray(index);
    }

    class texture2D {
    private:
        GLuint m_Texture;
        GLenum m_ID;
        int m_Width, m_Height, m_NrChannels;
        unsigned char* m_Data;
    public:
        texture2D(const std::string& path) {
            stbi_set_flip_vertically_on_load(true);
            m_Data = stbi_load(path.c_str(), &m_Width, &m_Height, &m_NrChannels, 0);
            if (!m_Data) {
                throw std::runtime_error("Failed to load texture: " + path + "\n");
                return;
            }
            glGenTextures(1, &m_Texture);
            glBindTexture(GL_TEXTURE_2D, m_Texture);

            GLenum format = m_NrChannels == 3 ? GL_RGB : GL_RGBA;
            glTexImage2D(GL_TEXTURE_2D, 0, format, m_Width, m_Height, 0, format, GL_UNSIGNED_BYTE, m_Data);
            glGenerateMipmap(GL_TEXTURE_2D);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // Trilinear filtering
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            if (glewIsSupported("GL_EXT_texture_filter_anisotropic")) {
                GLfloat maxAniso = 0.0f;
                glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso);
                glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAniso);
            }

            stbi_image_free(m_Data);
        }

        texture2D(unsigned char* data, int width, int height, int channels) {
            if (!data) throw std::runtime_error("Texture data is null");

            stbi_set_flip_vertically_on_load(true);

            glGenTextures(1, &m_Texture);
            glBindTexture(GL_TEXTURE_2D, m_Texture);

            GLenum format = GL_RGBA;
            if (channels == 3) format = GL_RGB;
            else if (channels == 1) format = GL_RED;

            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }

        void bind(GLenum textureUnit = GL_TEXTURE0) const {
            glActiveTexture(textureUnit);
            glBindTexture(GL_TEXTURE_2D, m_Texture);
        }

        ~texture2D() {
            glDeleteTextures(1, &m_Texture);
        }

        GLuint getTexture() const { return m_Texture; }

        auto getData() { return m_Data; }

        auto getWidth() { return m_Width; }

        auto getHeight() { return m_Height; }

        auto getNrChannel() { return m_NrChannels; }
    };

    class texture2DArray {
    private:
        GLuint m_Texture = 0;
        int m_Width = 0;
        int m_Height = 0;
        int m_Layers = 0;

    public:
        // Load multiple same-size images into a GL_TEXTURE_2D_ARRAY
        texture2DArray(const std::vector<std::string>& paths) {
            if (paths.empty()) throw std::runtime_error("No textures provided");

            m_Layers = static_cast<int>(paths.size());
            stbi_set_flip_vertically_on_load(true);

            // Load first image to determine width, height, and format
            int nrChannels;
            unsigned char* data = stbi_load(paths[0].c_str(), &m_Width, &m_Height, &nrChannels, 0);
            if (!data) throw std::runtime_error("Failed to load texture: " + paths[0]);

            GLenum format = (nrChannels == 4) ? GL_RGBA :
                (nrChannels == 3) ? GL_RGB :
                GL_RED;

            // Generate texture
            glGenTextures(1, &m_Texture);
            glBindTexture(GL_TEXTURE_2D_ARRAY, m_Texture);

            // Allocate storage for the array
            glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, format, m_Width, m_Height, m_Layers, 0, format, GL_UNSIGNED_BYTE, nullptr);

            // Upload first layer
            glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, m_Width, m_Height, 1, format, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);

            // Upload remaining layers
            for (int i = 1; i < m_Layers; ++i) {
                int w, h, c;
                unsigned char* d = stbi_load(paths[i].c_str(), &w, &h, &c, 0);
                if (!d) throw std::runtime_error("Failed to load texture: " + paths[i]);
                if (w != m_Width || h != m_Height)
                {
                    stbi_image_free(d);
                    throw std::runtime_error("All textures must have the same size: " + paths[i]);
                }
                glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, m_Width, m_Height, 1, format, GL_UNSIGNED_BYTE, d);
                stbi_image_free(d);
            }

            // Texture parameters
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);

            glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
        }

        void bind(GLenum textureUnit = GL_TEXTURE0) const {
            glActiveTexture(textureUnit);
            glBindTexture(GL_TEXTURE_2D_ARRAY, m_Texture);
        }

        GLuint id() const { return m_Texture; }
        int width() const { return m_Width; }
        int height() const { return m_Height; }
        int layers() const { return m_Layers; }

        ~texture2DArray() {
            if (m_Texture) glDeleteTextures(1, &m_Texture);
        }
    };

    class texture3D {
    private:
        GLuint m_Texture;
        GLuint m_Loc;
    public:
        texture3D() = delete;

        texture3D(const std::vector<std::string>& paths, GLuint loc) {
            m_Loc = loc;
            if (paths.empty()) throw std::runtime_error("No paths provided for 3D texture");

            int width = 0, height = 0;
            std::vector<unsigned char> dataAll;

            for (size_t i = 0; i < paths.size(); i++) {
                int w, h, c;
                unsigned char* slice = stbi_load(paths[i].c_str(), &w, &h, &c, 4); // force RGBA
                if (!slice) throw std::runtime_error("Failed to load slice: " + paths[i]);

                if (i == 0) { width = w; height = h; }
                else if (w != width || h != height) {
                    throw std::runtime_error("Slice dimensions mismatch: " + paths[i]);
                }

                size_t sliceSize = width * height * 4; // 4 channels
                dataAll.insert(dataAll.end(), slice, slice + sliceSize);
                stbi_image_free(slice);
            }

            glGenTextures(1, &m_Texture);
            glBindTexture(GL_TEXTURE_3D, m_Texture);

            glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, width, height, static_cast<GLsizei>(paths.size()), 0, GL_RGBA, GL_UNSIGNED_BYTE, dataAll.data());

            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }

        texture3D(const std::string* paths, unsigned size, GLuint loc) {
            m_Loc = loc;
            int width = 0, height = 0;
            std::vector<unsigned char> dataAll;

            for (size_t i = 0; i < size; i++) {
                int w, h, c;
                unsigned char* slice = stbi_load(paths[i].c_str(), &w, &h, &c, 4);
                if (!slice) throw std::runtime_error("Failed to load slice: " + paths[i]);

                if (i == 0) { width = w; height = h; }
                else if (w != width || h != height) {
                    throw std::runtime_error("Slice dimensions mismatch: " + paths[i]);
                }

                size_t sliceSize = width * height * 4; // 4 channels
                dataAll.insert(dataAll.end(), slice, slice + sliceSize);
                stbi_image_free(slice);
            }

            glGenTextures(1, &m_Texture);
            glBindTexture(GL_TEXTURE_3D, m_Texture);

            glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, width, height, static_cast<GLsizei>(size), 0, GL_RGBA, GL_UNSIGNED_BYTE, dataAll.data());

            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        }

        texture3D(const std::vector<std::string>& paths, GLuint shader, const std::string& name) {
            m_Loc = glGetUniformLocation(shader, name.c_str());
            if (paths.empty()) throw std::runtime_error("No paths provided for 3D texture");

            int width = 0, height = 0;
            std::vector<unsigned char> dataAll;

            for (size_t i = 0; i < paths.size(); i++) {
                int w, h, c;
                unsigned char* slice = stbi_load(paths[i].c_str(), &w, &h, &c, 4); // force RGBA
                if (!slice) throw std::runtime_error("Failed to load slice: " + paths[i]);

                if (i == 0) { width = w; height = h; }
                else if (w != width || h != height) {
                    throw std::runtime_error("Slice dimensions mismatch: " + paths[i]);
                }

                size_t sliceSize = width * height * 4; // 4 channels
                dataAll.insert(dataAll.end(), slice, slice + sliceSize);
                stbi_image_free(slice);
            }

            glGenTextures(1, &m_Texture);
            glBindTexture(GL_TEXTURE_3D, m_Texture);

            glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, width, height, static_cast<GLsizei>(paths.size()), 0, GL_RGBA, GL_UNSIGNED_BYTE, dataAll.data());

            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        }

        texture3D(const std::string* paths, unsigned size, GLuint shader, const std::string& name) {
            m_Loc = glGetUniformLocation(shader, name.c_str());
            int width = 0, height = 0;
            std::vector<unsigned char> dataAll;

            for (size_t i = 0; i < size; i++) {
                int w, h, c;
                unsigned char* slice = stbi_load(paths[i].c_str(), &w, &h, &c, 4);
                if (!slice) throw std::runtime_error("Failed to load slice: " + paths[i]);

                if (i == 0) { width = w; height = h; }
                else if (w != width || h != height) {
                    throw std::runtime_error("Slice dimensions mismatch: " + paths[i]);
                }

                size_t sliceSize = width * height * 4; // 4 channels
                dataAll.insert(dataAll.end(), slice, slice + sliceSize);
                stbi_image_free(slice);
            }

            glGenTextures(1, &m_Texture);
            glBindTexture(GL_TEXTURE_3D, m_Texture);

            glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, width, height, static_cast<GLsizei>(size), 0, GL_RGBA, GL_UNSIGNED_BYTE, dataAll.data());

            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }

        void bind(GLenum textureUnit = GL_TEXTURE0) {
            glActiveTexture(textureUnit);
            glBindTexture(GL_TEXTURE_3D, m_Texture);
            glUniform1i(m_Loc, textureUnit - GL_TEXTURE0);
        }

        ~texture3D() {
            glDeleteTextures(1, &m_Texture);
        }

        GLuint getTexture() const { return m_Texture; }
    };

    void bindTexture(GLsizei n, GLuint& texture, GLenum mode) {
        glGenTextures(n, &texture);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(mode, texture);
    }

    void generateTexture(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, unsigned char* data) {
        glTexImage2D(target, level, internalFormat, width, height, border, format, type, (const void*)data);
        glGenerateMipmap(target);
        stbi_image_free(data);
    }

    class shader {
    private:
        struct UniformInfo {
            std::unordered_map<std::string, GLint> sca;
            std::unordered_map<std::string, GLint> arr; // base name -> location of [0]
        };

        GLuint m_ShaderProgram;
        UniformInfo m_Uniforms;

        UniformInfo getShaderUniforms(GLuint program) {
            UniformInfo uniforms;

            GLint numUniforms = 0;
            glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &numUniforms);

            GLint maxNameLength = 0;
            glGetProgramiv(program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxNameLength);
            std::vector<char> nameBuffer(maxNameLength);

            for (GLint i = 0; i < numUniforms; ++i) {
                GLsizei length = 0;
                GLint size = 0;
                GLenum type = 0;

                glGetActiveUniform(program, i, maxNameLength, &length, &size, &type, nameBuffer.data());
                std::string name(nameBuffer.data(), length);

                GLint location = glGetUniformLocation(program, name.c_str());
                if (location < 0) continue;

                // Detect if it is an array (usually ends with "[0]")
                size_t bracket = name.find('[');
                if (bracket == std::string::npos) {
                    uniforms.sca[name] = location;
                }
                else {         
                    std::string baseName = name.substr(0, bracket);
                    if (!uniforms.arr.count(baseName))
                        uniforms.arr[baseName] = location;
                }
            }

            return uniforms;
        }

        inline std::filesystem::path getShaderPath(const std::string& relativePath) {
            // Resolve absolute path relative to the executable's working directory
            std::filesystem::path path = std::filesystem::current_path() / relativePath;

            // Ensure file exists
            if (!std::filesystem::exists(path)) {
                throw std::runtime_error(
                    "Shader file does not exist: " + path.string() +
                    "\nWorking directory: " + std::filesystem::current_path().string()
                );
            }

            return path;
        }

        // Read file contents in a binary-safe, fast way
        std::string getShader(const std::string& filename) {
            auto fullPath = getShaderPath(filename);

            std::ifstream file(fullPath, std::ios::binary | std::ios::ate);
            if (!file.is_open())
                throw std::runtime_error("Failed to open shader file: " + fullPath.string());

            std::streamsize size = file.tellg();
            if (size <= 0)
                throw std::runtime_error("Shader file is empty or unreadable: " + fullPath.string());

            std::string buffer(size, '\0');

            file.seekg(0, std::ios::beg);
            if (!file.read(buffer.data(), size))
                throw std::runtime_error("Failed to read shader file: " + fullPath.string());

            return buffer;
        }

        GLuint compileShader(const std::string& filePath, GLenum type) {
            // Load shader source from file
            std::string source = getShader(filePath);
            const char* src = source.c_str();

            // Create shader object
            GLuint shader = glCreateShader(type);
            glShaderSource(shader, 1, &src, nullptr);
            glCompileShader(shader);

            // Check compilation
            GLint success = 0;
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

            if (!success) {
                GLint logLength = 0;
                glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);

                std::string log(logLength, '\0');
                glGetShaderInfoLog(shader, logLength, nullptr, log.data());

                glDeleteShader(shader); // avoid leaks

                std::string typeName =
                    (type == GL_VERTEX_SHADER) ? "VERTEX" :
                    (type == GL_FRAGMENT_SHADER) ? "FRAGMENT" : "UNKNOWN";

                throw std::runtime_error(
                    "Compilation failed (" + typeName + "):\n" +
                    "File: " + filePath + "\n" +
                    log
                );
            }

            return shader;
        }

        GLuint createProgram(const std::string& vertexShaderPath, const std::string& fragmentShaderPath) {
            GLuint vertex = compileShader(vertexShaderPath, GL_VERTEX_SHADER);
            GLuint fragment = compileShader(fragmentShaderPath, GL_FRAGMENT_SHADER);

            GLuint program = glCreateProgram();
            glAttachShader(program, vertex);
            glAttachShader(program, fragment);
            glLinkProgram(program);

            // shaders can be deleted immediately after linking
            glDeleteShader(vertex);
            glDeleteShader(fragment);

            GLint success = 0;
            glGetProgramiv(program, GL_LINK_STATUS, &success);

            if (!success) {
                GLint logLength = 0;
                glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);

                std::string log(logLength, '\0');
                glGetProgramInfoLog(program, logLength, nullptr, log.data());

                glDeleteProgram(program);

                throw std::runtime_error("Program linking failed:\n" + log);
            }

            return program;
        }

    public:
        shader(const char* vertexShaderName, const char* fragmentShaderName) {
            m_ShaderProgram = createProgram(vertexShaderName, fragmentShaderName);
            m_Uniforms = getShaderUniforms(m_ShaderProgram);
        }

        void useProgram() const { glUseProgram(m_ShaderProgram); }

        GLuint getProgram() const { return m_ShaderProgram; }

        // Scalar uniform
        GLint getUniformLoc(const std::string& name) const {
            auto it = m_Uniforms.sca.find(name);
            return (it != m_Uniforms.sca.end()) ? it->second : -1;
        }

        // Array uniform
        GLint getUniformLoc(const std::string& baseName, int index) const {
            auto it = m_Uniforms.arr.find(baseName);
            return (it != m_Uniforms.arr.end()) ? it->second + index : -1;
        }

        void setUniformMatrix4fv(const std::string& name, const glm::mat4& data) {
            GLint loc = getUniformLoc(name);
            if (loc >= 0)
                glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(data));
        }
    };

    class camera {
    private:
        glm::vec3 m_Target;
        glm::vec3 m_UpVector;

        GLuint m_Shader;

        glm::vec3 Position;
        glm::vec3 Front;
        glm::vec3 Right;
        glm::vec3 Up;

        float mouseSens;

        float Fov;
    public:

        camera(const glm::vec3& position, const glm::vec3& target, const glm::vec3& upVector, const GLuint& shader)
            : m_Target(target), m_UpVector(upVector), m_Shader(shader),
            Position(position), Front(glm::normalize(target - position)), 
            Right(0.0f), Up(upVector), mouseSens(0.03f), Fov(60.0f)
        {
        }

        const GLuint getShader() const { return m_Shader; }

        const float getFov() const { return Fov; }

        void setFov(const float& other) { Fov = other; }

        glm::mat4 getProjectionMatrix(const float& aspectRatio, const float& fov) const { return glm::perspective(glm::radians(fov), aspectRatio, 0.1f, GL_VIEW_DISTANCE); }

        const glm::mat4 getViewMatrix() const { return glm::lookAt(Position, Position + Front, Up); }

        void setTarget(const glm::vec3& target) { m_Target = target; }

        void setUpVector(const glm::vec3& upVector) { m_UpVector = upVector; }

        void setSens(const float& other) { mouseSens = other; }

        void setPos(const glm::vec3& other) { Position = other; }

        const float getSens() const { return mouseSens; }

        const glm::vec3 getPos() const { return Position; }

        const glm::vec3 getTarget() const { return m_Target; }

        const glm::vec3 getUpVector() const { return m_UpVector; }

        const glm::vec3 getRight() const { return Right; }

        const glm::vec3 getFront() const { return Front; }

        void setFront(const glm::vec3& front) { Front = front; }
    };

    //template <class T>
    class uniform {
    private:
        glm::mat4 m_Data;
        GLuint m_Location;
    public:
        uniform() = default;

        uniform(glm::mat4 value, GLuint location)
            : m_Location(location), m_Data(value)
        {
        }

        uniform& operator=(const uniform& other) {
            if (this != &other) {
                this->m_Data = other.m_Data;
                this->m_Location = other.m_Location;
            }
            return *this;
        }

        uniform& operator=(const glm::mat4& other) {
            this->m_Data = other;
            return *this;
        }

        const glm::mat4 getValue() const { return m_Data; }

        const GLuint getlocation() const { return m_Location; }

        void uniformMatrix4fv() { glUniformMatrix4fv(m_Location, 1, GL_FALSE, glm::value_ptr(m_Data)); }

        void setValue(glm::mat4 value) { m_Data = value; }

        void translate(glm::vec3 value) { m_Data = glm::translate(m_Data, value); }

        void rotate(float radians, glm::vec3 pos) { m_Data = glm::rotate(m_Data, radians, pos); }

        void scale(glm::vec3 value) { m_Data = glm::scale(m_Data, value); }

        void lookAt(glm::vec3 position, glm::vec3 target, glm::vec3 upVector) { m_Data = glm::lookAt(position, target, upVector); }

        void lookAt(gl::camera camera) { m_Data = glm::lookAt(camera.getPos(), camera.getTarget(), camera.getUpVector()); }
    };

}
