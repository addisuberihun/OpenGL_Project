
# OpenGL Project

A modern OpenGL project using GLFW, GLAD, and GLM for 3D graphics rendering.

## Features

- Modern OpenGL implementation
- Shader-based rendering
- 3D mathematics using GLM library

## Prerequisites

- CMake 3.10 or higher
- MinGW-w64 with GCC
- GLFW3
- GLM

## Building the Project

```bash
mkdir build
cd build
cmake -G "MinGW Makefiles" ..
cmake --build .
```

## Project Structure

- `include/` - Header files including third-party libraries
- `src/` - Source files
- `main.cpp` - Main application entry point