# CyaeghaSCons.cmake - Bridge CMake options to SCons invocations.

include_guard(GLOBAL)

find_package(Python3 COMPONENTS Interpreter REQUIRED)

# SCons is typically installed via pip and invoked as `python -m SCons`.
set(_scons_candidates
    "${Python3_EXECUTABLE}" "-m" "SCons"
)

find_program(_scons_script NAMES scons scons.py scons.exe
    PATHS
        "${Python3_EXECUTABLE_DIR}"
        "${Python3_ROOT_DIR}/Scripts"
    NO_DEFAULT_PATH
)

if(_scons_script)
    set(CYAEgha_SCONS_COMMAND "${Python3_EXECUTABLE}" "${_scons_script}")
else()
    set(CYAEgha_SCONS_COMMAND ${_scons_candidates})
endif()

function(cyaegha_scons_build_args out_var)
    set(_args)

    if(CYAEgha_PLATFORM)
        list(APPEND _args "platform=${CYAEgha_PLATFORM}")
    endif()

    if(CYAEgha_TARGET)
        list(APPEND _args "target=${CYAEgha_TARGET}")
    endif()

    if(CYAEgha_ARCH AND NOT CYAEgha_ARCH STREQUAL "auto")
        list(APPEND _args "arch=${CYAEgha_ARCH}")
    endif()

    if(CYAEgha_DEV_BUILD)
        list(APPEND _args "dev_build=yes")
    endif()

    if(CYAEgha_DEBUG_SYMBOLS)
        list(APPEND _args "debug_symbols=yes")
    endif()

    if(CYAEgha_VERBOSE)
        list(APPEND _args "verbose=yes")
    endif()

    if(CYAEgha_USE_NINJA)
        list(APPEND _args "ninja=yes")
        if(NOT CYAEgha_NINJA_AUTO_RUN)
            list(APPEND _args "ninja_auto_run=no")
        endif()
        if(CYAEgha_NINJA_FILE)
            list(APPEND _args "ninja_file=${CYAEgha_NINJA_FILE}")
        endif()
    endif()

    if(CYAEgha_COMPILEDB)
        list(APPEND _args "compiledb=yes")
    endif()

    if(CYAEgha_USE_MINGW)
        list(APPEND _args "use_mingw=yes")
    endif()

    if(CYAEgha_ACCESSKIT)
        list(APPEND _args "accesskit=yes")
    else()
        list(APPEND _args "accesskit=no")
    endif()

    if(CYAEgha_D3D12)
        list(APPEND _args "d3d12=yes")
    else()
        list(APPEND _args "d3d12=no")
    endif()

    if(CYAEgha_ANGLE)
        list(APPEND _args "angle=yes")
    else()
        list(APPEND _args "angle=no")
    endif()

    if(CYAEgha_JOBS GREATER 0)
        list(APPEND _args "-j${CYAEgha_JOBS}")
    elseif(CYAEgha_JOBS_STRING)
        list(APPEND _args "num_jobs=${CYAEgha_JOBS_STRING}")
    endif()

    if(CYAEgha_SCONS_EXTRA_ARGS)
        separate_arguments(_extra UNIX_COMMAND "${CYAEgha_SCONS_EXTRA_ARGS}")
        list(APPEND _args ${_extra})
    endif()

    set(${out_var} ${_args} PARENT_SCOPE)
endfunction()

function(cyaegha_guess_output_binary out_var)
    set(_suffix ".${CYAEgha_PLATFORM}.${CYAEgha_TARGET}")
    if(CYAEgha_DEV_BUILD)
        set(_suffix "${_suffix}.dev")
    endif()
    if(CYAEgha_ARCH AND NOT CYAEgha_ARCH STREQUAL "auto")
        set(_suffix "${_suffix}.${CYAEgha_ARCH}")
    else()
        set(_suffix "${_suffix}.x86_64")
    endif()

    if(WIN32)
        set(_binary "${CMAKE_SOURCE_DIR}/bin/godot${_suffix}.exe")
    else()
        set(_binary "${CMAKE_SOURCE_DIR}/bin/godot${_suffix}")
    endif()

    set(${out_var} "${_binary}" PARENT_SCOPE)
endfunction()
