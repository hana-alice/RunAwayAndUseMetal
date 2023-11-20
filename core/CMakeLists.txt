file(GLOB_RECURSE utils_raum_h ${CMAKE_CURRENT_LIST_DIR}/utils/*.h)
source_group("Header Files" FILES ${utils_raum_h})
file(GLOB_RECURSE utils_raum_cpp ${CMAKE_CURRENT_LIST_DIR}/utils/*.cpp)

file(GLOB_RECURSE core_raum_h ${CMAKE_CURRENT_LIST_DIR}/*.h)
source_group("Header Files" FILES ${core_raum_h})
file(GLOB_RECURSE core_raum_cpp ${CMAKE_CURRENT_LIST_DIR}/*.cpp)

add_library(core STATIC 
    ${utils_raum_h}
    ${utils_raum_cpp}
    ${core_raum_h}
    ${core_raum_cpp}
)

set_target_properties(core PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(core PUBLIC 
    ${CMAKE_CURRENT_LIST_DIR}/utils/
    ${CMAKE_CURRENT_LIST_DIR}/
    ${dependency_dir}/spdlog/include
)

target_link_libraries(core PUBLIC
    spdlog)
