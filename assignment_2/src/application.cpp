//#include "Image.h"
#include "render/mesh.h"
#include "render/texture.h"
#include "ui/camera.h"
#include "ui/light.h"
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

// 1 if DEBUG
// 0 if NOT DEBUG
bool DEBUG = false;

int32_t WINDOW_WIDTH = 1024;
int32_t WINDOW_HEIGHT = 1024;
const float CAMERA_FOV = glm::radians(60.0f);
float CAMERA_ASPECT_RATIO = static_cast<float>(WINDOW_WIDTH) / static_cast<float>(WINDOW_HEIGHT);
float DISTANCE_CLIPPING = 1000.0f;

// Just for the sake of clarity;
// Define the contents of an object 
// file (list of meshes) as what it is;
using ObjectFile = std::vector<GPUMesh>;

class Application {
public:
    Application()
        : m_window("Final Project", glm::ivec2(WINDOW_WIDTH, WINDOW_HEIGHT), OpenGLVersion::GL41)
        , m_texture(RESOURCE_ROOT "resources/checkerboard.png")
        , m_butterfly_texture(RESOURCE_ROOT "resources/wing-texture.png")
        , m_butterfly_texture0(RESOURCE_ROOT "resources/wing-texture0.png")
        , m_butterfly_body_texture(RESOURCE_ROOT "resources/body-texture.png")
    { // Initialize the application
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

	// Load the textures!
	

    }

