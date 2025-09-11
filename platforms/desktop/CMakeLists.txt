# Glint3D Desktop Platform CMake Configuration
cmake_minimum_required(VERSION 3.16)

# Platform-specific source files
set(DESKTOP_SOURCES
    main.cpp
    imgui/imgui.cpp
    imgui/imgui_impl_glfw.cpp
    imgui/imgui_impl_opengl3.cpp
    panels/scene_tree_panel.cpp
)

set(DESKTOP_HEADERS
    imgui/imgui.h
    imgui/imgui_impl_glfw.h
    imgui/imgui_impl_opengl3.h
    imgui/imgui_ui_layer.h
    include_panels/scene_tree_panel.h
)

# Create desktop platform library
add_library(glint3d_desktop_platform STATIC ${DESKTOP_SOURCES} ${DESKTOP_HEADERS})

# Link to engine core
target_link_libraries(glint3d_desktop_platform PUBLIC glint3d_engine)

# Platform-specific dependencies
find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)

target_link_libraries(glint3d_desktop_platform PUBLIC 
    glfw 
    ${OPENGL_LIBRARIES}
)

# Include directories
target_include_directories(glint3d_desktop_platform PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/include_panels
)

# Desktop executable
add_executable(glint3d_desktop main.cpp)
target_link_libraries(glint3d_desktop glint3d_desktop_platform)