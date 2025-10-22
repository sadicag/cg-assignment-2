//#include "Image.h"
#include "render/mesh.h"
#include "render/texture.h"
#include "ui/camera.h"
// Always include window first (because it includes glfw, which includes GL which needs to be included AFTER glew).
// Can't wait for modules to fix this stuff...
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glad/glad.h>
// Include glad before glfw3
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <imgui/imgui.h>
DISABLE_WARNINGS_POP()
#include <framework/shader.h>
#include <framework/window.h>
#include <functional>
#include <iostream>
#include <vector>

int32_t WINDOW_WIDTH = 1024;
int32_t WINDOW_HEIGHT = 1024;
const float CAMERA_FOV = glm::radians(90.0f);
float CAMERA_ASPECT_RATIO = static_cast<float>(WINDOW_WIDTH) / static_cast<float>(WINDOW_HEIGHT);

// Just for the sake of clarity, define 
// the contents of an object file (list of meshes)
// as what it is;
using ObjectFile = std::vector<GPUMesh>;

class Application {
public:
    Application()
        : m_window("Final Project", glm::ivec2(WINDOW_WIDTH, WINDOW_HEIGHT), OpenGLVersion::GL41)
        , m_texture(RESOURCE_ROOT "resources/checkerboard.png")
    {
        m_window.registerKeyCallback([this](int key, int scancode, int action, int mods) {
            if (action == GLFW_PRESS)
                onKeyPressed(key, mods);
            else if (action == GLFW_RELEASE)
                onKeyReleased(key, mods);
        });
        m_window.registerMouseMoveCallback(std::bind(&Application::onMouseMove, this, std::placeholders::_1));
        m_window.registerMouseButtonCallback([this](int button, int action, int mods) {
            if (action == GLFW_PRESS)
                onMouseClicked(button, mods);
            else if (action == GLFW_RELEASE)
                onMouseReleased(button, mods);
        });

	// Initialize the meshes
	butterfly_body_meshes = GPUMesh::loadMeshGPU(RESOURCE_ROOT "resources/butterfly-body.obj");
	butterfly_wing_meshes = GPUMesh::loadMeshGPU(RESOURCE_ROOT "resources/butterfly-wings.obj");

        try {
            ShaderBuilder defaultBuilder;
            defaultBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/shader_vert.glsl");
            defaultBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/shader_frag.glsl");
            m_defaultShader = defaultBuilder.build();

            ShaderBuilder shadowBuilder;
            shadowBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/shadow_vert.glsl");
            shadowBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "Shaders/shadow_frag.glsl");
            m_shadowShader = shadowBuilder.build();

            // Any new shaders can be added below in similar fashion.
            // ==> Don't forget to reconfigure CMake when you do!
            //     Visual Studio: PROJECT => Generate Cache for ComputerGraphics
            //     VS Code: ctrl + shift + p => CMake: Configure => enter
            // ....
        } catch (ShaderLoadingException e) {
            std::cerr << e.what() << std::endl;
        }
    }

    void setMainCamera(glm::vec3& position, glm::vec3& forward)
    { // Add a camera
	mainCamera = Camera(&m_window, position, forward);
	mainCamera.setUserInteraction(true);
    }

    void imgui()
    { // Section for user interface!
	int dummyInteger = 0; // Initialized to 0
	// Use ImGui for easy input/output of ints, floats, strings, etc...
	ImGui::Begin("Window");
	ImGui::InputInt("This is an integer input", &dummyInteger); // Use ImGui::DragInt or ImGui::DragFloat for larger range of numbers.
	ImGui::Text("Value is: %i", dummyInteger); // Use C printf formatting rules (%i is a signed integer)
	ImGui::Checkbox("Use material if no texture", &m_useMaterial);
	ImGui::End();
    }

    void render_butterfly(glm::mat3 normalModelMatrix, glm::mat4 mvpMatrix)
    { // Function to render our butterfly
	// --- BUTTERFLY BODY MESHES
	for (GPUMesh& mesh : butterfly_body_meshes)
	{
	    m_defaultShader.bind();
	    glUniformMatrix4fv(m_defaultShader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(mvpMatrix));
	    //Uncomment this line when you use the modelMatrix (or fragmentPosition)
	    //glUniformMatrix4fv(m_defaultShader.getUniformLocation("modelMatrix"), 1, GL_FALSE, glm::value_ptr(m_modelMatrix));
	    glUniformMatrix3fv(m_defaultShader.getUniformLocation("normalModelMatrix"), 1, GL_FALSE, glm::value_ptr(normalModelMatrix));
	    if (mesh.hasTextureCoords()) 
	    {
		m_texture.bind(GL_TEXTURE0);
		glUniform1i(m_defaultShader.getUniformLocation("colorMap"), 0);
		glUniform1i(m_defaultShader.getUniformLocation("hasTexCoords"), GL_TRUE);
		glUniform1i(m_defaultShader.getUniformLocation("useMaterial"), GL_FALSE);
	    }
	    else 
	    {
		glUniform1i(m_defaultShader.getUniformLocation("hasTexCoords"), GL_FALSE);
		glUniform1i(m_defaultShader.getUniformLocation("useMaterial"), m_useMaterial);
	    }
	    mesh.draw(m_defaultShader);
	}

	// --- BUTTERFLY WINGS MESHES
	for (GPUMesh& mesh : butterfly_wing_meshes)
	{
	    m_defaultShader.bind();
	    glUniformMatrix4fv(m_defaultShader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(mvpMatrix));
	    //Uncomment this line when you use the modelMatrix (or fragmentPosition)
	    //glUniformMatrix4fv(m_defaultShader.getUniformLocation("modelMatrix"), 1, GL_FALSE, glm::value_ptr(m_modelMatrix));
	    glUniformMatrix3fv(m_defaultShader.getUniformLocation("normalModelMatrix"), 1, GL_FALSE, glm::value_ptr(normalModelMatrix));
	    if (mesh.hasTextureCoords()) 
	    {
		m_texture.bind(GL_TEXTURE0);
		glUniform1i(m_defaultShader.getUniformLocation("colorMap"), 0);
		glUniform1i(m_defaultShader.getUniformLocation("hasTexCoords"), GL_TRUE);
		glUniform1i(m_defaultShader.getUniformLocation("useMaterial"), GL_FALSE);
	    }
	    else 
	    {
		glUniform1i(m_defaultShader.getUniformLocation("hasTexCoords"), GL_FALSE);
		glUniform1i(m_defaultShader.getUniformLocation("useMaterial"), m_useMaterial);
	    }
	    mesh.draw(m_defaultShader);
	}
    }

    void update()
    {
        while (!m_window.shouldClose()) {
            // This is your game loop
            // Put your real-time logic and rendering in here
            m_window.updateInput();

	    // Interact with the imgui
	    imgui();

            // Clear the screen
            glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // ...
            glEnable(GL_DEPTH_TEST);

	    // Calculate view-projection matrix setup
	    // for the main camera
	    const glm::mat4 m_projection = glm::perspective(CAMERA_FOV, CAMERA_ASPECT_RATIO, 0.1f, 30.0f);
	    const glm::mat4 mvpMatrix = m_projection * mainCamera.viewMatrix();

            //const glm::mat4 mvpMatrix = m_projectionMatrix * m_viewMatrix * m_modelMatrix;
            // Normals should be transformed differently than positions (ignoring translations + dealing with scaling):
            // https://paroj.github.io/gltut/Illumination/Tut09%20Normal%20Transformation.html
            const glm::mat3 normalModelMatrix = glm::inverseTranspose(glm::mat3(m_modelMatrix));

	    // --- Rendering section!
	    render_butterfly(normalModelMatrix, mvpMatrix);

	    // --- Update the main camera input
	    mainCamera.updateInput();

            // Processes input and swaps the window buffer
            m_window.swapBuffers();
        }
    }

    // In here you can handle key presses
    // key - Integer that corresponds to numbers in https://www.glfw.org/docs/latest/group__keys.html
    // mods - Any modifier keys pressed, like shift or control
    void onKeyPressed(int key, int mods)
    {
        std::cout << "Key pressed: " << key << std::endl;
    }

    // In here you can handle key releases
    // key - Integer that corresponds to numbers in https://www.glfw.org/docs/latest/group__keys.html
    // mods - Any modifier keys pressed, like shift or control
    void onKeyReleased(int key, int mods)
    {
        std::cout << "Key released: " << key << std::endl;
    }

    // If the mouse is moved this function will be called with the x, y screen-coordinates of the mouse
    void onMouseMove(const glm::dvec2& cursorPos)
    {
        std::cout << "Mouse at position: " << cursorPos.x << " " << cursorPos.y << std::endl;
    }

    // If one of the mouse buttons is pressed this function will be called
    // button - Integer that corresponds to numbers in https://www.glfw.org/docs/latest/group__buttons.html
    // mods - Any modifier buttons pressed
    void onMouseClicked(int button, int mods)
    {
        std::cout << "Pressed mouse button: " << button << std::endl;
    }

    // If one of the mouse buttons is released this function will be called
    // button - Integer that corresponds to numbers in https://www.glfw.org/docs/latest/group__buttons.html
    // mods - Any modifier buttons pressed
    void onMouseReleased(int button, int mods)
    {
        std::cout << "Released mouse button: " << button << std::endl;
    }

private:
    Window m_window;

    // Shader for default rendering and for depth rendering
    Shader m_defaultShader;
    Shader m_shadowShader;

    // --- Meshes of an object file!
    ObjectFile butterfly_body_meshes;
    ObjectFile butterfly_wing_meshes;

    // --- All the cameras!
    Camera mainCamera;

    Texture m_texture;
    bool m_useMaterial { true };

    // Projection and view matrices for you to fill in and use
    glm::mat4 m_projectionMatrix = glm::perspective(glm::radians(80.0f), 1.0f, 0.1f, 30.0f);
    glm::mat4 m_viewMatrix = glm::lookAt(glm::vec3(-1, 1, -1), glm::vec3(0), glm::vec3(0, 1, 0));
    glm::mat4 m_modelMatrix { 1.0f };
};

int main()
{

    glm::vec3 START_POSITION  = {12.0f, 3.0f, 0.0f};
    glm::vec3 LOOK_POSITION  = {-1.0f, 0.0f, 0.0f};

    Application app;
    app.setMainCamera(START_POSITION, LOOK_POSITION);
    app.update();

    return 0;
}
