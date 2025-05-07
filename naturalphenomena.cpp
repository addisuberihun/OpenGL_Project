#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/random.hpp>
#include <glm/gtc/noise.hpp>
#include <iostream>
#include <vector>
#include <cmath>

// Window settings
const unsigned int SCR_WIDTH = 1024;
const unsigned int SCR_HEIGHT = 768;
const float ASPECT_RATIO = (float)SCR_WIDTH / (float)SCR_HEIGHT;

// Camera settings
glm::vec3 cameraPos = glm::vec3(0.0f, 2.0f, 8.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
float cameraSpeed = 0.05f;
bool firstMouse = true;
float yaw = -90.0f;
float pitch = 0.0f;
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;

// Timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Particle system types
enum ParticleSystemType {
    RAIN,
    WATERFALL,
    FIRE,
    DUST,
    COFFEE_CEREMONY
};

ParticleSystemType currentSystem = RAIN;
bool showHelp = true;  // Always show help by default

// Particle structure
struct Particle {
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 color;
    float size;
    float life;
    float maxLife;
    float userData; // Used to store additional information (like which stream a particle belongs to)
};

// Forward declarations
class ParticleSystem;
void renderParticles(ParticleSystem& system, unsigned int shaderProgram, unsigned int VAO);

// Particle system
class ParticleSystem {
public:
    std::vector<Particle> particles;
    unsigned int maxParticles;
    glm::vec3 origin;
    glm::vec3 originVariation;
    glm::vec3 gravity;
    float spawnRate;
    float timeSinceLastSpawn;
    bool isActive;  // Add active flag
    
    ParticleSystem(unsigned int max, glm::vec3 orig, glm::vec3 origVar, glm::vec3 grav, float rate) :
        maxParticles(max), origin(orig), originVariation(origVar), gravity(grav), spawnRate(rate), 
        timeSinceLastSpawn(0.0f), isActive(true) {
        particles.reserve(maxParticles);
    }
    
    virtual void update(float dt) {
        if (!isActive) return;
        
        // Update existing particles
        for (auto it = particles.begin(); it != particles.end();) {
            it->life -= dt;
            if (it->life <= 0.0f) {
                it = particles.erase(it);
            } else {
                it->velocity += gravity * dt;
                it->position += it->velocity * dt;
                ++it;
            }
        }
        
        // Spawn new particles based on rate and max count
        timeSinceLastSpawn += dt;
        while (timeSinceLastSpawn >= spawnRate && particles.size() < maxParticles) {
            spawnParticle();
            timeSinceLastSpawn -= spawnRate;
        }
    }
    
    virtual void spawnParticle() = 0;
    
    void setActive(bool active) {
        isActive = active;
    }
};

// Rain system - compressed particles
class RainSystem : public ParticleSystem {
public:
    RainSystem() : ParticleSystem(1000, glm::vec3(0.0f, 10.0f, 0.0f), 
                                 glm::vec3(10.0f, 0.0f, 10.0f), 
                                 glm::vec3(0.0f, -9.8f, 0.0f), 0.001f) {
        // Pre-populate with some particles
        for (int i = 0; i < maxParticles / 4; i++) {
            spawnParticle();
        }
    }
    
    void spawnParticle() override {
        Particle p;
        p.position = origin + glm::vec3(
            glm::linearRand(-originVariation.x, originVariation.x),
            glm::linearRand(-originVariation.y, originVariation.y),
            glm::linearRand(-originVariation.z, originVariation.z)
        );
        p.velocity = glm::vec3(0.0f, -5.0f, 0.0f);
        // Bright blue color for rain with slight variation
        float blueVariation = glm::linearRand(0.8f, 1.0f);
        p.color = glm::vec3(0.3f, 0.5f, blueVariation);
        // Make particles compressed (thinner and longer)
        p.size = glm::linearRand(0.1f, 0.2f); // Thinner
        p.maxLife = p.life = glm::linearRand(1.0f, 2.0f);
        particles.push_back(p);
    }
};

// Waterfall system - small wave-form particles with greenish color
class WaterfallSystem : public ParticleSystem {
public:
    WaterfallSystem() : ParticleSystem(2000, glm::vec3(0.0f, 5.0f, -5.0f), 
                                      glm::vec3(2.0f, 0.1f, 0.1f), 
                                      glm::vec3(0.0f, -9.8f, 0.0f), 0.0005f) {
        // Pre-populate with some particles
        for (int i = 0; i < maxParticles / 4; i++) {
            spawnParticle();
        }
    }
    
    void spawnParticle() override {
        Particle p;
        p.position = origin + glm::vec3(
            glm::linearRand(-originVariation.x, originVariation.x),
            glm::linearRand(-originVariation.y, originVariation.y),
            glm::linearRand(-originVariation.z, originVariation.z)
        );
        
        // Wave-like motion
        float angle = glm::linearRand(0.0f, glm::two_pi<float>());
        p.velocity = glm::vec3(
            sin(angle) * 0.3f, // Sideways wave motion
            glm::linearRand(-1.0f, 0.0f),
            cos(angle) * 0.3f + glm::linearRand(0.0f, 2.0f) // Forward with wave motion
        );
        
        // More greenish-blue color for waterfall
        float greenVariation = glm::linearRand(0.6f, 0.8f);
        float blueVariation = glm::linearRand(0.7f, 0.9f);
        p.color = glm::vec3(0.2f, greenVariation, blueVariation);
        
        // Smaller size
        p.size = glm::linearRand(0.3f, 0.6f);
        p.maxLife = p.life = glm::linearRand(2.0f, 4.0f);
        particles.push_back(p);
    }
};

// Fire system - dispersed wave-like particles without green
class FireSystem : public ParticleSystem {
public:
    FireSystem() : ParticleSystem(800, glm::vec3(0.0f, 0.0f, -2.0f), 
                                 glm::vec3(1.0f, 0.1f, 1.0f), 
                                 glm::vec3(0.0f, 2.0f, 0.0f), 0.0005f) {
        // Pre-populate with some particles
        for (int i = 0; i < maxParticles / 4; i++) {
            spawnParticle();
        }
    }
    
    void spawnParticle() override {
        Particle p;
        // Wider origin variation for more dispersion
        p.position = origin + glm::vec3(
            glm::linearRand(-originVariation.x * 1.5f, originVariation.x * 1.5f),
            glm::linearRand(-originVariation.y, originVariation.y),
            glm::linearRand(-originVariation.z * 1.5f, originVariation.z * 1.5f)
        );
        
        // More dispersed movement away from center
        float angle = glm::linearRand(0.0f, glm::two_pi<float>());
        float dispersionForce = glm::linearRand(0.5f, 1.5f);
        p.velocity = glm::vec3(
            cos(angle) * dispersionForce,
            glm::linearRand(0.5f, 2.0f),
            sin(angle) * dispersionForce
        );
        
        // Fire colors without green (red to orange to yellow)
        float t = glm::linearRand(0.0f, 1.0f);
        if (t < 0.6f) {
            // More red particles
            p.color = glm::vec3(1.0f, glm::linearRand(0.1f, 0.3f), 0.0f);
        } else {
            // Some orange-yellow particles
            p.color = glm::vec3(1.0f, glm::linearRand(0.4f, 0.7f), 0.0f);
        }
        
        // Smaller size for less defined shape
        p.size = glm::linearRand(0.5f, 1.0f);
        p.maxLife = p.life = glm::linearRand(0.5f, 1.5f);
        particles.push_back(p);
    }
};

// Dust system - smaller air-like particles
class DustSystem : public ParticleSystem {
public:
    DustSystem() : ParticleSystem(600, glm::vec3(0.0f, 0.1f, 0.0f), 
                                 glm::vec3(10.0f, 0.1f, 10.0f), 
                                 glm::vec3(0.0f, 0.05f, 0.0f), 0.01f) {
        // Pre-populate with some particles
        for (int i = 0; i < maxParticles / 4; i++) {
            spawnParticle();
        }
    }
    
    void spawnParticle() override {
        Particle p;
        p.position = origin + glm::vec3(
            glm::linearRand(-originVariation.x, originVariation.x),
            glm::linearRand(-originVariation.y, originVariation.y),
            glm::linearRand(-originVariation.z, originVariation.z)
        );
        
        // Air-like movement - more random and floating
        float angle = glm::linearRand(0.0f, glm::two_pi<float>());
        float speed = glm::linearRand(0.05f, 0.2f); // Slower for air-like behavior
        p.velocity = glm::vec3(
            cos(angle) * speed,
            glm::linearRand(-0.05f, 0.1f), // Slight up/down drift
            sin(angle) * speed
        );
        
        // Tan/brown color for dust with variation
        float brownVariation = glm::linearRand(0.8f, 1.0f);
        p.color = glm::vec3(0.9f * brownVariation, 0.7f * brownVariation, 0.5f * brownVariation);
        
        // Much smaller size
        p.size = glm::linearRand(0.2f, 0.4f);
        
        // Longer life for floating effect
        p.maxLife = p.life = glm::linearRand(5.0f, 10.0f);
        particles.push_back(p);
    }
};

// Coffee ceremony smoke - two thin zigzag/wave-form smoke lines from a single source
class CoffeeCeremonySystem : public ParticleSystem {
private:
    // Position of the coffee pot (jebena)
    glm::vec3 jebenaPosition;
    
public:
    CoffeeCeremonySystem() : ParticleSystem(500, glm::vec3(0.0f, 0.2f, -2.5f), 
                                           glm::vec3(0.05f, 0.02f, 0.05f), 
                                           glm::vec3(0.0f, 0.4f, 0.0f), 0.008f) {
        // Set the coffee pot position slightly above ground
        jebenaPosition = glm::vec3(0.0f, 0.2f, -2.5f);
        origin = jebenaPosition;
        
        // Pre-populate with some particles
        for (int i = 0; i < maxParticles / 4; i++) {
            spawnParticle();
        }
    }
    
    void spawnParticle() override {
        Particle p;
        
        // Start exactly at the spout of the coffee pot with minimal variation
        p.position = origin + glm::vec3(
            glm::linearRand(-0.03f, 0.03f), // Tiny variation at source
            glm::linearRand(0.0f, 0.03f),   // Just above the pot
            glm::linearRand(-0.03f, 0.03f)  // Tiny variation at source
        );
        
        // Determine which of the two streams this particle will join
        bool leftStream = (rand() % 2 == 0);
        
        // Initial velocity with slight angle to separate into two streams
        float angle = leftStream ? -0.2f : 0.2f;
        
        p.velocity = glm::vec3(
            angle,                        // Initial angle for stream separation
            glm::linearRand(0.7f, 0.9f),  // Upward
            0.0f                          // No initial z velocity
        );
        
        // Store which stream this particle belongs to
        p.userData = leftStream ? 0.0f : 1.0f;
        
        // Smoke color - slightly darker at the base, lighter as it rises
        float grayValue = glm::linearRand(0.75f, 0.85f);
        p.color = glm::vec3(grayValue, grayValue, grayValue);
        
        // Very thin and long particles
        p.size = glm::linearRand(0.15f, 0.25f);
        p.maxLife = p.life = glm::linearRand(4.0f, 6.0f); // Longer life for taller streams
        particles.push_back(p);
    }
    
    void update(float dt) override {
        if (!isActive) return;
        
        // Update existing particles
        for (auto it = particles.begin(); it != particles.end();) {
            it->life -= dt;
            if (it->life <= 0.0f) {
                it = particles.erase(it);
            } else {
                // Apply gravity (reduced for smoke)
                it->velocity += gravity * dt * 0.7f;
                
                // Apply stream-specific behavior
                bool leftStream = (it->userData < 0.5f);
                float time = glfwGetTime();
                
                // Calculate height from source for wave amplitude
                float heightRatio = it->position.y - jebenaPosition.y;
                
                // Create zigzag/wave pattern that increases with height
                // Different frequencies for each stream
                float waveFreq = leftStream ? 2.5f : 3.2f;
                
                // Wave amplitude increases with height
                float waveAmp = 0.2f * std::min(1.0f, heightRatio / 2.0f);
                
                // Calculate zigzag direction based on time and height
                // This creates a zigzag pattern that changes direction periodically
                float zigzagX = sin(time * waveFreq + heightRatio * 2.0f) * waveAmp;
                float zigzagZ = cos(time * waveFreq * 0.7f + heightRatio * 1.5f) * waveAmp * 0.5f;
                
                // Apply zigzag motion
                glm::vec3 zigzagMotion = glm::vec3(zigzagX, 0.0f, zigzagZ) * dt;
                
                // Base path for each stream (diverging slightly)
                float basePathX = (leftStream ? -0.2f : 0.2f) * heightRatio;
                
                // Force to keep particles following the zigzag path of their stream
                float pathForce = 0.3f * dt;
                float targetX = jebenaPosition.x + basePathX + zigzagX * 3.0f;
                float targetZ = jebenaPosition.z + zigzagZ * 3.0f;
                
                glm::vec3 pathCorrection = glm::vec3(
                    (targetX - it->position.x) * pathForce,
                    0.0f,
                    (targetZ - it->position.z) * pathForce
                );
                
                // Update position with velocity and zigzag motion
                it->position += it->velocity * dt + zigzagMotion + pathCorrection;
                
                // Gradually lighten the color as the smoke rises
                float heightFactor = std::min(1.0f, heightRatio / 3.0f);
                float grayValue = glm::mix(0.8f, 0.95f, heightFactor);
                it->color = glm::vec3(grayValue, grayValue, grayValue);
                
                ++it;
            }
        }
        
        // Spawn new particles based on rate and max count
        timeSinceLastSpawn += dt;
        while (timeSinceLastSpawn >= spawnRate && particles.size() < maxParticles) {
            spawnParticle();
            timeSinceLastSpawn -= spawnRate;
        }
    }
    
    // Draw the coffee pot (jebena)
    void renderJebena(unsigned int shaderProgram, unsigned int VAO) {
        glUseProgram(shaderProgram);
        
        // Set up view and projection matrices
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 projection = glm::perspective(glm::radians(60.0f), ASPECT_RATIO, 0.1f, 100.0f);
        
        // Set the matrices in the shader
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        
        // Draw the pot body (dark brown)
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, jebenaPosition + glm::vec3(0.0f, -0.1f, 0.0f));
        model = glm::scale(model, glm::vec3(0.3f, 0.2f, 0.3f));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
        
        glm::vec3 potColor = glm::vec3(0.3f, 0.15f, 0.05f); // Dark brown
        glUniform3fv(glGetUniformLocation(shaderProgram, "color"), 1, glm::value_ptr(potColor));
        glUniform1f(glGetUniformLocation(shaderProgram, "particleAlpha"), 1.0f);
        
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        
        // Draw the spout (slightly lighter brown)
        model = glm::mat4(1.0f);
        model = glm::translate(model, jebenaPosition + glm::vec3(0.0f, 0.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.1f, 0.1f, 0.1f));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
        
        glm::vec3 spoutColor = glm::vec3(0.4f, 0.2f, 0.1f); // Lighter brown
        glUniform3fv(glGetUniformLocation(shaderProgram, "color"), 1, glm::value_ptr(spoutColor));
        
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    }
};

