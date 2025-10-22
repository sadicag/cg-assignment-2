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
	// Create directional light
	Light(glm::vec3 position, glm::vec3 forward);
	// Create spotlight
	Light(glm::vec3 position);
	// Get light position
	glm::vec3 getPos();
	// Get direction position
	// Returns vec3(-1) if spotlight
	glm::vec3 getFor();
	// Check if spotlight
	bool isSpot();
    private:
	glm::vec3 m_position;
	glm::vec3 m_forward;
	bool m_isSpotlight;
};