    // --- Camera Stuff
    //⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣀⣀⣀⣀⡤⢤⣄⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
    //⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣤⡤⠔⠒⠒⠋⠉⣉⣉⣁⣠⠤⠤⠞⢿⡲⢤⣀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
    //⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⡴⠋⠉⠓⣦⣄⡀⠀⡼⠛⠿⣗⡦⣄⡀⠀⢀⣻⣀⡬⠽⢶⣄⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣀⣀⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
    //⠀⠀⠀⠀⠀⠀⠀⠀⠀⣴⠋⠀⠀⣠⢞⣿⠟⠙⠿⣗⠦⢄⡀⠉⢳⡿⠋⠉⠀⠀⠀⠀⠀⠹⡙⠦⣄⡀⠀⠀⣀⣀⣀⣀⣀⠀⠀⠀⢀⡤⢴⡾⣿⣿⣧⣄⡀⠀⠀⠀⠀⠀⠀⠀⠀
    //⠀⠀⠀⠀⠀⠀⠀⠀⠀⡇⠀⠀⢰⢫⡾⠃⠀⠀⠀⠈⠛⠦⣍⠳⣾⣀⣀⣠⠤⠴⠒⠚⠛⠋⠉⠉⠉⠻⣾⣻⣷⣿⣿⣿⣯⡿⢷⣄⣼⣿⣡⡞⠈⣏⠙⣦⣿⠦⣄⠀⠀⠀⠀⠀⠀
    //⠀⠀⠀⠀⠀⠀⠀⠀⠀⡇⠀⠀⡟⣿⠃⠀⠀⠀⠀⠀⠀⠀⠈⢻⠋⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠹⣿⡶⣿⣿⣷⡾⡿⣿⠏⠀⠈⠙⠛⠛⠛⠛⢛⣯⣤⡈⠓⢦⡀⠀⠀⠀
    //⠀⠀⠀⠀⠀⠀⠀⠀⠀⢷⡀⠀⡇⡿⠀⠀⠀⠀⠀⠀⠀⠀⠀⡾⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠹⣽⢳⣸⣷⢧⢧⣿⠀⠀⣾⣿⣿⡆⠀⠀⠙⠿⣿⣃⣤⡴⠿⣦⡀⠀
    //⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠹⠶⢾⣷⡀⠀⠀⠀⠀⠀⠀⠀⢠⡇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣀⣀⣸⣿⣧⣿⣿⣼⣿⠀⠀⣀⣩⣥⣤⣶⣾⠟⠛⠙⠻⢤⡀⠀⠘⣧⠀
    //⠀⠀⠀⠀⠀⠀⠀⠀⢀⣀⡤⠖⠊⠉⠓⢦⣄⡀⠀⠀⠀⠀⢸⠃⠀⠀⠀⢀⣀⣀⣤⠴⠶⠖⠚⠋⣉⣉⣁⣤⣶⡿⠶⠶⠾⠿⠟⠛⣿⢿⣍⠁⠘⡿⡜⢦⣀⠀⠀⢸⠿⡀⠀⠸⡆
    //⠀⠀⠀⣀⣠⡴⠖⠋⠉⠀⠀⠀⠀⠀⠀⠀⠀⠉⠓⢦⣄⠀⣾⠶⣶⣾⣯⣭⣿⠶⠶⢶⣚⣛⣛⣋⣁⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣿⠀⠈⠳⢤⡹⣏⠲⢬⣍⣋⣁⣴⠇⠀⠀⢹
    //⠀⡴⠛⠹⢷⣦⣀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣀⣠⣤⣶⣿⡿⠛⠋⣉⣯⣶⣶⠿⠟⠛⢛⣻⣭⠭⠭⠽⠶⠦⣀⠀⠀⠀⠀⠀⠀⠈⣟⠳⣄⡀⠀⠉⠛⢿⣖⡂⠐⠛⠁⠀⠀⠀⣸
    //⢸⠁⣰⠚⠙⣮⠉⣻⡶⠒⠒⠒⠒⠒⠚⠛⠛⠉⠉⢁⣏⡞⢀⣴⡾⠟⠉⢁⣠⢴⣾⣿⣿⣶⠶⠟⠛⠛⠛⢛⡛⣷⣄⠀⠀⠀⠀⠀⣿⠀⠀⠉⠓⢤⡀⠀⠀⠉⠙⠒⠒⠦⠤⠖⠉
    //⡾⠀⣿⣿⣿⣸⣷⠏⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⡼⣾⣵⠟⠋⠀⣠⠖⣭⣶⡿⠟⣋⣥⡴⣒⠚⠙⠻⣍⠉⠉⠻⣝⢷⣤⡀⠀⠀⣿⠀⠀⠀⠀⠀⠉⠓⠦⣤⣀⠀⠀⠀⠀⠀⢀
    //⡇⠀⠘⣿⣿⠉⢸⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣰⢷⡟⠁⢀⡴⠋⣰⡾⠛⣡⡴⠚⣿⣤⡀⠈⠀⠀⠀⠈⠳⣄⣀⣈⣶⡽⠿⣶⣄⣿⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⠉⠉⠛⠛⠙⡏
    //⡗⠀⠀⣿⣿⡄⢸⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣿⠏⠀⣠⠊⣠⣾⠋⣠⣞⢹⠀⠀⠙⠉⠀⠀⠀⣀⣤⠶⠒⠛⠋⠉⠁⠀⠀⠈⠛⢿⣦⣄⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢰⠁
    //⡇⠀⠀⣿⠿⠇⢸⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣼⡟⠀⡼⢃⣾⠟⣩⡟⠁⠉⠹⠆⠀⠀⠀⢀⣴⠟⠋⠁⠀⠀⠀⠀⢀⣀⣠⣴⣶⣿⣿⣿⣿⣿⣷⣤⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⡆
    //⡇⠀⠀⣿⠀⠀⢸⡇⠀⠀⠀⠀⠀⠀⠀⠀⢀⡿⠀⡸⠁⣼⠏⣰⢣⡀⠀⠀⠀⠀⠀⠀⣴⠟⠁⠀⠀⠀⠀⠀⣠⣶⣿⣿⡿⠟⠛⠉⠀⠀⠈⠙⠻⣿⣿⣦⡀⠀⠀⠀⠀⠀⠀⠀⢃
    //⣧⡀⠀⡏⠀⠀⢸⡇⠀⠀⠀⠀⠀⠀⠀⠀⣾⠇⢀⡇⣸⡏⣼⠃⠀⠙⠂⠀⠀⠀⢀⡾⠁⠀⠀⠀⠀⢀⣴⣾⣿⡿⠛⠁⠀⠀⣀⣀⣀⣀⡀⠀⠀⠈⠻⣿⣷⡀⠀⠀⠀⠀⠀⠀⢸
    //⡇⠙⢦⡇⠀⠀⠸⡇⠀⠀⠀⠀⠀⠀⠀⠀⣿⠀⢸⡇⣿⣧⣏⠀⠀⠀⠀⠀⠀⢠⣾⠃⠀⠀⠀⠀⣴⣿⡿⠟⠁⢀⣠⣴⣾⡻⢷⣄⠀⠀⠉⠳⣄⠀⠀⠹⣿⣷⡀⠀⠀⠀⠀⠀⢸
    //⡇⠀⠀⡇⠀⠀⠀⡇⠀⠀⠀⠀⠀⠀⠀⠀⣿⠀⢸⡇⣿⣿⣿⣟⡶⠀⠀⠀⠀⣿⠃⠀⠀⠀⢀⣼⣿⠟⠀⣀⣶⣿⣿⣭⠝⣷⠈⣏⢷⡀⠀⠀⠹⣧⠀⠀⢿⣿⡇⠀⠀⠀⠀⠀⠸
    //⡇⠀⠀⣧⠀⠀⠀⡇⠀⠀⠀⠀⠀⠀⠀⠀⢻⡄⠸⣇⣿⣿⡇⠉⠁⠀⠀⠀⢸⡏⠀⠀⠀⠀⣾⡿⠋⢀⣼⣿⣿⡿⠋⠀⠀⣿⠀⢸⠀⢳⠀⠀⠀⠘⡇⠀⢸⣿⡇⠀⠀⠀⠀⠀⡄
    //⡇⠀⢸⡏⠱⢄⠀⡇⠀⠀⠀⠀⠀⠀⠀⠀⠘⣧⠀⢿⣿⣿⡇⠀⠀⠀⠀⢠⣿⠁⠀⠀⠀⣸⡿⠃⠀⣾⣿⢻⡏⠀⠀⠀⢀⣿⠀⡼⠀⢸⠀⠀⠀⠀⡇⠀⢸⣿⡇⠀⠀⠀⠀⠀⡇
    //⣇⠀⠸⡇⠀⠀⠳⣇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⢦⡸⣿⣿⣿⡀⠀⠀⠀⢸⡏⠀⠀⠀⢠⣿⠃⠀⢸⣿⠇⡟⠀⠀⠀⢀⣼⠏⣠⠇⠀⡼⠀⠀⠀⢠⡇⠀⢸⣿⡇⠀⠀⠀⠀⠀⡇
    //⣿⠀⠀⡇⠀⠀⠀⢹⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠙⢿⣿⣿⣇⠀⠀⠀⣼⡇⠀⠀⠀⣼⣿⠀⠀⣿⣿⣆⡀⠀⣀⣴⡿⢋⣴⠋⢀⡜⠁⠀⠀⠀⡾⠀⢠⣿⡟⠀⠀⠀⠀⠀⢠⠁
    //⢾⣄⠀⡇⠀⠀⠀⢸⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠙⠿⣿⡆⠀⠀⢻⡁⠀⠀⠀⣿⣿⠀⠀⣟⢿⣿⣍⠉⢉⣡⣴⠟⣁⡴⠋⠀⠀⠀⠀⡼⠁⣠⣿⡟⠀⠀⠀⠀⣀⡤⠊⠀
    //⠀⠙⢶⣇⠀⠀⠀⢸⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠉⠙⠲⣼⣿⠀⠀⠀⣿⣿⠀⠀⣷⠀⠉⠛⠛⠛⠛⠒⠊⠁⠀⠀⠀⠀⢀⡜⠁⣴⣿⡿⠉⠉⠉⠉⠉⠀⠀⠀⠀
    //⠀⠀⠀⠙⢦⡀⠀⢸⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣀⣤⠽⢿⣦⡀⠀⠸⣿⡆⠀⠸⣷⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣀⡴⠋⣠⣾⣿⡟⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀
    //⠀⠀⠀⠀⠀⠙⢦⣸⠀⠀⠀⠀⠀⠀⠀⠀⢀⣀⣤⡤⠴⠒⠚⠉⠉⠀⠀⠀⠀⠈⠙⢦⡀⢻⣿⡄⠀⠻⣦⣀⠀⠀⠀⠀⢀⣠⡤⠚⢫⣣⣾⣿⡟⠋⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
    //⠀⠀⠀⠀⠀⠀⠀⠙⠒⠒⠒⠒⠒⠋⠉⠉⠉⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⠳⢿⣿⣆⡀⠀⠉⠙⠒⠲⠚⠉⣀⣠⣴⣾⡿⠟⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
    //⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠙⣻⣿⣶⣤⣤⣴⣶⣶⣿⣿⣿⠛⠉⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
    void initCamera(glm::vec3& position, glm::vec3& forward, bool isInteractive)
    { // Initialize the camera
	// Create new camera
	Camera c = Camera(&m_window, position, forward);
	// Initialize the active camera index to be zero
	camera_idx = 0;
	c.setUserInteraction(isInteractive);
	// Add camera to the list of cameras
	cameras.push_back(c);
    }