// Function prototypes
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
unsigned int createParticleVAO();
void renderParticles(ParticleSystem& system, unsigned int shaderProgram, unsigned int VAO);
void renderGround(unsigned int shaderProgram, unsigned int VAO);
void renderSkybox(unsigned int shaderProgram, unsigned int VAO);
void renderHelp(GLFWwindow* window);

// Add these global variables for atmosphere colors
glm::vec3 skyTopColor = glm::vec3(0.3f, 0.5f, 0.9f);    // Deep blue at zenith
glm::vec3 skyHorizonColor = glm::vec3(0.7f, 0.8f, 1.0f); // Light blue at horizon
glm::vec3 groundColor = glm::vec3(0.2f, 0.5f, 0.2f);    // Natural green
glm::vec3 sunColor = glm::vec3(1.0f, 0.9f, 0.7f);       // Warm sunlight

// Basic shader creation function
unsigned int createShaderProgram() {
    const char* vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        uniform vec3 particlePos;
        uniform float particleSize;
        
        out vec2 TexCoord;
        
        void main() {
            // Billboard effect - always face camera
            vec3 position = particlePos + aPos * particleSize;
            gl_Position = projection * view * vec4(position, 1.0);
            
            // Pass texture coordinates
            TexCoord = aPos.xy + vec2(0.5, 0.5);
        }
    )";
    
    const char* fragmentShaderSource = R"(
        #version 330 core
        in vec2 TexCoord;
        
        uniform vec3 particleColor;
        uniform float particleAlpha;
        uniform vec3 color;
        
        out vec4 FragColor;
        
        void main() {
            // Simple circular particle
            float dist = length(TexCoord - vec2(0.5, 0.5));
            if (dist > 0.5) discard; // Discard pixels outside circle
            
            // Fade out towards edges
            float alpha = particleAlpha * smoothstep(0.5, 0.2, dist);
            
            // Use either particle color or uniform color
            vec3 finalColor = length(particleColor) > 0.0 ? particleColor : color;
            
            FragColor = vec4(finalColor, alpha);
        }
    )";
    
    // Compile vertex shader
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    
    // Check for errors
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    
    // Compile fragment shader
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    
    // Check for errors
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    
    // Create shader program
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    
    // Check for errors
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    
    // Clean up
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return shaderProgram;
}

