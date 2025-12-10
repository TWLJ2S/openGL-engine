#pragma once

#include <Utils.hpp>
#include <Window.hpp>

namespace gl {

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

            if (!m_Data)
                throw std::runtime_error("Failed to load texture: " + path);

            glGenTextures(1, &m_Texture);
            glBindTexture(GL_TEXTURE_2D, m_Texture);

            GLenum format;
            switch (m_NrChannels) {
            case 1:
                format = GL_RED;
                break;
            case 3:
                format = GL_RGB;
                break;
            case 4:
                format = GL_RGBA;
                break;
            }

            glTexImage2D(GL_TEXTURE_2D, 0, format, m_Width, m_Height, 0, format, GL_UNSIGNED_BYTE, m_Data);
            glGenerateMipmap(GL_TEXTURE_2D);

            // Wrapping rules ¨C GL_RED cannot safely use GL_REPEAT on some GPUs
            if (format == GL_RED) {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            }
            else {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            }

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            // Anisotropy ¡ª only if supported and not a single-channel texture
            if (m_NrChannels > 1 && glewIsSupported("GL_EXT_texture_filter_anisotropic")) {
                GLfloat maxAniso = 0.0f;
                glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso);
                glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAniso);
            }

            stbi_image_free(m_Data);
        }

        texture2D(unsigned char* data, int width, int height, int channels) {
            if (!data)
                throw std::runtime_error("Texture data is null");

            glGenTextures(1, &m_Texture);
            glBindTexture(GL_TEXTURE_2D, m_Texture);

            GLenum format;
            switch (channels) {
            case 1:
                format = GL_RED;
                break;
            case 3:
                format = GL_RGB;
                break;
            case 4:
                format = GL_RGBA;
                break;
            }

            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);

            // Wrapping rules ¡ª GL_RED must clamp to avoid GL_INVALID_ENUM
            if (format == GL_RED) {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            }
            else {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            }

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
        GLuint m_Texture;
        int m_Width, m_Height, m_Layers;
    public:
        texture2DArray(const std::vector<texture2D*>& textures) {
            if (textures.empty()) throw std::runtime_error("No textures provided for array");

            m_Layers = static_cast<int>(textures.size());

            glGenTextures(1, &m_Texture);
            glBindTexture(GL_TEXTURE_2D_ARRAY, m_Texture);

            // Allocate empty array storage
            glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, m_Width, m_Height, m_Layers, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

            // Upload each layer
            for (int i = 0; i < m_Layers; ++i) {
                unsigned char* data = textures[i]->getData(); // you need a getter for raw pixels
                glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, m_Width, m_Height, 1, GL_RGBA, GL_UNSIGNED_BYTE, data);
            }

            // Filtering / wrapping
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

        ~texture2DArray() { glDeleteTextures(1, &m_Texture); }

        int layers() const { return m_Layers; }
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

}