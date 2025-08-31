cmake_minimum_required(VERSION 3.15)
project(OpenGLOBJViewer C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optional: KTX2/Basis (libktx)
option(ENABLE_KTX2 "Enable KTX2/Basis texture support via libktx" OFF)

# Source lists
set(SRC_DIR Project1/src)
set(IMGUI_DIR Project1)

set(APP_SOURCES
    ${SRC_DIR}/main.cpp
    ${SRC_DIR}/application.cpp
    ${SRC_DIR}/ai_instructions.cpp
    ${SRC_DIR}/json_ops_v1.cpp
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
    ${SRC_DIR}/BVHNode.cpp
    ${SRC_DIR}/raytracer.cpp
    ${SRC_DIR}/commands.cpp
    ${SRC_DIR}/nl_executor.cpp
    ${SRC_DIR}/ai_bridge.cpp
    ${SRC_DIR}/assimp_loader.cpp
    ${SRC_DIR}/userinput.cpp
    ${SRC_DIR}/triangle.cpp
)

set(IMGUI_SOURCES
    Project1/Libraries/include/imgui/imgui.cpp
    Project1/Libraries/include/imgui/imgui_draw.cpp
    Project1/Libraries/include/imgui/imgui_tables.cpp
    Project1/Libraries/include/imgui/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/imgui_impl_opengl3.cpp
)

# Web (Emscripten) build
if (EMSCRIPTEN)
    add_definitions(-D__EMSCRIPTEN__)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")

    add_executable(objviewer
        ${APP_SOURCES}
        ${IMGUI_SOURCES}
    )

    target_include_directories(objviewer PRIVATE
        Project1/include
        Project1
        Project1/Libraries/include
        Project1/Libraries/include/stb
        Project1/Libraries/include/imgui
        Project1/Libraries/include/imgui/backends
    )

    if (ENABLE_KTX2)
        # Expect libktx to be available as a port or prebuilt library
        # Define macro; linking depends on environment configuration
        target_compile_definitions(objviewer PRIVATE KTX2_ENABLED=1)
        # If a CMake package exists, try it (may be skipped on web)
        find_package(ktx CONFIG QUIET)
        if (ktx_FOUND)
            if (TARGET KTX::ktx)
                target_link_libraries(objviewer PRIVATE KTX::ktx)
            elseif (TARGET ktx)
                target_link_libraries(objviewer PRIVATE ktx)
            endif()
        endif()
    endif()

    # Emscripten ports and flags
    target_link_options(objviewer PRIVATE
        -sUSE_GLFW=3
        -sFULL_ES3=1
        -sWASM=1
        -sALLOW_MEMORY_GROWTH=1
        -sMIN_WEBGL_VERSION=2
        -sMAX_WEBGL_VERSION=2
        -sFORCE_FILESYSTEM=1
        "-sEXPORTED_FUNCTIONS=['_main','_app_apply_ops_json']"
        "-sEXPORTED_RUNTIME_METHODS=['ccall','cwrap']"
        "SHELL:--preload-file ${CMAKE_SOURCE_DIR}/Project1/assets@/assets"
        "SHELL:--preload-file ${CMAKE_SOURCE_DIR}/Project1/shaders@/shaders"
        "SHELL:--preload-file ${CMAKE_SOURCE_DIR}/Project1/cow-tex-fin.jpg@/assets/cow-tex-fin.jpg"
    )

    # ImGui ES3 path
    target_compile_definitions(objviewer PRIVATE IMGUI_IMPL_OPENGL_ES3)

# Desktop build
else()
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
        Project1/Libraries/include/stb
        Project1/Libraries/include/imgui
        Project1/Libraries/include/imgui/backends
    )

    # OpenGL
    find_package(OpenGL REQUIRED)

    # GLFW (prefer config, fallback to module)
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
        message(WARNING "glfw3 not found. Attempting to link by name.")
        target_link_libraries(objviewer PRIVATE OpenGL::GL glfw)
    endif()

    if (WIN32)
        target_link_libraries(objviewer PRIVATE gdi32 user32 shell32)
    endif()

    # ---- Assimp on desktop only ----
    find_package(assimp CONFIG QUIET)
    if (assimp_FOUND)
        target_link_libraries(objviewer PRIVATE assimp::assimp)
        target_compile_definitions(objviewer PRIVATE HAVE_ASSIMP=1 USE_ASSIMP=1)
    else()
        message(WARNING "Assimp not found. glTF/FBX/DAE will be disabled in this build.")
    endif()

    # Optional: Intel Open Image Denoise (desktop-only)
    find_package(OpenImageDenoise CONFIG QUIET)
    if (OpenImageDenoise_FOUND)
        target_link_libraries(objviewer PRIVATE OpenImageDenoise)
        target_compile_definitions(objviewer PRIVATE OIDN_ENABLED=1)
    else()
        message(STATUS "OpenImageDenoise not found; denoiser disabled.")
    endif()

    if (ENABLE_KTX2)
        find_package(ktx CONFIG QUIET)
        if (ktx_FOUND)
            if (TARGET KTX::ktx)
                target_link_libraries(objviewer PRIVATE KTX::ktx)
            elseif (TARGET ktx)
                target_link_libraries(objviewer PRIVATE ktx)
            else()
                message(WARNING "libktx found but target not recognized; link it manually.")
            endif()
            target_compile_definitions(objviewer PRIVATE KTX2_ENABLED=1)
        else()
            message(WARNING "ENABLE_KTX2=ON but libktx not found. KTX2 at runtime will be disabled.")
        endif()
    endif()
endif()
