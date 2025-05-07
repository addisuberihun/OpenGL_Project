cmake_minimum_required(VERSION 3.10)
project(CEStudents)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(OpenGL REQUIRED)

# Set include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

# Add source files
set(SOURCES
    CEStudents.cpp
    src/glad.c
    src/model_loader.cpp
    src/stb_image.cpp
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Set library paths
link_directories(
    ${CMAKE_SOURCE_DIR}/lib
)

# Link libraries
target_link_libraries(${PROJECT_NAME}
    ${OPENGL_LIBRARIES}
    glfw3
    assimp
    noise
    ozz_animation
    ozz_base
)

# Copy DLLs to build directory for Windows
if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/bin/libassimp.dll"
            "${CMAKE_SOURCE_DIR}/bin/noise.dll"
            "${CMAKE_SOURCE_DIR}/bin/ozz_animation.dll"
            "${CMAKE_SOURCE_DIR}/bin/ozz_base.dll"
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )
endif()







