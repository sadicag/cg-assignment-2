#pragma once

// Suppress warnings in third-party code.
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/vec3.hpp>
DISABLE_WARNINGS_POP()
#include <framework/window.h>

// Define the contents of a light source!
class Light {
    public:
	glm::vec3 position;
	glm::vec3 forward;
	glm::vec3 color;
	bool isSpotlight;

	// Create directional light
	Light(glm::vec3 n_color, glm::vec3 n_position, glm::vec3 n_forward);
	// Create spotlight
	Light(glm::vec3 color, glm::vec3 position);
};

