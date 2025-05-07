@echo off
echo Compiling scene3D.cpp...
C:\msys64\mingw64\bin\g++.exe scene3D.cpp src/glad.c -o scene3D -I./include -I"C:/msys64/mingw64/include/freetype2" -lglfw3 -lopengl32 -lgdi32 -lfreetype
if %ERRORLEVEL% EQU 0 (
    echo Compilation successful! Running scene3D.exe...
    scene3D.exe
) else (
    echo Compilation failed with error code %ERRORLEVEL%
)
pause
