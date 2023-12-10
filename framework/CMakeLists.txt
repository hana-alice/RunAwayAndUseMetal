file(GLOB_RECURSE framework_platform_h ${CMAKE_CURRENT_LIST_DIR}/platform/windows/include/*.h)
file(GLOB_RECURSE framework_platform_cpp ${CMAKE_CURRENT_LIST_DIR}/platform/windows/source/*.cpp)
source_group("Header Files" FILES ${framework_platform_h})

find_package(Vulkan REQUIRED)
add_library(framework STATIC 
    ${framework_platform_h} 
    ${framework_platform_cpp}
)

target_include_directories(framework PUBLIC 
    ${CMAKE_CURRENT_LIST_DIR}/platform/windows/include
    ${dependency_dir}/sdl/include
    ${Vulkan_INCLUDE_DIR}
)

target_link_libraries(framework PRIVATE SDL3::SDL3)

