file(GLOB_RECURSE framework_platform_h ${CMAKE_CURRENT_LIST_DIR}/platform/windows/include/*.h)
file(GLOB_RECURSE framework_platform_cpp ${CMAKE_CURRENT_LIST_DIR}/platform/windows/source/*.cpp)

file(GLOB_RECURSE framework_asset_h ${CMAKE_CURRENT_LIST_DIR}/asset/include/*.h)
file(GLOB_RECURSE framework_asset_cpp ${CMAKE_CURRENT_LIST_DIR}/asset/source/*.cpp)
source_group("Header Files" FILES ${framework_platform_h} ${framework_asset_h})

find_package(Vulkan REQUIRED)

add_library(framework STATIC 
    ${framework_platform_h} 
    ${framework_platform_cpp}
    ${framework_asset_h} 
    ${framework_asset_cpp}
)

target_include_directories(framework PUBLIC 
    ${CMAKE_CURRENT_LIST_DIR}/platform/windows/include
    ${CMAKE_CURRENT_LIST_DIR}/asset/include
    ${core_dir}/utils
    ${dependency_dir}/tinyobjloader
    ${dependency_dir}/sdl/include
    ${Vulkan_INCLUDE_DIR}
)

target_link_libraries(framework PRIVATE 
    SDL3::SDL3 
    tinyobjloader
    core
)

