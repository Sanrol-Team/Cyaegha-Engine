# Windows MSVC compiler/link settings for native CMake builds.

function(godot_configure_windows_msvc)
    add_compile_definitions(${GODOT_MODULE_DEFINES})
    add_compile_definitions(
        WINDOWS_ENABLED
        WASAPI_ENABLED
        WINMIDI_ENABLED
        TYPED_METHOD_BIND
        WIN32
        WINVER=0x0A00
        _WIN32_WINNT=0x0A00
        NOMINMAX
        _WIN64
        THREADS_ENABLED
        MINIZIP_ENABLED
        BROTLI_ENABLED
        CLIPPER2_ENABLED
        ZSTD_STATIC_LINKING_ONLY
        VK_USE_PLATFORM_WIN32_KHR
        USE_VOLK
        VMA_EXTERNAL_MEMORY_WIN32=0
        VULKAN_ENABLED
        GLES3_ENABLED
        RD_ENABLED
        GODOT_MODULE
    )

    if(GODOT_EDITOR_BUILD)
        add_compile_definitions(
            TOOLS_ENABLED DEBUG_ENABLED ENGINE_UPDATE_CHECK_ENABLED OVERRIDE_PATH_ENABLED
        )
    endif()

    if(GODOT_DEV_BUILD)
        add_compile_definitions(DEV_ENABLED)
    else()
        add_compile_definitions(NDEBUG)
    endif()

    add_compile_options(
        /nologo
        /utf-8
        /bigobj
        /fp:strict
        /Gd
        /GR
        /EHsc
        /MD
        /W3
        /std:c++17
        /FS
    )

    set(GODOT_WINDOWS_LIBS
        winmm dsound kernel32 ole32 oleaut32 sapi user32 gdi32 IPHLPAPI
        Shlwapi Shcore wsock32 Ws2_32 shell32 advapi32 dinput8 dxguid imm32
        bcrypt Crypt32 Avrt dwmapi dwrite wbemuuid ntdll hid mincore
        psapi dbghelp
    )
endfunction()