int main()
{
    // Initialize GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create window with a more descriptive title
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, 
        "Ethiopian Natural Phenomena - Press F1 to release mouse, ESC to exit", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);

    // Add a startup message for beginners
    std::cout << "\n=== ETHIOPIAN NATURAL PHENOMENA SIMULATION ===\n" << std::endl;
    std::cout << "Starting up... If this is your first time, read the instructions below!" << std::endl;

    // Capture mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Configure global OpenGL state
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Create particle systems
    RainSystem rainSystem;
    WaterfallSystem waterfallSystem;
    FireSystem fireSystem;
    DustSystem dustSystem;
    CoffeeCeremonySystem coffeeSystem;
    
    // Create VAOs
    unsigned int particleVAO = createParticleVAO();
    unsigned int groundVAO = createParticleVAO(); // Reuse for simplicity
    unsigned int skyboxVAO = createParticleVAO(); // Reuse for simplicity
    
    // Create shader program (simplified for this example)
    unsigned int shaderProgram = createShaderProgram();
    
    // Main rendering loop
    while (!glfwWindowShouldClose(window))
    {
        // Calculate delta time
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        
        // Process input
        processInput(window);
        
        // Clear the screen
        glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Update and render the current particle system
        switch (currentSystem) {
            case RAIN:
                rainSystem.update(deltaTime);
                renderParticles(rainSystem, shaderProgram, particleVAO);
                break;
            case WATERFALL:
                waterfallSystem.update(deltaTime);
                renderParticles(waterfallSystem, shaderProgram, particleVAO);
                break;
            case FIRE:
                fireSystem.update(deltaTime);
                renderParticles(fireSystem, shaderProgram, particleVAO);
                break;
            case DUST:
                dustSystem.update(deltaTime);
                renderParticles(dustSystem, shaderProgram, particleVAO);
                break;
            case COFFEE_CEREMONY:
                coffeeSystem.update(deltaTime);
                renderParticles(coffeeSystem, shaderProgram, particleVAO);
                break;
        }
        
        // Render ground and skybox
        renderGround(shaderProgram, groundVAO);
        renderSkybox(shaderProgram, skyboxVAO);
        
        // Show help text if enabled
        if (showHelp) {
            renderHelp(window);
        }
        
        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    // Clean up
    glDeleteVertexArrays(1, &particleVAO);
    glDeleteVertexArrays(1, &groundVAO);
    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteProgram(shaderProgram);
    
    glfwTerminate();
    return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    // Make sure that when pitch is out of bounds, screen doesn't get flipped
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

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    cameraSpeed += yoffset * 0.01f;
    if (cameraSpeed < 0.01f)
        cameraSpeed = 0.01f;
    if (cameraSpeed > 0.2f)
        cameraSpeed = 0.2f;
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float speed = cameraSpeed * deltaTime * 100.0f;
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
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS)
    {
        if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            firstMouse = true;
        }
        else
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            firstMouse = true;
        }
    }
    
    // Toggle help display
    if (key == GLFW_KEY_H && action == GLFW_PRESS)
        showHelp = !showHelp;
    
    // Switch between particle systems
    if (key == GLFW_KEY_1 && action == GLFW_PRESS)
        currentSystem = RAIN;
    if (key == GLFW_KEY_2 && action == GLFW_PRESS)
        currentSystem = WATERFALL;
    if (key == GLFW_KEY_3 && action == GLFW_PRESS)
        currentSystem = FIRE;
    if (key == GLFW_KEY_4 && action == GLFW_PRESS)
        currentSystem = DUST;
    if (key == GLFW_KEY_5 && action == GLFW_PRESS)
        currentSystem = COFFEE_CEREMONY;
}

