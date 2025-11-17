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

    inline std::filesystem::path getShaderPath(const std::string& relativePath) {
        std::filesystem::path exePath = std::filesystem::current_path();
        return exePath / relativePath;
    }

    std::string getShader(const std::string& filename) {
        std::ifstream file(getShaderPath(filename), std::ios::binary | std::ios::ate);
        if (!file) {
            throw std::runtime_error("Failed to open shader file: " + filename + '\n');
        }

        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::string buffer(size, '\0');
        if (!file.read(buffer.data(), size)) {
            throw std::runtime_error("Failed to read shader file: " + filename + '\n');
        }

        return buffer;
    }

    GLuint compileShader(const std::string& path, GLenum type) {
        std::string sourceStr = gl::getShader(path);
        const char* source = sourceStr.c_str();

        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);

        GLint success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            GLint logLength;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
            std::string log(logLength, ' ');
            glGetShaderInfoLog(shader, logLength, nullptr, log.data());
            throw std::runtime_error("Shader compilation failed [" + path + "]: " + log + '\n');
            glDeleteShader(shader);
            return 0;
        }
        return shader;
    }

    void createProgram(GLuint& shaderProgram, GLuint vertexShader, GLuint fragmentShader) {
        shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);

        GLint success;
        glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
        if (!success) {
            GLint logLength;
            glGetProgramiv(shaderProgram, GL_INFO_LOG_LENGTH, &logLength);
            std::string log(logLength, ' ');
            glGetProgramInfoLog(shaderProgram, logLength, nullptr, log.data());
            throw std::runtime_error("Program linking failed: " + log + '\n');
            glDeleteProgram(shaderProgram);
            shaderProgram = 0;
        }

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
    }

    GLuint createProgram(GLuint vertexShader, GLuint fragmentShader) {
        GLuint shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);

        GLint success;
        glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
        if (!success) {
            GLint logLength;
            glGetProgramiv(shaderProgram, GL_INFO_LOG_LENGTH, &logLength);
            std::string log(logLength, ' ');
            glGetProgramInfoLog(shaderProgram, logLength, nullptr, log.data());
            throw std::runtime_error("Program linking failed: " + log + '\n');
            glDeleteProgram(shaderProgram);
            shaderProgram = 0;
        }

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        return shaderProgram;
    }

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
        GLuint m_VertexShader;
        GLuint m_FragmentShader;
        GLuint m_ShaderProgram;

        friend void useProgram(const gl::shader& shader);
    public:
        shader(const char* vertexShaderName, const char* fragmentShaderName) {
            m_VertexShader = gl::compileShader(vertexShaderName, GL_VERTEX_SHADER);
            m_FragmentShader = gl::compileShader(fragmentShaderName, GL_FRAGMENT_SHADER);
            m_ShaderProgram = gl::createProgram(m_VertexShader, m_FragmentShader);
        }
        //vertexshader, fragmentshader

        const GLuint getProgram() const { return m_ShaderProgram; }

        const void useProgram() { glUseProgram(m_ShaderProgram); }

        const GLuint getUniformLocation(const char* name) const { return glGetUniformLocation(m_ShaderProgram, name); }

        void setUniformMatrix4fv(const char* name, glm::mat4 data) { glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, glm::value_ptr(data)); }
    };

    void useProgram(const gl::shader& shader) {
        glUseProgram(shader.m_ShaderProgram);
    }

    class camera {
    private:
        glm::vec3 m_Target;
        glm::vec3 m_UpVector;

        GLuint m_Shader;

        glm::vec3 Position;
        glm::vec3 Front;
        glm::vec3 Right;
        glm::vec3 Up;

        float Yaw;
        float Pitch;

        float mouseSens;

        float Fov;

        double lastY;
        double lastX;

        glm::vec3 speed;
        glm::vec3 velocity;
    public:

        camera(const glm::vec3& position, const glm::vec3& target, const glm::vec3& upVector, const GLuint& shader)
            : m_Target(target), m_UpVector(upVector), m_Shader(shader),
            Position(position), Front(glm::normalize(target - position)), Right(0.0f), Up(upVector),
            velocity(glm::vec3(0.0f)), Yaw(0.0f), Pitch(0.0f), mouseSens(0.03f), Fov(60.0f),
            lastY(0.0f), lastX(0.0f), speed(20.0f)
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

        const float getYaw() const { return Yaw; }

        const float getPitch() const { return Pitch; }

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
