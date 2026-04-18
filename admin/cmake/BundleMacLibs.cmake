# Copyright (C) 2026 Paul McKinney
# BundleMacLibs.cmake
# Post-build script that bundles vcpkg dynamic libraries into a macOS app bundle.
#
# Required variables (passed via -D on the cmake -P command line):
#   VCPKG_LIB        — project-local vcpkg lib/ (or debug/lib/) for the current build config
#   VCPKG_GLOBAL_LIB — global vcpkg lib/ directory (fallback source for libpq and other libs)
#   FRAMEWORKS       — path to Contents/Frameworks inside the built app bundle
#   BUNDLE           — root of the app bundle (e.g. ProjectNotesRemoteHost.app/)
#   EXECUTABLE       — path to the app's main executable (Contents/MacOS/<AppName>)

cmake_minimum_required(VERSION 3.16)

# ── helpers ───────────────────────────────────────────────────────────────────

# Silently add an rpath if not already present (install_name_tool errors if duplicate).
function(add_rpath_safe target rpath)
    execute_process(COMMAND otool -l "${target}"
        OUTPUT_VARIABLE _out OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
    if(NOT _out MATCHES "${rpath}")
        execute_process(COMMAND install_name_tool -add_rpath "${rpath}" "${target}"
            OUTPUT_QUIET ERROR_QUIET)
    endif()
endfunction()

# For a given dylib, find any load-command path that contains needle and rewrite
# it to @rpath/<new_name>. Handles absolute paths and any existing @rpath/versioned
# variant that differs from the target name.
function(retarget_dep dylib needle new_name)
    execute_process(COMMAND otool -L "${dylib}"
        OUTPUT_VARIABLE _links OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
    string(REPLACE "\n" ";" _lines "${_links}")
    foreach(_line ${_lines})
        string(FIND "${_line}" "${needle}" _pos)
        if(_pos GREATER -1)
            # Extract the path token (first non-whitespace field on the line)
            string(REGEX REPLACE "^[ \t]*([^ \t]+).*" "\\1" _old "${_line}")
            string(STRIP "${_old}" _old)
            if(_old AND NOT "${_old}" STREQUAL "@rpath/${new_name}")
                message(STATUS "  patching: ${_old} -> @rpath/${new_name}")
                execute_process(COMMAND install_name_tool
                    -change "${_old}" "@rpath/${new_name}" "${dylib}"
                    OUTPUT_QUIET ERROR_QUIET)
            endif()
        endif()
    endforeach()
endfunction()

# Re-sign a dylib with an ad-hoc signature.
# install_name_tool invalidates the original signature; on Apple Silicon with SIP,
# loading an unsigned dylib causes an immediate SIGKILL (Code Signature Invalid).
function(resign dylib)
    execute_process(COMMAND codesign --force --sign - "${dylib}"
        OUTPUT_QUIET ERROR_QUIET)
endfunction()

# Copy a dylib from src_dir to FRAMEWORKS if not already present there.
# Returns the name in out_var if copied (empty string if skipped).
function(bundle_lib_if_missing src_dir name out_var)
    set(${out_var} "" PARENT_SCOPE)
    set(_dest "${FRAMEWORKS}/${name}")
    if(NOT EXISTS "${_dest}")
        set(_src "${src_dir}/${name}")
        if(EXISTS "${_src}")
            get_filename_component(_real "${_src}" REALPATH)
            file(COPY_FILE "${_real}" "${_dest}" ONLY_IF_DIFFERENT)
            set(${out_var} "${name}" PARENT_SCOPE)
            message(STATUS "Bundled (global): ${name}")
        endif()
    endif()
endfunction()

# ── 1. Copy project-local vcpkg dylibs → Frameworks/ ─────────────────────────
# Resolves symlinks so every versioned name (e.g. libpq.5.dylib) is copied as a
# real file, not a symlink — dyld inside a bundle cannot follow symlinks reliably.

file(MAKE_DIRECTORY "${FRAMEWORKS}")
set(_bundled_names "")

if(EXISTS "${VCPKG_LIB}")
    file(GLOB _all_libs "${VCPKG_LIB}/*.dylib")
    foreach(_src ${_all_libs})
        get_filename_component(_name "${_src}" NAME)
        # Resolve symlink so we always write a real file under the original name
        get_filename_component(_real "${_src}" REALPATH)
        file(COPY_FILE "${_real}" "${FRAMEWORKS}/${_name}" ONLY_IF_DIFFERENT)
        list(APPEND _bundled_names "${_name}")
        message(STATUS "Bundled: ${_name}")
    endforeach()

    if(NOT _bundled_names)
        message(WARNING
            "BundleMacLibs: no dylibs found in ${VCPKG_LIB}\n"
            "Ensure vcpkg is configured with a dynamic triplet (e.g. arm64-osx-dynamic).")
    endif()
else()
    message(WARNING
        "BundleMacLibs: vcpkg lib dir not found: ${VCPKG_LIB}\n"
        "Run CMake to trigger vcpkg install with a dynamic triplet, then rebuild.")
endif()

# ── 1b. Supplement with global vcpkg dylibs missing from the local install ────
# libpq and related PostgreSQL client libs may not be in the project's manifest
# (to avoid a full from-source build), but the global vcpkg may have them.

if(EXISTS "${VCPKG_GLOBAL_LIB}")
    file(GLOB _global_libs "${VCPKG_GLOBAL_LIB}/*.dylib")
    foreach(_src ${_global_libs})
        get_filename_component(_name "${_src}" NAME)
        if(NOT EXISTS "${FRAMEWORKS}/${_name}")
            get_filename_component(_real "${_src}" REALPATH)
            file(COPY_FILE "${_real}" "${FRAMEWORKS}/${_name}" ONLY_IF_DIFFERENT)
            list(APPEND _bundled_names "${_name}")
            message(STATUS "Bundled (global vcpkg): ${_name}")
        endif()
    endforeach()
endif()

# ── 2. Fix install names and cross-references in each bundled dylib ───────────

foreach(_name ${_bundled_names})
    set(_lib "${FRAMEWORKS}/${_name}")
    message(STATUS "Fixing ${_name}:")

    # Set own install name
    execute_process(COMMAND install_name_tool -id "@rpath/${_name}" "${_lib}"
        OUTPUT_QUIET ERROR_QUIET)

    # Rewrite any cross-references to other bundled dylibs
    foreach(_dep_name ${_bundled_names})
        if(NOT "${_dep_name}" STREQUAL "${_name}")
            retarget_dep("${_lib}" "${_dep_name}" "${_dep_name}")
        endif()
    endforeach()

    # @loader_path lets co-located Frameworks dylibs find each other
    add_rpath_safe("${_lib}" "@loader_path")

    # Re-sign: install_name_tool invalidates the code signature, and Apple Silicon
    # with SIP will SIGKILL any process that maps an unsigned dylib page.
    resign("${_lib}")
endforeach()

# ── 3. Patch every libqsqlpsql.dylib in the bundle ───────────────────────────
# macdeployqt places a copy in Contents/PlugIns/sqldrivers/ (for distribution),
# and we place one in Contents/MacOS/sqldrivers/ (for Qt Creator dev runs).
# Both need the same treatment: retarget the libpq reference and add the rpath.
# The relative path from either sqldrivers/ location to Contents/Frameworks/ is
# always ../../Frameworks (both are two levels below Contents/).

file(GLOB_RECURSE _psql_plugins "${BUNDLE}/*libqsqlpsql.dylib")
if(_psql_plugins)
    foreach(_plugin ${_psql_plugins})
        message(STATUS "Patching ${_plugin}")
        retarget_dep("${_plugin}" "libpq.5.dylib" "libpq.5.dylib")
        add_rpath_safe("${_plugin}" "@loader_path/../../Frameworks")
        resign("${_plugin}")
    endforeach()
else()
    message(WARNING "BundleMacLibs: no libqsqlpsql.dylib found in ${BUNDLE}")
endif()

# ── 4. Ensure executable can find Frameworks dylibs ──────────────────────────

if(EXISTS "${EXECUTABLE}")
    add_rpath_safe("${EXECUTABLE}" "@executable_path/../Frameworks")
endif()
