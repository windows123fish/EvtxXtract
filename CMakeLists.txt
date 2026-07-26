cmake_minimum_required(VERSION 3.25)

project(EvtxXtract
        VERSION 1.0.0
        DESCRIPTION "High-performance streaming parser for Windows .evtx files"
        LANGUAGES CXX)

# Require C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable warnings
if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
endif()

# ImGui source files
set(IMGUI_SRC
    external/imgui/imgui.cpp
    external/imgui/imgui_draw.cpp
    external/imgui/imgui_tables.cpp
    external/imgui/imgui_widgets.cpp
    external/imgui/imgui_demo.cpp
    external/imgui/backends/imgui_impl_win32.cpp
    external/imgui/backends/imgui_impl_dx11.cpp
)

# Add source files for console version
add_executable(EvtxXtract
    src/main.cpp
    src/evtx_parser.cpp
    src/evtx_structs.cpp
)

# Include headers
target_include_directories(EvtxXtract PRIVATE include)

# Debug tool for analyzing .evtx file header
add_executable(debug_header
    src/debug_header.cpp
)

# GUI version with ImGui
add_executable(EvtxXtract-GUI
    src/gui_main.cpp
    src/gui_window.cpp
    src/evtx_parser.cpp
    src/evtx_structs.cpp
    ${IMGUI_SRC}
)

# Include directories for GUI
target_include_directories(EvtxXtract-GUI PRIVATE
    include
    external/imgui
    external/imgui/backends
)

# Link against Win32 libraries
target_link_libraries(EvtxXtract-GUI PRIVATE
    gdi32
    user32
    shell32
    dwmapi
)

# Enable testing
enable_testing()

# --- vcpkg integration for Phase 2 ---
# When nlohmann_json is needed for Phase 2, uncomment the following:
# find_package(nlohmann_json REQUIRED)
# target_link_libraries(EvtxXtract PRIVATE nlohmann_json::nlohmann_json)
