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

// Just for the sake of clarity;
// Define the contents of an object 
// file (list of meshes) as what it is;
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

    void render_butterfly(glm::mat3 normalModelMatrix, glm::mat4 mvpMatrix, Light li)
    { // Function to render our butterfly for the current light
        //mvpMatrix = glm::scale(mvpMatrix, glm::vec3(0.4, 0.4, 0.4));       to control the size of the butterfly
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

	    glUniformMatrix4fv(m_defaultShader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(mvpMatrix));
	    //Uncomment this line when you use the modelMatrix (or fragmentPosition)
	    glUniformMatrix4fv(m_defaultShader.getUniformLocation("modelMatrix"), 1, GL_FALSE, glm::value_ptr(m_modelMatrix));
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

	// --- RENDER BUTTERFLY WINGS MESHES

    glm::mat4 leftWingMvpMatrix = glm::rotate(mvpMatrix, m_flapAngle, glm::vec3(0.0f, 0.0f, 1.0f));   
    glm::mat4 leftWingModelMatrix = glm::rotate(m_modelMatrix, m_flapAngle, glm::vec3(0.0f, 0.0f, 1.0f));  
    glm::mat3 leftWingNormalMatrix = glm::inverseTranspose(glm::mat3(leftWingModelMatrix));

	for (GPUMesh& mesh : butterfly_wing_meshes)
	{
	    m_defaultShader.bind();

	    // Light properties
	    glUniform3fv(m_defaultShader.getUniformLocation("lightPosition"), 1, glm::value_ptr(li.getPos()));
	    glUniform3fv(m_defaultShader.getUniformLocation("lightDirection_optional"), 1, glm::value_ptr(li.getFor()));
	    glUniform3fv(m_defaultShader.getUniformLocation("lightColor"), 1, glm::value_ptr(li.getCol()));
	    glUniform1i(m_defaultShader.getUniformLocation("isSpot"), li.isSpot());
	    
	    glUniformMatrix4fv(m_defaultShader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(leftWingMvpMatrix));
	    //Uncomment this line when you use the modelMatrix (or fragmentPosition)
	    glUniformMatrix4fv(m_defaultShader.getUniformLocation("modelMatrix"), 1, GL_FALSE, glm::value_ptr(leftWingModelMatrix));
	    glUniformMatrix3fv(m_defaultShader.getUniformLocation("normalModelMatrix"), 1, GL_FALSE, glm::value_ptr(leftWingNormalMatrix));
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
    
    glm::mat4 rightWingMvpMatrix = glm::translate(glm::rotate(mvpMatrix, glm::radians(104.0f) - m_flapAngle , glm::vec3(0.0f, 0.0f, 1.0f)), glm::vec3(0.3f, 0.1f, 0.0f));   //for the second wing
    glm::mat4 rightWingModelMatrix = glm::translate(glm::rotate(m_modelMatrix, glm::radians(104.0f) - m_flapAngle, glm::vec3(0.0f, 0.0f, 1.0f)), glm::vec3(0.3f, 0.1f, 0.0f));  //for the second wing
    glm::mat3 rightWingNormalMatrix = glm::inverseTranspose(glm::mat3(rightWingModelMatrix));
    
    for (GPUMesh& mesh : butterfly_wing_meshes)
    {
        m_defaultShader.bind(); 

        // Light properties
        glUniform3fv(m_defaultShader.getUniformLocation("lightPosition"), 1, glm::value_ptr(li.getPos()));
        glUniform3fv(m_defaultShader.getUniformLocation("lightDirection_optional"), 1, glm::value_ptr(li.getFor()));
        glUniform3fv(m_defaultShader.getUniformLocation("lightColor"), 1, glm::value_ptr(li.getCol()));
        glUniform1i(m_defaultShader.getUniformLocation("isSpot"), li.isSpot());


        // Send NEW matrices for the second wing
        glUniformMatrix4fv(m_defaultShader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(rightWingMvpMatrix)); 
        glUniformMatrix4fv(m_defaultShader.getUniformLocation("modelMatrix"), 1, GL_FALSE, glm::value_ptr(rightWingModelMatrix)); 
        glUniformMatrix3fv(m_defaultShader.getUniformLocation("normalModelMatrix"), 1, GL_FALSE, glm::value_ptr(rightWingNormalMatrix)); 

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

            // Clear the screen
            glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // ...
            glEnable(GL_DEPTH_TEST);

	    // Calculate view-projection matrix setup
	    // for the main camera
	    const glm::mat4 m_projection = glm::perspective(CAMERA_FOV, CAMERA_ASPECT_RATIO, 0.1f, 30.0f);
	    const glm::mat4 mvpMatrix = m_projection * (cameras[camera_idx]).viewMatrix();

            //const glm::mat4 mvpMatrix = m_projectionMatrix * m_viewMatrix * m_modelMatrix;
            // Normals should be transformed differently than positions (ignoring translations + dealing with scaling):
            // https://paroj.github.io/gltut/Illumination/Tut09%20Normal%20Transformation.html
            const glm::mat3 normalModelMatrix = glm::inverseTranspose(glm::mat3(m_modelMatrix));

	    // --- Rendering section!
	    for (Light li : lights)
	    { // For each light
		render_butterfly(normalModelMatrix, mvpMatrix, li);
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

    Texture m_texture;
    bool m_useMaterial { true };

    // Projection and view matrices for you to fill in and use
    glm::mat4 m_modelMatrix { 1.0f };

    float m_flapAngle{ 0.0f }; //to make the wings flap!!
};

int main()
{
    // --- SET CAMERAS
    // Position and Forward for the first camera
    glm::vec3 pos0  = {9.84f, 2.75f, -10.66f};
    glm::vec3 for0  = {-0.67f, 0.10f, 0.73f};
    
    // Position and Forward for the second camera
    glm::vec3 pos1  = {-9.15f, 6.21f, -10.1f};
    glm::vec3 for1 = {0.75f, -0.2f, 0.6f};

    // --- SET LIGHTS
    // Position and Forward for the first light
    glm::vec3 colL0 = {1.0f, 1.0f, 1.0f};
    glm::vec3 posL0 = {0.0f, 0.8f, 0.0f};
    //glm::vec3 forL0 = {-1.0f, -1.0f, -1.0f};
    Light li = Light(colL0, posL0);

    // --- Create the app
    Application app;

    // Initialize first camera
    app.initCamera(pos0, for0, false);
    // Add second camera
    app.addCamera(pos1, for1, true);
    // Add first light
    app.addLight(li);

    // App start
    app.startLoop();

    return 0;
}
