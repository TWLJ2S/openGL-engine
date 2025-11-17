#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm.hpp>            
#include <gtc/matrix_transform.hpp>  
#include <gtc/type_ptr.hpp>  

//#include <imgui.h>
//#include <imgui_impl_glfw.h>
//#include <imgui_impl_opengl3.h>
#include <stdio.h>

#include <string>
#include <vector>
#include <functional>
#include <stdexcept>
#include <iostream>

#define ENABLE_VSYNC 1
#define ENABLE_ADAPTIVE_VSYNC -1
#define DISABLE_VSYNC 0

namespace gl {

	class camera;

	class window {
	private:
		struct RGBA {
			GLfloat r, g, b, a;
		};

		static void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
			if (width <= 0 || height <= 0) return;

			gl::window* self = static_cast<gl::window*>(glfwGetWindowUserPointer(window));

			self->m_Window.width = width;
			self->m_Window.height = height;

			glViewport(0, 0, width, height);
		}

		static void mouse_pos_callback(GLFWwindow* window, double xpos, double ypos) {
			// Get the instance of your window class
			//if (ImGui::GetIO().WantCaptureMouse) return;

			gl::window* self = static_cast<gl::window*>(glfwGetWindowUserPointer(window));

			//self->m_CursorCallBack.lastXPos = self->m_CursorCallBack.xPos;
			//self->m_CursorCallBack.lastYPos = self->m_CursorCallBack.yPos;

			// Forward the data to your instance method
			self->m_CursorCallBack.x = xpos;
			self->m_CursorCallBack.y = ypos;
		}

		static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
			// Get the instance of your window class
			//if (ImGui::GetIO().WantCaptureMouse) return;

			gl::window* self = static_cast<gl::window*>(glfwGetWindowUserPointer(window));

			self->m_CursorCallBack.button = button;
			self->m_CursorCallBack.action = action;
			self->m_CursorCallBack.mode = mods;

			if (!self->m_CursorCallBack.hideCursor && button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
				glfwSetCursorPos(self->m_Window.window, 0.0f, 0.0f);
				glfwSetInputMode(self->m_Window.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
				glfwSetCursorPosCallback(self->m_Window.window, mouse_pos_callback);
				self->m_CursorCallBack.hideCursor = true;
			}
		}

		static void window_focus_callback(GLFWwindow* window, int focused) {
			gl::window* self = static_cast<gl::window*>(glfwGetWindowUserPointer(window));

			self->m_CursorCallBack.hideCursor = focused;
		}

		static void window_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
			//if (ImGui::GetIO().WantCaptureKeyboard)
			//	return;

			gl::window* self = static_cast<gl::window*>(glfwGetWindowUserPointer(window));

			self->m_KeyCallBack.key = key;
			self->m_KeyCallBack.scanCode = scancode;
			self->m_KeyCallBack.action = action;
			self->m_KeyCallBack.mods = mods;

			if (self->m_CursorCallBack.hideCursor && key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
				glfwSetCursorPosCallback(self->m_Window.window, nullptr);
				glfwSetCursorPos(self->m_Window.window, (float)self->m_Window.width / -2.0f, (float)self->m_Window.height / -2.0f);
				glfwSetInputMode(self->m_Window.window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
				self->m_CursorCallBack.hideCursor = false;
			}
		}

		static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
			// xoffset → horizontal scroll (usually 0)
			// yoffset → vertical scroll (positive = scroll up, negative = scroll down)
			//if (ImGui::GetIO().WantCaptureMouse) return;

