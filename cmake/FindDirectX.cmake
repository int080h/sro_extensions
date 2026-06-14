include(FindPackageHandleStandardArgs)

if(WIN32)
    find_path(DirectX_ROOT_DIR Include/d3d9.h PATHS
        "${DirectX_ROOT}"
        "${DirectX_ROOT}/dxsdk"
        "$ENV{DXSDK_DIR}/"
        NO_DEFAULT_PATH)

    if(DirectX_ROOT_DIR)
        set(DirectX_INCLUDE_DIRS "${DirectX_ROOT_DIR}/Include")
        set(DirectX_LIBRARY_PATHS "${DirectX_ROOT_DIR}/Lib")
        find_library(DirectX_D3D9_LIBRARY d3d9 PATHS ${DirectX_LIBRARY_PATHS} NO_DEFAULT_PATH)
        find_library(DirectX_D3DX9_LIBRARY d3dx9 PATHS ${DirectX_LIBRARY_PATHS} NO_DEFAULT_PATH)
    else()
        file(GLOB _winkit_versions LIST_DIRECTORIES true
            "C:/Program Files (x86)/Windows Kits/10/Include/*")
        list(SORT _winkit_versions)
        list(REVERSE _winkit_versions)
        list(GET _winkit_versions 0 _winkit_include_root)

        if(_winkit_include_root)
            get_filename_component(_winkit_ver "${_winkit_include_root}" NAME)
            set(DirectX_INCLUDE_DIRS
                "C:/Program Files (x86)/Windows Kits/10/Include/${_winkit_ver}/um"
                "C:/Program Files (x86)/Windows Kits/10/Include/${_winkit_ver}/shared")
            set(DirectX_LIBRARY_PATHS
                "C:/Program Files (x86)/Windows Kits/10/Lib/${_winkit_ver}/um/x86")
            find_library(DirectX_D3D9_LIBRARY NAMES d3d9 PATHS ${DirectX_LIBRARY_PATHS} NO_DEFAULT_PATH)
            find_library(DirectX_D3DX9_LIBRARY NAMES d3dx9 PATHS ${DirectX_LIBRARY_PATHS} NO_DEFAULT_PATH)
        endif()
    endif()

    if(NOT DirectX_D3D9_LIBRARY)
        set(DirectX_D3D9_LIBRARY d3d9)
    endif()

    set(DirectX_LIBRARIES ${DirectX_D3D9_LIBRARY})
    if(DirectX_D3DX9_LIBRARY)
        list(APPEND DirectX_LIBRARIES ${DirectX_D3DX9_LIBRARY})
    endif()

    find_package_handle_standard_args(DirectX DEFAULT_MSG DirectX_INCLUDE_DIRS DirectX_D3D9_LIBRARY)
endif()
