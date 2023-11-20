file(GLOB_RECURSE glfw_raum_h ${CMAKE_CURRENT_LIST_DIR}/include/*.h)
file(GLOB_RECURSE glfw_raum_cpp ${CMAKE_CURRENT_LIST_DIR}/source/*.cpp)
source_group("Header Files" FILES ${glfw_raum_h})

find_package(Vulkan REQUIRED)
add_library(glfw_raum STATIC ${glfw_raum_h} ${glfw_raum_cpp})
set_target_properties(glfw_raum PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(glfw_raum PUBLIC 
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${dependency_dir}/glfw/include
    ${Vulkan_INCLUDE_DIR}
)

target_link_libraries(glfw_raum PRIVATE glfw)
