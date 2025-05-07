#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/easing.hpp>
#include <glm/gtc/random.hpp>
#include <iostream>
#include <vector>
#include <cmath>

// Window settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;
const float ASPECT_RATIO = (float)SCR_WIDTH / (float)SCR_HEIGHT;

// Animation settings
const float BOUNCE_DURATION = 1.0f; // 1 second per bounce cycle
const int NUM_BALLS = 10;

// Environment settings
const float GROUND_HEIGHT = -0.8f;
const float GROUND_WIDTH = 2.0f * ASPECT_RATIO;
const float SKY_HEIGHT = 1.6f;

// Ball properties
struct Ball {
    glm::vec2 position;
    glm::vec2 size;
    glm::vec3 color;
    float phase; // Offset for the bounce animation (0.0 to 1.0)
    float speed; // Multiplier for animation speed
    float bounceHeight; // Maximum height of bounce
};

// Environment objects
struct Environment {
    // Ground (grass)
    glm::vec2 groundPos;
    glm::vec2 groundSize;
    glm::vec3 groundColor;
    
    // Sky
    glm::vec2 skyPos;
    glm::vec2 skySize;
    glm::vec3 skyTopColor;
    glm::vec3 skyBottomColor;
    
    // Sun
    glm::vec2 sunPos;
    float sunSize;
    glm::vec3 sunColor;
    
    // Clouds
    std::vector<glm::vec2> cloudPositions;
    std::vector<float> cloudSizes;
};

// Shader sources
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
uniform mat4 model;
uniform mat4 projection;

void main()
{
    gl_Position = projection * model * vec4(aPos, 0.0, 1.0);
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;
uniform vec3 color;
uniform bool isGradient;
uniform vec3 bottomColor;

void main()
{
    if (isGradient) {
        // Calculate gradient based on y position (0 at bottom, 1 at top)
        float t = gl_FragCoord.y / 600.0;
        vec3 gradientColor = mix(bottomColor, color, t);
        FragColor = vec4(gradientColor, 1.0);
    } else {
        FragColor = vec4(color, 1.0);
    }
}
)";

// Function prototypes
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void renderBalls(std::vector<Ball>& balls, unsigned int shaderProgram, unsigned int VAO, float time);
void renderEnvironment(Environment& env, unsigned int shaderProgram, unsigned int quadVAO, unsigned int circleVAO);
void initEnvironment(Environment& env);

