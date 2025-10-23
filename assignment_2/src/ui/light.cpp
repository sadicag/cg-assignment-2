#include "light.h"

// Suppress warnings in third-party code.
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/vec3.hpp>
DISABLE_WARNINGS_POP()
#include <framework/window.h>

// Define the contents of a light source!
Light::Light(glm::vec3 color, glm::vec3 position, glm::vec3 forward)
{ // Initialize light source (NOT SPOTLIGHT)
    m_isSpotlight = false;
    m_position = position;
    m_forward = forward;
    m_color = color;
}

Light::Light(glm::vec3 color, glm::vec3 position)
{ // Initialize light source (SPOTLIGHT)
    m_isSpotlight = true;
    m_position = position;
    m_forward = glm::vec3(-1);
    m_color = color;
}

glm::vec3 Light::getPos()
{ // Get Light Position
    return m_position;
}

glm::vec3 Light::getFor()
{ // Get Light Forward
    return m_forward;
}

glm::vec3 Light::getCol()
{ // Get Light Color
    return m_color;
}

bool Light::isSpot()
{ // Get Light Position
    return m_isSpotlight;
}

