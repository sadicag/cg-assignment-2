#include "camera.h"
// Suppress warnings in third-party code.
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
DISABLE_WARNINGS_POP()

Camera::Camera()
    : Camera(NULL, glm::vec3(0), glm::vec3(0, 0, -1))
{
}

Camera::Camera(Window* pWindow)
    : Camera(pWindow, glm::vec3(0), glm::vec3(0, 0, -1))
{
}

Camera::Camera(Window* pWindow, const glm::vec3& pos, const glm::vec3& forward)
    : m_position(pos)
    , m_forward(glm::normalize(forward))
    , m_pWindow(pWindow)
{
}

void Camera::setUserInteraction(bool enabled)
{
    m_userInteraction = enabled;
}

glm::vec3 Camera::cameraPos() const
{
    return m_position;
}

glm::vec3 Camera::cameraFor() const
{
    return m_forward;
}

glm::mat4 Camera::viewMatrix() const
{
    return glm::lookAt(m_position, m_position + m_forward, m_up);
}

void Camera::rotateX(float angle)
{
    const glm::vec3 horAxis = glm::cross(s_yAxis, m_forward);

    m_forward = glm::normalize(glm::angleAxis(angle, horAxis) * m_forward);
    m_up = glm::normalize(glm::cross(m_forward, horAxis));
}

void Camera::rotateY(float angle)
{
    const glm::vec3 horAxis = glm::cross(s_yAxis, m_forward);

    m_forward = glm::normalize(glm::angleAxis(angle, s_yAxis) * m_forward);
    m_up = glm::normalize(glm::cross(m_forward, horAxis));
}

void Camera::updateInput()
{
    float moveSpeed = 0.3f;
    float lookSpeed = 0.003f;

    if (m_userInteraction) {
        glm::vec3 localMoveDelta { 0 };
        const glm::vec3 right = glm::normalize(glm::cross(m_forward, m_up));

	// Shift faster!
        if (m_pWindow->isKeyPressed(GLFW_KEY_LEFT_SHIFT))
	{
	    moveSpeed = 3.0f;
	}
	else
	{
	    moveSpeed = 0.3f;
	}

        if (m_pWindow->isKeyPressed(GLFW_KEY_A))
            m_position -= moveSpeed * right;
        if (m_pWindow->isKeyPressed(GLFW_KEY_D))
            m_position += moveSpeed * right;
        if (m_pWindow->isKeyPressed(GLFW_KEY_W))
            m_position += moveSpeed * m_forward;
        if (m_pWindow->isKeyPressed(GLFW_KEY_S))
            m_position -= moveSpeed * m_forward;
        if (m_pWindow->isKeyPressed(GLFW_KEY_UP))
            m_position += moveSpeed * m_up;
        if (m_pWindow->isKeyPressed(GLFW_KEY_DOWN))
            m_position -= moveSpeed * m_up;

        const glm::dvec2 cursorPos = m_pWindow->getCursorPos();
        const glm::vec2 delta = lookSpeed * glm::vec2(m_prevCursorPos - cursorPos);
        m_prevCursorPos = cursorPos;

        if (m_pWindow->isMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT)) {
            if (delta.x != 0.0f)
                rotateY(delta.x);
            if (delta.y != 0.0f)
                rotateX(delta.y);
        }
    } else {
        m_prevCursorPos = m_pWindow->getCursorPos();
    }
}
