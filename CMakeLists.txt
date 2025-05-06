cmake_minimum_required(VERSION 3.10)
project(OpenGL_Project)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(OpenGL REQUIRED)

# Find GLM (if installed system-wide)
find_package(glm QUIET)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# If GLM was found via find_package, add its include dirs
if(glm_FOUND)
    include_directories(${GLM_INCLUDE_DIRS})
endif()

# Add executable
add_executable(${PROJECT_NAME} 
    main.cpp
    src/glad.c
    src/shader.cpp
)

# Link libraries
target_link_libraries(${PROJECT_NAME}
    ${OPENGL_LIBRARIES}
    glfw3
    opengl32
    gdi32
)

# Copy executable to root directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}
)


