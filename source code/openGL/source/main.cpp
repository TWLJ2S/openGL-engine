#include <Game.hpp>
#include <window.hpp>
#include <Utils.hpp>
#include <Mesh.hpp>

#include <iostream>
#include <memory>
#include <thread>
#include <chrono>

#include <windows.h>

//void GLAPIENTRY glDebugCallback(GLenum source, GLenum type, GLuint id,
//    GLenum severity, GLsizei length,
//    const GLchar* message, const void* userParam)
//{
//    std::cout << "[GL ERROR] " << message << " (type=" << type
//        << ", id=" << id << ", severity=" << severity << ")\n";
//}

int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow
) {
    if (!glfwInit()) return -1;
    auto window = std::make_shared<gl::window>(960, 540, "window");
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);

    if (glewInit() != GLEW_OK) return -1;

    window->vsync(0);

    auto shader = std::make_shared<gl::shader>("resource/shader/vert.glsl", "resource/shader/frag.glsl");

    shader->useProgram();

    gl::player player(gl::camera(glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)), 
        window, shader);

    bool ifpress = false;
    bool pause = true;

    int targetedFps = 500;

    window->addUIWindow(
        "pause",
        glm::vec2(window->getWidth(), window->getHeight()),
        glm::vec2(0.0f),
        true,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove,
        [window, &pause, &ifpress, parent = &window->getUIWindow("pause")]() {
            if (parent->renderIsSuppressed) return;

            parent->size = glm::vec2((float)window->getWidth(), (float)window->getHeight());
            float aspect = (float)window->getWidth() / 3.2f;
            ImVec2 size(aspect, 0.75f * aspect);

            if (ImGui::Button("resume", size)) pause = false;

            if (ifpress) {
                if (window->getKeyRelease(GLFW_KEY_ESCAPE)) ifpress = false;
            }
            else {
                if (window->getKeyPress(GLFW_KEY_ESCAPE)) {
                    pause = !pause;
                    ifpress = true;
                }
            }

            if (auto child = parent->getChild("video settings")) {
                if (ImGui::Button("video settings", size)) {
                    child->renderIsSuppressed = false;
                    parent->renderIsSuppressed = true;
                }
            }
        }
    );

    window->addChildUIWindow(
        "pause",
        "video settings",
        glm::vec2(window->getWidth(), window->getHeight()),
        glm::vec2(0.0f),
        true, // open
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove,
        [&window, parent = &window->getUIWindow("pause"), &targetedFps, &ifpress]() {
            auto ui = parent->getChild("video settings");
            if (ui->renderIsSuppressed) return;

            ui->size = glm::vec2((float)window->getWidth(), (float)window->getHeight());

            ImGui::DragInt("FPS Limit: ", &targetedFps, 1.0f, 15, 1000, "%d", 0);

            // Close child if Escape pressed
            if (!ifpress && window->getKeyPress(GLFW_KEY_ESCAPE)) {
                parent->renderIsSuppressed = false;
                ui->renderIsSuppressed = true;
                ifpress = true;
                return;
            }
        }
    );

    window->addUIWindow("fps counter", glm::vec2(200.0f, 150.0f), glm::vec2(0.0f), true, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove,
        [&window]() {
            ImGui::Text("fps: %.3f", window->getFps());
        }
    );

    glm::vec3 offset(-0.22f, -0.26f, 0.38f);
    float scale = 1.2f;
    glm::vec2 rot(-10.46f, 3.39f);

    char weapon = 0;

    const glm::mat4 modelMat(
        30.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 30.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 30.0f, 0.0f,
        0.0f, 0.0f, -5.0f, 1.0f
    );

    while (window->ifRun()) {
        auto frameStart = std::chrono::high_resolution_clock::now();

        window->clearColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
  
        window->imguiNewFrame();
  
        if (pause) {
            window->setInputMode(GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            window->getUIWindow("pause").render();
        }
        else {

            if (ifpress) {
                if (window->getKeyRelease(GLFW_KEY_ESCAPE)) {
                    ifpress = false;
                }
            }
            else {
                if (window->getKeyPress(GLFW_KEY_ESCAPE)) {
                    pause = !pause;
                    ifpress = true;
                }
            }

            window->getUIWindow("fps counter").render();
            window->setInputMode(GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            player.update();

            weapon += window->getCursorCallBack().scrollY;
        }
  
        window->imguiRender();
  
        window->setScrollX(0.0f);
        window->setScrollY(0.0f);

        window->swapBuffers();

        /*while (true) {
            GLenum err = glGetError();
            if (err == GL_NO_ERROR) break;
            else std::cout << "opengl error: " << err << std::endl;
        }*/
        double sleepTime = (1.0 / (double)targetedFps) - std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - frameStart).count();
        if (sleepTime > 0.0) std::this_thread::sleep_for(std::chrono::duration<double>(sleepTime));
    }

    glfwTerminate();

    //std::cin.get();
}
