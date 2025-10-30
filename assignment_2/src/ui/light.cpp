#include "light.h"

// Suppress warnings in third-party code.
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/vec3.hpp>
DISABLE_WARNINGS_POP()
#include <framework/window.h>

// Define the contents of a light source!
Light::Light(glm::vec3 n_color, glm::vec3 n_position, glm::vec3 n_forward)
{ // Initialize light source (NOT SPOTLIGHT)
    isSpotlight = false;
    position = n_position;
    forward = n_forward;
    color = n_color;
}

Light::Light(glm::vec3 n_color, glm::vec3 n_position)
{ // Initialize light source (SPOTLIGHT)
    isSpotlight = true;
    position = n_position;
    forward = glm::vec3(-1);
    color = n_color;
}
