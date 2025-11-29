#pragma once

#define GLM_ENABLE_EXPERIMENTAL

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <gtx/quaternion.hpp>
#include <gtc/quaternion.hpp>
#include <gtx/euler_angles.hpp>

#include <cmath>

#include <Window.hpp>
#include <Utils.hpp>

#ifndef GL_MAX_VIEW_DISTANCE

#define GL_MAX_VIEW_DISTANCE 10000.0f

#endif

namespace gl {

	class player {
	private:
		gl::camera m_Camera;
		glm::vec3 m_Velocity;
		gl::uniform m_Model;
		gl::uniform m_View;
		gl::uniform m_Proj;

		float Yaw, Pitch;
		double thisX, thisY;

		void processInput(gl::window& window) {
			double sens = m_Camera.getFov() / 60 * m_Camera.getSens();

			double lastX = thisX;
			double lastY = thisY;

			thisX = window.getCursorX();
			thisY = window.getCursorY();

			// Skip first frame to avoid huge delta from initialization

			double deltaX = lastX - thisX;
			double deltaY = lastY - thisY;

			// Limit delta to prevent huge jumps (e.g., from window focus changes)
			deltaX = glm::clamp(deltaX, (double)-100.0f, (double)100.0f);
			deltaY = glm::clamp(deltaY, (double)-100.0f, (double)100.0f);

			Yaw += sens * -deltaX;
			Pitch += sens * deltaY;

			// Clamp Pitch to prevent gimbal lock (must be applied immediately)
			Pitch = glm::clamp(Pitch, -89.99f, 89.99f);
			
			// Wrap Yaw to keep it in [-180, 180) range
			Yaw = std::fmod(Yaw + 180.0f, 360.0f) - 180.0f;

			//std::cout << "X: " << Yaw << "\nY: " << Pitch << std::endl;

			//scope in/out

			static bool ifpress = false;
			static unsigned char zoom = 0;
			static float fov = 60.0f;
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

					switch (zoom) {
					case 0: {
						fov = 60.0f;
						break;
					}
					case 1: {
						fov = 35.0f;
						break;
					}
					case 2: {
						fov = 20.0f;
						break;
					}
					default: {
						fov = 60.0f;
						zoom = 0;
						break;
					}
					}
				}
			}

			// update the projection matrix
			if (fov != getFov()) setFov(glm::mix(getFov(), fov, 15.0f * window.getDeltaTime()), (float)window.getWidth() / (float)window.getHeight());

