file(GLOB_RECURSE renderer_raum_h ${CMAKE_CURRENT_LIST_DIR}/rhi/include/*.h)
file(GLOB_RECURSE renderer_raum_cpp ${CMAKE_CURRENT_LIST_DIR}/rhi/source/*.cpp)
source_group("Header Files" FILES ${renderer_raum_h})

find_package(Vulkan REQUIRED)
add_library(renderer STATIC ${renderer_raum_h} ${renderer_raum_cpp})
set_target_properties(renderer PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(renderer PUBLIC 
    ${CMAKE_CURRENT_LIST_DIR}/rhi/include
    ${core_dir}
    ${Vulkan_INCLUDE_DIR}
)

target_link_libraries(renderer PUBLIC 
    ${Vulkan_LIBRARIES}
    core)