unsigned int createParticleVAO() {
    float vertices[] = {
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
         0.5f,  0.5f, 0.0f,
        -0.5f,  0.5f, 0.0f
    };
    
    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    
    glBindVertexArray(VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    return VAO;
}

void renderParticles(ParticleSystem& system, unsigned int shaderProgram, unsigned int VAO)
{
    // Use the shader program
    glUseProgram(shaderProgram);
    
    // Set up view and projection matrices
    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    glm::mat4 projection = glm::perspective(glm::radians(60.0f), ASPECT_RATIO, 0.1f, 100.0f);
    
    // Set the matrices in the shader
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    
    // Special rendering for each particle type
    bool isRain = dynamic_cast<RainSystem*>(&system) != nullptr;
    bool isWaterfall = dynamic_cast<WaterfallSystem*>(&system) != nullptr;
    bool isFire = dynamic_cast<FireSystem*>(&system) != nullptr;
    bool isDust = dynamic_cast<DustSystem*>(&system) != nullptr;
    bool isCoffee = dynamic_cast<CoffeeCeremonySystem*>(&system) != nullptr;
    
    // For each particle, render a billboard facing the camera
    for (const auto& particle : system.particles) {
        // Calculate alpha based on remaining life
        float alpha = particle.life / particle.maxLife;
        
        // Set uniforms for position, color, size, etc.
        glUniform3fv(glGetUniformLocation(shaderProgram, "particlePos"), 1, glm::value_ptr(particle.position));
        glUniform3fv(glGetUniformLocation(shaderProgram, "particleColor"), 1, glm::value_ptr(particle.color));
        
        if (isCoffee) {
            // Very thin, long smoke particles
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, particle.position);
            
            // Make particles extremely thin and tall
            float heightFactor = std::min(1.0f, (particle.position.y - system.origin.y) / 4.0f);
            
            // Width stays very thin
            float width = particle.size * 0.1f;
            
            // Height increases with altitude
            float height = particle.size * (1.5f + heightFactor * 2.0f);
            
            // Rotate particles to follow the zigzag pattern
            // Calculate rotation based on the particle's velocity or position change
            float time = glfwGetTime();
            bool leftStream = (particle.userData < 0.5f);
            float waveFreq = leftStream ? 2.5f : 3.2f;
            float angle = sin(time * waveFreq + particle.position.y * 2.0f) * 0.3f;
            
            // Apply rotation to follow zigzag pattern
            model = glm::rotate(model, angle, glm::vec3(0.0f, 0.0f, 1.0f));
            
            // Apply scaling
            model = glm::scale(model, glm::vec3(width, height, width));
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
            
            // Particles become more transparent as they rise
            alpha *= (1.0f - heightFactor * 0.6f);
        }
        else if (isRain) {
            // Compressed rain drops (taller than wide)
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, particle.position);
            model = glm::scale(model, glm::vec3(particle.size * 0.3f, particle.size * 2.0f, particle.size * 0.3f));
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
        } 
        else if (isWaterfall) {
            // Wave-form water particles
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, particle.position);
            // Add slight wave effect to size based on time
            float waveEffect = sin(glfwGetTime() * 5.0f + particle.position.x * 2.0f) * 0.2f + 1.0f;
            model = glm::scale(model, glm::vec3(particle.size * waveEffect, particle.size, particle.size));
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
        }
        else {
            // Default size for other particles
            glUniform1f(glGetUniformLocation(shaderProgram, "particleSize"), particle.size);
        }
        
        glUniform1f(glGetUniformLocation(shaderProgram, "particleAlpha"), alpha);
        
        // Draw the particle
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    }
}

