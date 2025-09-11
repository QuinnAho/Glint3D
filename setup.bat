@echo off
setlocal enabledelayedexpansion

echo ================================================================
echo                    Glint3D Development Setup
echo ================================================================
echo.

:: Check if vcpkg is already available globally
where vcpkg >nul 2>&1
if %errorlevel% == 0 (
    echo ✅ vcpkg found globally - using system installation
    goto :install_deps
)

:: Check for local vcpkg
if exist "vcpkg\vcpkg.exe" (
    echo ✅ Using local vcpkg installation
    set VCPKG_ROOT=%cd%\vcpkg
    set PATH=%VCPKG_ROOT%;%PATH%
    goto :install_deps
)

if exist "engine\libraries\include\vcpkg\vcpkg.exe" (
    echo ✅ Using project vcpkg installation
    set VCPKG_ROOT=%cd%\engine\libraries\include\vcpkg
    set PATH=%VCPKG_ROOT%;%PATH%
    goto :install_deps
)

:: Install vcpkg locally if not found
echo 📦 vcpkg not found - installing locally...
echo.

git clone https://github.com/Microsoft/vcpkg.git
if %errorlevel% neq 0 (
    echo ❌ Failed to clone vcpkg repository
    echo Please ensure git is installed and available
    pause
    exit /b 1
)

cd vcpkg
call bootstrap-vcpkg.bat
if %errorlevel% neq 0 (
    echo ❌ Failed to bootstrap vcpkg
    pause
    exit /b 1
)

cd ..
set VCPKG_ROOT=%cd%\vcpkg
set PATH=%VCPKG_ROOT%;%PATH%

:install_deps
echo.
echo 🔧 Installing dependencies via vcpkg manifest...
echo Dependencies defined in vcpkg.json will be installed automatically during CMake configure
echo.

:: Set up cmake toolchain file
set CMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake

echo 🏗️  Configuring CMake project...
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE="%CMAKE_TOOLCHAIN_FILE%" -DCMAKE_BUILD_TYPE=Release
if %errorlevel% neq 0 (
    echo ❌ CMake configuration failed
    echo Check that Visual Studio 2019/2022 is installed
    pause
    exit /b 1
)

echo.
echo ✅ Setup complete!
echo.
echo To build the project:
echo   cmake --build build --config Release
echo.
echo To open in Visual Studio:
echo   start build\glint.sln
echo.
pause