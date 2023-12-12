add_subdirectory(vulkan)

file(GLOB_RECURSE RHI_HEADERS ${CMAKE_CURRENT_LIST_DIR}/*.h)

add_library(rhi INTERFACE ${RHI_HEADERS})

target_include_directories(rhi INTERFACE ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(rhi INTERFACE backend)