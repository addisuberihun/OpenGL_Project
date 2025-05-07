#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <cmath>
#include <algorithm>

// Include Assimp headers
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "include/model_loader.h"

// Additional libraries for realistic environment
#include <bullet/btBulletDynamicsCommon.h>
#include <noise/noise.h>
#include <stb_image.h>

// Window settings
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;
const float ASPECT_RATIO = (float)SCR_WIDTH / (float)SCR_HEIGHT;

// Camera settings
glm::vec3 cameraPos = glm::vec3(0.0f, 5.0f, 15.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, -0.2f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
float cameraSpeed = 0.05f;
bool firstMouse = true;
float yaw = -90.0f;
float pitch = -10.0f;
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;

// Timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Track settings
const int NUM_TRACKS = 5;
const int NUM_STUDENTS = 32;
const float TRACK_WIDTH = 1.5f;
const float TRACK_LENGTH = 50.0f;
const float TRACK_SPACING = 0.5f;
const float TOTAL_WIDTH = NUM_TRACKS * TRACK_WIDTH + (NUM_TRACKS - 1) * TRACK_SPACING;

// Forward declarations
class Model;

// Define the HumanModel struct before Student
struct HumanModel {
    Model model;
    bool loaded;
    unsigned int faceTexture;
    
    HumanModel() : loaded(false), faceTexture(0) {}
};

// Now the Student struct can use HumanModel
struct Student {
    std::string name;
    int trackNumber;
    float position;
    float speed;
    float armPhase;
    float legPhase;
    glm::vec3 color;
    float finishTime;
    bool finished;
    HumanModel humanModel;
    
    Student() : 
        name(""), 
        trackNumber(0), 
        position(0.0f), 
        speed(1.0f), 
        color(1.0f, 1.0f, 1.0f), 
        finishTime(0.0f), 
        finished(false),
        legPhase(0.0f),
        armPhase(0.0f) {}
};

// Text rendering
unsigned int textVAO, textVBO;
std::map<char, struct Character> Characters;

struct Character {
    unsigned int TextureID;
    glm::ivec2 Size;
    glm::ivec2 Bearing;
    unsigned int Advance;
};

// Environment settings
const float GRAVITY = -9.81f;
const float TERRAIN_SIZE = 100.0f;
const float TERRAIN_HEIGHT = 10.0f;
const int TERRAIN_RESOLUTION = 128;

// Skybox settings
const bool ENABLE_SKYBOX = true;
const bool ENABLE_DYNAMIC_WEATHER = true;

// Physics world
btDiscreteDynamicsWorld* dynamicsWorld;
btRigidBody* groundRigidBody;
std::vector<btRigidBody*> studentRigidBodies;

// Environment textures
unsigned int terrainTexture;
unsigned int grassTexture;
unsigned int skyboxTexture;
unsigned int rainTexture;

// Weather system
enum WeatherType { SUNNY, CLOUDY, RAINY, FOGGY };
WeatherType currentWeather = SUNNY;
float weatherIntensity = 0.0f;
float weatherChangeTime = 0.0f;

// Function prototypes
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window, std::vector<Student>& students);
unsigned int createShaderProgram();
unsigned int createTextShaderProgram();
void renderText(unsigned int shader, std::string text, float x, float y, float scale, glm::vec3 color);
void renderText3D(unsigned int shader, std::string text, glm::vec3 position, float scale, glm::vec3 color);
void loadFonts();
unsigned int createTrack();
unsigned int createGround();
unsigned int createSkybox();
void renderStudent(unsigned int shader, Student& student, float time);
void initializeStudents(std::vector<Student>& students);
void drawCube();
void loadHumanModels(std::vector<Student>& students);
void initPhysics();
void cleanupPhysics();
void updatePhysics(float deltaTime);
unsigned int createTerrain();
void renderEnvironment(unsigned int shader, float time);
void updateWeather(float time);
void renderWeatherEffects(unsigned int shader, float time);
unsigned int loadTexture(const char* path);
void initializeEnvironment();
void renderScene(unsigned int shader, float currentTime);
void renderSkybox(unsigned int shader);

// Race state
bool raceStarted = false;
bool raceFinished = false;
float raceStartTime = 0.0f;
float raceDuration = 0.0f;

// Add this global variable to track font loading status
bool fontLoaded = false;

