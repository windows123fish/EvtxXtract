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
)

# Add source files for console version
add_executable(EvtxXtract
    src/main.cpp
    src/evtx_parser.cpp
    src/evtx_structs