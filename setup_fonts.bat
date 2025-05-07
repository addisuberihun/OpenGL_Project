@echo off
echo Setting up fonts for CEStudents...

mkdir fonts 2>nul
echo Copying Arial font from Windows...
copy "C:\Windows\Fonts\arial.ttf" "fonts\" >nul 2>&1

if exist "fonts\arial.ttf" (
    echo Font setup successful!
) else (
    echo Could not copy Arial font automatically.
    echo Please manually copy arial.ttf to the fonts directory.
)

pause