// Function to reset the race
void resetRace(std::vector<Student>& students) {
    // Re-initialize students at starting positions
    initializeStudents(students);
    raceStarted = false;
    raceFinished = false;
}

// Function to check if race is complete
bool isRaceComplete(const std::vector<Student>& students) {
    for (const auto& student : students) {
        if (!student.finished) {
            return false;
        }
    }
    return true;
}

// Function to display race results
void displayResults(unsigned int shader, unsigned int textShader, const std::vector<Student>& students) {
    // Sort students by finish time
    std::vector<std::reference_wrapper<const Student>> sortedStudents;
    for (const auto& student : students) {
        if (student.finished) {
            sortedStudents.push_back(std::ref(student));
        }
    }
    
    std::sort(sortedStudents.begin(), sortedStudents.end(), 
        [](const Student& a, const Student& b) {
            return a.finishTime < b.finishTime;
        });
    
    // Display results board
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 5.0f, -TRACK_LENGTH / 2.0f - 5.0f));
    model = glm::scale(model, glm::vec3(10.0f, 8.0f, 0.1f));
    glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform3f(glGetUniformLocation(shader, "objectColor"), 0.2f, 0.2f, 0.2f);
    drawCube();
    
    // Display top 10 results
    for (int i = 0; i < std::min(10, (int)sortedStudents.size()); i++) {
        const Student& student = sortedStudents[i];
        std::string resultText = std::to_string(i + 1) + ". " + student.name + " - " + 
                                std::to_string(student.finishTime).substr(0, 5) + "s";
        renderText3D(textShader, resultText, 
                    glm::vec3(-4.0f, 8.0f - i * 0.7f, -TRACK_LENGTH / 2.0f - 4.9f), 
                    0.02f, glm::vec3(1.0f, 1.0f, 0.0f));
    }
}

