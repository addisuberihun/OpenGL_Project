#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <iomanip>

// Game constants
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;
const float PADDLE_WIDTH = 0.2f;
const float PADDLE_HEIGHT = 0.03f;
const float BALL_SIZE = 0.02f;
const float BRICK_WIDTH = 0.1f;
const float BRICK_HEIGHT = 0.04f;
const int BRICK_ROWS = 5;
const int BRICK_COLS = 8;
const float BALL_VELOCITY = 0.01f;
const int WAIT_TIME_SECONDS = 240; // 4 minutes in seconds

// Game state
struct GameObject {
    glm::vec2 position;
    glm::vec2 size;
    glm::vec3 color;
    bool active = true;
};

struct GameState {
    GameObject paddle;
    GameObject ball;
    glm::vec2 ballVelocity;
    std::vector<GameObject> bricks;
    bool gameOver = false;
    int score = 0;
    bool gameStarted = false;
    double startTime = 0.0;
};

GameState gameState;

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

void main()
{
    FragColor = vec4(color, 1.0);
}
)";

// Function prototypes
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void initGame();
void updateGame(double currentTime);
void renderGame(unsigned int shaderProgram, unsigned int VAO);
bool checkCollision(const GameObject& a, const GameObject& b);
void resetBall();
void displayCountdown(double remainingTime);

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
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "2D Breakout", NULL, NULL);
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
    
    // Set up vertex data for a quad
    float vertices[] = {
        // positions
        -0.5f, -0.5f,  // bottom left
         0.5f, -0.5f,  // bottom right
         0.5f,  0.5f,  // top right
        -0.5f,  0.5f   // top left
    };
    
    unsigned int indices[] = {
        0, 1, 2,  // first triangle
        0, 2, 3   // second triangle
    };
    
    // Create buffers/arrays
    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    
    // Bind the Vertex Array Object first
    glBindVertexArray(VAO);
    
    // Bind and set vertex buffer(s)
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    // Bind and set element buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    
    // Configure vertex attributes
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Unbind VBO and VAO
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    // Initialize game state
    initGame();
    
    // Set start time
    gameState.startTime = glfwGetTime();
    
    std::cout << "Game will start in 4 minutes. Please wait..." << std::endl;
    
    // Render loop
    while (!glfwWindowShouldClose(window) && !gameState.gameOver) {
        // Process input
        processInput(window);
        
        // Get current time
        double currentTime = glfwGetTime();
        double elapsedTime = currentTime - gameState.startTime;
        
        // Check if waiting period is over
        if (elapsedTime >= WAIT_TIME_SECONDS) {
            if (!gameState.gameStarted) {
                std::cout << "Game started!" << std::endl;
                gameState.gameStarted = true;
            }
            
            // Update game state
            updateGame(currentTime);
        } else {
            // Display countdown
            displayCountdown(WAIT_TIME_SECONDS - elapsedTime);
        }
        
        // Render
        glClearColor(0.0f, 0.0f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        // Use shader program
        glUseProgram(shaderProgram);
        
        // Set up orthographic projection
        glm::mat4 projection = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);
        unsigned int projectionLoc = glGetUniformLocation(shaderProgram, "projection");
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
        
        // Only render game objects if game has started
        if (gameState.gameStarted) {
            renderGame(shaderProgram, VAO);
        }
        
        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    // Display final score
    if (gameState.gameStarted) {
        std::cout << "Game Over! Final Score: " << gameState.score << std::endl;
    } else {
        std::cout << "Game closed before starting." << std::endl;
    }
    
    // Deallocate resources
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);
    
    // Clean up
    glfwTerminate();
    return 0;
}

void displayCountdown(double remainingTime) {
    // Only update the display every second to avoid console spam
    static int lastSecond = -1;
    int currentSecond = static_cast<int>(remainingTime);
    
    if (currentSecond != lastSecond) {
        lastSecond = currentSecond;
        
        // Convert seconds to minutes and seconds
        int minutes = currentSecond / 60;
        int seconds = currentSecond % 60;
        
        // Clear the console line and display the countdown
        std::cout << "\rWaiting to start game: " << minutes << ":" 
                  << std::setfill('0') << std::setw(2) << seconds << std::flush;
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
    
    // Only process paddle movement if game has started
    if (gameState.gameStarted) {
        // Paddle movement
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
            gameState.paddle.position.x -= 0.03f;
            if (gameState.paddle.position.x - gameState.paddle.size.x / 2 < -1.0f)
                gameState.paddle.position.x = -1.0f + gameState.paddle.size.x / 2;
        }
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
            gameState.paddle.position.x += 0.03f;
            if (gameState.paddle.position.x + gameState.paddle.size.x / 2 > 1.0f)
                gameState.paddle.position.x = 1.0f - gameState.paddle.size.x / 2;
        }
    }
}