			gl::window* self = static_cast<gl::window*>(glfwGetWindowUserPointer(window));
			self->m_CursorCallBack.scrollX = xoffset;
			self->m_CursorCallBack.scrollY = yoffset;
		}

		struct keyCallBack{
			int key;
			int scanCode;
			int action;
			int mods;
		};

		struct cursorCallBack {
			int button;
			int action;
			int mode;

			double scrollX;
			double scrollY;

			double x;
			double y;

			bool hideCursor;
		};

		struct windowCallBack {
			GLFWwindow* window;
			GLFWmonitor* monitor;
			GLFWwindow* share;

			int width;
			int height;
			std::string name;
			bool ifinit;
			double targetAspect;
			float deltaTime;
			float currentFrame;
			float lastFrame;
			int vsync;
		};

		struct UI {
			std::string name;
		};

	private:
		GLuint m_Shader;

		windowCallBack m_Window;
		keyCallBack m_KeyCallBack;
		cursorCallBack m_CursorCallBack;

		friend const void setFov(const float& other);
	public:
		window() = delete;

		window(int width, int height, const char* name, GLFWmonitor* monitor = nullptr, GLFWwindow* share = nullptr) {
			m_Window.monitor = monitor;
			m_Window.share = share;

			m_Window.width = width;
			m_Window.height = height;
			m_Window.name = name;
			m_Window.targetAspect = (float)width / (float)height;
			m_Window.deltaTime = glfwGetTime();
			m_Window.currentFrame = 0.0f;
			m_Window.lastFrame = 0.0f;
			m_Window.vsync = 0;

			m_CursorCallBack.x = 0.0f;
			m_CursorCallBack.y = 0.0f;

			glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
			glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

			m_Window.window = glfwCreateWindow(width, height, name, monitor, share);

			if (!m_Window.window) {
				m_Window.ifinit = false;
				throw std::runtime_error("Failed to create GLFW window");
			}

			glfwSetWindowUserPointer(m_Window.window, this);

			glfwSetKeyCallback(m_Window.window, window_key_callback);
			glfwSetFramebufferSizeCallback(m_Window.window, framebuffer_size_callback);
			glfwSetWindowFocusCallback(m_Window.window, window_focus_callback);
			glfwSetScrollCallback(m_Window.window, scroll_callback);
			glfwSetMouseButtonCallback(m_Window.window, mouse_button_callback);
			glfwSetCursorPosCallback(m_Window.window, mouse_pos_callback);

			glfwSetInputMode(m_Window.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			
			glfwMakeContextCurrent(m_Window.window);
		}

		~window() {
			glfwDestroyWindow(m_Window.window);
		}

		bool run() {
			glfwMakeContextCurrent(m_Window.window);
			glfwPollEvents();

			m_Window.currentFrame = (float)glfwGetTime();
			m_Window.deltaTime = m_Window.currentFrame - m_Window.lastFrame;
			m_Window.lastFrame = m_Window.currentFrame;

			return !(glfwGetKey(m_Window.window, GLFW_KEY_HOME) == GLFW_PRESS || glfwWindowShouldClose(m_Window.window));
		}

		void addUI() {

		}

		inline const cursorCallBack getCursorCallBack() const { return m_CursorCallBack; }

		inline const keyCallBack getKeyCallBack() const { return m_KeyCallBack; }

		inline const windowCallBack getWindowCallBack() const { return m_Window; }

		inline const int getCursorButton() const { return m_CursorCallBack.button; }

		inline const int getCursorAction() const { return m_CursorCallBack.action; }

		inline const int getCursorMode() const { return m_CursorCallBack.mode; }

		inline const int getWidth() const { return m_Window.width; }

		inline const int getHeight() const { return m_Window.height; }

		inline GLFWwindow* getWindow() const { return m_Window.window; }

		inline const GLuint getShader() const { return m_Shader; }

		inline const float getTargetAspect() const { return m_Window.targetAspect; }

		inline void setTargetAspect(float aspect) { m_Window.targetAspect = aspect; }

		inline const double getCursorX() const { return m_CursorCallBack.x; }

		inline const double getCursorY() const { return m_CursorCallBack.y; }

		inline void setCursorX(const double& other) { m_CursorCallBack.x = other; }

		inline void setCursorY(const double& other) { m_CursorCallBack.y = other; }

		inline const double getScrollX() const { return m_CursorCallBack.scrollX; }

		inline const double getScrollY() const { return m_CursorCallBack.scrollY; }

		inline void setScrollX(const double& other) { m_CursorCallBack.scrollX = other; }

		inline void setScrollY(const double& other) { m_CursorCallBack.scrollY = other; }

		inline const bool ifHideCursor() const { return m_CursorCallBack.hideCursor; }

		inline const float getDeltaTime() const { return m_Window.deltaTime; }

		inline const float getFps() const { return 1.0f / m_Window.deltaTime; }

		const int vsync() const { return m_Window.vsync; }

		int getKeyPress(const int& key) { return glfwGetKey(m_Window.window, key) == GLFW_PRESS; }

		int getKeyHold(const int& key) { return glfwGetKey(m_Window.window, key) == GLFW_REPEAT; }

		int getKeyRelease(const int& key) { return glfwGetKey(m_Window.window, key) == GLFW_RELEASE; }

		bool operator!() { return !m_Window.ifinit; }

		void clearColor(RGBA color) {
			glClearColor(color.r, color.g, color.b, color.a);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		}

		void clearColor(glm::vec4 color) {
			glClearColor(color.r, color.g, color.b, color.a);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		}

		void clearColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a) {
			glClearColor(r, g, b, a);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		}

		void swapBuffers() {
			glfwSwapBuffers(m_Window.window);
		}

		void linkShader(GLuint shaderProgram) {
			m_Shader = shaderProgram;
			glfwSetWindowUserPointer(m_Window.window, this);
		}

		void drawArray(GLuint shaderProgram, GLuint VAO, GLenum mode, GLint first, GLsizei count) {
			glUseProgram(shaderProgram);
			glBindVertexArray(VAO);
			glDrawArrays(mode, first, count);
		}

		void drawElements(GLuint shaderProgram, GLuint VAO, GLsizei count, const GLvoid* indices) {
			glUseProgram(shaderProgram);
			glBindVertexArray(VAO);
			glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, indices);
		}

		void drawElements(GLuint VAO, GLsizei count, const GLvoid* indices) {
			glBindVertexArray(VAO);
			glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, indices);
		}

		void drawElementsWithTexture3D(GLuint VAO, GLsizei count, const GLvoid* indices, GLuint texture) {
			glBindTexture(GL_TEXTURE_3D, texture);
			glBindVertexArray(VAO);
			glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, indices);
		}

		void renderQuad(GLuint shaderProgram, GLuint VAO, GLenum mode, GLint first, GLsizei count) {
			drawArray(shaderProgram, VAO, mode, first, count);
		}

		void vsync(int type) { 
			glfwSwapInterval(type); 
			m_Window.vsync = type;
		}
	};

	struct UIElement {
		int type;
		std::string label;
		void* data;
		float minVal, maxVal;
	};

}