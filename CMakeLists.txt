cmake_minimum_required(VERSION 3.20)

project(CorePulse
    VERSION 0.1.0
    DESCRIPTION "Open source mech combat game"
    LANGUAGES CXX
)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler-specific options
if(MSVC)
    add_compile_options(/W4 /permissive-)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-g -O0)
    else()
        add_compile_options(-O3 -DNDEBUG)
    endif()
endif()

# vcpkg integration
if(DEFINED CMAKE_TOOLCHAIN_FILE)
    message(STATUS "Using vcpkg toolchain: ${CMAKE_TOOLCHAIN_FILE}")
endif()

# Find packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(GLEW REQUIRED glew)
find_package(OpenGL REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# Try to find Lua
find_package(Lua)
if(NOT LUA_FOUND)
    # Manual fallback for system-installed Lua
    find_path(LUA_INCLUDE_DIR lua.h PATHS /usr/include/lua5.3 /usr/include/lua5.4 /usr/include/lua /usr/local/include/lua5.3 /usr/local/include/lua5.4)
    find_library(LUA_LIBRARIES NAMES lua5.3 lua5.4 lua liblua PATHS /usr/lib /usr/local/lib)
    
    if(LUA_INCLUDE_DIR AND LUA_LIBRARIES)
        set(LUA_FOUND TRUE)
        message(STATUS "Found Lua (manual): ${LUA_LIBRARIES}")
    else()
        message(WARNING "Lua not found - mission system will be disabled")
        set(LUA_FOUND FALSE)
    endif()
endif()

# Include directories
include_directories(include)
include_directories(external/glm)

# Source files
file(GLOB_RECURSE COREPULSE_SOURCES 
    "src/*.cpp"
)

file(GLOB_RECURSE COREPULSE_HEADERS
    "include/*.h"
    "include/*.hpp"
)

# Create executable
add_executable(corepulse ${COREPULSE_SOURCES})

# Link libraries
target_link_libraries(corepulse
    PRIVATE
        ${SDL2_LIBRARIES}
        ${GLEW_LIBRARIES}
        OpenGL::GL
        nlohmann_json::nlohmann_json
)

# Add Lua linking if found
if(LUA_FOUND)
    target_link_libraries(corepulse PRIVATE ${LUA_LIBRARIES})
    target_compile_definitions(corepulse PRIVATE COREPULSE_HAS_LUA)
endif()

# Add include directories
target_include_directories(corepulse PRIVATE 
    ${SDL2_INCLUDE_DIRS}
    ${GLEW_INCLUDE_DIRS}
)

# Add Lua include directories if found
if(LUA_FOUND)
    target_include_directories(corepulse PRIVATE ${LUA_INCLUDE_DIR})
endif()

target_compile_options(corepulse PRIVATE 
    ${SDL2_CFLAGS_OTHER}
    ${GLEW_CFLAGS_OTHER}
)

# Platform-specific linking
if(WIN32)
    target_link_libraries(corepulse PRIVATE opengl32)
elseif(UNIX AND NOT APPLE)
    target_link_libraries(corepulse PRIVATE GL)
endif()

# Set output directories
set_target_properties(corepulse PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

# Copy assets to build directory
file(COPY ${CMAKE_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR})

# Testing
enable_testing()
add_subdirectory(tests)

# Installation
install(TARGETS corepulse
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY assets/ DESTINATION share/corepulse/assets)