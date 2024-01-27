file(GLOB_RECURSE graph_h ${CMAKE_CURRENT_LIST_DIR}/include/*.h)
file(GLOB_RECURSE graph_cpp ${CMAKE_CURRENT_LIST_DIR}/source/*.cpp)
source_group("Header Files" FILES ${graph_h})

set(CMAKE_CXX_STANDARD 20)

add_library(graph STATIC ${graph_h} ${graph_cpp})
set(Boost_USE_STATIC_LIBS OFF) 
set(Boost_USE_MULTITHREADED ON)  
set(Boost_USE_STATIC_RUNTIME OFF) 
target_include_directories(graph PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include/
    ${core_dir}
    ${dependency_dir}/glm/glm
    ${renderer_dir}/rhi/include
    ${renderer_dir}/graph/include
    
    ${dependency_dir}/boost/libs/graph/include
    ${dependency_dir}/boost/libs/algorithm/include
    ${dependency_dir}/boost/libs/any/include
    ${dependency_dir}/boost/libs/array/include
    ${dependency_dir}/boost/libs/assert/include
    ${dependency_dir}/boost/libs/bimap/include
    ${dependency_dir}/boost/libs/bind/include
    ${dependency_dir}/boost/libs/concept_check/include
    ${dependency_dir}/boost/libs/config/include
    ${dependency_dir}/boost/libs/container_hash/include
    ${dependency_dir}/boost/libs/conversion/include
    ${dependency_dir}/boost/libs/core/include
    ${dependency_dir}/boost/libs/detail/include
    ${dependency_dir}/boost/libs/foreach/include
    ${dependency_dir}/boost/libs/function/include
    ${dependency_dir}/boost/libs/integer/include
    ${dependency_dir}/boost/libs/iterator/include
    ${dependency_dir}/boost/libs/lexical_cast/include
    ${dependency_dir}/boost/libs/math/include
    ${dependency_dir}/boost/libs/move/include
    ${dependency_dir}/boost/libs/mpl/include
    ${dependency_dir}/boost/libs/multi_index/include
    ${dependency_dir}/boost/libs/optional/include
    ${dependency_dir}/boost/libs/parameter/include
    ${dependency_dir}/boost/libs/preprocessor/include
    ${dependency_dir}/boost/libs/property_map/include
    ${dependency_dir}/boost/libs/property_tree/include
    ${dependency_dir}/boost/libs/random/include
    ${dependency_dir}/boost/libs/range/include
    ${dependency_dir}/boost/libs/serialization/include
    ${dependency_dir}/boost/libs/smart_ptr/include
    ${dependency_dir}/boost/libs/spirit/include
    ${dependency_dir}/boost/libs/static_assert/include
    ${dependency_dir}/boost/libs/throw_exception/include
    ${dependency_dir}/boost/libs/tti/include
    ${dependency_dir}/boost/libs/tuple/include
    ${dependency_dir}/boost/libs/type_traits/include
    ${dependency_dir}/boost/libs/typeof/include
    ${dependency_dir}/boost/libs/unordered/include
    ${dependency_dir}/boost/libs/utility/include
    ${dependency_dir}/boost/libs/xpressive/include
    ${dependency_dir}/boost/libs/regex/include
    ${dependency_dir}/boost/libs/mp11/include
    ${dependency_dir}/boost/libs/describe/include

    ${dependency_dir}/boost/libs/function_types/include

    ${dependency_dir}/boost/libs/regex/include
    ${dependency_dir}/boost/libs/lexical_cast/include
    ${dependency_dir}/boost/libs/container/include
    ${dependency_dir}/boost/libs/predef/include
    ${dependency_dir}/boost/libs/numeric/conversion/include

    ${dependency_dir}/boost/libs/json/include
    ${dependency_dir}/boost/libs/align/include
    ${dependency_dir}/boost/libs/intrusive/include
    ${dependency_dir}/boost/libs/system/include
    ${dependency_dir}/boost/libs/variant2/include

    ${dependency_dir}/boost/libs/algorithm/include
)

if(WIN32)
    target_include_directories (graph PUBLIC
        ${dependency_dir}/boost/libs/winapi/include
    )
endif()

if(MSVC)
    target_compile_definitions(graph PRIVATE
        BOOST_ALL_NO_LIB=1
        BOOST_JSON_NO_LIB=1
    )
endif()

target_link_libraries(graph PUBLIC 
    core
)