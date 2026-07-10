# Build Godot static libraries and executable from generated source lists.

function(godot_add_static_lib target sources)
    add_library(${target} STATIC ${sources})
    godot_apply_common(${target})
    set_target_properties(${target} PROPERTIES FOLDER "Godot")
endfunction()

function(godot_add_module_lib target mod_upper sources includes)
    godot_add_static_lib(${target} "${sources}")
    if(includes)
        target_include_directories(${target} PUBLIC ${includes})
    endif()
endfunction()

function(godot_build_targets)
    godot_add_static_lib(godot_core "${GODOT_CORE_SOURCES}")
    godot_add_static_lib(godot_servers "${GODOT_SERVERS_SOURCES}")
    godot_add_static_lib(godot_scene "${GODOT_SCENE_SOURCES}")

    if(GODOT_EDITOR_BUILD)
        godot_add_static_lib(godot_editor "${GODOT_EDITOR_SOURCES}")
    endif()

    if(GODOT_DRIVERS_INCLUDES)
        godot_add_static_lib(godot_drivers "${GODOT_DRIVERS_SOURCES}")
        target_include_directories(godot_drivers PUBLIC ${GODOT_DRIVERS_INCLUDES})
    else()
        godot_add_static_lib(godot_drivers "${GODOT_DRIVERS_SOURCES}")
    endif()

    godot_add_static_lib(godot_platform "${GODOT_PLATFORM_SOURCES}")
    godot_add_static_lib(godot_main "${GODOT_MAIN_SOURCES}")
    godot_add_static_lib(godot_modules "${GODOT_MODULES_SOURCES}")

    foreach(mod_target IN LISTS GODOT_MODULE_TARGETS)
        string(REGEX REPLACE "^godot_module_" "" mod_name "${mod_target}")
        string(TOUPPER "${mod_name}" mod_upper)
        set(src_var "GODOT_MODULE_${mod_upper}_SOURCES")
        set(inc_var "GODOT_MODULE_${mod_upper}_INCLUDES")
        if(DEFINED ${inc_var})
            godot_add_module_lib(${mod_target} ${mod_upper} "${${src_var}}" "${${inc_var}}")
        else()
            godot_add_module_lib(${mod_target} ${mod_upper} "${${src_var}}" "")
        endif()
    endforeach()

    enable_language(RC)
    add_executable(${GODOT_BINARY_NAME} ${GODOT_WINDOWS_EXE_SOURCES})
    godot_apply_common(${GODOT_BINARY_NAME})

    if(GODOT_EDITOR_BUILD)
        target_sources(${GODOT_BINARY_NAME} PRIVATE platform/windows/godot_res.rc)
    else()
        target_sources(${GODOT_BINARY_NAME} PRIVATE platform/windows/godot_res_template.rc)
    endif()

    target_link_libraries(${GODOT_BINARY_NAME} PRIVATE ${GODOT_LINK_LIBS} ${GODOT_WINDOWS_LIBS})
    set_target_properties(${GODOT_BINARY_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/bin"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/bin"
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_SOURCE_DIR}/bin"
        RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_SOURCE_DIR}/bin"
    )
endfunction()
