cmake_minimum_required(VERSION 3.24)

project(RAUM_Sample C CXX)

if (NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

set(dependency_dir ${CMAKE_CURRENT_SOURCE_DIR}/dependencies CACHE INTERNAL "dependency_dir")
set(core_dir ${CMAKE_CURRENT_SOURCE_DIR}/core CACHE INTERNAL "core_dir")
set(renderer_dir ${CMAKE_CURRENT_SOURCE_DIR}/renderer CACHE INTERNAL "renderer_dir")

add_subdirectory(
    ${dependency_dir}/sdl
)
set_target_properties(SDL3-shared PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

# 3rd party
include(${dependency_dir}/CMakeLists.txt)

add_subdirectory(core)
add_subdirectory(framework)
add_subdirectory(renderer/rhi/vulkan)
add_subdirectory(renderer/scene)
add_subdirectory(renderer/graph)

file(GLOB_RECURSE Sample
    ${CMAKE_CURRENT_SOURCE_DIR}/Sample/*.*
)

add_executable(RAUM_Sample ${Sample})

target_link_libraries(
    RAUM_Sample
    framework
    rhi
    graph
)

target_include_directories(RAUM_Sample PUBLIC
    ${dependency_dir}/glm
)

set_target_properties(
    RAUM_Sample PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/")

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

file(COPY framework/asset/resources DESTINATION ${CMAKE_BINARY_DIR})

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT RAUM_Sample)
