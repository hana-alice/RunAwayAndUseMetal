file(GLOB_RECURSE backend_vk_h ${CMAKE_CURRENT_LIST_DIR}/include/*.h)
file(GLOB_RECURSE backend_vk_cpp ${CMAKE_CURRENT_LIST_DIR}/source/*.cpp)
source_group("Header Files" FILES ${backend_vk_h})

file(GLOB_RECURSE rhi_abstract_h ${renderer_dir}/rhi/include/*.h)
source_group("Abstract Layer" FILES ${rhi_abstract_h})


find_package(Vulkan REQUIRED)
add_library(rhi STATIC ${rhi_abstract_h} ${backend_vk_h} ${backend_vk_cpp})

target_include_directories(rhi PUBLIC 
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${core_dir}
    ${renderer_dir}/rhi/include
    ${Vulkan_INCLUDE_DIR}
    ${dependency_dir}/vma/include
    ${dependency_dir}/glm/glm
)

target_link_libraries(rhi PUBLIC 
    GPUOpen::VulkanMemoryAllocator
    ${Vulkan_LIBRARIES}
    core
    shaderc
)
