cmake_minimum_required(VERSION 3.10)

project(RAUM_Sample C CXX)

if (NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(dependency_dir ${CMAKE_CURRENT_SOURCE_DIR}/dependencies CACHE INTERNAL "dependency_dir")
set(core_dir ${CMAKE_CURRENT_SOURCE_DIR}/core CACHE INTERNAL "core_dir")

add_subdirectory(
    ${dependency_dir}/sdl
)

add_subdirectory(
    ${dependency_dir}/spdlog
)

add_subdirectory(core)
add_subdirectory(platform/windows)

add_subdirectory(renderer)

file(GLOB_RECURSE Sample
    ${CMAKE_CURRENT_SOURCE_DIR}/Sample/*.*
)

add_executable(RAUM_Sample ${Sample})

set_target_properties(RAUM_Sample PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(
    RAUM_Sample
    win_raum
    renderer
)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT RAUM_Sample)