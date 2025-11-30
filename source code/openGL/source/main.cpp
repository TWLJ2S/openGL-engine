#include <Game.hpp>
#include <window.hpp>
#include <Utils.hpp>
#include <Mesh.hpp>

#include <iostream>
#include <thread>
#include <chrono>

int main() {
    if (!glfwInit()) return -1;
    gl::window window(960, 540, "window");
    glDisable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);

    window.vsync(GL_DISABLE_VSYNC);    

    if (glewInit() != GLEW_OK) return -1;

    gl::shader shader("resource/shader/vert.glsl", "resource/shader/frag.glsl");

    shader.useProgram();

    gl::object M4A1("resource/model/M4A1.glb");
    gl::object awp("resource/model/awp.glb");
    gl::object usp("resource/model/USPS.glb");
    gl::object model("resource/model/model.glb");

    // Add lights to objects so they can be rendered
    model.addLight(glm::vec3(5.0f, 5.0f, 5.0f), glm::vec3(1.0f, 1.0f, 1.0f));
    model.addLight(glm::vec3(-5.0f, 5.0f, -5.0f), glm::vec3(0.5f, 0.5f, 0.8f));

    gl::player player(gl::camera(glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), shader.getProgram()));

    bool ifpress = false;
    bool pause = true;

    window.addUI("settings", glm::vec2(window.getWidth(), window.getHeight()), glm::vec2(0.0f), (bool*)1, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove, 
        [&](){
            float aspect = (float)window.getWidth() / 4.0f;
            ImVec2 size(4.0f / 10.0f * aspect, 3.0f / 10.0f * aspect);
            if (ImGui::Button("resume", size)) {
                pause = false;
            }
        }
    );

    window.addUI("fps counter", glm::vec2(200.0f, 150.0f), glm::vec2(0.0f), (bool*)1, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove,
        [&]() {
            ImGui::Text("%s", std::string("fps: " + std::to_string(window.getFps())).c_str());
        }
    );

    glm::vec3 offset(-0.22f, -0.26f, 0.38f);
    float scale = 1.2f;
    glm::vec2 rot(-10.46f, 3.39f);

    char weapon = 0;

    window.addUI("offset", glm::vec2(500.0f, 300.0f), glm::vec2(0.0f, 150.0f), (bool*)1, ImGuiWindowFlags_None,
        [&]() {
            ImGui::DragFloat("x: ", &offset.x, 0.01f, -5.0f, 5.0f);
            ImGui::DragFloat("y: ", &offset.y, 0.01f, -5.0f, 5.0f);
            ImGui::DragFloat("z: ", &offset.z, 0.01f, -5.0f, 5.0f);

            ImGui::DragFloat("scale: ", &scale, 0.01f, -3.0f, 3.0f);

            ImGui::DragFloat("rot x: ", &rot.x, 0.01f, -30.0f, 30.0f);
            ImGui::DragFloat("rot y: ", &rot.y, 0.01f, -30.0f, 30.0f);

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
                window.renderUI(1);
                //window.renderUI(2);
            }

            weapon += window.getCursorCallBack().scrollY;
            // Draw with yaw offset (adjust the angle as needed - positive = rotate right, negative = rotate left)

            switch (weapon) {
            case -1: {
                awp.draw(shader.getProgram(), gl::getItemModelQuat(player.getCam(), offset, scale, glm::quat(1.0f, 0.0f, 0.0f, 0.0f), rot));
                break;
            }
            case 0: {
                M4A1.draw(shader.getProgram(), gl::getItemModelQuat(player.getCam(), offset, scale, glm::quat(1.0f, 0.0f, 0.0f, 0.0f), rot));
                break;
            }
            case 1: {
                usp.draw(shader.getProgram(), gl::getItemModelQuat(player.getCam(), offset, scale, glm::quat(1.0f, 0.0f, 0.0f, 0.0f), rot));
                break;
            }
            }        
            
            // Draw model in front of camera (adjust position as needed)
            model.draw(shader.getProgram(), glm::vec3(0.0f, 0.0f, -5.0f), glm::vec3(30.0f), glm::vec3(0.0f));

            window.imguiRender();

            window.swapBuffers();
        } catch (const std::exception& e) {
			std::cout << e.what() << std::endl;
		}
        if(window.getFps() > 60.0f) std::this_thread::sleep_for((std::chrono::milliseconds)5);
    }

    glfwTerminate();

    //std::cin.get();
}
