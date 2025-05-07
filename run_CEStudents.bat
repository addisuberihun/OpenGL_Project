@echo off
echo Compiling CEStudents.cpp...
C:\msys64\mingw64\bin\g++.exe CEStudents.cpp src/glad.c -o CEStudents -I./include -I"C:/msys64/mingw64/include/freetype2" -lglfw3 -lopengl32 -lgdi32 -lfreetype
if %ERRORLEVEL% EQU 0 (
    echo Compilation successful! Running CEStudents.exe...
    CEStudents.exe
) else (
    echo Compilation failed with error code %ERRORLEVEL%
    pause
    exit /b %ERRORLEVEL%
)
pause