int main() {
    // Initialize GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create window
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "CE Students Running Animation", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // Capture mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Configure global OpenGL state
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Create shader programs
    unsigned int shaderProgram = createShaderProgram();
    unsigned int textShaderProgram = createTextShaderProgram();
    unsigned int skyboxShader = createShaderProgram(); // Use same shader for simplicity

    // Load fonts
    loadFonts();

    // Create VAOs
    unsigned int trackVAO = createTrack();
    unsigned int groundVAO = createGround();
    unsigned int skyboxVAO = createSkybox();

    // Initialize environment
    initializeEnvironment();

    // Initialize students
    std::vector<Student> students;
    initializeStudents(students);

    // Load human models
    loadHumanModels(students);

    // Render loop
    while (!glfwWindowShouldClose(window)) {
        // Per-frame time logic
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Process input
        processInput(window, students);

        // Clear the color and depth buffer
        glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Activate shader
        glUseProgram(shaderProgram);

        // Set up view and projection matrices
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), ASPECT_RATIO, 0.1f, 100.0f);

        // Set the matrices in the shader
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        // Render environment
        renderScene(shaderProgram, currentFrame);

        // Render tracks
        for (int i = 0; i < NUM_TRACKS; i++) {
            glm::mat4 model = glm::mat4(1.0f);
            float xPos = -TOTAL_WIDTH / 2.0f + i * (TRACK_WIDTH + TRACK_SPACING);
            model = glm::translate(model, glm::vec3(xPos, 0.01f, 0.0f));
            model = glm::scale(model, glm::vec3(TRACK_WIDTH, 0.01f, TRACK_LENGTH));
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
            glUniform3f(glGetUniformLocation(shaderProgram, "objectColor"), 0.8f, 0.8f, 0.8f);
            glBindVertexArray(trackVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        // Update and render students
        for (auto& student : students) {
            if (!student.finished) {
                student.position -= student.speed * deltaTime;
                if (student.position <= -TRACK_LENGTH / 2.0f) {
                    student.finished = true;
                    student.finishTime = currentFrame;
                }
            }
            renderStudent(shaderProgram, student, currentFrame);
        }

        // Render text
        glm::mat4 textProjection = glm::ortho(0.0f, static_cast<float>(SCR_WIDTH), 0.0f, static_cast<float>(SCR_HEIGHT), -1.0f, 1.0f);
        glUseProgram(textShaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(textShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(textProjection));

        // Render student names and times
        for (const auto& student : students) {
            // Calculate student position on the track
            float xPos = -TOTAL_WIDTH / 2.0f + student.trackNumber * (TRACK_WIDTH + TRACK_SPACING) + TRACK_WIDTH / 2.0f;
            float zPos = student.position;
            
            // Render student name above their position
            glm::vec3 textPos = glm::vec3(xPos, 1.6f, zPos);
            renderText3D(textShaderProgram, student.name, textPos, 0.01f, glm::vec3(1.0f, 1.0f, 1.0f));
            
            // Render finish time for students who have finished
            if (student.finished) {
                std::string timeStr = std::to_string(student.finishTime);
                timeStr = timeStr.substr(0, timeStr.find('.') + 3); // Keep only 2 decimal places
                renderText3D(textShaderProgram, timeStr + "s", glm::vec3(xPos, 1.8f, -TRACK_LENGTH / 2.0f), 0.01f, glm::vec3(1.0f, 1.0f, 0.0f));
            }
        }

        // Render UI text
        renderText(textShaderProgram, "CE Students Running Animation", 25.0f, SCR_HEIGHT - 30.0f, 0.5f, glm::vec3(1.0f, 1.0f, 1.0f));
        renderText(textShaderProgram, "Press ESC to exit", 25.0f, 25.0f, 0.5f, glm::vec3(1.0f, 1.0f, 1.0f));

        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Clean up
    glDeleteVertexArrays(1, &trackVAO);
    glDeleteVertexArrays(1, &groundVAO);
    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteVertexArrays(1, &textVAO);
    glDeleteBuffers(1, &textVBO);
    glDeleteProgram(shaderProgram);
    glDeleteProgram(textShaderProgram);
    
    // Clean up physics
    cleanupPhysics();

    glfwTerminate();
    return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    cameraSpeed += yoffset * 0.01f;
    if (cameraSpeed < 0.01f)
        cameraSpeed = 0.01f;
    if (cameraSpeed > 0.2f)
        cameraSpeed = 0.2f;
}

void processInput(GLFWwindow* window, std::vector<Student>& students) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Camera movement controls (existing code)
    float speed = cameraSpeed * deltaTime * 10.0f;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += speed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= speed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * speed;
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        cameraPos += cameraUp * speed;
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        cameraPos -= cameraUp * speed;
    
    // Race control keys
    static bool spacePressed = false;
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        if (!spacePressed) {
            spacePressed = true;
            
            // Toggle race state
            if (!raceStarted) {
                // Start race
                raceStarted = true;
                raceStartTime = static_cast<float>(glfwGetTime());
                std::cout << "Race started!" << std::endl;
            } else if (!raceFinished) {
                // Pause race
                raceStarted = false;
                std::cout << "Race paused!" << std::endl;
            }
        }
    } else {
        spacePressed = false;
    }
    
    // Reset race with R key
    static bool rPressed = false;
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        if (!rPressed) {
            rPressed = true;
            resetRace(students); // Now passing the students vector as parameter
            std::cout << "Race reset!" << std::endl;
        }
    } else {
        rPressed = false;
    }
}

unsigned int createShaderProgram() {
    const char* vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aNormal;
        layout (location = 2) in vec2 aTexCoords;

        out vec3 FragPos;
        out vec3 Normal;
        out vec2 TexCoords;

        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;

        void main() {
            FragPos = vec3(model * vec4(aPos, 1.0));
            Normal = mat3(transpose(inverse(model))) * aNormal;
            TexCoords = aTexCoords;
            gl_Position = projection * view * vec4(FragPos, 1.0);
        }
    )";

    const char* fragmentShaderSource = R"(
        #version 330 core
        out vec4 FragColor;

        in vec3 FragPos;
        in vec3 Normal;
        in vec2 TexCoords;

        uniform vec3 objectColor;
        uniform vec3 lightPos;
        uniform vec3 viewPos;
        uniform sampler2D faceTexture;
        uniform int hasCustomFace;

        void main() {
            // Ambient
            float ambientStrength = 0.3;
            vec3 ambient = ambientStrength * vec3(1.0, 1.0, 1.0);
            
            // Diffuse
            vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0) - FragPos);
            float diff = max(dot(Normal, lightDir), 0.0);
            vec3 diffuse = diff * vec3(1.0, 1.0, 1.0);
            
            // Specular
            float specularStrength = 0.5;
            vec3 viewDir = normalize(viewPos - FragPos);
            vec3 reflectDir = reflect(-lightDir, Normal);
            float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
            vec3 specular = specularStrength * spec * vec3(1.0, 1.0, 1.0);
            
            vec3 result;
            if (hasCustomFace > 0 && TexCoords.y > 0.7) {
                // Apply face texture to the head area
                vec4 texColor = texture(faceTexture, TexCoords);
                result = (ambient + diffuse + specular) * mix(objectColor, texColor.rgb, texColor.a);
            } else {
                result = (ambient + diffuse + specular) * objectColor;
            }
            
            FragColor = vec4(result, 1.0);
        }
    )";

    // Vertex shader
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    
    // Check for shader compile errors
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    
    // Fragment shader
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    
    // Check for shader compile errors
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    
    // Link shaders
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    
    // Check for linking errors
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    
    // Delete the shaders as they're linked into our program now and no longer necessary
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return shaderProgram;
}

