# Use standard flags C++ latest for MSVC
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_compile_definitions(
    _CRT_SECURE_NO_DEPRECATE
    _CRT_NON_CONFORMING_SWPRINTFS
    _SILENCE_STDEXT_HASH_DEPRECATION_WARNINGS
    WINVER=0x0500
    _WIN32_WINNT=0x0500
)

if(MSVC)
    add_compile_options(/FS /MP /EHsc /std:c++latest)
    add_link_options(/SAFESEH:NO)
    link_libraries(legacy_stdio_definitions.lib)
    
    # Enable PDB generation and optimize compiler flags (/O2) for Release configuration
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi /O2 /Oi /Gy")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG:FULL /OPT:REF /OPT:ICF /MAP")
endif()

if(MSVC AND CMAKE_GENERATOR MATCHES "Ninja")
    get_filename_component(_msvc_cl_dir "${CMAKE_CXX_COMPILER}" DIRECTORY)
    get_filename_component(_msvc_host_dir "${_msvc_cl_dir}" DIRECTORY)
    get_filename_component(_msvc_bin_dir "${_msvc_host_dir}" DIRECTORY)
    get_filename_component(_msvc_tools_dir "${_msvc_bin_dir}" DIRECTORY)

    set(_msvc_include_dir "${_msvc_tools_dir}/include")
    set(_msvc_lib_dir "${_msvc_tools_dir}/lib/x86")

    if(EXISTS "${_msvc_include_dir}/cstddef")
        include_directories(SYSTEM "${_msvc_include_dir}")
    endif()
    if(EXISTS "${_msvc_lib_dir}/libcmt.lib")
        link_directories("${_msvc_lib_dir}")
    endif()

    file(GLOB _winkit_include_roots LIST_DIRECTORIES true
        "C:/Program Files (x86)/Windows Kits/10/Include/*")
    list(SORT _winkit_include_roots)
    list(REVERSE _winkit_include_roots)

    foreach(_winkit_include_root IN LISTS _winkit_include_roots)
        if(EXISTS "${_winkit_include_root}/ucrt" AND EXISTS "${_winkit_include_root}/um")
            get_filename_component(_winkit_ver "${_winkit_include_root}" NAME)
            set(_winkit_lib_root "C:/Program Files (x86)/Windows Kits/10/Lib/${_winkit_ver}")
            include_directories(SYSTEM
                "${_winkit_include_root}/ucrt"
                "${_winkit_include_root}/um"
                "${_winkit_include_root}/shared")
            if(EXISTS "${_winkit_lib_root}/ucrt/x86/libucrt.lib")
                link_directories("${_winkit_lib_root}/ucrt/x86")
            endif()
            if(EXISTS "${_winkit_lib_root}/um/x86/kernel32.lib")
                link_directories("${_winkit_lib_root}/um/x86")
            endif()
            break()
        endif()
    endforeach()

    unset(_msvc_cl_dir)
    unset(_msvc_host_dir)
    unset(_msvc_bin_dir)
    unset(_msvc_tools_dir)
    unset(_msvc_include_dir)
    unset(_msvc_lib_dir)
    unset(_winkit_include_roots)
    unset(_winkit_include_root)
    unset(_winkit_ver)
    unset(_winkit_lib_root)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/../output")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/../output")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/../output")
