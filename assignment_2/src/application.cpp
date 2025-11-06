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
float DISTANCE_CLIPPING = 10000000.0f;

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
        , m_chunk_texture(RESOURCE_ROOT "resources/chunk-texture.png")
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
            ShaderBuilder butterflyBuilder;
            butterflyBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/shader_vert.glsl");
            butterflyBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/shader_frag.glsl");
            m_butterflyShader = butterflyBuilder.build();

            ShaderBuilder chunkBuilder;
            chunkBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/chunk_vert.glsl");
            chunkBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/chunk_frag.glsl");
            m_chunkShader = chunkBuilder.build();

            ShaderBuilder shadowBuilder;
            shadowBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/shadow_vert.glsl");
            shadowBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/shadow_frag.glsl");
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

    void selectNextLight()
    {
	selectedLightIndex = (selectedLightIndex + 1) % lights.size();
    }

    void selectPreviousLight()
    {
	if (selectedLightIndex == 0)
	    selectedLightIndex = lights.size() - 1;
	else
	    --selectedLightIndex;
    }
	
    void imgui()
    { // Section for user interface!
	// Use ImGui for easy input/output of ints, floats, strings, etc...
	ImGui::Begin("Window");

	// --- ANIMATION
	if (ImGui::CollapsingHeader("Butterfly Parameters"))
	{
	    ImGui::Text("Blue Orbit Offset");
	    ImGui::InputFloat("BX", &m_butterflyOffset0.x);
	    ImGui::InputFloat("BY", &m_butterflyOffset0.y);
	    ImGui::InputFloat("BZ", &m_butterflyOffset0.z);

	    ImGui::Separator();
	    
	    ImGui::Text("Redd Orbit Offset");
	    ImGui::InputFloat("RX", &m_butterflyOffset1.x);
	    ImGui::InputFloat("RY", &m_butterflyOffset1.y);
	    ImGui::InputFloat("RZ", &m_butterflyOffset1.z);

	    ImGui::Separator();

	    ImGui::Text("General Orbit");
	    ImGui::SliderInt("Radius", &m_flightRadius, 1, 500);
	    ImGui::SliderInt("Sway Amplitude", &m_swayAmplitude, 0, 50);
	    ImGui::SliderInt("Speed", &m_flightSpeed, 1, 100);
	}

	// --- LIGHTS

	if (ImGui::CollapsingHeader("Scene Lightning"))
	{
	    // Light list
	    std::vector<std::string> itemStrings = {};
	    for (size_t i = 0; i < lights.size(); i++) {
		auto string = "Light " + std::to_string(i);
		itemStrings.push_back(string);
	    }

	    std::vector<const char*> itemCStrings = {};
	    for (const auto& string : itemStrings) {
		itemCStrings.push_back(string.c_str());
	    }

	    int tempSelectedItem = static_cast<int>(selectedLightIndex);
	    if (ImGui::ListBox("Lights", &tempSelectedItem, itemCStrings.data(), (int) itemCStrings.size(), 4)) {
		selectedLightIndex = static_cast<size_t>(tempSelectedItem);
	    }

	    // Properties for selected child
	    ImGui::BeginChild("##Container", ImVec2(0.0f, 260.0f), ImGuiWindowFlags_AlwaysUseWindowPadding);
	    ImGui::Text("Properties");

	    // Change selected light position
	    ImGui::InputFloat("Possition X", &lights[selectedLightIndex].position.x);
	    ImGui::InputFloat("Position Y", &lights[selectedLightIndex].position.y);
	    ImGui::InputFloat("Position Z", &lights[selectedLightIndex].position.z);

	    // Change selected light position
	    ImGui::Text("");

	    // Checkbox for spotlight
	    ImGui::Checkbox("Spotlight", &lights[selectedLightIndex].isSpotlight);
	    if(!lights[selectedLightIndex].isSpotlight)
	    {
		ImGui::InputFloat("Direction X", &lights[selectedLightIndex].forward.x);
		ImGui::InputFloat("Direction Y", &lights[selectedLightIndex].forward.y);
		ImGui::InputFloat("Direction Z", &lights[selectedLightIndex].forward.z);
	    }
	
	    // Change selected light color
	    ImGui::ColorEdit3("Color", &lights[selectedLightIndex].color[0]);

	    
	    ImGui::EndChild();

	    // Select previous light
	    if (ImGui::Button("Prev"))
	    {
		selectPreviousLight();
	    }
	    ImGui::SameLine();
	    // Select next light
	    if (ImGui::Button("Next"))
	    {
		selectNextLight();;
	    }
	    ImGui::SameLine();
	    // Add new light
	    if (ImGui::Button("Add"))
	    {
		lights.push_back(Light { glm::vec3(1), glm::vec3(1) });
		selectedLightIndex = lights.size()-1;
	    }
	    ImGui::SameLine();
	    // Remove selected light
	    if (ImGui::Button("Remove"))
	    {
		if (lights.size() > 1)
		{
		    lights.erase(lights.begin() + selectedLightIndex);
		    std::cout << "Removed the light" << selectedLightIndex << std::endl;
		    if (selectedLightIndex > 0)
			selectedLightIndex--;
		}
		else
		{
		    std::cout << "Can not remove the only light" << std::endl;
		}
	    }
	}

	// --- CHUNKS
	
	if (ImGui::CollapsingHeader("Chunk Properties"))
	{
	    ImGui::Text("Update Chunks after changing the variables below:");
	    ImGui::InputInt("Chunk Seed", &chunk_seed);
	    ImGui::InputInt("Chunk Hills", &chunk_hills);
	    ImGui::InputInt("Max Hill Height", &max_hill_height);
	    ImGui::InputInt("Chunk Size", &chunk_size);
	    //ImGui::SliderFloat("Chunk Scale", &chunk_scale, 0.1f, 100.0f);
	    ImGui::SliderFloat("Bezier Hill Steepness", &hill_steepness, 0.0f, 3.0f);

	    ImGui::Separator();

	    ImGui::Separator();
	    if (ImGui::Button("Update Chunk"))
	    { // Update the chunk :D
		update_chunks();
	    }

	    ImGui::Separator();
	    ImGui::Text("Variables below can be changed live:");
	    ImGui::InputInt("Chunk Tiles", &chunk_tiles);
	    ImGui::InputFloat("Chunk Y-Axis Offset", &chunk_y);
	}


	ImGui::Separator();
	ImGui::Checkbox("PBR Shading", &m_useMaterial);

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
	glm::vec3 direction = clockWise? 
	    glm::normalize(glm::vec3(cos(angle), 0.0f, -sin(angle)))
	    : glm::normalize(glm::vec3(-cos(angle), 0.0f, sin(angle)));
	    
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
	    glm::mat4 viewProjMatrix, 
	    glm::mat4 modelMatrix,
	    Light li, 
	    bool isBlue
    )
    { // Function to render our butterfly for the current light

	// --- Butterfly Body Calculation
	glm::mat4 bodyModelMatrix = modelMatrix;
	glm::mat4 bodyMvpMatrix = viewProjMatrix * bodyModelMatrix; 
	glm::mat3 bodyNormalMatrix = glm::inverseTranspose(glm::mat3(bodyModelMatrix));

	m_butterflyShader.bind();
	glUniform3fv(m_butterflyShader.getUniformLocation("cameraPosition"), 1, glm::value_ptr(cameras[camera_idx].cameraPos()));
	glUniform1f(m_butterflyShader.getUniformLocation("metallic"), 0.2f);
	glUniform1f(m_butterflyShader.getUniformLocation("roughness"), 0.5f);

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
	    m_butterflyShader.bind();

	    // Light properties
	    glUniform3fv(m_butterflyShader.getUniformLocation("lightPosition"), 1, glm::value_ptr(li.position));
	    glUniform3fv(m_butterflyShader.getUniformLocation("lightDirection_optional"), 1, glm::value_ptr(li.forward));
	    glUniform3fv(m_butterflyShader.getUniformLocation("lightColor"), 1, glm::value_ptr(li.color));
	    glUniform1i(m_butterflyShader.getUniformLocation("isSpot"), li.isSpotlight);

	    // Bind the butterfly texture!
	    m_butterfly_body_texture.bind(GL_TEXTURE2);
	    glUniform1i(m_butterflyShader.getUniformLocation("textureMap"), 2);

	    glUniformMatrix4fv(m_butterflyShader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(bodyMvpMatrix));
	    glUniformMatrix4fv(m_butterflyShader.getUniformLocation("modelMatrix"), 1, GL_FALSE, glm::value_ptr(bodyModelMatrix));
	    glUniformMatrix3fv(m_butterflyShader.getUniformLocation("normalModelMatrix"), 1, GL_FALSE, glm::value_ptr(bodyNormalMatrix));
	    if (mesh.hasTextureCoords()) 
	    {
		m_texture.bind(GL_TEXTURE0);
		glUniform1i(m_butterflyShader.getUniformLocation("colorMap"), 0);
		glUniform1i(m_butterflyShader.getUniformLocation("useMaterial"), GL_FALSE);
	    }
	    else 
	    {
		glUniform1i(m_butterflyShader.getUniformLocation("useMaterial"), m_useMaterial);
	    }
	    mesh.draw(m_butterflyShader);

	}

	// --- RENDER BUTTERFLY WINGS MESHES

	glm::mat4 leftWingModelMatrix = glm::rotate(modelMatrix, m_flapAngle, glm::vec3(0.0f, 0.0f, 1.0f));
	glm::mat4 leftWingMvpMatrix = viewProjMatrix * leftWingModelMatrix; 
	glm::mat3 leftWingNormalMatrix = glm::inverseTranspose(glm::mat3(leftWingModelMatrix));

	for (GPUMesh& mesh : butterfly_wing_meshes)
	{
	    m_butterflyShader.bind();

	    // Light properties
	    glUniform3fv(m_butterflyShader.getUniformLocation("lightPosition"), 1, glm::value_ptr(li.position));
	    glUniform3fv(m_butterflyShader.getUniformLocation("lightDirection_optional"), 1, glm::value_ptr(li.forward));
	    glUniform3fv(m_butterflyShader.getUniformLocation("lightColor"), 1, glm::value_ptr(li.color));
	    glUniform1i(m_butterflyShader.getUniformLocation("isSpot"), li.isSpotlight);

	    // Bind the butterfly texture!
	    if (isBlue)
	    {
		m_butterfly_texture.bind(GL_TEXTURE1);
	    }
	    else
	    {
		m_butterfly_texture0.bind(GL_TEXTURE1);
	    }
	    glUniform1i(m_butterflyShader.getUniformLocation("textureMap"), 1);
	    
	    glUniformMatrix4fv(m_butterflyShader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(leftWingMvpMatrix));
	    glUniformMatrix4fv(m_butterflyShader.getUniformLocation("modelMatrix"), 1, GL_FALSE, glm::value_ptr(leftWingModelMatrix));
	    glUniformMatrix3fv(m_butterflyShader.getUniformLocation("normalModelMatrix"), 1, GL_FALSE, glm::value_ptr(leftWingNormalMatrix));
	    if (mesh.hasTextureCoords()) 
	    {
		m_texture.bind(GL_TEXTURE0);
		glUniform1i(m_butterflyShader.getUniformLocation("colorMap"), 0);
		glUniform1i(m_butterflyShader.getUniformLocation("useMaterial"), GL_FALSE);
	    }
	    else 
	    {
		glUniform1i(m_butterflyShader.getUniformLocation("useMaterial"), m_useMaterial);
	    }
	    mesh.draw(m_butterflyShader);
	}
    
	glm::mat4 rightWingModelMatrix = glm::translate(glm::rotate(modelMatrix, glm::radians(104.0f) - m_flapAngle, glm::vec3(0.0f, 0.0f, 1.0f)),glm::vec3(0.3f, 0.1f, 0.0f));
	glm::mat4 rightWingMvpMatrix = viewProjMatrix * rightWingModelMatrix;
	glm::mat3 rightWingNormalMatrix = glm::inverseTranspose(glm::mat3(rightWingModelMatrix));
	
	for (GPUMesh& mesh : butterfly_wing_meshes)
	{
	    m_butterflyShader.bind(); 

	    // Light properties
	    glUniform3fv(m_butterflyShader.getUniformLocation("lightPosition"), 1, glm::value_ptr(li.position));
	    glUniform3fv(m_butterflyShader.getUniformLocation("lightDirection_optional"), 1, glm::value_ptr(li.forward));
	    glUniform3fv(m_butterflyShader.getUniformLocation("lightColor"), 1, glm::value_ptr(li.position));
	    glUniform1i(m_butterflyShader.getUniformLocation("isSpot"), li.isSpotlight);

	    // Bind the butterfly texture!
	    if (isBlue)
	    {
		m_butterfly_texture.bind(GL_TEXTURE1);
	    }
	    else
	    {
		m_butterfly_texture0.bind(GL_TEXTURE1);
	    }
	    glUniform1i(m_butterflyShader.getUniformLocation("textureMap"), 1);

	    // Send NEW matrices for the second wing
	    glUniformMatrix4fv(m_butterflyShader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(rightWingMvpMatrix)); 
	    glUniformMatrix4fv(m_butterflyShader.getUniformLocation("modelMatrix"), 1, GL_FALSE, glm::value_ptr(rightWingModelMatrix)); 
	    glUniformMatrix3fv(m_butterflyShader.getUniformLocation("normalModelMatrix"), 1, GL_FALSE, glm::value_ptr(rightWingNormalMatrix)); 

	    if (mesh.hasTextureCoords())
	    {
		m_texture.bind(GL_TEXTURE0);
		glUniform1i(m_butterflyShader.getUniformLocation("colorMap"), 0);
		glUniform1i(m_butterflyShader.getUniformLocation("useMaterial"), GL_FALSE);
	    }
	    else
	    {
		glUniform1i(m_butterflyShader.getUniformLocation("useMaterial"), m_useMaterial);
	    }
	    mesh.draw(m_butterflyShader);
	}  
    }

    glm::vec2 bezier_get_point(glm::vec2 P0, glm::vec2 P1, glm::vec2 P2, glm::vec2 P3, float t)
    {
	float u = 1.0f - t;
	float tt = t * t;
	float uu = u * u;
	float uuu = uu * u;
	float ttt = tt * t;

	glm::vec2 p = uuu * P0;           // (1 - t)^3 * P0
	p += 3.0f * uu * t * P1;          // 3(1 - t)^2 * t * P1
	p += 3.0f * u * tt * P2;          // 3(1 - t) * t^2 * P2
	p += ttt * P3;                    // t^3 * P3
	
	return p;
    }

    void create_chunk()
    { 
	// Create a chunk with hills on it!
	//          /\
	//         /**\
	//        /****\   /\
	//       /      \ /**\
	//      /  /\    /    \        /\    /\  /\      /\            /\/\/\  /\
	//     /  /  \  /      \      /  \/\/  \/  \  /\/  \/\  /\  /\/ / /  \/  \
	//    /  /    \/ /\     \    /    \ \  /    \/ /   /  \/  \/  \  /    \   \
	//   /  /      \/  \/\   \  /      \    /   /    \
	//__/__/_______/___/__\___\__________________________________________________

	chunk_coordinates.clear();
	chunk_coordinates.reserve(chunk_size * chunk_size);

	// --- Step 1: Base grid, scaled
	for (int z = 0; z < chunk_size; z++)
	{
	    for (int x = 0; x < chunk_size; x++)
	    {
		chunk_coordinates.emplace_back(glm::vec3(
		    static_cast<float>(x) * chunk_scale,
		    0.0f,
		    static_cast<float>(z) * chunk_scale
		));
	    }
	}

	// --- Step 2: Generate hills
	srand(chunk_seed);
	for (int h = 0; h < chunk_hills; h++)
	{
	    float centerX = static_cast<float>(rand() % chunk_size) * chunk_scale;
	    float centerZ = static_cast<float>(rand() % chunk_size) * chunk_scale;
	    float hillHeight = static_cast<float>(rand() % max_hill_height) * chunk_scale;
	    float hillRadius = chunk_size * 0.4f * chunk_scale;
	    
	    for (auto& coord : chunk_coordinates)
	    {
		float dx = coord.x - centerX;
		float dz = coord.z - centerZ;
		float distance = sqrt(dx * dx + dz * dz);
		
		if (distance < hillRadius)
		{
		    float t = glm::clamp(
			static_cast<float>(pow(distance / hillRadius, hill_steepness)), 
			0.0f, 1.0f
		    );
		    
		    // Bezier curve for smooth hill influence
		    glm::vec2 P0(0.0f, 1.0f);
		    glm::vec2 P1(0.2f, 0.95f);
		    glm::vec2 P2(0.8f, 0.3f);
		    glm::vec2 P3(1.0f, 0.0f);
		    glm::vec2 point = bezier_get_point(P0, P1, P2, P3, t);
		    float influence = point.y;
		    
		    // Calculate edge blending factor, this is to make sure the chunk is tileable!
		    float edgeBlendDistance = chunk_size * 0.15f * chunk_scale; // Blend over 15% of chunk
		    float distToEdgeX = glm::min(coord.x, (chunk_size - 1) * chunk_scale - coord.x);
		    float distToEdgeZ = glm::min(coord.z, (chunk_size - 1) * chunk_scale - coord.z);
		    float distToEdge = glm::min(distToEdgeX, distToEdgeZ);
		    
		    float edgeFade = glm::clamp(distToEdge / edgeBlendDistance, 0.0f, 1.0f);
		    edgeFade = edgeFade * edgeFade * (3.0f - 2.0f * edgeFade); // Smoothstep
		    
		    coord.y += hillHeight * influence * edgeFade;
		}
	    }
	}

    }

    void build_chunk_mesh()
    { 

	std::vector<Vertex> vertices;
	std::vector<glm::uvec3> triangles;

	vertices.reserve(chunk_coordinates.size());
	triangles.reserve((chunk_size - 1) * (chunk_size - 1) * 2);

	// --- Step 1: Vertices
	for (const auto& coord : chunk_coordinates)
	{
	    vertices.push_back({
		coord,  // already scaled
		glm::vec3(0.0f, 1.0f, 0.0f),
		glm::vec2(coord.x / (chunk_size * chunk_scale),
			  coord.z / (chunk_size * chunk_scale))
	    });
	}

	// --- Step 2: Triangle indices
	for (int z = 0; z < chunk_size - 1; z++)
	{
	    for (int x = 0; x < chunk_size - 1; x++)
	    {
		unsigned int topLeft     = z * chunk_size + x;
		unsigned int topRight    = topLeft + 1;
		unsigned int bottomLeft  = (z + 1) * chunk_size + x;
		unsigned int bottomRight = bottomLeft + 1;

		triangles.emplace_back(topLeft, bottomLeft, topRight);
		triangles.emplace_back(topRight, bottomLeft, bottomRight);
	    }
	}

	// --- Step 3: Normals
	for (auto& v : vertices) v.normal = glm::vec3(0.0f);

	for (const auto& tri : triangles)
	{
	    glm::vec3 edge1 = vertices[tri.y].position - vertices[tri.x].position;
	    glm::vec3 edge2 = vertices[tri.z].position - vertices[tri.x].position;
	    glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));

	    vertices[tri.x].normal += normal;
	    vertices[tri.y].normal += normal;
	    vertices[tri.z].normal += normal;
	}

	for (auto& v : vertices)
	{
	    if (glm::length(v.normal) > 0.0f)
		v.normal = glm::normalize(v.normal);
	    else
		v.normal = glm::vec3(0.0f, 1.0f, 0.0f);
	}

	// --- Step 4: Create mesh
	Mesh cpuMesh;
	cpuMesh.vertices = std::move(vertices);
	cpuMesh.triangles = std::move(triangles);
	cpuMesh.material.kd = glm::vec3(0.3f, 0.7f, 0.3f);
	cpuMesh.material.ks = glm::vec3(0.1f, 0.1f, 0.1f);
	cpuMesh.material.shininess = 8.0f;
	cpuMesh.material.transparency = 1.0f;

	m_chunkMesh.emplace(std::move(cpuMesh));

    }

    void render_chunk(const glm::mat4& viewProjMatrix, glm::mat4 modelMatrix, Light li)
    { 
	if (!m_chunkMesh.has_value()) return;

	// Calculate MVP matrix correctly
	glm::mat4 mvpMatrix = viewProjMatrix * modelMatrix; 
	glm::mat3 normalMatrix = glm::inverseTranspose(glm::mat3(modelMatrix));

	m_chunkShader.bind();

	// Camera and material properties
	glUniform3fv(m_chunkShader.getUniformLocation("cameraPosition"), 1,
		     glm::value_ptr(cameras[camera_idx].cameraPos()));
	glUniform1f(m_chunkShader.getUniformLocation("metallic"), 0.0f);
	glUniform1f(m_chunkShader.getUniformLocation("roughness"), 0.8f);

	// Light properties
	glUniform3fv(m_chunkShader.getUniformLocation("lightPosition"), 1,
		     glm::value_ptr(li.position));
	glUniform3fv(m_chunkShader.getUniformLocation("lightDirection_optional"), 1,
		     glm::value_ptr(li.forward));
	glUniform3fv(m_chunkShader.getUniformLocation("lightColor"), 1,
		     glm::value_ptr(li.color));
	glUniform1i(m_chunkShader.getUniformLocation("isSpot"), li.isSpotlight);

	// Bind a texture for the chunks (using checkerboard)
	m_chunk_texture.bind(GL_TEXTURE3);
	glUniform1i(m_chunkShader.getUniformLocation("textureMap"), 3);

	// Matrix uniforms
	glUniformMatrix4fv(m_chunkShader.getUniformLocation("mvpMatrix"), 1, GL_FALSE,
			   glm::value_ptr(mvpMatrix));
	glUniformMatrix4fv(m_chunkShader.getUniformLocation("modelMatrix"), 1, GL_FALSE,
			   glm::value_ptr(modelMatrix));
	glUniformMatrix3fv(m_chunkShader.getUniformLocation("normalModelMatrix"), 1, GL_FALSE,
			   glm::value_ptr(normalMatrix));

	// Enable material rendering
	glUniform1i(m_chunkShader.getUniformLocation("useMaterial"), m_useMaterial);

	m_chunkMesh->draw(m_chunkShader);	
    }

    void update_chunks()
    {
	create_chunk();
	build_chunk_mesh();
    }

    void startLoop()
    {
	// Create the chunks!
	update_chunks();
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
	    m_flightAngle = m_flightAngle + (m_flightSpeed/1000.0f);

            // Clear the screen
            glClearColor(0.2f, 0.3f, 0.4f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // ...
            glEnable(GL_DEPTH_TEST);

            //const glm::mat4 mvpMatrix = m_projectionMatrix * m_viewMatrix * m_butterflyMatrix;
            // Normals should be transformed differently than positions (ignoring translations + dealing with scaling):
            // https://paroj.github.io/gltut/Illumination/Tut09%20Normal%20Transformation.html

	    const glm::mat4 projectionMatrix = glm::perspective(
		    CAMERA_FOV, 
		    CAMERA_ASPECT_RATIO, 
		    1.0f, 
		    DISTANCE_CLIPPING
	    );

	    const glm::mat4 viewMatrix = cameras[camera_idx].viewMatrix(); // includes camera position and rotation
	    const glm::mat4 viewProjMatrix = projectionMatrix * viewMatrix;

	    // Update butterflies
	    m_butterflyMatrix0 = update_butterflyMatrix(m_butterflyMatrix0, m_butterflyOffset0, true);
	    m_butterflyMatrix1 = update_butterflyMatrix(m_butterflyMatrix1, m_butterflyOffset1, false);

	    // --- Render everything!
	    float chunkWorldSize = (chunk_size - 1) * chunk_scale; // Total size of chunks

	    for (Light li : lights)
	    {
		// --- Render the butterflies
		render_butterfly(viewProjMatrix, m_butterflyMatrix0, li, true);
		render_butterfly(viewProjMatrix, m_butterflyMatrix1, li, false);


		for (int tz = -chunk_tiles / 2; tz < chunk_tiles / 2; tz++)
		{
		    for (int tx = -chunk_tiles / 2; tx < chunk_tiles / 2; tx++)
		    {
			glm::vec3 tileOffset = glm::vec3(
			    tx * chunkWorldSize,
			    chunk_y * chunkWorldSize,
			    tz * chunkWorldSize
			);

			// Model matrix: translate, then scale
			glm::mat4 chunkModel = glm::translate(glm::mat4(1.0f), tileOffset);

			glm::mat4 chunkMvp = viewProjMatrix * chunkModel;
			glm::mat3 chunkNormalMat = glm::inverseTranspose(glm::mat3(chunkModel));

			render_chunk(chunkNormalMat, chunkMvp, li);
		    }
		}

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
    Shader m_butterflyShader;
    Shader m_chunkShader;
    Shader m_shadowShader;

    // --- Meshes of an object file!
    ObjectFile butterfly_body_meshes;
    ObjectFile butterfly_wing_meshes;

    // --- Data for procedurally generated chunk!
    int chunk_seed = 15; // Must be bigger than 0
    int chunk_hills = 25;
    int max_hill_height = 5;
    int chunk_size = 15;
    float chunk_y = -1.0f;
    std::vector<glm::vec3> chunk_coordinates; // Size of chunk_size^2
    glm::mat4 m_chunkMatrix { 1.0f }; // Identity Matrix
    std::optional<GPUMesh> m_chunkMesh;
    float chunk_scale = 20.0f;
    float hill_steepness = 0.575f;
    int chunk_tiles = 50;

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
    Texture m_chunk_texture;
    bool m_useMaterial { true };

    // Projection and view matrices for you to fill in and use
    glm::mat4 m_butterflyMatrix0 { 1.0f };
    glm::vec3 m_butterflyOffset0 = glm::vec3(-5.0f, 4.0f, 1.0f);
    glm::mat4 m_butterflyMatrix1 { 1.0f };
    glm::vec3 m_butterflyOffset1 = glm::vec3(3.0f, -2.0f, 30.0f);

    // Lights selected Index
    int selectedLightIndex = 0;

    // --- All the animation stuff!
    int m_flightSpeed = 35;
    int m_flightRadius = 50;
    int m_swayAmplitude = 25;
    float m_flightAngle = 0.0f ;
    float m_flapAngle{ 0.0f }; //to make the wings flap!!
};

int main()
{
    // --- Create the app
    Application app;

    // --- SET CAMERAS
    // Position and Forward for the first camera
    glm::vec3 pos0  = {-68.23f, -0.035f, -105.23f};
    glm::vec3 for0  = {0.482f, -0.077f, 0.872f};

    // Position and Forward for the second camera
    glm::vec3 pos1  = {-9.15f, 6.21f, -10.1f};
    glm::vec3 for1 = {0.75f, -0.2f, 0.6f};

    // Position and Forward for the third camera
    glm::vec3 pos2  = {9.84f, 2.75f, -10.66f};
    glm::vec3 for2  = {-0.67f, 0.10f, 0.73f};

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
    app.addLight(
	Light(
	    glm::vec3(1.0f, 1.0f, 1.0f), // colour
	    glm::vec3(10.0f, 200.0f, -10.0f), // position
	    glm::vec3(-0.3f, -1.0f, -0.2f) // forward
	)
    );

    // App start
    app.startLoop();

    return 0;
}
