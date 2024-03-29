cmake_minimum_required(VERSION 3.24)
set(CMAKE_CXX_STANDARD 20)

# set(CMAKE_TOOLCHAIN_FILE ${VCPKG_ROOT})

project(raum_sample_proj C CXX)

if (NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
endif()

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# if(MSVC)
#     set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
# endif()

set(dependency_dir ${CMAKE_CURRENT_SOURCE_DIR}/dependencies CACHE INTERNAL "dependency_dir")
set(core_dir ${CMAKE_CURRENT_SOURCE_DIR}/core CACHE INTERNAL "core_dir")
set(renderer_dir ${CMAKE_CURRENT_SOURCE_DIR}/renderer CACHE INTERNAL "renderer_dir")

# core
find_package(spdlog CONFIG REQUIRED)

add_definitions(-DGLM_ENABLE_EXPERIMENTAL)
find_package(glm CONFIG REQUIRED)

file(GLOB_RECURSE raum_utils_h ${CMAKE_CURRENT_LIST_DIR}/core/utils/*.h)
source_group("raum_utils_h" FILES ${raum_utils_h})
file(GLOB_RECURSE raum_utils_cpp ${CMAKE_CURRENT_LIST_DIR}/core/utils/*.cpp)
source_group("raum_utils_cpp" FILES ${raum_utils_cpp})

file(GLOB_RECURSE raum_core_h ${CMAKE_CURRENT_LIST_DIR}/core/*.h)
source_group("raum_core_h" FILES ${raum_core_h})
file(GLOB_RECURSE raum_core_cpp ${CMAKE_CURRENT_LIST_DIR}/core/*.cpp)
source_group("raum_core_cpp" FILES ${raum_core_cpp})

add_library(raum_core
    ${raum_utils_h}
    ${raum_utils_cpp}
    ${raum_core_h}
    ${raum_core_cpp}
)
target_include_directories(raum_core PUBLIC
    ./
)
target_link_libraries(raum_core PUBLIC
    spdlog::spdlog
)

# framework
find_package(Qt6 REQUIRED COMPONENTS Widgets)
file(GLOB_RECURSE raum_framework_platform_h ${CMAKE_CURRENT_LIST_DIR}/framework/platform/windows/include/*.h)
file(GLOB_RECURSE raum_framework_platform_cpp ${CMAKE_CURRENT_LIST_DIR}/framework/platform/windows/source/*.cpp)
source_group("raum_framework_platform_h" FILES ${raum_framework_platform_h})
source_group("raum_framework_platform_cpp" FILES ${raum_framework_platform_cpp})

add_library(raum_framework
    ${raum_framework_platform_h}
    ${raum_framework_platform_cpp}
)
target_include_directories(raum_framework PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/framework/platform/windows/include/
)
target_link_libraries(raum_framework PUBLIC
    Qt6::Widgets
    raum_core
)

# renderer
file(GLOB_RECURSE raum_graph_h ${CMAKE_CURRENT_LIST_DIR}/renderer/graph/include/*.h)
file(GLOB_RECURSE raum_graph_cpp ${CMAKE_CURRENT_LIST_DIR}/renderer/graph/source/*.cpp)
source_group("raum_graph_h" FILES ${raum_graph_h})
source_group("raum_graph_cpp" FILES ${raum_graph_cpp})

file(GLOB_RECURSE raum_scene_h ${CMAKE_CURRENT_LIST_DIR}/renderer/scene/include/*.h)
file(GLOB_RECURSE raum_scene_cpp ${CMAKE_CURRENT_LIST_DIR}/renderer/scene/source/*.cpp)
source_group("raum_scene_h" FILES ${raum_scene_h})
source_group("raum_scene_cpp" FILES ${raum_scene_cpp})

# renderer-rhi
add_subdirectory(renderer/rhi/vulkan)

find_package(Boost REQUIRED COMPONENTS graph json)

add_library(raum_renderer
    ${raum_graph_h}
    ${raum_graph_cpp}
    ${raum_scene_h}
    ${raum_scene_cpp}
)
target_include_directories(raum_renderer PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/renderer/graph/include
    ${CMAKE_CURRENT_LIST_DIR}/renderer/scene/include
)
target_link_libraries(raum_renderer PUBLIC 
    raum_rhi
    Boost::graph
    Boost::json
)

#asset
find_package(stb REQUIRED)
find_package(assimp CONFIG REQUIRED)
file(GLOB_RECURSE raum_asset_h ${CMAKE_CURRENT_LIST_DIR}/asset/include/*.h)
file(GLOB_RECURSE raum_asset_cpp ${CMAKE_CURRENT_LIST_DIR}/asset/source/*.cpp)
source_group("raum_asset_h" FILES ${raum_asset_h})
source_group("raum_asset_cpp" FILES ${raum_asset_cpp})

add_library(raum_asset
    ${raum_asset_h}
    ${raum_asset_cpp}
)
target_include_directories(raum_asset PUBLIC
    ${Stb_INCLUDE_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/asset/include
)
target_link_libraries(raum_asset PUBLIC
    raum_core
    raum_renderer
        assimp::assimp
)

file(GLOB_RECURSE Sample
    ${CMAKE_CURRENT_SOURCE_DIR}/Sample/main.cpp
)

add_executable(raum_sample
    ${Sample}
)
# if(WIN32)
#     set_target_properties(raum_sample
#     PROPERTIES 
#         LINK_FLAGS_DEBUG "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup"
#         LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup"
#         LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup"
#         LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup"
#     )
# endif(WIN32)


target_link_libraries(raum_sample PRIVATE
    raum_renderer
    raum_framework
    raum_asset
    raum_core
)
file(COPY asset/resources DESTINATION ${CMAKE_BINARY_DIR})

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT raum_sample)
