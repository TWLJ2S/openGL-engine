#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm.hpp>            
#include <gtc/matrix_transform.hpp>  
#include <gtc/type_ptr.hpp>  

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <stdio.h>
#include <string>
#include <vector>
#include <functional>
#include <stdexcept>
#include <iostream>
#include <memory>

#define GL_ENABLE_VSYNC 1
#define GL_ENABLE_ADAPTIVE_VSYNC 2
#define GL_DISABLE_VSYNC 0

namespace gl {

	class window {
	private:

		struct RGBA {
			GLfloat r, g, b, a;
		};

		static void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
			if (width <= 0 || height <= 0) return;

			gl::window* self = static_cast<gl::window*>(glfwGetWindowUserPointer(window));

			glViewport(0, 0, width, height);

			self->m_WindowCallBack.width = width;
			self->m_WindowCallBack.height = height;
		}

		static void mouse_pos_callback(GLFWwindow* window, double xpos, double ypos) {
			// Get the instance of your window class
			if (ImGui::GetIO().WantCaptureMouse) return;

			gl::window* self = static_cast<gl::window*>(glfwGetWindowUserPointer(window));

			// Forward the data to your instance method
			self->m_CursorCallBack.x = xpos;
			self->m_CursorCallBack.y = ypos;
		}

		static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
			// Get the instance of your window class
			if (ImGui::GetIO().WantCaptureMouse) return;

			gl::window* self = static_cast<gl::window*>(glfwGetWindowUserPointer(window));

