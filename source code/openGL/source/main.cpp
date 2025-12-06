#include <Game.hpp>
#include <window.hpp>
#include <Utils.hpp>
#include <Mesh.hpp>

#include <iostream>
#include <memory>
#include <thread>
#include <chrono>

//void GLAPIENTRY glDebugCallback(GLenum source, GLenum type, GLuint id,
//    GLenum severity, GLsizei length,
//    const GLchar* message, const void* userParam)
//{
//    std::cout << "[GL ERROR] " << message << " (type=" << type
//        << ", id=" << id << ", severity=" << severity << ")\n";
//}

int main() {
    if (!glfwInit()) return -1;
    auto window = std::make_shared<gl::window>(960, 540, "window");
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);

    if (glewInit() != GLEW_OK) return -1;

    auto shader = std::make_shared<gl::shader>("resource/shader/vert.glsl", "resource/shader/frag.glsl");

    shader->useProgram();

    gl::object M4A1("resource/model/M4A1.glb", shader);
    gl::object awp("resource/model/awp.glb", shader);
    gl::object usp("resource/model/USPS.glb", shader);
    gl::object model("resource/model/model.glb", shader);

    // Add lights to objects so they can be rendered
    model.addLight(glm::vec3(5.0f, 5.0f, 5.0f), glm::vec3(1.0f, 1.0f, 1.0f));
    model.addLight(glm::vec3(-5.0f, 5.0f, -5.0f), glm::vec3(0.5f, 0.5f, 0.8f));

    gl::player player(gl::camera(glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)), 
        window, shader);

    bool ifpress = false;
    bool pause = true;

    window->getUI().addUI(
        "settings",
        glm::vec2(window->getWidth(), window->getHeight()),
        glm::vec2(0.0f),
        true, // open
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove,
        [&]() {
            float aspect = (float)window->getWidth() / 4.0f;
            ImVec2 size(0.4f * aspect, 0.3f * aspect);
            if (ImGui::Button("resume", size)) {
                pause = false;
            }
        }
    );

    window->getUI().addUI("fps counter", glm::vec2(200.0f, 150.0f), glm::vec2(0.0f), true, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove,
        [&]() {
            ImGui::Text("fps: %.3f", window->getFps());
        }
    );

    glm::vec3 offset(-0.22f, -0.26f, 0.38f);
    float scale = 1.2f;
    glm::vec2 rot(-10.46f, 3.39f);

    char weapon = 0;

    /*window.addUI("offset", glm::vec2(500.0f, 300.0f), glm::vec2(0.0f, 150.0f), (bool*)true, ImGuiWindowFlags_None,
        [&]() {
            ImGui::DragFloat("x: ", &offset.x, 0.01f, -5.0f, 5.0f);
            ImGui::DragFloat("y: ", &offset.y, 0.01f, -5.0f, 5.0f);
            ImGui::DragFloat("z: ", &offset.z, 0.01f, -5.0f, 5.0f);

            ImGui::DragFloat("scale: ", &scale, 0.01f, -3.0f, 3.0f);

            ImGui::DragFloat("rot x: ", &rot.x, 0.01f, -30.0f, 30.0f);
            ImGui::DragFloat("rot y: ", &rot.y, 0.01f, -30.0f, 30.0f);

        }
    );*/

    const glm::mat4 modelMat(
        30.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 30.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 30.0f, 0.0f,
        0.0f, 0.0f, -5.0f, 1.0f
    );

    while (window->ifRun()) {
        window->clearColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
  
        window->imguiNewFrame();
  
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
  
        if (pause) {
            window->setInputMode(GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            window->getUI().getUI("settings").size = glm::vec2(window->getWidth(), window->getHeight());
            window->getUI().render("settings");
        }
        else {
            window->getUI().render("fps counter");
            window->setInputMode(GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            player.update();
        }
  
        weapon += window->getCursorCallBack().scrollY;
  
        switch (weapon) {
        case -2:
            weapon = 1;
        case -1: {
            awp.draw(gl::getItemModel(player.getCam(), offset, scale, glm::quat(1.0f, 0.0f, 0.0f, 0.0f), rot));
            break;
        }
        case 0: {
            M4A1.draw(gl::getItemModel(player.getCam(), offset, scale, glm::quat(1.0f, 0.0f, 0.0f, 0.0f), rot));
            break;
        }
        case 1: {
            usp.draw(gl::getItemModel(player.getCam(), offset, scale, glm::quat(1.0f, 0.0f, 0.0f, 0.0f), rot));
            break;
        }
        case 2:
            weapon = -1;
        }        
        
        // Draw model in front of camera (adjust position as needed)
        model.draw(modelMat);
  
        window->imguiRender();
  
        window->setScrollX(0.0f);
        window->setScrollY(0.0f);

        window->swapBuffers();

        /*while (true) {
            GLenum err = glGetError();
            if (err == GL_NO_ERROR) break;
            else std::cout << "opengl error: " << err << std::endl;
        }*/

        std::this_thread::sleep_for((std::chrono::milliseconds)1);
    }

    glfwTerminate();

    //std::cin.get();
}
