@echo off 
REM Glint3D wrapper script 
REM Automatically detects project root directory
set "SCRIPT_DIR=%~dp0"
for %%i in ("%SCRIPT_DIR%\..\") do set "GLINT_ROOT=%%~fi"
 
REM Change to Glint root directory so asset paths work 
cd /d "%GLINT_ROOT%" 
 
REM Try release build first, fallback to debug 
if exist "%GLINT_ROOT%builds\desktop\cmake\Release\glint.exe" ( 
    "%GLINT_ROOT%builds\desktop\cmake\Release\glint.exe" %* 
) else if exist "%GLINT_ROOT%builds\desktop\cmake\Debug\glint.exe" ( 
    "%GLINT_ROOT%builds\desktop\cmake\Debug\glint.exe" %* 
) else ( 
    echo ERROR: No Glint executable found. Please build the project first. 
    exit /b 1 
) 
