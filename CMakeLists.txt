# =============================================================================
# CMake build for TreeViz (Windows MSYS2/MinGW, Linux, macOS)
#   cmake -S . -B build-cmake -DCMAKE_BUILD_TYPE=Release
#   cmake --build build-cmake
# =============================================================================
cmake_minimum_required(VERSION 3.10)
project(TreeViz LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release)
endif()

find_package(SDL2 REQUIRED)

file(GLOB TREEVIZ_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
add_executable(TreeViz ${TREEVIZ_SOURCES})
target_include_directories(TreeViz PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# SDL2main provides WinMain on Windows (harmless elsewhere).
if(TARGET SDL2::SDL2main)
    target_link_libraries(TreeViz PRIVATE SDL2::SDL2main)
endif()
if(TARGET SDL2::SDL2)
    target_link_libraries(TreeViz PRIVATE SDL2::SDL2)
else()
    # very old SDL2 packages only define variables
    target_include_directories(TreeViz PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(TreeViz PRIVATE ${SDL2_LIBRARIES})
endif()

if(MSVC)
    target_compile_options(TreeViz PRIVATE /W4)
else()
    target_compile_options(TreeViz PRIVATE -Wall -Wextra)
endif()
