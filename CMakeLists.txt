cmake_minimum_required(VERSION 3.15)
project(OpenGLOBJViewer C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source lists
set(SRC_DIR Project1/src)
set(IMGUI_DIR Project1)

set(APP_SOURCES
    ${SRC_DIR}/main.cpp
    ${SRC_DIR}/application.cpp
    ${SRC_DIR}/shader.cpp
    ${SRC_DIR}/texture.cpp
    ${SRC_DIR}/texture_cache.cpp
    ${SRC_DIR}/grid.cpp
    ${SRC_DIR}/axisrenderer.cpp
    ${SRC_DIR}/gizmo.cpp
    ${SRC_DIR}/objloader.cpp
    ${SRC_DIR}/mesh_loader.cpp
    ${SRC_DIR}/material.cpp
    ${SRC_DIR}/light.cpp
    ${SRC_DIR}/InputHandler.cpp
    ${SRC_DIR}/RayUtils.cpp
    ${SRC_DIR}/raytracer.cpp
    ${SRC_DIR}/commands.cpp
    ${SRC_DIR}/nl_executor.cpp
    ${SRC_DIR}/ai_bridge.cpp
    ${SRC_DIR}/assimp_loader.cpp
    ${SRC_DIR}/userinput.cpp
    ${SRC_DIR}/triangle.cpp
)

set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/imgui_impl_opengl3.cpp
)

if (EMSCRIPTEN)
    # Build a single-page app with WebGL2.
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    add_executable(objviewer
        ${APP_SOURCES}
        ${IMGUI_SOURCES}
    )
    target_include_directories(objviewer PRIVATE
        Project1/include
        Project1
        Project1/Libraries/include
    )
    # Emscripten ports and flags
    target_link_options(objviewer PRIVATE
        "-s" "USE_GLFW=3"
        "-s" "FULL_ES3=1"
        "-s" "WASM=1"
        # keep memory flexible for assets
        "-s" "ALLOW_MEMORY_GROWTH=1"
        # WebGL2 only
        "-s" "MIN_WEBGL_VERSION=2"
        "-s" "MAX_WEBGL_VERSION=2"
        # Preload assets and shaders into VFS
        "--preload-file" "Project1/assets@/assets"
        "--preload-file" "Project1/shaders@/shaders"
        "--preload-file" "Project1/cow-tex-fin.jpg@/assets/cow-tex-fin.jpg"
    )
else()
    # Desktop build (GLAD + GLFW + OpenGL)
    add_executable(objviewer
        ${APP_SOURCES}
        ${IMGUI_SOURCES}
        ${SRC_DIR}/glad.c
        ${SRC_DIR}/gl43_compat.cpp
        ${SRC_DIR}/compute_shader.cpp
    )
    target_include_directories(objviewer PRIVATE
        Project1/include
        Project1
        Project1/Libraries/include
    )

    # Try to find glfw and OpenGL
    find_package(OpenGL REQUIRED)
    # First search for a CMake package, then a module
    find_package(glfw3 CONFIG QUIET)
    if (NOT glfw3_FOUND)
        find_package(glfw3 QUIET)
    endif()

    if (TARGET glfw OR TARGET glfw3)
        if (TARGET glfw)
            target_link_libraries(objviewer PRIVATE glfw OpenGL::GL)
        else()
            target_link_libraries(objviewer PRIVATE glfw3 OpenGL::GL)
        endif()
    else()
        message(WARNING "glfw3 not found by CMake. You may need to provide it manually or use the existing MSVC solution.")
        # Fallback: attempt to link by name (may succeed if present in default linker paths)
        target_link_libraries(objviewer PRIVATE OpenGL::GL glfw)
    endif()

    if (WIN32)
        target_link_libraries(objviewer PRIVATE gdi32 user32 shell32)
    endif()
endif()

if (EMSCRIPTEN)
    # ImGui: ensure ES3 path
    target_compile_definitions(objviewer PRIVATE IMGUI_IMPL_OPENGL_ES3)
endif()

