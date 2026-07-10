# Shared compile settings for all Godot targets (enables MSVC incremental .obj reuse).

add_library(godot_common INTERFACE)

target_compile_features(godot_common INTERFACE cxx_std_17)
target_include_directories(godot_common INTERFACE
    "${CMAKE_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/platform/windows"
    "${CMAKE_SOURCE_DIR}/thirdparty/zlib"
    "${CMAKE_SOURCE_DIR}/thirdparty/brotli/include"
    "${CMAKE_SOURCE_DIR}/thirdparty/clipper2/include"
    "${CMAKE_SOURCE_DIR}/thirdparty/zstd"
    "${CMAKE_SOURCE_DIR}/thirdparty/zstd/common"
    "${CMAKE_SOURCE_DIR}/thirdparty/vulkan/include"
    "${CMAKE_SOURCE_DIR}/thirdparty/volk"
    "${CMAKE_SOURCE_DIR}/thirdparty/spirv-headers/include"
    "${CMAKE_SOURCE_DIR}/thirdparty/libpng"
)

target_compile_definitions(godot_common INTERFACE GODOT_MODULE)

if(MSVC)
    target_compile_options(godot_common INTERFACE
        /Zc:__cplusplus
        /FS
        /utf-8
        /bigobj
        /fp:strict
        /EHsc
        /W3
    )
endif()

function(godot_apply_common target)
    target_link_libraries(${target} PUBLIC godot_common)
    set_target_properties(${target} PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
    )
endfunction()
