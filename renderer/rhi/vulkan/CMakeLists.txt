file(GLOB_RECURSE backend_vk_h ${CMAKE_CURRENT_LIST_DIR}/include/*.h)
file(GLOB_RECURSE backend_vk_cpp ${CMAKE_CURRENT_LIST_DIR}/source/*.cpp)
source_group("Header Files" FILES ${backend_vk_h})

find_package(Vulkan REQUIRED)
add_library(backend STATIC ${backend_vk_h} ${backend_vk_cpp})

target_include_directories(backend PUBLIC 
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${core_dir}
    ${renderer_dir}/rhi
    ${Vulkan_INCLUDE_DIR}
    ${dependency_dir}/vma/include
)

target_link_libraries(backend PUBLIC 
    GPUOpen::VulkanMemoryAllocator
    ${Vulkan_LIBRARIES}
    core
)
