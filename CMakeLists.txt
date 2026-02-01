cmake_minimum_required(VERSION 3.15)

project(FastWorldGenGUI VERSION 1.0 LANGUAGES CXX)

# ------------------------------------------------------------
# Language / standard
# ------------------------------------------------------------
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ------------------------------------------------------------
# Sources
# ------------------------------------------------------------
file(GLOB_RECURSE FWG_GUI_SOURCES
    CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp
)

file(GLOB_RECURSE FWG_GUI_HEADERS
    CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp
)

# GLAD
add_library(GLAD STATIC external/glad/src/glad.cpp)
target_include_directories(GLAD PUBLIC external/glad/include)
set_target_properties(GLAD PROPERTIES LANGUAGE CXX)

include(FetchContent)

# ------------------------------------------------------------
# GLFW (cross-platform window + OpenGL context)
# ------------------------------------------------------------
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
# Force GLFW to build only with X11 on Linux
if(UNIX AND NOT APPLE)
    set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_X11 ON CACHE BOOL "" FORCE)
endif()

FetchContent_MakeAvailable(glfw)


# ------------------------------------------------------------
# ImGui
# ------------------------------------------------------------
file(GLOB IMGUI_CORE
    ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/misc/cpp/*.cpp
)

set(IMGUI_BACKENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/backends/imgui_impl_glfw.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/backends/imgui_impl_opengl3.cpp
)

add_library(ImGui STATIC
    ${IMGUI_CORE}
    ${IMGUI_BACKENDS}
)

target_include_directories(ImGui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/misc/cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/external/glad/include
    ${glfw_SOURCE_DIR}/include
)



# -----------------------------
# AddressSanitizer (Clang/GCC only)
# -----------------------------
option(ENABLE_ASAN "Enable AddressSanitizer" ON)

if (ENABLE_ASAN AND CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    message(STATUS "Building with AddressSanitizer")

    add_compile_options(
        -fsanitize=address
        -fsanitize=undefined
        -fno-omit-frame-pointer
        -g
    )

    add_link_options(
        -fsanitize=address
        -fsanitize=undefined
        -fno-omit-frame-pointer
    )

elseif (ENABLE_ASAN AND MSVC)
    message(WARNING "ASan requested but MSVC does NOT support it — use Clang on Windows.")
endif()


# ------------------------------------------------------------
# FastWorldGen dependency
# ------------------------------------------------------------
if (NOT TARGET FastWorldGenLib)
    message(STATUS "FastWorldGen not found — adding subdirectory")
    add_subdirectory(
        ../FastWorldGen
        ${CMAKE_BINARY_DIR}/FastWorldGen_build
    )
endif()

# ------------------------------------------------------------
# GUI Library
# ------------------------------------------------------------
add_library(FastWorldGenGUILib
    ${FWG_GUI_SOURCES}
    ${FWG_GUI_HEADERS}
)

target_include_directories(FastWorldGenGUILib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(FastWorldGenGUILib PUBLIC
    FastWorldGenLib
    ImGui
    GLAD 
    glfw
)

# Platform-specific ImGui deps
if (WIN32)
    target_link_libraries(ImGui PUBLIC opengl32)
elseif(UNIX)
    find_package(OpenGL REQUIRED)
    target_link_libraries(ImGui PUBLIC OpenGL::GL)
endif()


# Compiler warnings
if (MSVC)
    target_compile_options(FastWorldGenGUILib PRIVATE /W4 /MP /Gy)
else()
    target_compile_options(FastWorldGenGUILib PRIVATE -Wall -Wextra -Wpedantic)
endif()

# ------------------------------------------------------------
# Executable
# ------------------------------------------------------------
add_executable(FastWorldGenGUI main.cpp)

target_link_libraries(FastWorldGenGUI PRIVATE
    FastWorldGenGUILib
)
