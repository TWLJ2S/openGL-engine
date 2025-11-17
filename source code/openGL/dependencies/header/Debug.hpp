#pragma once

#define GLM_ENABLE_EXPERIMENTAL

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <gtx/quaternion.hpp>
#include <gtc/quaternion.hpp>
#include <gtx/euler_angles.hpp>

#include <iostream>

namespace db {

	void cmdOutputVec3(const glm::vec3& vec3) {
		std::cout << vec3.x << ' ' << vec3.y << ' ' << vec3.z << std::endl;
	}

	void cmdOutputVec2(const glm::vec2& vec2) {
		std::cout << vec2.x << ' ' << vec2.y << std::endl;
	}
}