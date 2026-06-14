# Expand 8.3 short paths in compile_commands.json so tooling can resolve MSVC system headers.
function(normalize_compile_commands json_path)
    if(NOT WIN32 OR NOT EXISTS "${json_path}")
        return()
    endif()

    file(READ "${json_path}" _content)
    if(_content MATCHES "~[0-9]")
        set(_script "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../scripts/normalize_compile_commands.ps1")
        if(EXISTS "${_script}")
            execute_process(
                COMMAND powershell -NoProfile -ExecutionPolicy Bypass -File "${_script}" -Path "${json_path}"
                RESULT_VARIABLE _result)
            if(NOT _result EQUAL 0)
                message(WARNING "normalize_compile_commands.ps1 failed (${_result}); IDE may show false errors")
            endif()
        endif()
    endif()
endfunction()

function(install_compile_commands_symlink src dst)
    if(NOT EXISTS "${src}")
        return()
    endif()

    normalize_compile_commands("${src}")

    if(EXISTS "${dst}")
        file(REMOVE "${dst}")
    endif()

    file(CREATE_LINK "${src}" "${dst}" COPY_ON_ERROR SYMBOLIC)
endfunction()