    void addCamera(glm::vec3& position, glm::vec3& forward, bool isInteractive)
    { // Add a new camera
	Camera c = Camera(&m_window, position, forward);
	c.setUserInteraction(isInteractive);
	cameras.push_back(c);
    }

    void changeCamera(uint32_t idx)
    { // Change the camera used to the idx!
	if (idx < cameras.size())
	{
	    camera_idx = idx;
	}
    }

    // --- Light Stuff
    //                   )                    `
    //                  /(                    /)
    //                 (  \                  / (
    //                 ) # )                ( , )
    //                  \#/                  \#'
    //                .-"#'-.             .-"#"=,
    //             (  |"-.='|            '|"-,-"|
    //             )\ |     |  ,        /(|     | /(         ,
    //    (       /  )|     | (\       (  \     | ) )       ((
    //    )\     (   (|     | ) )      ) , )    |/ (        ) )
    //   /  )     ) . )     |/  (     ( # (     ( , )      /   )
    //  (   (      \#/|     (`# )      `#/|     |`#/      (  '(
    //   \#/     .-"#'-.   .-"#'-,   .-"#'-.   .-=#"-;     `#/
    // .-"#'-.   |"=,-"|   |"-.-"|)  1"-.-"|   |"-.-"|   ,-"#"-.
    // |"-.-"|   |  !  |   |     |   |     |   |     !   |"-.-"|
    // |     |   |     |._,|     |   |     |._,|     a   |     |
    // |     |   |     |   |     |   |     |   |     p   |     |
    // |     |   |     |   |     |   |     |   |     x   |     |
    // '-._,-'   '-._,-'   '-._,-'   '-._,-'   '-._,-"   '-._,-'
    void addLight(Light li)
    { // Create and add a new spotlight
	lights.push_back(li);
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

    glm::mat4 update_butterflyMatrix(
	    glm::mat4 butterflyMatrix, 
	    glm::vec3 centerOffset, 
	    bool clockWise
    )
    { // Function to update the butterfly movement
	
	float angle = clockWise ? -m_flightAngle : m_flightAngle;

	float new_x = sin(angle) * m_flightRadius;
	float new_z = cos(angle) * m_flightRadius;
	float new_y = sin(angle * 2.0f) * m_swayAmplitude; // amplitude ~ 0.2f–0.5f

	// Compute translation matrix
	glm::mat4 translateMatrix = glm::translate(
	    glm::mat4(1.0f),
	    glm::vec3(new_x, new_y, new_z) + centerOffset
	);

	// Compute direction (tangent to flight circle)
	glm::vec3 direction = glm::normalize(glm::vec3(-cos(angle), 0.0f, sin(angle)));

	// Compute right and up vectors
	glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::vec3 right = glm::normalize(glm::cross(up, direction));
	up = glm::cross(direction, right);

	// Build rotation matrix so butterfly faces flight direction
	glm::mat4 rotationMatrix(1.0f);
	rotationMatrix[0] = glm::vec4(right, 0.0f);
	rotationMatrix[1] = glm::vec4(up, 0.0f);
	rotationMatrix[2] = glm::vec4(direction, 0.0f);

	// Combine translation and rotation
	return (translateMatrix * rotationMatrix);
    }

    void render_butterfly(
	    glm::mat3 normalModelMatrix, 
	    glm::mat4 mvpMatrix, 
	    Light li, 
	    glm::mat4 butterflyMatrix,
	    bool isBlue
    )
    { // Function to render our butterfly for the current light

	// --- Butterfly Body Calculation
	glm::mat4 bodyModelMatrix = butterflyMatrix;
	glm::mat4 bodyMvpMatrix = mvpMatrix * bodyModelMatrix; 
	glm::mat3 bodyNormalMatrix = glm::inverseTranspose(glm::mat3(bodyModelMatrix));

	m_defaultShader.bind();
	glUniform3fv(m_defaultShader.getUniformLocation("cameraPosition"), 1, glm::value_ptr(cameras[camera_idx].cameraPos()));
	glUniform1f(m_defaultShader.getUniformLocation("metallic"), 0.2f);
	glUniform1f(m_defaultShader.getUniformLocation("roughness"), 0.5f);

	//⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⢔⣶⠀⠀
	//⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⡼⠗⡿⣾⠀⠀
	//⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⡼⠓⡞⢩⣯⡀⠀
	//⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣀⣀⣀⣀⠀⠀⠀⠀⠀⠀⠀⠰⡹⠁⢰⠃⣩⣿⡇⠀
	//⠀⠀⠀⠀⠀⠀⠀⠀⠀⠘⢷⣿⠿⣉⣩⠛⠲⢶⡠⢄⠐⣣⠃⣰⠗⠋⢀⣯⠁⠀
	//⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠙⣯⣠⠬⠦⢤⣀⠈⠓⢽⣾⢔⣡⡴⠞⠻⠙⢳⡄
	//⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠘⣵⣳⠖⠉⠉⢉⣩⣵⣿⣿⣒⢤⣴⠤⠽⣬⡇
	//⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⠙⢻⣟⠟⠋⢡⡎⢿⢿⠳⡕⢤⡉⡷⡽⠁
	//⣧⢮⢭⠛⢲⣦⣀⠀⠀⠀⠠⡀⠀⠀⠀⡾⣥⣏⣖⡟⠸⢺⠀⠀⠈⠙⠋⠁⠀⠀
	//⠈⠻⣶⡛⠲⣄⠀⠙⠢⣀⠀⢇⠀⠀⠀⠘⠿⣯⣮⢦⠶⠃⠀⠀⠀⠀⠀⠀⠀⠀
	//⠀⠀⢻⣿⣥⡬⠽⠶⠤⣌⣣⣼⡔⠊⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
	//⠀⠀⢠⣿⣧⣤⡴⢤⡴⣶⣿⣟⢯⡙⠒⠤⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
	//⠀⠀⠘⣗⣞⣢⡟⢋⢜⣿⠛⡿⡄⢻⡮⣄⠈⠳⢦⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
	//⠀⠀⠀⠈⠻⠮⠴⠵⢋⣇⡇⣷⢳⡀⢱⡈⢋⠛⣄⣹⣲⡀⠀⠀⠀⠀⠀⠀⠀⠀
	//⠀⠀⠀⠀⠀⠀⠀⠀⠈⢿⣱⡇⣦⢾⣾⠿⠟⠿⠷⠷⣻⠧⠀⠀⠀⠀⠀⠀⠀⠀
	//⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⠙⠻⠽⠞⠊⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
	
	// --- RENDER BUTTERFLY BODY MESHES
	for (GPUMesh& mesh : butterfly_body_meshes)
	{
	    m_defaultShader.bind();

	    // Light properties
	    glUniform3fv(m_defaultShader.getUniformLocation("lightPosition"), 1, glm::value_ptr(li.getPos()));
	    glUniform3fv(m_defaultShader.getUniformLocation("lightDirection_optional"), 1, glm::value_ptr(li.getFor()));
	    glUniform3fv(m_defaultShader.getUniformLocation("lightColor"), 1, glm::value_ptr(li.getCol()));
	    glUniform1i(m_defaultShader.getUniformLocation("isSpot"), li.isSpot());

	    // Bind the butterfly texture!
	    m_butterfly_body_texture.bind(GL_TEXTURE2);
	    glUniform1i(m_defaultShader.getUniformLocation("textureMap"), 2);

	    glUniformMatrix4fv(m_defaultShader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(bodyMvpMatrix));
	    glUniformMatrix4fv(m_defaultShader.getUniformLocation("modelMatrix"), 1, GL_FALSE, glm::value_ptr(bodyModelMatrix));
	    glUniformMatrix3fv(m_defaultShader.getUniformLocation("normalModelMatrix"), 1, GL_FALSE, glm::value_ptr(bodyNormalMatrix));
	    if (mesh.hasTextureCoords()) 
	    {
		m_texture.bind(GL_TEXTURE0);
		glUniform1i(m_defaultShader.getUniformLocation("colorMap"), 0);
		glUniform1i(m_defaultShader.getUniformLocation("useMaterial"), GL_FALSE);
	    }
	    else 
	    {
		glUniform1i(m_defaultShader.getUniformLocation("useMaterial"), m_useMaterial);
	    }
	    mesh.draw(m_defaultShader);

	}

	// --- RENDER BUTTERFLY WINGS MESHES

	glm::mat4 leftWingModelMatrix = glm::rotate(butterflyMatrix, m_flapAngle, glm::vec3(0.0f, 0.0f, 1.0f));
	glm::mat4 leftWingMvpMatrix = mvpMatrix * leftWingModelMatrix; 
	glm::mat3 leftWingNormalMatrix = glm::inverseTranspose(glm::mat3(leftWingModelMatrix));

	for (GPUMesh& mesh : butterfly_wing_meshes)
	{
	    m_defaultShader.bind();

	    // Light properties
	    glUniform3fv(m_defaultShader.getUniformLocation("lightPosition"), 1, glm::value_ptr(li.getPos()));
	    glUniform3fv(m_defaultShader.getUniformLocation("lightDirection_optional"), 1, glm::value_ptr(li.getFor()));
	    glUniform3fv(m_defaultShader.getUniformLocation("lightColor"), 1, glm::value_ptr(li.getCol()));
	    glUniform1i(m_defaultShader.getUniformLocation("isSpot"), li.isSpot());

	    // Bind the butterfly texture!
	    if (isBlue)
	    {
		m_butterfly_texture.bind(GL_TEXTURE1);
	    }
	    else
	    {
		m_butterfly_texture0.bind(GL_TEXTURE1);
	    }
	    glUniform1i(m_defaultShader.getUniformLocation("textureMap"), 1);
	    
	    glUniformMatrix4fv(m_defaultShader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(leftWingMvpMatrix));
	    glUniformMatrix4fv(m_defaultShader.getUniformLocation("modelMatrix"), 1, GL_FALSE, glm::value_ptr(leftWingModelMatrix));
	    glUniformMatrix3fv(m_defaultShader.getUniformLocation("normalModelMatrix"), 1, GL_FALSE, glm::value_ptr(leftWingNormalMatrix));
	    if (mesh.hasTextureCoords()) 
	    {
		m_texture.bind(GL_TEXTURE0);
		glUniform1i(m_defaultShader.getUniformLocation("colorMap"), 0);
		glUniform1i(m_defaultShader.getUniformLocation("useMaterial"), GL_FALSE);
	    }
	    else 
	    {
		glUniform1i(m_defaultShader.getUniformLocation("useMaterial"), m_useMaterial);
	    }
	    mesh.draw(m_defaultShader);
	}
    
	glm::mat4 rightWingModelMatrix = glm::translate(glm::rotate(butterflyMatrix, glm::radians(104.0f) - m_flapAngle, glm::vec3(0.0f, 0.0f, 1.0f)),glm::vec3(0.3f, 0.1f, 0.0f));
	glm::mat4 rightWingMvpMatrix = mvpMatrix * rightWingModelMatrix;
	glm::mat3 rightWingNormalMatrix = glm::inverseTranspose(glm::mat3(rightWingModelMatrix));
	
	for (GPUMesh& mesh : butterfly_wing_meshes)
	{
	    m_defaultShader.bind(); 

	    // Light properties
	    glUniform3fv(m_defaultShader.getUniformLocation("lightPosition"), 1, glm::value_ptr(li.getPos()));
	    glUniform3fv(m_defaultShader.getUniformLocation("lightDirection_optional"), 1, glm::value_ptr(li.getFor()));
	    glUniform3fv(m_defaultShader.getUniformLocation("lightColor"), 1, glm::value_ptr(li.getCol()));
	    glUniform1i(m_defaultShader.getUniformLocation("isSpot"), li.isSpot());

	    // Bind the butterfly texture!
	    if (isBlue)
	    {
		m_butterfly_texture.bind(GL_TEXTURE1);
	    }
	    else
	    {
		m_butterfly_texture0.bind(GL_TEXTURE1);
	    }
	    glUniform1i(m_defaultShader.getUniformLocation("textureMap"), 1);

	    // Send NEW matrices for the second wing
	    glUniformMatrix4fv(m_defaultShader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(rightWingMvpMatrix)); 
	    glUniformMatrix4fv(m_defaultShader.getUniformLocation("modelMatrix"), 1, GL_FALSE, glm::value_ptr(rightWingModelMatrix)); 
	    glUniformMatrix3fv(m_defaultShader.getUniformLocation("normalModelMatrix"), 1, GL_FALSE, glm::value_ptr(rightWingNormalMatrix)); 

	    if (mesh.hasTextureCoords())
	    {
		m_texture.bind(GL_TEXTURE0);
		glUniform1i(m_defaultShader.getUniformLocation("colorMap"), 0);
		glUniform1i(m_defaultShader.getUniformLocation("useMaterial"), GL_FALSE);
	    }
	    else
	    {
		glUniform1i(m_defaultShader.getUniformLocation("useMaterial"), m_useMaterial);
	    }
	    mesh.draw(m_defaultShader);
	}  
    }

    void startLoop()
    {
        while (!m_window.shouldClose()) {
            // This is your game loop
            // Put your real-time logic and rendering in here
            m_window.updateInput();

	    // Interact with the imgui
	    imgui();

	    float time = (float)glfwGetTime();
	    float flapSpeed = 10.0f;
	    float flapAmplitude = glm::radians(45.0f);
	    m_flapAngle = flapAmplitude * (sin(time * flapSpeed) - 0.4f * sin(time * flapSpeed * 2.0f));
	    m_flightAngle = m_flightAngle + m_flightSpeed;

            // Clear the screen
            glClearColor(0.2f, 0.3f, 0.4f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // ...
            glEnable(GL_DEPTH_TEST);

	    // Calculate view-projection matrix setup
	    // for the main camera
	    const glm::mat4 m_projection = glm::perspective(CAMERA_FOV, CAMERA_ASPECT_RATIO, 0.1f, DISTANCE_CLIPPING);
	    const glm::mat4 mvpMatrix = m_projection * (cameras[camera_idx]).viewMatrix();

            //const glm::mat4 mvpMatrix = m_projectionMatrix * m_viewMatrix * m_butterflyMatrix;
            // Normals should be transformed differently than positions (ignoring translations + dealing with scaling):
            // https://paroj.github.io/gltut/Illumination/Tut09%20Normal%20Transformation.html
            const glm::mat3 normalModelMatrix0 = glm::inverseTranspose(glm::mat3(m_butterflyMatrix0));
            const glm::mat3 normalModelMatrix1 = glm::inverseTranspose(glm::mat3(m_butterflyMatrix1));
	    m_butterflyMatrix0 = update_butterflyMatrix(m_butterflyMatrix0, glm::vec3(-5.0f, 0.0f, 1.0f), true); 
	    m_butterflyMatrix1 = update_butterflyMatrix(m_butterflyMatrix1, glm::vec3(3.0f, -2.0f, 30.0f), false); 

	    // --- Rendering section!
	    for (Light li : lights)
	    { // For each light
		render_butterfly(normalModelMatrix0, mvpMatrix, li, m_butterflyMatrix0, true);
		render_butterfly(normalModelMatrix1, mvpMatrix, li, m_butterflyMatrix1, false);
	    }

	    // --- Update the main camera input
	    (cameras[camera_idx]).updateInput();

	    if (DEBUG)
	    {
		std::cout << "Position x: ";
		std::cout << cameras[camera_idx].cameraPos().x << " y:";
		std::cout << cameras[camera_idx].cameraPos().y << " z:";
		std::cout << cameras[camera_idx].cameraPos().z;
		std::cout << std::endl;

		cameras[camera_idx].updateInput();
		std::cout << "Forward x: ";
		std::cout << cameras[camera_idx].cameraFor().x << " y:";
		std::cout << cameras[camera_idx].cameraFor().y << " z:";
		std::cout << cameras[camera_idx].cameraFor().z;
		std::cout << std::endl;
	    }
		
            // Processes input and swaps the window buffer
            m_window.swapBuffers();
        }
    }

    // In here you can handle key presses
    // key - Integer that corresponds to numbers in https://www.glfw.org/docs/latest/group__keys.html
    // mods - Any modifier keys pressed, like shift or control
    void onKeyPressed(int key, int mods)
    {
	if (DEBUG)
	    std::cout << "Key pressed: " << key << std::endl;
    }

    // In here you can handle key releases
    // key - Integer that corresponds to numbers in https://www.glfw.org/docs/latest/group__keys.html
    // mods - Any modifier keys pressed, like shift or control
    void onKeyReleased(int key, int mods)
    {

	// Change camera to index 1
	if (key == GLFW_KEY_1)
	    changeCamera(0);

	// Change camera to index 2
	if (key == GLFW_KEY_2)
	    changeCamera(1);

	// Change camera to index 3
	if (key == GLFW_KEY_3)
	    changeCamera(2);

	// Change camera to index 4
	if (key == GLFW_KEY_4)
	    changeCamera(3);

	// Change camera to index 5
	if (key == GLFW_KEY_5)
	    changeCamera(4);

	// Change camera to index 6
	if (key == GLFW_KEY_6)
	    changeCamera(5);

	// Change camera to index 7
	if (key == GLFW_KEY_7)
	    changeCamera(6);

	// Change camera to index 8

	if (key == GLFW_KEY_8)
	    changeCamera(7);

	// Change camera to index 9
	if (key == GLFW_KEY_9)
	    changeCamera(8);

	if (key == GLFW_KEY_0)	
	{ // Toggle the debug prints when pressing 0
	    if (DEBUG)
	    {
		DEBUG = false;
	    }
	    else if (!DEBUG)
	    {
		DEBUG = true;
	    }
	}

	if (DEBUG)
	    std::cout << "Key released: " << key << std::endl;
    }

    // If the mouse is moved this function will be called with the x, y screen-coordinates of the mouse
    void onMouseMove(const glm::dvec2& cursorPos)
    {
	if (DEBUG)
	    std::cout << "Mouse at position: " << cursorPos.x << " " << cursorPos.y << std::endl;
    }

    // If one of the mouse buttons is pressed this function will be called
    // button - Integer that corresponds to numbers in https://www.glfw.org/docs/latest/group__buttons.html
    // mods - Any modifier buttons pressed
    void onMouseClicked(int button, int mods)
    {
	if (DEBUG)
	    std::cout << "Pressed mouse button: " << button << std::endl;
    }

    // If one of the mouse buttons is released this function will be called
    // button - Integer that corresponds to numbers in https://www.glfw.org/docs/latest/group__buttons.html
    // mods - Any modifier buttons pressed
    void onMouseReleased(int button, int mods)
    {
	if (DEBUG)
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
    std::vector<Camera> cameras;
    uint32_t camera_idx;

    // --- All the lights!
    std::vector<Light> lights;

    // --- All the textures!
    Texture m_butterfly_texture;
    Texture m_butterfly_texture0;
    Texture m_butterfly_body_texture;
    Texture m_texture;
    bool m_useMaterial { true };

    // Projection and view matrices for you to fill in and use
    glm::mat4 m_butterflyMatrix0 { 1.0f };
    glm::mat4 m_butterflyMatrix1 { 1.0f };

    // --- All the animation stuff!
    float m_flightRadius = 50.0f;
    float m_swayAmplitude = 5.0f;
    float m_flightSpeed{ 0.025f };
    float m_flightAngle{ 0.0f };
    float m_flapAngle{ 0.0f }; //to make the wings flap!!
};

int main()
{
    // --- SET CAMERAS
    // Position and Forward for the first camera
    glm::vec3 pos0  = {-86.67f, -0.026f, -93.40f};
    glm::vec3 for0  = {0.678f, -0.143f, 0.720f};

    // Position and Forward for the second camera
    glm::vec3 pos1  = {-9.15f, 6.21f, -10.1f};
    glm::vec3 for1 = {0.75f, -0.2f, 0.6f};

    // Position and Forward for the third camera
    glm::vec3 pos2  = {9.84f, 2.75f, -10.66f};
    glm::vec3 for2  = {-0.67f, 0.10f, 0.73f};

    // --- SET LIGHTS
    // Position and Forward for the first light
    glm::vec3 colL0 = {1.0f, 1.0f, 1.0f};
    glm::vec3 posL0 = {0.0f, 5.8f, 0.0f};
    //glm::vec3 forL0 = {-1.0f, -1.0f, -1.0f};
    Light li0 = Light(colL0, posL0);

    glm::vec3 colL1 = {1.0f, 1.0f, 1.0f};
    glm::vec3 posL1 = {-2.0f, 10.0f, 4.0f};
    //glm::vec3 forL0 = {-1.0f, -1.0f, -1.0f};
    Light li1 = Light(colL1, posL1);

    // --- Create the app
    Application app;

    // --- Add the cameras
    // Initialize first camera
    app.initCamera(pos0, for0, false);
    // Add second camera
    app.addCamera(pos1, for1, true);
    // Add third camera
    app.addCamera(pos2, for2, true);
    // Add fourth camera
    app.addCamera(pos0, for0, true);

    // --- Add the lights
    app.addLight(li0);
    app.addLight(li1);

    // App start
    app.startLoop();

    return 0;
}