			self->m_CursorCallBack.button = button;
			self->m_CursorCallBack.action = action;
			self->m_CursorCallBack.mode = mods;
		}

		static void window_focus_callback(GLFWwindow* window, int focused) {
			//gl::window* self = static_cast<gl::window*>(glfwGetWindowUserPointer(window));
		}

		static void window_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
			if (ImGui::GetIO().WantCaptureKeyboard) return;

			gl::window* self = static_cast<gl::window*>(glfwGetWindowUserPointer(window));

			self->m_KeyCallBack.key = key;
			self->m_KeyCallBack.scanCode = scancode;
			self->m_KeyCallBack.action = action;
			self->m_KeyCallBack.mods = mods;
		}

		static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
			// xoffset → horizontal scroll (usually 0)
			// yoffset → vertical scroll (positive = scroll up, negative = scroll down)
			if (ImGui::GetIO().WantCaptureMouse) return;

			gl::window* self = static_cast<gl::window*>(glfwGetWindowUserPointer(window));

			self->m_CursorCallBack.scrollX = xoffset;
			self->m_CursorCallBack.scrollY = yoffset;
		}

		void imguiInit(void (*func)(ImGuiStyle* dst), ImGuiStyle* dst = nullptr) {
			IMGUI_CHECKVERSION();
			ImGui::CreateContext();

			if (func) func(dst); // safe

			ImGui_ImplGlfw_InitForOpenGL(m_WindowCallBack.window, true);
			ImGui_ImplOpenGL3_Init("#version 330");

			m_WindowCallBack.imgui = &ImGui::GetIO();
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
		};

		struct windowCallBack {
			GLFWwindow* window;
			GLFWmonitor* monitor;
			GLFWwindow* share;

			ImGuiIO* imgui = nullptr;

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

		struct UIWindow {
			std::string name;
			std::function<void()> element;
			glm::vec2 size{ 200.0f, 200.0f };
			glm::vec2 pos{ 50.0f, 50.0f };
			bool open{ true };
			ImGuiWindowFlags flags{ ImGuiWindowFlags_None };

			UIWindow() = default;

			UIWindow(const std::string& n,
				const glm::vec2& s = { 200.0f, 200.0f },
				const glm::vec2& p = { 50.0f, 50.0f },
				bool o = true,
				ImGuiWindowFlags fs = ImGuiWindowFlags_None,
				std::function<void()> f = nullptr
			)
				: name(n), size(s), pos(p), open(o), flags(fs), element(std::move(f))
			{
			}

			// Set the UI element; only stores if f is valid
			void setElement(std::function<void()> f) {
				if (f) element = std::move(f);
			}

			// Render the window (between ImGui::NewFrame() and ImGui::Render())
			void render() const {
				ImGui::SetNextWindowSize(ImVec2(size.x, size.y), ImGuiCond_Always);
				ImGui::SetNextWindowPos(ImVec2(pos.x, pos.y), ImGuiCond_Always);

				ImGui::Begin(name.c_str(), const_cast<bool*>(&open), flags);

				if (element) element();

				ImGui::End();
			}
		};

		class UI {
		private:
			std::unordered_map<std::string, unsigned> iMap;
			std::vector<UIWindow> ui;

		public:
			UI() = default;

			// Get index safely
			inline unsigned getUIIndex(const std::string& name) const {
				auto it = iMap.find(name);
				if (it == iMap.end()) throw std::runtime_error("UI not found: " + name);
				return it->second;
			}

			// Get UIWindow by index
			inline UIWindow& getUI(size_t index) { return ui.at(index); }
			inline const UIWindow& getUI(size_t index) const { return ui.at(index); }

			// Get UIWindow by name
			inline UIWindow& getUI(const std::string& name) { return ui.at(getUIIndex(name)); }
			inline const UIWindow& getUI(const std::string& name) const { return ui.at(getUIIndex(name)); }

			// Render
			inline void render(size_t index) const { ui.at(index).render(); }
			inline void render(const std::string& name) const { ui.at(getUIIndex(name)).render(); }

			// Set element function
			inline void setUIElement(size_t index, std::function<void()> f) { ui.at(index).setElement(std::move(f)); }
			inline void setUIElement(const std::string& name, std::function<void()> f) { ui.at(getUIIndex(name)).setElement(std::move(f)); }

			// Add new UI window
			inline void addUI(const std::string& n,
				const glm::vec2& s = { 200.0f, 200.0f },
				const glm::vec2& p = { 50.0f, 50.0f },
				bool o = true,
				ImGuiWindowFlags fs = ImGuiWindowFlags_None,
				std::function<void()> f = nullptr)
			{
				if (iMap.find(n) != iMap.end())
					throw std::runtime_error("UI name already exists: " + n);

				ui.emplace_back(UIWindow(n, s, p, o, fs));
				if (f) ui.back().setElement(std::move(f));

				iMap[n] = static_cast<unsigned>(ui.size() - 1);
			}

			// Replace UIWindow by name
			inline void setUI(const std::string& oldName,
				const std::string& n,
				const glm::vec2& s = { 200.0f, 200.0f },
				const glm::vec2& p = { 50.0f, 50.0f },
				bool o = true,
				ImGuiWindowFlags fs = ImGuiWindowFlags_None,
				std::function<void()> f = nullptr)
			{
				unsigned index = getUIIndex(oldName);
				ui.at(index) = UIWindow(n, s, p, o, fs);
				if (f) ui.at(index).setElement(std::move(f));
				iMap.erase(oldName);
				iMap[n] = index;
			}

			// Replace UIWindow by index
			inline void setUI(size_t index,
				const std::string& n,
				const glm::vec2& s = { 200.0f, 200.0f },
				const glm::vec2& p = { 50.0f, 50.0f },
				bool o = true,
				ImGuiWindowFlags fs = ImGuiWindowFlags_None,
				std::function<void()> f = nullptr)
			{
				ui.at(index) = UIWindow(n, s, p, o, fs);
				if (f) ui.at(index).setElement(std::move(f));
			}
		};

	private:
		windowCallBack m_WindowCallBack;
		keyCallBack m_KeyCallBack;
		cursorCallBack m_CursorCallBack;
		UI m_UI;
	public:	

		window() = delete;

		window(int width, int height, const char* name, GLFWmonitor* monitor = nullptr, GLFWwindow* share = nullptr) {
			m_WindowCallBack.monitor = monitor;
			m_WindowCallBack.share = share;

			m_WindowCallBack.width = width;
			m_WindowCallBack.height = height;
			m_WindowCallBack.name = name;
			m_WindowCallBack.targetAspect = (float)width / (float)height;
			m_WindowCallBack.deltaTime = glfwGetTime();
			m_WindowCallBack.currentFrame = 0.0f;
			m_WindowCallBack.lastFrame = 0.0f;
			m_WindowCallBack.vsync = 0;

			m_CursorCallBack.x = 0.0f;
			m_CursorCallBack.y = 0.0f;

			glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
			glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

			m_WindowCallBack.window = glfwCreateWindow(width, height, name, monitor, share);

			if (!m_WindowCallBack.window) {
				m_WindowCallBack.ifinit = false;
				throw std::runtime_error("Failed to create GLFW window");
			}

			glfwSetWindowUserPointer(m_WindowCallBack.window, this);

			glfwSetKeyCallback(m_WindowCallBack.window, window_key_callback);
			glfwSetFramebufferSizeCallback(m_WindowCallBack.window, framebuffer_size_callback);
			glfwSetWindowFocusCallback(m_WindowCallBack.window, window_focus_callback);
			glfwSetScrollCallback(m_WindowCallBack.window, scroll_callback);
			glfwSetMouseButtonCallback(m_WindowCallBack.window, mouse_button_callback);
			glfwSetCursorPosCallback(m_WindowCallBack.window, mouse_pos_callback);

			glfwSetInputMode(m_WindowCallBack.window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			
			glfwMakeContextCurrent(m_WindowCallBack.window);

			imguiInit(ImGui::StyleColorsClassic);
		}

		~window() {
			glfwDestroyWindow(m_WindowCallBack.window);
		}

		bool ifRun() {
			glfwMakeContextCurrent(m_WindowCallBack.window);
			glfwPollEvents();

			m_WindowCallBack.currentFrame = (float)glfwGetTime();
			m_WindowCallBack.deltaTime = m_WindowCallBack.currentFrame - m_WindowCallBack.lastFrame;
			m_WindowCallBack.lastFrame = m_WindowCallBack.currentFrame;

			return !(glfwGetKey(m_WindowCallBack.window, GLFW_KEY_HOME) == GLFW_PRESS || glfwWindowShouldClose(m_WindowCallBack.window));
		}

		void imguiNewFrame() {
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();
		}

		void imguiRender() {
			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

			ImGui::EndFrame();
		}

		gl::window::UI& getUI() { return m_UI; }

		void setUIFont(const char* path, float size = 20.0f) {
			ImGui::GetIO().Fonts->Clear();

			// Load font at size 20
			ImGui::GetIO().FontDefault = ImGui::GetIO().Fonts->AddFontFromFileTTF(
				path, size
			);

			// Rebuild font atlas
			ImGui_ImplOpenGL3_DestroyFontsTexture();
			ImGui_ImplOpenGL3_CreateFontsTexture();
		}

		inline const void setImguiStyle(void (*func)(ImGuiStyle* dst), ImGuiStyle* dst = nullptr) const { func(dst); }

		inline const cursorCallBack getCursorCallBack() const { return m_CursorCallBack; }

		inline const keyCallBack getKeyCallBack() const { return m_KeyCallBack; }

		inline const windowCallBack getWindowCallBack() const { return m_WindowCallBack; }

		inline const int getCursorButton() const { return m_CursorCallBack.button; }

		inline const int getCursorAction() const { return m_CursorCallBack.action; }

		inline const int getCursorMode() const { return m_CursorCallBack.mode; }

		inline const int getWidth() const { return m_WindowCallBack.width; }

		inline const int getHeight() const { return m_WindowCallBack.height; }

		inline GLFWwindow* getWindow() const { return m_WindowCallBack.window; }

		inline const float getTargetAspect() const { return m_WindowCallBack.targetAspect; }

		inline void setTargetAspect(float aspect) { m_WindowCallBack.targetAspect = aspect; }

		inline const double getCursorX() const { return m_CursorCallBack.x; }

		inline const double getCursorY() const { return m_CursorCallBack.y; }

		inline void setCursorX(const double& other) { m_CursorCallBack.x = other; }

		inline void setCursorY(const double& other) { m_CursorCallBack.y = other; }

		inline const double getScrollX() const { return m_CursorCallBack.scrollX; }

		inline const double getScrollY() const { return m_CursorCallBack.scrollY; }

		inline void setScrollX(const double& other) { m_CursorCallBack.scrollX = other; }

		inline void setScrollY(const double& other) { m_CursorCallBack.scrollY = other; }

		inline const float getDeltaTime() const { return m_WindowCallBack.deltaTime; }

		inline const float getFps() const { return 1.0f / m_WindowCallBack.deltaTime; }

		inline const int vsync() const { return m_WindowCallBack.vsync; }

		int getKeyPress(const int& key) { return glfwGetKey(m_WindowCallBack.window, key) == GLFW_PRESS; }

		int getKeyHold(const int& key) { return glfwGetKey(m_WindowCallBack.window, key) == GLFW_REPEAT; }

		int getKeyRelease(const int& key) { return glfwGetKey(m_WindowCallBack.window, key) == GLFW_RELEASE; }

		inline const void setKeyCallback(void(*callBack)(GLFWwindow* window, int key, int scancode, int action, int mods)) const { 
			glfwSetKeyCallback(m_WindowCallBack.window, callBack); 
		}

		inline const void setFramebufferSizeCallback(void(*callBack)(GLFWwindow* window, int width, int height)) const {
			glfwSetFramebufferSizeCallback(m_WindowCallBack.window, framebuffer_size_callback);
		}

		inline const void setWindowFocusCallback(void(*callBack)(GLFWwindow* window, int focused)) const {
			glfwSetWindowFocusCallback(m_WindowCallBack.window, callBack);
		}

		inline const void setScrollCallback(void(*callBack)(GLFWwindow* window, double xoffset, double yoffset)) const {
			glfwSetScrollCallback(m_WindowCallBack.window, callBack);
		}

		inline const void setMouseButtonCallback(void(*callBack)(GLFWwindow* window, int button, int action, int mods)) const {
			glfwSetMouseButtonCallback(m_WindowCallBack.window, callBack);
		}

		inline const void setCursorPosCallback(void(*callBack)(GLFWwindow* window, double xpos, double ypos)) const {
			glfwSetCursorPosCallback(m_WindowCallBack.window, callBack);
		}

		inline const void setInputMode(int mode, int value) const {
			glfwSetInputMode(m_WindowCallBack.window, mode, value);
		}

		inline bool operator!() { return !m_WindowCallBack.ifinit; }

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
			glfwSwapBuffers(m_WindowCallBack.window);
		}

		void vsync(int type) { 
			glfwSwapInterval(type); 
			m_WindowCallBack.vsync = type;
		}
	};

}