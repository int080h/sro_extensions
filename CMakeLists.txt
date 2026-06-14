cmake_minimum_required(VERSION 3.20)

project(sro_ext_client LANGUAGES C CXX)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "sro_ext_client must be built as Win32 (x86). Use the win32-release preset or build.bat.")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

set(ISROR_DEVKIT_ROOT "${CMAKE_SOURCE_DIR}/../isror_devkit" CACHE PATH "Path to isror_devkit for third-party deps")

include(cmake/toolchain.cmake)

set(_devkit_dxsdk "${ISROR_DEVKIT_ROOT}/source/third-party/dxsdk/Include/d3d9.h")
if(EXISTS "${_devkit_dxsdk}")
    set(DirectX_ROOT "${ISROR_DEVKIT_ROOT}/source/third-party")
else()
    set(DirectX_ROOT "")
    unset(DirectX_ROOT_DIR CACHE)
    unset(DirectX_D3D9_LIBRARY CACHE)
    unset(DirectX_D3DX9_LIBRARY CACHE)
endif()
find_package(DirectX REQUIRED)

add_subdirectory(third-party/minhook)
add_subdirectory(version_proxy)
add_subdirectory(ext_client)

if(CMAKE_EXPORT_COMPILE_COMMANDS)
    include(cmake/normalize_compile_commands.cmake)
    cmake_language(DEFER CALL install_compile_commands_symlink
        "${CMAKE_BINARY_DIR}/compile_commands.json"
        "${CMAKE_SOURCE_DIR}/compile_commands.json")
endif()
