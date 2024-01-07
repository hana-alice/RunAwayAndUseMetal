file(GLOB_RECURSE scene_h ${CMAKE_CURRENT_LIST_DIR}/include/*.h)
file(GLOB_RECURSE scene_cpp ${CMAKE_CURRENT_LIST_DIR}/source/*.cpp)
source_group("Header Files" FILES ${scene_h})

add_library(scene STATIC ${scene_h} ${scene_cpp})

target_include_directories(scene PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include/
    ${core_dir}
    ${dependency_dir}/glm/glm
)

target_link_libraries(scene PUBLIC 
    core
)