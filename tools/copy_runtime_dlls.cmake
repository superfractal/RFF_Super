# Created by Opus 5.5 on 2026-10-07.
#
# Copies the DLLs an executable loads from the MSYS2 prefix it was built against beside it, replacing stale bundled copies.

# Usage: cmake -DRFF_EXE=<exe> -DRFF_DLL_SOURCE=<prefix>/bin [-DRFF_DLL_DEST=<dir>] -P copy_runtime_dlls.cmake

if(POLICY CMP0207)
    cmake_policy(SET CMP0207 NEW)
endif()

if(NOT RFF_EXE OR NOT RFF_DLL_SOURCE)
    message(FATAL_ERROR "copy_runtime_dlls.cmake needs RFF_EXE and RFF_DLL_SOURCE")
endif()
if(NOT RFF_DLL_DEST)
    get_filename_component(RFF_DLL_DEST "${RFF_EXE}" DIRECTORY)
endif()

# objdump from the same prefix reads the PE import tables; script mode has to be told both.
get_filename_component(RFF_PREFIX "${RFF_DLL_SOURCE}" DIRECTORY)
set(CMAKE_GET_RUNTIME_DEPENDENCIES_PLATFORM "windows+pe")
set(CMAKE_GET_RUNTIME_DEPENDENCIES_TOOL "objdump")
set(CMAKE_GET_RUNTIME_DEPENDENCIES_COMMAND "${RFF_DLL_SOURCE}/objdump.exe")

# Probed from an empty folder: beside the real exe, the loader order would resolve to the very copies being refreshed.
set(probe_dir "$ENV{TEMP}/rff_runtime_dll_probe")
file(REMOVE_RECURSE "${probe_dir}")
file(MAKE_DIRECTORY "${probe_dir}")
file(COPY_FILE "${RFF_EXE}" "${probe_dir}/probe.exe")
file(GET_RUNTIME_DEPENDENCIES
    EXECUTABLES "${probe_dir}/probe.exe"
    DIRECTORIES "${RFF_DLL_SOURCE}"
    RESOLVED_DEPENDENCIES_VAR resolved
    UNRESOLVED_DEPENDENCIES_VAR unresolved
    # Windows API sets resolve at load time, not to a file.
    PRE_EXCLUDE_REGEXES "^api-ms-" "^ext-ms-"
    # Windows' own DLLs, and the Vulkan loader, which belongs to the GPU driver and must not be bundled.
    POST_EXCLUDE_REGEXES "[/\\\\][Ww][Ii][Nn][Dd][Oo][Ww][Ss][/\\\\]" "[/\\\\]vulkan-1\\.dll$"
)

set(copied 0)
foreach(dll IN LISTS resolved)
    cmake_path(IS_PREFIX RFF_PREFIX "${dll}" NORMALIZE inside)
    if(NOT inside)
        message(STATUS "Not bundled (outside ${RFF_PREFIX}): ${dll}")
        continue()
    endif()
    get_filename_component(name "${dll}" NAME)
    file(COPY_FILE "${dll}" "${RFF_DLL_DEST}/${name}" ONLY_IF_DIFFERENT RESULT failed)
    if(failed)
        message(WARNING "Could not copy ${name}: ${failed}")
    else()
        math(EXPR copied "${copied} + 1")
    endif()
endforeach()

# Only system DLLs and the Vulkan loader are expected here; anything else will be missing at run time.
foreach(dll IN LISTS unresolved)
    if(NOT dll MATCHES "^(api-ms-|ext-ms-)" AND NOT dll MATCHES "^[Vv][Uu][Ll][Kk][Aa][Nn]-1\\.[Dd][Ll][Ll]$")
        message(WARNING "Runtime DLL not found in ${RFF_DLL_SOURCE}: ${dll}")
    endif()
endforeach()
file(REMOVE_RECURSE "${probe_dir}")
message(STATUS "Runtime DLLs in ${RFF_DLL_DEST} match ${RFF_DLL_SOURCE} (${copied} checked)")
