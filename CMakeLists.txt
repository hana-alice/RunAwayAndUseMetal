cmake_minimum_required(VERSION 3.24)
set(CMAKE_CXX_STANDARD 20)

# set(CMAKE_TOOLCHAIN_FILE ${VCPKG_ROOT})

project(raum_sample_proj C CXX)

if (NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
endif ()

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

#set(CMAKE_AUTOMOC ON)

set(RAUM_DEFAULT_ASSET_DIR "${RAUM_DEFAULT_ASSET_DIR}")

#add_compile_options(-fsanitize=address)
#add_link_options(-fsanitize=address)

#find_package(Qt6Vulkan CONFIG REQUIRED)
# if(MSVC)
#     set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
# endif()
find_package(SDL3 CONFIG REQUIRED)
set(dependency_dir ${CMAKE_CURRENT_SOURCE_DIR}/dependencies CACHE INTERNAL "dependency_dir")
set(core_dir ${CMAKE_CURRENT_SOURCE_DIR}/core CACHE INTERNAL "core_dir")
set(renderer_dir ${CMAKE_CURRENT_SOURCE_DIR}/renderer CACHE INTERNAL "renderer_dir")
set(asset_dir ${CMAKE_CURRENT_SOURCE_DIR}/asset CACHE INTERNAL "asset_dir")

# core
find_package(spdlog CONFIG REQUIRED)

add_definitions(-DGLM_ENABLE_EXPERIMENTAL -DGLM_FORCE_DEPTH_ZERO_TO_ONE -DGLM_FORCE_LEFT_HANDED)
find_package(glm CONFIG REQUIRED)

file(GLOB_RECURSE raum_utils_h ${CMAKE_CURRENT_LIST_DIR}/core/utils/*.h)
source_group("raum_utils_h" FILES ${raum_utils_h})
file(GLOB_RECURSE raum_utils_cpp ${CMAKE_CURRENT_LIST_DIR}/core/utils/*.cpp)
source_group("raum_utils_cpp" FILES ${raum_utils_cpp})

file(GLOB_RECURSE raum_thread_h ${CMAKE_CURRENT_LIST_DIR}/core/thread/*.h)
source_group("raum_thread_h" FILES ${raum_thread_h})
file(GLOB_RECURSE raum_thread_cpp ${CMAKE_CURRENT_LIST_DIR}/core/thread/*.cpp)
source_group("raum_thread_cpp" FILES ${raum_thread_cpp})

file(GLOB_RECURSE raum_core_h ${CMAKE_CURRENT_LIST_DIR}/core/*.h)
source_group("raum_core_h" FILES ${raum_core_h})
file(GLOB_RECURSE raum_core_cpp ${CMAKE_CURRENT_LIST_DIR}/core/*.cpp)
source_group("raum_core_cpp" FILES ${raum_core_cpp})

add_library(raum_core
        ${raum_utils_h}
        ${raum_utils_cpp}
        ${raum_core_h}
        ${raum_core_cpp}
        ${raum_thread_h}
        ${raum_thread_cpp}
)

target_include_directories(raum_core PUBLIC
        ./
)
target_link_libraries(raum_core PUBLIC
        spdlog::spdlog
)
# framework
#file(GLOB_RECURSE raum_framework_platform_h ${CMAKE_CURRENT_LIST_DIR}/framework/platform/windows/*.h)
#file(GLOB_RECURSE raum_framework_platform_cpp ${CMAKE_CURRENT_LIST_DIR}/framework/platform/windows/*.cpp)
file(GLOB_RECURSE raum_framework_platform_common_h ${CMAKE_CURRENT_LIST_DIR}/framework/platform/common/*.h)
file(GLOB_RECURSE raum_framework_platform_common_cpp ${CMAKE_CURRENT_LIST_DIR}/framework/platform/common/*.cpp)

file(GLOB_RECURSE raum_framework_game_h ${CMAKE_CURRENT_LIST_DIR}/framework/game/*.h)
file(GLOB_RECURSE raum_framework_game_cpp ${CMAKE_CURRENT_LIST_DIR}/framework/game/*.cpp)

file(GLOB_RECURSE raum_framework_h ${CMAKE_CURRENT_LIST_DIR}/framework/event/*.h)
file(GLOB_RECURSE raum_framework_cpp ${CMAKE_CURRENT_LIST_DIR}/framework/event/*.cpp)
source_group("raum_framework_h" FILES ${raum_framework_h})
source_group("raum_framework_cpp" FILES ${raum_framework_cpp})
list(APPEND raum_framework_h ${raum_framework_platform_common_h})
list(APPEND raum_framework_cpp ${raum_framework_platform_common_cpp})
list(APPEND raum_framework_h ${raum_framework_game_h})
list(APPEND raum_framework_cpp ${raum_framework_game_cpp})

if (WIN32)
    source_group("raum_framework_platform_specific_cpp" FILES ${raum_framework_platform_specific_cpp})
endif ()

add_library(raum_framework
        ${raum_framework_platform_h}
        ${raum_framework_platform_cpp}
        ${raum_framework_h}
        ${raum_framework_cpp}
)
target_include_directories(raum_framework PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/framework/platform/common/
        ${CMAKE_CURRENT_LIST_DIR}/framework/event/
        ${CMAKE_CURRENT_LIST_DIR}/framework/game/
        ${CMAKE_CURRENT_LIST_DIR}/renderer/graph/
)
target_link_libraries(raum_framework PRIVATE
        raum_core
        raum_renderer
        raum_asset
        SDL3::SDL3
)

# renderer
file(GLOB_RECURSE raum_graph_h ${CMAKE_CURRENT_LIST_DIR}/renderer/graph/*.h)
file(GLOB_RECURSE raum_graph_cpp ${CMAKE_CURRENT_LIST_DIR}/renderer/graph/*.cpp)
source_group("raum_graph_h" FILES ${raum_graph_h})
source_group("raum_graph_cpp" FILES ${raum_graph_cpp})

file(GLOB_RECURSE raum_scene_h ${CMAKE_CURRENT_LIST_DIR}/renderer/scene/*.h)
file(GLOB_RECURSE raum_scene_cpp ${CMAKE_CURRENT_LIST_DIR}/renderer/scene/*.cpp)
source_group("raum_scene_h" FILES ${raum_scene_h})
source_group("raum_scene_cpp" FILES ${raum_scene_cpp})

file(GLOB_RECURSE raum_feature_h ${CMAKE_CURRENT_LIST_DIR}/renderer/feature/*.h)
file(GLOB_RECURSE raum_feature_cpp ${CMAKE_CURRENT_LIST_DIR}/renderer/feature/*.cpp)
source_group("raum_feature_h" FILES ${raum_feature_h})
source_group("raum_feature_cpp" FILES ${raum_feature_cpp})


# renderer-rhi
add_subdirectory(renderer/rhi/vulkan)

add_library(raum_renderer
        ${raum_graph_h}
        ${raum_graph_cpp}
        ${raum_scene_h}
        ${raum_scene_cpp}
        ${raum_feature_h}
        ${raum_feature_cpp}
)
target_include_directories(raum_renderer PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/renderer/graph
        ${CMAKE_CURRENT_LIST_DIR}/renderer/scene
        ${CMAKE_CURRENT_LIST_DIR}/renderer/feature
)
target_link_libraries(raum_renderer PUBLIC
        raum_rhi
        Boost::graph
)

# Get the include directories for the target.
get_target_property(LIBA_INCLUDES raum_rhi INCLUDE_DIRECTORIES)

# Construct the compiler string for the include directories.
foreach (dir ${LIBA_INCLUDES})
    string(APPEND INCLUDE_COMPILER_STRING "/I${dir} ")
endforeach ()

#asset
find_package(Boost REQUIRED COMPONENTS graph json)
find_package(stb REQUIRED)
find_package(cereal CONFIG REQUIRED)
find_path(TINYGLTF_INCLUDE_DIRS "tiny_gltf.h")

file(GLOB_RECURSE raum_asset_loader_h ${CMAKE_CURRENT_LIST_DIR}/asset/loader/*.h)
file(GLOB_RECURSE raum_asset_loader_cpp ${CMAKE_CURRENT_LIST_DIR}/asset/loader/*.cpp)
source_group("raum_asset_loader_h" FILES ${raum_asset_loader_h})
source_group("raum_asset_loader_cpp" FILES ${raum_asset_loader_cpp})

file(GLOB_RECURSE raum_asset_serialize_h ${CMAKE_CURRENT_LIST_DIR}/asset/serialization/*.h)
file(GLOB_RECURSE raum_asset_serialize_cpp ${CMAKE_CURRENT_LIST_DIR}/asset/serialization/*.cpp)
source_group("raum_asset_serialize_h" FILES ${raum_asset_serialize_h})
source_group("raum_asset_serialize_cpp" FILES ${raum_asset_serialize_cpp})

file(GLOB_RECURSE raum_asset_builtinres_h ${CMAKE_CURRENT_LIST_DIR}/asset/resources/builtin/*.h)
file(GLOB_RECURSE raum_asset_builtinres_cpp ${CMAKE_CURRENT_LIST_DIR}/asset/resources/builtin/*.cpp)
source_group("raum_asset_builtinres_h" FILES ${raum_asset_builtinres_h})
source_group("raum_asset_builtinres_cpp" FILES ${raum_asset_builtinres_cpp})

add_library(raum_asset
        ${raum_asset_loader_h}
        ${raum_asset_loader_cpp}
        ${raum_asset_serialize_h}
        ${raum_asset_serialize_cpp}
        ${raum_asset_builtinres_h}
        ${raum_asset_builtinres_cpp}
)
target_include_directories(raum_asset PUBLIC
        ${Stb_INCLUDE_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/asset/loader
        ${CMAKE_CURRENT_LIST_DIR}/asset/serialization
        ${CMAKE_CURRENT_LIST_DIR}/asset/resources/builtin
        ${TINYGLTF_INCLUDE_DIRS}
)
target_link_libraries(raum_asset PUBLIC
        Boost::json
        cereal::cereal
        raum_core
        raum_renderer
)

file(GLOB_RECURSE Sample
        ${CMAKE_CURRENT_SOURCE_DIR}/Sample/*.*
        ${CMAKE_CURRENT_SOURCE_DIR}/Sample/shadow/*.*
)

if (OFFLINE_TOOLS_MESHOPTIMIZER)
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/tools/meshoptimize)
else ()
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

    add_definitions(-DRAUM_DEFAULT_ASSET_DIR="${RAUM_DEFAULT_ASSET_DIR}")
    target_link_libraries(raum_sample PRIVATE
            raum_renderer
            raum_framework
            raum_asset
            raum_core
    )

    # set_target_properties(raum_sample PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})


    file(COPY asset/resources/files DESTINATION ${CMAKE_BINARY_DIR})
    #file(COPY framework/platform/windows/builds/qt6.conf DESTINATION ${CMAKE_BINARY_DIR})
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT raum_sample)
endif ()
