@echo off
echo Compiling naturalphenomena.cpp...
C:\msys64\mingw64\bin\g++.exe naturalphenomena.cpp src/glad.c -o naturalphenomena -I./include -lglfw3 -lopengl32 -lgdi32
if %ERRORLEVEL% EQU 0 (
    echo Compilation successful! Running naturalphenomena.exe...
    naturalphenomena.exe
) else (
    echo Compilation failed with error code %ERRORLEVEL%
    pause
    exit /b %ERRORLEVEL%
)