void renderGround(unsigned int shaderProgram, unsigned int VAO)
{
    // Ground rendering with more natural features
    glUseProgram(shaderProgram);
    
    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    glm::mat4 projection = glm::perspective(glm::radians(60.0f), ASPECT_RATIO, 0.1f, 100.0f);
    
    // Set the matrices in the shader
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    
    // Main ground plane
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, -1.0f, 0.0f));
    model = glm::scale(model, glm::vec3(40.0f, 1.0f, 40.0f));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform3fv(glGetUniformLocation(shaderProgram, "color"), 1, glm::value_ptr(groundColor));
    
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    
    // Add some hills/mountains in the distance with lighter green
    for (int i = 0; i < 5; i++) {
        model = glm::mat4(1.0f);
        float xPos = -20.0f + i * 10.0f;
        model = glm::translate(model, glm::vec3(xPos, -0.5f, -15.0f));
        model = glm::scale(model, glm::vec3(5.0f, 3.0f, 5.0f));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
        
        // Slightly lighter green for mountains (multiply by 1.2 instead of 0.8)
        glm::vec3 mountainColor = groundColor * 1.2f;
        // Ensure we don't exceed 1.0 in any component
        mountainColor = glm::clamp(mountainColor, glm::vec3(0.0f), glm::vec3(1.0f));
        glUniform3fv(glGetUniformLocation(shaderProgram, "color"), 1, glm::value_ptr(mountainColor));
        
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    }
    
    // Add a horizon line (slightly transparent white) between earth and sky
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 0.0f, -20.0f));
    model = glm::scale(model, glm::vec3(40.0f, 0.2f, 1.0f));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    
    // Set color to slightly transparent white
    glUniform3fv(glGetUniformLocation(shaderProgram, "color"), 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 1.0f)));
    glUniform1f(glGetUniformLocation(shaderProgram, "particleAlpha"), 0.7f); // Set transparency
    
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void renderSkybox(unsigned int shaderProgram, unsigned int VAO)
{
    // Skybox rendering with gradient
    glUseProgram(shaderProgram);
    
    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    glm::mat4 projection = glm::perspective(glm::radians(60.0f), ASPECT_RATIO, 0.1f, 100.0f);
    
    // Set the matrices in the shader
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    
    // Top dome of sky (deep blue)
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 10.0f, 0.0f));
    model = glm::scale(model, glm::vec3(40.0f, 20.0f, 40.0f));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform3fv(glGetUniformLocation(shaderProgram, "color"), 1, glm::value_ptr(skyTopColor));
    
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    
    // Horizon (lighter blue) - position it slightly higher to create a clear separation
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 0.5f, 0.0f)); // Moved up from 0.0 to 0.5
    model = glm::scale(model, glm::vec3(40.0f, 10.0f, 40.0f));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform3fv(glGetUniformLocation(shaderProgram, "color"), 1, glm::value_ptr(skyHorizonColor));
    
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    
    // Add a sun
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(20.0f, 15.0f, -20.0f));
    model = glm::scale(model, glm::vec3(5.0f, 5.0f, 5.0f));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform3fv(glGetUniformLocation(shaderProgram, "color"), 1, glm::value_ptr(sunColor));
    
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    
    // Add some clouds near the horizon
    glm::vec3 cloudColor = glm::vec3(1.0f, 1.0f, 1.0f);
    for (int i = 0; i < 7; i++) {
        model = glm::mat4(1.0f);
        float xPos = -15.0f + i * 5.0f;
        model = glm::translate(model, glm::vec3(xPos, 2.0f, -15.0f));
        model = glm::scale(model, glm::vec3(3.0f, 1.0f, 1.0f));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniform3fv(glGetUniformLocation(shaderProgram, "color"), 1, glm::value_ptr(cloudColor));
        glUniform1f(glGetUniformLocation(shaderProgram, "particleAlpha"), 0.8f);
        
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    }
}

void renderHelp(GLFWwindow* window)
{
    // In a real implementation, this would render text on screen
    // For this simplified example, we'll just print to console
    static bool helpPrinted = false;
    
    if (!helpPrinted) {
        std::cout << "\n=== CONTROLS ===\n" << std::endl;
        std::cout << "WASD - Move camera" << std::endl;
        std::cout << "SPACE/CTRL - Move up/down" << std::endl;
        std::cout << "Mouse - Look around" << std::endl;
        std::cout << "F1 - Toggle mouse capture" << std::endl;
        std::cout << "H - Toggle help display" << std::endl;
        std::cout << "1-5 - Switch particle systems:" << std::endl;
        std::cout << "  1 - Rain (Ethiopian highlands)" << std::endl;
        std::cout << "  2 - Waterfall (Blue Nile Falls)" << std::endl;
        std::cout << "  3 - Fire (Traditional cooking fire)" << std::endl;
        std::cout << "  4 - Dust (Dry season dust)" << std::endl;
        std::cout << "  5 - Coffee Ceremony smoke" << std::endl;
        std::cout << "ESC - Exit" << std::endl;
        
        helpPrinted = true;
    }
}
































