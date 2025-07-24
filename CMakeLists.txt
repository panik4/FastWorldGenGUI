# Minimum CMake version
cmake_minimum_required(VERSION 3.15)

# Project name and version
project(FastWorldGenGUI VERSION 1.0 LANGUAGES CXX)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# For Release build (force optimizations)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /Ob2")
# Enable Link Time Optimization (LTO)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

# Enable MSVC-specific optimizations (if using MSVC compiler)
if(MSVC)
    add_compile_options(/MP)  # Enables multi-threaded compilation
    add_compile_options(/GL)  # Whole program optimization
    add_compile_options(/Gy)  # Enable function-level linking
endif()

# Core library (FastWorldGenLib) ------------------------------------
# Find all core source files (excluding main.cpp) and header files
file(GLOB_RECURSE SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp)
file(GLOB_RECURSE HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h FastWorldGenGUI/include/*.hpp)

# imgui
file(GLOB IMGUI_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/misc/cpp/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/backends/imgui_impl_dx11.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/backends/imgui_impl_win32.cpp
)

add_library(ImGui STATIC ${IMGUI_SOURCES})

# Add the ImGui include directory
target_include_directories(ImGui PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/misc/cpp
)

# Add the library target
add_library(FastWorldGenGUILib ${SOURCE_FILES} ${HEADER_FILES})
# Link ImGui to FastWorldGenLib
target_link_libraries(FastWorldGenGUILib PUBLIC ImGui)

# Add the FastWorldGenLib subdirectory first
if(NOT TARGET FastWorldGenLib)
    # Allow standalone development
    message(STATUS "Building standalone: adding FastWorldGen manually")
    add_subdirectory(../FastWorldGen ${CMAKE_BINARY_DIR}/FastWorldGen_build)
endif()
# Query the include directories for FastWorldGenLib to ensure they are properly set
get_target_property(inc_dirs FastWorldGenGUILib INCLUDE_DIRECTORIES)


# Ensure that RandomParadox depends on FastWorldGenLib so that the libraries are built first
add_dependencies(FastWorldGenGUILib FastWorldGenLib)

# Link the FastWorldGenLib library to RandomParadox
target_link_libraries(FastWorldGenGUILib PUBLIC FastWorldGenLib)
# Link ImGui to RandomParadoxLib
target_link_libraries(FastWorldGenGUILib PUBLIC ImGui)
target_include_directories(FastWorldGenGUILib PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

get_target_property(fwgUI_inc_dirs FastWorldGenGUILib INCLUDE_DIRECTORIES)
message(STATUS "FastWorldGenGUI include directories: ${fwgUI_inc_dirs}")

# Executable target (FastWorldGen) ------------------------------------
add_executable(${PROJECT_NAME} main.cpp)
# Link executable with the library
target_link_libraries(${PROJECT_NAME} PUBLIC FastWorldGenGUILib)

