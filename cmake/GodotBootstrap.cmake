# Run bootstrap/codegen only when inputs change (incremental configure).

function(godot_run_bootstrap)
    set(_stamp "${CMAKE_BINARY_DIR}/.godot_bootstrap.stamp")
    set(_manifest "${CMAKE_SOURCE_DIR}/cmake/generated/codegen_manifest.json")
    set(_build_cmake "${CMAKE_SOURCE_DIR}/cmake/generated/godot_build.cmake")

    set(_inputs
        "${CMAKE_SOURCE_DIR}/cmake/scripts/godot_bootstrap.py"
        "${CMAKE_SOURCE_DIR}/cmake/scripts/godot_codegen.py"
        "${CMAKE_SOURCE_DIR}/cmake/scripts/scsub_parser.py"
    )

    set(_need_bootstrap OFF)
    if(NOT EXISTS "${_build_cmake}" OR NOT EXISTS "${_manifest}")
        set(_need_bootstrap ON)
    endif()

    if(NOT _need_bootstrap AND EXISTS "${_stamp}")
        file(TIMESTAMP "${_stamp}" _stamp_time)
        foreach(_input IN LISTS _inputs)
            file(TIMESTAMP "${_input}" _input_time)
            if(_input_time GREATER _stamp_time)
                set(_need_bootstrap ON)
                break()
            endif()
        endforeach()
    elseif(NOT EXISTS "${_stamp}")
        set(_need_bootstrap ON)
    endif()

    if(GODOT_FORCE_BOOTSTRAP)
        set(_need_bootstrap ON)
    endif()

    if(_need_bootstrap)
        message(STATUS "Running Godot bootstrap (codegen + source scan)...")
        set(_args
            "${CMAKE_SOURCE_DIR}/cmake/scripts/godot_bootstrap.py"
            --platform "${GODOT_PLATFORM}"
            --target "${GODOT_TARGET}"
            --arch "${GODOT_ARCH}"
            --platform-exporters "${GODOT_PLATFORM}"
            --platform-apis "${GODOT_PLATFORM}"
        )
        if(GODOT_DEV_BUILD)
            list(APPEND _args --dev-build)
        endif()
        if(GODOT_DISABLED_MODULES)
            list(APPEND _args --disabled-modules "${GODOT_DISABLED_MODULES}")
        endif()
        execute_process(
            COMMAND "${Python3_EXECUTABLE}" ${_args}
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
            RESULT_VARIABLE _bootstrap_result
            COMMAND_ECHO STDOUT
        )
        if(NOT _bootstrap_result EQUAL 0)
            message(FATAL_ERROR "godot_bootstrap.py failed with code ${_bootstrap_result}")
        endif()
        file(TOUCH "${_stamp}")
    else()
        message(STATUS "Godot bootstrap up-to-date (skipping codegen). Use -DGODOT_FORCE_BOOTSTRAP=ON to refresh.")
    endif()
endfunction()