void initGame()
{
    // Initialize paddle
    gameState.paddle.position = glm::vec2(0.0f, -0.9f);
    gameState.paddle.size = glm::vec2(PADDLE_WIDTH, PADDLE_HEIGHT);
    gameState.paddle.color = glm::vec3(0.2f, 0.6f, 1.0f);
    
    // Initialize ball
    resetBall();
    
    // Initialize bricks
    gameState.bricks.clear();
    for (int row = 0; row < BRICK_ROWS; row++) {
        for (int col = 0; col < BRICK_COLS; col++) {
            GameObject brick;
            float x = -0.9f + col * (BRICK_WIDTH + 0.02f);
            float y = 0.7f - row * (BRICK_HEIGHT + 0.02f);
            brick.position = glm::vec2(x, y);
            brick.size = glm::vec2(BRICK_WIDTH, BRICK_HEIGHT);
            
            // Different colors for different rows
            switch (row) {
                case 0: brick.color = glm::vec3(1.0f, 0.2f, 0.2f); break; // Red
                case 1: brick.color = glm::vec3(1.0f, 0.6f, 0.2f); break; // Orange
                case 2: brick.color = glm::vec3(1.0f, 1.0f, 0.2f); break; // Yellow
                case 3: brick.color = glm::vec3(0.2f, 1.0f, 0.2f); break; // Green
                case 4: brick.color = glm::vec3(0.2f, 0.4f, 1.0f); break; // Blue
                default: brick.color = glm::vec3(1.0f, 1.0f, 1.0f); break; // White
            }
            
            gameState.bricks.push_back(brick);
        }
    }
    
    // Reset game state
    gameState.gameOver = false;
    gameState.score = 0;
    gameState.gameStarted = false;
}

void resetBall()
{
    gameState.ball.position = glm::vec2(0.0f, -0.7f);
    gameState.ball.size = glm::vec2(BALL_SIZE, BALL_SIZE);
    gameState.ball.color = glm::vec3(1.0f, 1.0f, 1.0f);
    
    // Random angle between -45 and 45 degrees
    float angle = ((rand() % 90) - 45) * 3.14159f / 180.0f;
    gameState.ballVelocity.x = BALL_VELOCITY * sin(angle);
    gameState.ballVelocity.y = BALL_VELOCITY;
}

void updateGame(double currentTime)
{
    // Update ball position
    gameState.ball.position += gameState.ballVelocity;
    
    // Check for collisions with walls
    if (gameState.ball.position.x - gameState.ball.size.x / 2 < -1.0f ||
        gameState.ball.position.x + gameState.ball.size.x / 2 > 1.0f) {
        gameState.ballVelocity.x = -gameState.ballVelocity.x;
    }
    if (gameState.ball.position.y + gameState.ball.size.y / 2 > 1.0f) {
        gameState.ballVelocity.y = -gameState.ballVelocity.y;
    }
    
    // Check for collision with paddle
    if (checkCollision(gameState.ball, gameState.paddle)) {
        // Calculate reflection angle based on where the ball hit the paddle
        float hitPos = (gameState.ball.position.x - gameState.paddle.position.x) / (gameState.paddle.size.x / 2);
        float angle = hitPos * 60.0f * 3.14159f / 180.0f; // Max 60 degree reflection
        
        float speed = glm::length(gameState.ballVelocity);
        gameState.ballVelocity.x = speed * sin(angle);
        gameState.ballVelocity.y = speed * cos(angle);
        
        // Ensure ball is moving upward
        if (gameState.ballVelocity.y < 0) {
            gameState.ballVelocity.y = -gameState.ballVelocity.y;
        }
    }
    
    // Check for collisions with bricks
    for (auto& brick : gameState.bricks) {
        if (brick.active && checkCollision(gameState.ball, brick)) {
            brick.active = false;
            gameState.ballVelocity.y = -gameState.ballVelocity.y;
            gameState.score += 10;
            break;
        }
    }
    
    // Check if ball fell below screen
    if (gameState.ball.position.y - gameState.ball.size.y / 2 < -1.0f) {
        gameState.gameOver = true;
    }
    
    // Check if all bricks are destroyed
    bool allBricksDestroyed = true;
    for (const auto& brick : gameState.bricks) {
        if (brick.active) {
            allBricksDestroyed = false;
            break;
        }
    }
    
    if (allBricksDestroyed) {
        initGame(); // Reset the game with new bricks
        gameState.gameStarted = true; // Keep the game started
    }
}

void renderGame(unsigned int shaderProgram, unsigned int VAO)
{
    // Render paddle
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(gameState.paddle.position, 0.0f));
    model = glm::scale(model, glm::vec3(gameState.paddle.size, 1.0f));
    
    unsigned int modelLoc = glGetUniformLocation(shaderProgram, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    
    unsigned int colorLoc = glGetUniformLocation(shaderProgram, "color");
    glUniform3fv(colorLoc, 1, glm::value_ptr(gameState.paddle.color));
    
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    
    // Render ball
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(gameState.ball.position, 0.0f));
    model = glm::scale(model, glm::vec3(gameState.ball.size, 1.0f));
    
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniform3fv(colorLoc, 1, glm::value_ptr(gameState.ball.color));
    
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    
    // Render bricks
    for (const auto& brick : gameState.bricks) {
        if (brick.active) {
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(brick.position, 0.0f));
            model = glm::scale(model, glm::vec3(brick.size, 1.0f));
            
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glUniform3fv(colorLoc, 1, glm::value_ptr(brick.color));
            
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }
    }
}

bool checkCollision(const GameObject& a, const GameObject& b)
{
    // Check if two rectangles are colliding using AABB collision detection
    bool collisionX = a.position.x + a.size.x / 2 >= b.position.x - b.size.x / 2 &&
                      b.position.x + b.size.x / 2 >= a.position.x - a.size.x / 2;
    bool collisionY = a.position.y + a.size.y / 2 >= b.position.y - b.size.y / 2 &&
                      b.position.y + b.size.y / 2 >= a.position.y - a.size.y / 2;
    
    return collisionX && collisionY;
}