			// --- UPDATE FRONT VECTOR ---
			glm::vec3 front;
			front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
			front.y = sin(glm::radians(Pitch));
			front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));

			glm::vec3 Front = (glm::length(front) > 0.0f)
				? glm::normalize(front)
				: glm::vec3(0.0f, 0.0f, -1.0f);

			// Update camera's front vector
			m_Camera.setFront(Front);

			// --- INPUT DIRECTION ---
			glm::vec3 forward = glm::normalize(glm::vec3(Front.x, 0.0f, Front.z));

			glm::vec3 right = glm::normalize(glm::cross(Front, m_Camera.getUpVector()));

			float friction = 0.85f; // Friction coefficient

			// Use getKeyPress() to check if keys are currently held (not just callback events)
			// This allows continuous movement while keys are held down
			if (window.getKeyPress(GLFW_KEY_W)) m_Velocity += forward;
			if (window.getKeyPress(GLFW_KEY_S)) m_Velocity -= forward;
			if (window.getKeyPress(GLFW_KEY_A)) m_Velocity -= right;
			if (window.getKeyPress(GLFW_KEY_D)) m_Velocity += right;
			if (window.getKeyPress(GLFW_KEY_SPACE)) m_Velocity += m_Camera.getUpVector();
			if (window.getKeyPress(GLFW_KEY_LEFT_SHIFT)) m_Velocity -= m_Camera.getUpVector();
			if (window.getKeyPress(GLFW_KEY_C)) friction = 0.6f;
			
			// Apply friction to slow down velocity
			m_Velocity *= friction;

			m_Camera.setPos(m_Camera.getPos() + (m_Velocity * window.getDeltaTime()));

			//std::cout << "X: " << m_Camera.getPos().x << "\nY: " << m_Camera.getPos().y << "\nZ: " << m_Camera.getPos().z << std::endl;
		}

	public:
		player(gl::camera cam) 
			: m_Camera(cam), m_Velocity(glm::vec3(0.0f)), Yaw(0.0f), Pitch(0.0f), thisX(0.0f), thisY(0.0f)
		{
			m_Model = gl::uniform(glm::mat4(1.0f), glGetUniformLocation(m_Camera.getShader(), "model"));
			m_View = gl::uniform(glm::mat4(1.0f), glGetUniformLocation(m_Camera.getShader(), "view"));
			m_Proj = gl::uniform(glm::mat4(1.0f), glGetUniformLocation(m_Camera.getShader(), "projection"));
		}

		player(gl::camera cam, gl::shader shader)
			: m_Camera(cam), m_Velocity(glm::vec3(0.0f)), Yaw(0.0f), Pitch(0.0f), thisX(0.0f), thisY(0.0f)
		{
			m_Model = gl::uniform(glm::mat4(1.0f), shader.getUniformLocation("model"));
			m_View = gl::uniform(glm::mat4(1.0f), shader.getUniformLocation("view"));
			m_Proj = gl::uniform(glm::mat4(1.0f), shader.getUniformLocation("projection"));
		}

		player(gl::camera cam, GLuint modelLoc, GLuint viewLoc, GLuint projLoc)
			: m_Camera(cam), m_Velocity(glm::vec3(0.0f)), Yaw(0.0f), Pitch(0.0f), thisX(0.0f), thisY(0.0f)
		{
			m_Model = gl::uniform(glm::mat4(1.0f), modelLoc);
			m_View = gl::uniform(glm::mat4(1.0f), viewLoc);
			m_Proj = gl::uniform(glm::mat4(1.0f), projLoc);
		}

		void update(gl::window& window, gl::shader shader) {
			shader.useProgram();

			// --- MOUSE LOOK ---
			processInput(window);

			m_View = m_Camera.getViewMatrix();
			m_Proj = glm::infinitePerspective(glm::radians(m_Camera.getFov()), (float)window.getWidth() / (float)window.getHeight(), 0.1f);

			m_Model.uniformMatrix4fv();
			m_View.uniformMatrix4fv();
			m_Proj.uniformMatrix4fv();

			GLint camPosLoc = glGetUniformLocation(shader.getProgram(), "camPos");
			if (camPosLoc >= 0) {
				glUniform3fv(camPosLoc, 1, glm::value_ptr(getPos()));
			}
		}

		void setFov(const float& fov, const float& aspect) { m_Camera.setFov(fov); }

		const float getFov() const { return m_Camera.getFov(); }

		void setSens(const float& other) { m_Camera.setSens(other); }

		void setModel(glm::mat4& other) { m_Model = other; }

		void setView(glm::mat4& other) { m_View = other; }

		void setProj(glm::mat4& other) { m_Proj = other; }

		glm::mat4 getProj() const { return m_Proj.getValue(); }

		gl::camera getCam() { return m_Camera; }

		glm::vec3 getPos() const { return m_Camera.getPos(); }

		glm::vec3 getDirRadians() const { return m_Camera.getFront(); }
	};

	glm::mat4 getItemModelMat4(const camera& cam, const glm::vec3& offset, const glm::vec3& scale) {
		return glm::scale(
			glm::translate(
				glm::translate(glm::mat4(1.0f), cam.getPos()) *
				glm::inverse(glm::lookAt(glm::vec3(0.0f), -cam.getFront(), cam.getUpVector())),
				offset
			),
			scale
		);
	}

	glm::mat4 getItemModelQuat(
		const gl::camera& cam,
		const glm::vec3& offset,
		const float& scale,
		const glm::quat& additionalRotation = glm::identity<glm::quat>(),
		const glm::vec2& rot = glm::vec2(0.0f)
	) {

		return glm::scale(
			glm::translate(glm::mat4(1), cam.getPos()) *
			glm::toMat4(
				glm::quat_cast(glm::mat3(glm::inverse(glm::lookAt(glm::vec3(0), -cam.getFront(), cam.getUpVector())))) *
				glm::angleAxis(glm::radians(rot.x), glm::vec3(0, 1, 0)) *
				glm::angleAxis(glm::radians(rot.y), glm::vec3(1, 0, 0)) *
				additionalRotation
			) *
			glm::translate(glm::mat4(1), offset),
			glm::vec3(scale)
		);
	}

}