unsigned int createTextShaderProgram() {
    const char* vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec4 vertex; // <vec2 pos, vec2 tex>
        
        out vec2 TexCoords;
        
        uniform mat4 projection;
        uniform mat4 view;
        uniform mat4 model;
        
        void main() {
            gl_Position = projection * view * model * vec4(vertex.xy, 0.0, 1.0);
            TexCoords = vertex.zw;
        }
    )";

    const char* fragmentShaderSource = R"(
        #version 330 core
        in vec2 TexCoords;
        out vec4 color;
        
        uniform sampler2D text;
        uniform vec3 textColor;
        
        void main() {
            vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);
            color = vec4(textColor, 1.0) * sampled;
        }
    )";

    // Vertex shader
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    
    // Check for shader compile errors
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    
    // Fragment shader
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    
    // Check for shader compile errors
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    
    // Link shaders
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    
    // Check for linking errors
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    
    // Delete the shaders as they're linked into our program now and no longer necessary
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return shaderProgram;
}

void loadFonts() {
    // Initialize FreeType
    FT_Library ft;
    if (FT_Init_FreeType(&ft)) {
        std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
        return;
    }

    // Try multiple font paths
    std::vector<std::string> fontPaths = {
        "fonts/arial.ttf",
        "arial.ttf",
        "C:/Windows/Fonts/arial.ttf",  // Common Windows font location
        "./fonts/arial.ttf",
        "../fonts/arial.ttf"
    };
    
    bool fontLoaded = false;
    FT_Face face;
    
    for (const auto& path : fontPaths) {
        if (FT_New_Face(ft, path.c_str(), 0, &face) == 0) {
            std::cout << "Successfully loaded font from: " << path << std::endl;
            fontLoaded = true;
            break;
        }
    }
    
    if (!fontLoaded) {
        std::cout << "ERROR::FREETYPE: Failed to load font from any location" << std::endl;
        std::cout << "Please place arial.ttf in the fonts/ directory or in the current directory" << std::endl;
        return;
    }

    // Set size to load glyphs as
    FT_Set_Pixel_Sizes(face, 0, 48);

    // Disable byte-alignment restriction
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // Load first 128 characters of ASCII set
    for (unsigned char c = 0; c < 128; c++) {
        // Load character glyph 
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            std::cout << "ERROR::FREETYTPE: Failed to load Glyph for character: " << c << std::endl;
            continue;
        }

        // Generate texture
        unsigned int texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RED,
            face->glyph->bitmap.width,
            face->glyph->bitmap.rows,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            face->glyph->bitmap.buffer
        );

        // Set texture options
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Store character for later use
        Character character = {
            texture,
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            static_cast<unsigned int>(face->glyph->advance.x)
        };
        Characters.insert(std::pair<char, Character>(c, character));
    }

    // Clean up FreeType resources
    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    if (fontLoaded) {
        // Configure VAO/VBO for text quads
        glGenVertexArrays(1, &textVAO);
        glGenBuffers(1, &textVBO);
        glBindVertexArray(textVAO);
        glBindBuffer(GL_ARRAY_BUFFER, textVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
}

void renderText(unsigned int shader, std::string text, float x, float y, float scale, glm::vec3 color) {
    if (Characters.empty()) {
        // Font failed to load, don't try to render text
        return;
    }
    
    // Activate corresponding render state
    


















