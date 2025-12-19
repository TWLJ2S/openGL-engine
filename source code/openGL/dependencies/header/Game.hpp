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

namespace gl {

	class player {
	private:
		gl::camera m_Camera;
		glm::vec3 m_Velocity;
		std::shared_ptr<gl::shader> m_Shader;
		std::shared_ptr<gl::window> m_Window;
		glm::mat4 m_Model;
		glm::mat4 m_View;
		glm::mat4 m_Proj;

		double Yaw, Pitch;
		double thisX, thisY;

		void processInput() {
			double sens = m_Camera.getFov() / 60.0f * m_Camera.getSens();

			double lastX = thisX;
			double lastY = thisY;

			thisX = m_Window->getCursorX();
			thisY = m_Window->getCursorY();

			// Clamp Pitch to prevent gimbal lock (must be applied immediately)
			Pitch = glm::clamp(Pitch + (double)(sens * (lastY - thisY)), (double)-89.0f, (double)89.0f);
			
			// Wrap Yaw to keep it in [-180, 180) range
			Yaw = std::fmod(Yaw + (double)(sens * -(lastX - thisX)) + (double)180.0f, (double)360.0f) - (double)180.0f;

			//std::cout << "X: " << Yaw << "\nY: " << Pitch << std::endl;

			//scope in/out

			static bool ifpress = false;
			static unsigned char zoom = 0;
			static float fov = 60.0f;
			if (m_Window->getCursorCallBack().button == GLFW_MOUSE_BUTTON_RIGHT) {
				if (ifpress) {
					if (m_Window->getCursorAction() == GLFW_RELEASE) ifpress = false;
				}
				else if (m_Window->getCursorAction() == GLFW_PRESS) {
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
			if (fov != m_Camera.getFov()) setFov(glm::mix(m_Camera.getFov(), fov, 15.0 * m_Window->getDeltaTime()), (float)m_Window->getWidth() / (float)m_Window->getHeight());

			// --- UPDATE FRONT VECTOR ---
			glm::vec3 front;
			front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
			front.y = sin(glm::radians(Pitch));
			front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));

			glm::vec3 Front = (glm::length(front) > 0.0f) ? glm::normalize(front) : glm::vec3(0.0f, 0.0f, -1.0f);

			// Update camera's front vector
			m_Camera.setFront(Front);

			// --- INPUT DIRECTION ---
			glm::vec3 forward = glm::normalize(glm::vec3(Front.x, 0.0f, Front.z));

			glm::vec3 right = glm::normalize(glm::cross(Front, m_Camera.getUpVector()));

			// Use getKeyPress() to check if keys are currently held (not just callback events)
			// This allows continuous movement while keys are held down
			if (m_Window->getKeyPress(GLFW_KEY_W)) m_Velocity += forward;
			if (m_Window->getKeyPress(GLFW_KEY_S)) m_Velocity -= forward;
			if (m_Window->getKeyPress(GLFW_KEY_A)) m_Velocity -= right;
			if (m_Window->getKeyPress(GLFW_KEY_D)) m_Velocity += right;
			if (m_Window->getKeyPress(GLFW_KEY_SPACE)) m_Velocity += m_Camera.getUpVector();
			if (m_Window->getKeyPress(GLFW_KEY_LEFT_SHIFT)) m_Velocity -= m_Camera.getUpVector();
			m_Velocity *= m_Window->getKeyPress(GLFW_KEY_C) ? 0.75f : 0.92f;

			m_Camera.setPos(m_Camera.getPos() + m_Velocity * (float)m_Window->getDeltaTime());
		}

	public:
		player(const gl::camera& cam, const std::shared_ptr<gl::window>& window, const std::shared_ptr<gl::shader>& shader)
			: m_Camera(cam), m_Velocity(glm::vec3(0.0f)), m_Window(window), m_Shader(shader), m_Model(glm::mat4(1.0f)), m_View(glm::mat4(1.0f)), m_Proj(glm::mat4(1.0f))
		{
		}

		void update() {
			m_Shader->useProgram();

			// --- MOUSE LOOK ---
			processInput();

			m_Proj = glm::infinitePerspective(glm::radians(m_Camera.getFov()), (float)m_Window->getWidth() / (float)m_Window->getHeight(), 0.1f);

			m_Shader->setUniformMat4fv("model", m_Model);
			m_Shader->setUniformMat4fv("view", m_Camera.getViewMatrix());
			m_Shader->setUniformMat4fv("projection", m_Proj);
			m_Shader->setUniform3fv("camPos", getPos());
		}

		void setFov(const float& fov, const float& aspect) { m_Camera.setFov(fov); }

		const float getFov() const { return m_Camera.getFov(); }

		void setSens(const float& other) { m_Camera.setSens(other); }

		void setModel(glm::mat4& other) { m_Model = other; }

		void setView(glm::mat4& other) { m_View = other; }

		void setProj(glm::mat4& other) { m_Proj = other; }

		glm::mat4 getProj() const { return m_Proj; }

		gl::camera getCam() { return m_Camera; }

		glm::vec3 getPos() const { return m_Camera.getPos(); }

		glm::vec3 getDirRadians() const { return m_Camera.getFront(); }
	};

	glm::mat4 getItemModel(const camera& cam, const glm::vec3& offset, const glm::vec3& scale) {
		return glm::scale(
			glm::translate(
				glm::translate(glm::mat4(1.0f), cam.getPos()) *
				glm::inverse(glm::lookAt(glm::vec3(0.0f), -cam.getFront(), cam.getUpVector())),
				offset
			),
			scale
		);
	}

	glm::mat4 getItemModel(
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