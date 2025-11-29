#include <Game.hpp>
#include <window.hpp>
#include <Utils.hpp>
#include <Mesh.hpp>

#include <Debug.hpp>

#include <iostream>
#include <thread>
#include <chrono>

#define WEAPON_OFFSET glm::vec3(-0.2f, -0.35f, 0.4f)

void a(bool& pause){
    if (ImGui::Button("Press Me")) {
        std::cout << "Button pressed!" << std::endl;
        pause = false;
    }
}

int main() {
    if (!glfwInit()) return -1;
    gl::window window(960, 540, "window");
    glDisable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);

    window.vsync(ENABLE_VSYNC);    

    if (glewInit() != GLEW_OK) return -1;

    gl::shader shader("resource/shader/vert.glsl", "resource/shader/frag.glsl");

    shader.useProgram();

    gl::object awp("resource/model/awp.glb");
    gl::object model("resource/model/player.glb");

    // Add lights to objects so they can be rendered
    awp.addLight(glm::vec3(5.0f, 5.0f, 5.0f), glm::vec3(1.0f, 1.0f, 1.0f));
    awp.addLight(glm::vec3(-5.0f, 5.0f, -5.0f), glm::vec3(0.5f, 0.5f, 0.8f));
    model.addLight(glm::vec3(5.0f, 5.0f, 5.0f), glm::vec3(1.0f, 1.0f, 1.0f));
    model.addLight(glm::vec3(-5.0f, 5.0f, -5.0f), glm::vec3(0.5f, 0.5f, 0.8f));

    gl::player player(gl::camera(glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), shader.getProgram()));

    bool ifpress = false;
    bool pause = true;

    window.addUI("MyUI", glm::vec2(window.getWidth(), window.getHeight()), glm::vec2(0.0f), (bool*)1, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove, 
        [&](){
            float aspect = (float)window.getWidth() / 4.0f;
            if (ImGui::Button("resume", ImVec2(4.0f / 10.0f * aspect, 3.0f / 10.0f * aspect))) {
                pause = false;
            }
        }
    );

    while (window.ifRun()) {
        try {
            window.clearColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

            window.imguiNewFrame();

            if (ifpress) {
                if (window.getKeyRelease(GLFW_KEY_ESCAPE)) {
                    ifpress = false;
                }
            }
            else {
                if (window.getKeyPress(GLFW_KEY_ESCAPE)) {
                    pause = !pause;
                    ifpress = true;
                }
            }            

            if (pause) {
                window.setInputMode(GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                window.setUI(0, "settings", glm::vec2(window.getWidth(), window.getHeight()), glm::vec2(0.0f), 0, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove);
                window.renderUI(0);
            }
            else {
                window.setInputMode(GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                player.update(window, shader);
            }

            // Draw with yaw offset (adjust the angle as needed - positive = rotate right, negative = rotate left)
            awp.draw(shader.getProgram(), gl::getItemModelQuat(player.getCam(), WEAPON_OFFSET, 1.5f, glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::vec2(-20.0f, 15.0f)));

            // Draw model in front of camera (adjust position as needed)
            model.draw(shader.getProgram(), glm::vec3(0.0f, 0.0f, -5.0f), glm::vec3(1.0f), glm::vec3(0.0f));

            window.imguiRender();

            window.swapBuffers();
        } catch (const std::exception& e) {
			std::cout << e.what() << std::endl;
		}

        std::this_thread::sleep_for(std::chrono::milliseconds(6));
    }

    glfwTerminate();

    //std::cin.get();
}
