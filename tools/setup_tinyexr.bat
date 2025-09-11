@echo off
rem Setup TinyEXR and miniz for Windows builds

setlocal enabledelayedexpansion

set "SCRIPT_DIR=%~dp0"
set "PROJECT_ROOT=%SCRIPT_DIR%\.."
set "INCLUDE_DIR=%PROJECT_ROOT%\engine\libraries\include"
set "SRC_DIR=%PROJECT_ROOT%\engine\libraries\src"

echo Setting up TinyEXR and miniz...

rem Create directories
if not exist "%INCLUDE_DIR%" mkdir "%INCLUDE_DIR%"
if not exist "%SRC_DIR%" mkdir "%SRC_DIR%"

rem Download TinyEXR header if not present
set "TINYEXR_H=%INCLUDE_DIR%\tinyexr.h"
if not exist "%TINYEXR_H%" (
    echo Downloading tinyexr.h...
    powershell -Command "Invoke-WebRequest -Uri 'https://raw.githubusercontent.com/syoyo/tinyexr/master/tinyexr.h' -OutFile '%TINYEXR_H%'"
    echo Downloaded tinyexr.h
) else (
    echo tinyexr.h already exists
)

rem Download miniz header if not present
set "MINIZ_H=%INCLUDE_DIR%\miniz.h"
if not exist "%MINIZ_H%" (
    echo Downloading miniz.h...
    powershell -Command "Invoke-WebRequest -Uri 'https://raw.githubusercontent.com/richgel999/miniz/master/miniz.h' -OutFile '%MINIZ_H%'"
    echo Downloaded miniz.h
) else (
    echo miniz.h already exists
)

rem Download miniz source if not present
set "MINIZ_C=%SRC_DIR%\miniz.c"
if not exist "%MINIZ_C%" (
    echo Downloading miniz.c...
    powershell -Command "Invoke-WebRequest -Uri 'https://raw.githubusercontent.com/richgel999/miniz/master/miniz.c' -OutFile '%MINIZ_C%'"
    echo Downloaded miniz.c
) else (
    echo miniz.c already exists
)

rem Download additional miniz files for modular build
set "MINIZ_FILES=miniz_tdef.c miniz_tinfl.c miniz_zip.c"
for %%f in (%MINIZ_FILES%) do (
    set "MINIZ_FILE=%SRC_DIR%\%%f"
    if not exist "!MINIZ_FILE!" (
        echo Downloading %%f...
        powershell -Command "Invoke-WebRequest -Uri 'https://raw.githubusercontent.com/richgel999/miniz/master/%%f' -OutFile '!MINIZ_FILE!'"
        echo Downloaded %%f
    ) else (
        echo %%f already exists
    )
)

echo TinyEXR and miniz setup complete!
echo Files installed:
echo   Headers: %INCLUDE_DIR%\{tinyexr.h,miniz.h}
echo   Sources: %SRC_DIR%\{miniz.c,miniz_tdef.c,miniz_tinfl.c,miniz_zip.c}
echo.
echo You can now build with EXR support:
echo   cmake -S . -B builds/desktop/cmake -DENABLE_EXR=ON
echo   cmake --build builds/desktop/cmake --config Release

endlocal