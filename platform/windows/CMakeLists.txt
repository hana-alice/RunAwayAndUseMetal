file(GLOB_RECURSE win_raum_h ${CMAKE_CURRENT_LIST_DIR}/include/*.h)
file(GLOB_RECURSE win_raum_cpp ${CMAKE_CURRENT_LIST_DIR}/source/*.cpp)
source_group("Header Files" FILES ${win_raum_h})

find_package(Vulkan REQUIRED)
add_library(win_raum STATIC ${win_raum_h} ${win_raum_cpp})
set_target_properties(win_raum PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(win_raum PUBLIC 
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${dependency_dir}/sdl/include
    ${Vulkan_INCLUDE_DIR}
)

target_link_libraries(win_raum PRIVATE SDL3-static)