int main()
{
    // Initialize GLFW
    if (!glfwInit()) {
        std::cout << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    
    // Configure GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    // Create window
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Earth-like Bouncing Balls", NULL, NULL);
    if (!window) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    
    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    
    // Compile vertex shader
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    
    // Check for vertex shader compile errors
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
    
    // Check for fragment shader compile errors
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
    
    // Delete shaders as they're linked into our program now and no longer necessary
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    // Set up vertex data for a circle approximation (using a 32-sided polygon)
    const int segments = 32;
    std::vector<float> circleVertices;
    
    // Center vertex
    circleVertices.push_back(0.0f);
    circleVertices.push_back(0.0f);
    
    // Circle vertices
    for (int i = 0; i <= segments; i++) {
        float angle = 2.0f * 3.14159f * i / segments;
        float x = cos(angle);
        float y = sin(angle);
        circleVertices.push_back(x);
        circleVertices.push_back(y);
    }
    
    // Create indices for triangle fan
    std::vector<unsigned int> circleIndices;
    for (int i = 1; i <= segments; i++) {
        circleIndices.push_back(0);  // Center
        circleIndices.push_back(i);  // Current vertex
        circleIndices.push_back(i + 1 <= segments ? i + 1 : 1);  // Next vertex (wrap around)
    }
    
    // Set up vertex data for a quad (rectangle)
    float quadVertices[] = {
        // positions
        -0.5f, -0.5f,  // bottom left
         0.5f, -0.5f,  // bottom right
         0.5f,  0.5f,  // top right
        -0.5f,  0.5f   // top left
    };
    
    unsigned int quadIndices[] = {
        0, 1, 2,  // first triangle
        0, 2, 3   // second triangle
    };
    
    // Create buffers/arrays for circle
    unsigned int circleVBO, circleVAO, circleEBO;
    glGenVertexArrays(1, &circleVAO);
    glGenBuffers(1, &circleVBO);
    glGenBuffers(1, &circleEBO);
    
    // Bind the Circle Vertex Array Object
    glBindVertexArray(circleVAO);
    
    // Bind and set vertex buffer
    glBindBuffer(GL_ARRAY_BUFFER, circleVBO);
    glBufferData(GL_ARRAY_BUFFER, circleVertices.size() * sizeof(float), circleVertices.data(), GL_STATIC_DRAW);
    
    // Bind and set element buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, circleEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, circleIndices.size() * sizeof(unsigned int), circleIndices.data(), GL_STATIC_DRAW);
    
    // Configure vertex attributes
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Create buffers/arrays for quad
    unsigned int quadVBO, quadVAO, quadEBO;
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glGenBuffers(1, &quadEBO);
    
    // Bind the Quad Vertex Array Object
    glBindVertexArray(quadVAO);
    
    // Bind and set vertex buffer
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    
    // Bind and set element buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIndices), quadIndices, GL_STATIC_DRAW);
    
    // Configure vertex attributes
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Unbind buffers
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    // Enable alpha blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Initialize environment
    Environment environment;
    initEnvironment(environment);
    
    // Initialize balls with random properties
    std::vector<Ball> balls;
    for (int i = 0; i < NUM_BALLS; i++) {
        Ball ball;
        
        // Random horizontal position within screen bounds
        ball.position.x = glm::linearRand(-0.9f * ASPECT_RATIO, 0.9f * ASPECT_RATIO);
        ball.position.y = GROUND_HEIGHT; // Start at ground level
        
        // Random size
        float size = glm::linearRand(0.05f, 0.12f);
        ball.size = glm::vec2(size, size);
        
        // Random color (Earth-like objects: rocks, fruits, etc.)
        float hue = glm::linearRand(0.0f, 1.0f);
        if (hue < 0.3f) {
            // Rocks/stones (gray/brown)
            ball.color = glm::vec3(
                glm::linearRand(0.4f, 0.6f),
                glm::linearRand(0.3f, 0.5f),
                glm::linearRand(0.2f, 0.4f)
            );
        } else if (hue < 0.6f) {
            // Fruits (red/orange/yellow)
            ball.color = glm::vec3(
                glm::linearRand(0.7f, 1.0f),
                glm::linearRand(0.3f, 0.7f),
                glm::linearRand(0.0f, 0.3f)
            );
        } else {
            // Misc objects (blue/purple/green)
            ball.color = glm::vec3(
                glm::linearRand(0.0f, 0.5f),
                glm::linearRand(0.4f, 0.8f),
                glm::linearRand(0.5f, 1.0f)
            );
        }
        
        // Random phase offset (0.0 to 1.0)
        ball.phase = glm::linearRand(0.0f, 1.0f);
        
        // Random speed multiplier
        ball.speed = glm::linearRand(0.7f, 1.3f);
        
        // Random bounce height
        ball.bounceHeight = glm::linearRand(0.3f, 0.7f);
        
        balls.push_back(ball);
    }
    
    // Render loop
    while (!glfwWindowShouldClose(window)) {
        // Process input
        processInput(window);
        
        // Get current time for animation
        float time = static_cast<float>(glfwGetTime());
        
        // Render
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        // Use shader program
        glUseProgram(shaderProgram);
        
        // Set up orthographic projection
        glm::mat4 projection = glm::ortho(-ASPECT_RATIO, ASPECT_RATIO, -1.0f, 1.0f, -1.0f, 1.0f);
        unsigned int projectionLoc = glGetUniformLocation(shaderProgram, "projection");
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
        
        // Render environment
        renderEnvironment(environment, shaderProgram, quadVAO, circleVAO);
        
        // Render all balls
        renderBalls(balls, shaderProgram, circleVAO, time);
        
        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    // Deallocate resources
    glDeleteVertexArrays(1, &circleVAO);
    glDeleteBuffers(1, &circleVBO);
    glDeleteBuffers(1, &circleEBO);
    glDeleteVertexArrays(1, &quadVAO);
    glDeleteBuffers(1, &quadVBO);
    glDeleteBuffers(1, &quadEBO);
    glDeleteProgram(shaderProgram);
    
    // Clean up
    glfwTerminate();
    return 0;
}

void initEnvironment(Environment& env)
{
    // Ground (grass)
    env.groundPos = glm::vec2(0.0f, GROUND_HEIGHT - 0.1f);
    env.groundSize = glm::vec2(GROUND_WIDTH, 0.2f);
    env.groundColor = glm::vec3(0.2f, 0.6f, 0.3f); // Green grass
    
    // Sky
    env.skyPos = glm::vec2(0.0f, 0.0f);
    env.skySize = glm::vec2(GROUND_WIDTH, SKY_HEIGHT);
    env.skyTopColor = glm::vec3(0.3f, 0.5f, 0.9f); // Blue sky
    env.skyBottomColor = glm::vec3(0.7f, 0.8f, 1.0f); // Lighter at horizon
    
    // Sun
    env.sunPos = glm::vec2(0.7f * ASPECT_RATIO, 0.7f);
    env.sunSize = 0.15f;
    env.sunColor = glm::vec3(1.0f, 0.9f, 0.6f); // Yellow-ish sun
    
    // Clouds
    for (int i = 0; i < 5; i++) {
        float x = glm::linearRand(-0.9f * ASPECT_RATIO, 0.9f * ASPECT_RATIO);
        float y = glm::linearRand(0.2f, 0.8f);
        env.cloudPositions.push_back(glm::vec2(x, y));
        env.cloudSizes.push_back(glm::linearRand(0.1f, 0.2f));
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

void renderEnvironment(Environment& env, unsigned int shaderProgram, unsigned int quadVAO, unsigned int circleVAO)
{
    unsigned int modelLoc = glGetUniformLocation(shaderProgram, "model");
    unsigned int colorLoc = glGetUniformLocation(shaderProgram, "color");
    unsigned int isGradientLoc = glGetUniformLocation(shaderProgram, "isGradient");
    unsigned int bottomColorLoc = glGetUniformLocation(shaderProgram, "bottomColor");
    
    // Render sky with gradient
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(env.skyPos, 0.0f));
    model = glm::scale(model, glm::vec3(env.skySize, 1.0f));
    
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniform3fv(colorLoc, 1, glm::value_ptr(env.skyTopColor));
    glUniform1i(isGradientLoc, true);
    glUniform3fv(bottomColorLoc, 1, glm::value_ptr(env.skyBottomColor));
    
    glBindVertexArray(quadVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    
    // Render sun
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(env.sunPos, 0.0f));
    model = glm::scale(model, glm::vec3(env.sunSize, env.sunSize, 1.0f));
    
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniform3fv(colorLoc, 1, glm::value_ptr(env.sunColor));
    glUniform1i(isGradientLoc, false);
    
    glBindVertexArray(circleVAO);
    glDrawElements(GL_TRIANGLES, 32 * 3, GL_UNSIGNED_INT, 0);
    
    // Render clouds
    glm::vec3 cloudColor = glm::vec3(1.0f, 1.0f, 1.0f);
    for (size_t i = 0; i < env.cloudPositions.size(); i++) {
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(env.cloudPositions[i], 0.0f));
        model = glm::scale(model, glm::vec3(env.cloudSizes[i] * 2.0f, env.cloudSizes[i], 1.0f));
        
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniform3fv(colorLoc, 1, glm::value_ptr(cloudColor));
        
        glBindVertexArray(circleVAO);
        glDrawElements(GL_TRIANGLES, 32 * 3, GL_UNSIGNED_INT, 0);
    }
    
    // Render ground (grass)
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(env.groundPos, 0.0f));
    model = glm::scale(model, glm::vec3(env.groundSize, 1.0f));
    
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniform3fv(colorLoc, 1, glm::value_ptr(env.groundColor));
    glUniform1i(isGradientLoc, false);
    
    glBindVertexArray(quadVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void renderBalls(std::vector<Ball>& balls, unsigned int shaderProgram, unsigned int VAO, float time)
{
    unsigned int modelLoc = glGetUniformLocation(shaderProgram, "model");
    unsigned int colorLoc = glGetUniformLocation(shaderProgram, "color");
    unsigned int isGradientLoc = glGetUniformLocation(shaderProgram, "isGradient");
    
    glUniform1i(isGradientLoc, false);
    
    for (auto& ball : balls) {
        // Calculate animation time with phase offset and speed
        float animTime = fmodf((time * ball.speed + ball.phase), BOUNCE_DURATION) / BOUNCE_DURATION;
        
        // Use bounce easing function for y-position
        float bounceHeight = glm::bounceEaseOut(animTime) * ball.bounceHeight;
        
        // Create model matrix
        glm::mat4 model = glm::mat4(1.0f);
        
        // Apply position with bounce effect
        glm::vec2 animatedPosition = ball.position;
        animatedPosition.y = GROUND_HEIGHT + bounceHeight;
        
        model = glm::translate(model, glm::vec3(animatedPosition, 0.0f));
        
        // Apply size
        model = glm::scale(model, glm::vec3(ball.size, 1.0f));
        
        // Apply squash and stretch effect
        float squashFactor = 1.0f - (bounceHeight / 2.0f);
        float stretchFactor = 1.0f + (bounceHeight / 4.0f);
        
        // More squash at the bottom, more stretch at the top
        if (animTime < 0.5f) {
            // Going up - stretch vertically
            model = glm::scale(model, glm::vec3(1.0f / stretchFactor, stretchFactor, 1.0f));
        } else {
            // Going down - squash horizontally at the bottom
            float impact = 1.0f - (animTime - 0.5f) * 2.0f; // 1.0 at top of fall, 0.0 at bottom
            if (impact < 0.2f) {
                // Apply squash only near the ground
                float squashAmount = (0.2f - impact) / 0.2f;
                model = glm::scale(model, glm::vec3(1.0f + squashAmount * 0.3f, 1.0f - squashAmount * 0.3f, 1.0f));
            }
        }
        
        // Add shadow
        if (bounceHeight < 0.4f) {
            // Draw shadow (darker and flatter as ball gets closer to ground)
            float shadowDistance = bounceHeight * 0.1f;
            float shadowSize = 1.0f - bounceHeight * 0.5f;
            
            glm::mat4 shadowModel = glm::mat4(1.0f);
            shadowModel = glm::translate(shadowModel, glm::vec3(ball.position.x + shadowDistance, GROUND_HEIGHT + 0.001f, 0.0f));
            shadowModel = glm::scale(shadowModel, glm::vec3(ball.size.x * shadowSize, ball.size.y * 0.2f, 1.0f));
            
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(shadowModel));
            glUniform3fv(colorLoc, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f, 0.0f)));
            
            glBindVertexArray(VAO);
            glDrawElements(GL_TRIANGLES, 32 * 3, GL_UNSIGNED_INT, 0);
        }
        
        // Set uniforms for ball
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniform3fv(colorLoc, 1, glm::value_ptr(ball.color));
        
        // Draw ball
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 32 * 3, GL_UNSIGNED_INT, 0);
    }
}
