#include <Game.hpp>
#include <window.hpp>
#include <Utils.hpp>
#include <Mesh.hpp>

#include <Debug.hpp>

#include <iostream>
#include <thread>
#include <chrono>

#define WEAPON_OFFSET glm::vec3(-0.2f, -0.35f, 0.4f)

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

    unsigned char zoom = 0;
    bool ifpress = false;
    float fov = 60.0f;

    while (window.run()) {
        // Clear screen
        window.clearColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)); // Dark blue-gray background

        if (window.getCursorCallBack().button == GLFW_MOUSE_BUTTON_RIGHT) {
            if (ifpress) {
                if (window.getCursorAction() == GLFW_RELEASE) ifpress = false;
            }
            else if (window.getCursorAction() == GLFW_PRESS) {
                ifpress = true;

                zoom = (zoom + 1) % 3;

                if (zoom == 0) fov = 60.0f;
                else if (zoom == 1) fov = 35.0f;
                else if (zoom == 2) fov = 20.0f;                
            }
        }

        // update the projection matrix
        if(fov != player.getFov()) player.setFov(glm::mix(player.getFov(), fov, 15.0f * window.getDeltaTime()), (float)window.getWidth() / (float)window.getHeight());

        player.update(window, shader);
        
        // Set camera position for fragment shader
        GLint camPosLoc = glGetUniformLocation(shader.getProgram(), "camPos");
        if (camPosLoc >= 0) {
            glUniform3fv(camPosLoc, 1, glm::value_ptr(player.getPos()));
        }
        
        // Draw with yaw offset (adjust the angle as needed - positive = rotate right, negative = rotate left)
        awp.draw(shader.getProgram(), gl::getItemModelQuat(player.getCam(), WEAPON_OFFSET, glm::vec3(1.5f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::vec2(-20.0f, 15.0f)));

        // Draw model in front of camera (adjust position as needed)
        model.draw(shader.getProgram(), glm::vec3(0.0f, 0.0f, -5.0f), glm::vec3(1.0f), glm::vec3(0.0f));

        window.swapBuffers();

        std::this_thread::sleep_for(std::chrono::milliseconds(6));
    }

    glfwTerminate();

    //std::cin.get();
}
