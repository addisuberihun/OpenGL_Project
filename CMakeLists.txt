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

# Add executables
add_executable(${PROJECT_NAME} 
    main.cpp
    src/glad.c
    src/shader.cpp
)

# Add the 2D game executable
add_executable(Game2D
    game2D.cpp
    src/glad.c
    src/shader.cpp
)

# Add the 3D scene executable
add_executable(scene3D
    scene3D.cpp
    src/glad.c
)

# Link libraries for main project
target_link_libraries(${PROJECT_NAME}
    ${OPENGL_LIBRARIES}
    glfw3
    opengl32
    gdi32
)

# Link libraries for 2D game
target_link_libraries(Game2D
    ${OPENGL_LIBRARIES}
    glfw3
    opengl32
    gdi32
)

# Link libraries for 3D scene
target_link_libraries(scene3D
    ${OPENGL_LIBRARIES}
    glfw3
    opengl32
    gdi32
)

# Copy executable to root directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}
)





