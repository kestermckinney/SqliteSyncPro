cmake_minimum_required(VERSION 3.16)

project(ProjectNotesRemoteHost VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Sql Network)

# ── Platform-specific executable icon ────────────────────────────────────────

set(PLATFORM_SOURCES "")

if(WIN32)
    # Embed the .ico via a Windows resource file.
    # The .rc file must sit next to the .ico at build time; we copy it.
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/icons/sqlsyncadmin.rc"
        "${CMAKE_CURRENT_BINARY_DIR}/sqlsyncadmin.rc"
        COPYONLY)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/icons/sqlsyncadmin.ico"
        "${CMAKE_CURRENT_BINARY_DIR}/sqlsyncadmin.ico"
        COPYONLY)
    list(APPEND PLATFORM_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/sqlsyncadmin.rc")
elseif(APPLE)
    # Bundle the .icns for macOS (generate it from the PNGs with iconutil on a Mac).
    # See the note at the bottom of this file for the iconutil command.
    set(ICNS_FILE "${CMAKE_CURRENT_SOURCE_DIR}/icons/sqlsyncadmin.icns")
    set_source_files_properties("${ICNS_FILE}" PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    list(APPEND PLATFORM_SOURCES "${ICNS_FILE}")
    set(MACOSX_BUNDLE_ICON_FILE sqlsyncadmin.icns)
    set(APP_ICON_MACOS "${CMAKE_CURRENT_SOURCE_DIR}/icons/sqlsyncadmin.icns")
endif()

# ── Executable ────────────────────────────────────────────────────────────────

qt_add_executable(ProjectNotesRemoteHost WIN32
    main.cpp
    backend/ServerMode.h
    backend/AdminController.h
    backend/AdminController.cpp
    backend/SetupWorker.h
    backend/SetupWorker.cpp
    backend/TeardownWorker.h
    backend/TeardownWorker.cpp
    backend/UpgradeWorker.h
    backend/UpgradeWorker.cpp
    ui/MainWindow.h
    ui/MainWindow.cpp
    ui/WizardSidebar.h
    ui/WizardSidebar.cpp
    ui/pages/ConnectionPage.h
    ui/pages/ConnectionPage.cpp
    ui/pages/ConfigurePage.h
    ui/pages/ConfigurePage.cpp
    ui/pages/InstallPage.h
    ui/pages/InstallPage.cpp
    ui/pages/SummaryPage.h
    ui/pages/SummaryPage.cpp
    ui/pages/UsersPage.h
    ui/pages/UsersPage.cpp
    ui/pages/TeardownPage.h
    ui/pages/TeardownPage.cpp
    icons/sqlsyncadmin.qrc
    ${PLATFORM_SOURCES}
)

set_target_properties(ProjectNotesRemoteHost PROPERTIES OUTPUT_NAME "Project Notes Remote Host")

if(APPLE)
    set_target_properties(ProjectNotesRemoteHost PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_ICON_FILE sqlsyncadmin
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.sqlitesyncpro.sqladmin"
        MACOSX_BUNDLE_BUNDLE_NAME "Project Notes Remote Host"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    )

    get_target_property(_qmake_loc Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin "${_qmake_loc}" DIRECTORY)

    # Step 1: Copy the app icon into Contents/Resources/.
    # set_source_files_properties is not reliably honoured by qt_add_executable,
    # so we use an explicit POST_BUILD copy instead.
    add_custom_command(TARGET ProjectNotesRemoteHost POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_BUNDLE_DIR:ProjectNotesRemoteHost>/Contents/Resources"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${ICNS_FILE}"
            "$<TARGET_BUNDLE_DIR:ProjectNotesRemoteHost>/Contents/Resources/sqlsyncadmin.icns"
        COMMENT "Copying app icon into bundle Resources"
        VERBATIM
    )

    # Step 2: Copy sqldrivers next to the executable for Qt Creator dev runs.
    # Qt resolves plugins relative to the executable when the bundle hasn't been
    # fully deployed, so Contents/MacOS/sqldrivers/ is needed for in-IDE runs.
    add_custom_command(TARGET ProjectNotesRemoteHost POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:ProjectNotesRemoteHost>/sqldrivers"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${_qt_bin}/../plugins/sqldrivers"
            "$<TARGET_FILE_DIR:ProjectNotesRemoteHost>/sqldrivers"
        COMMENT "Copying Qt SQL plugins next to executable"
        VERBATIM
    )

    # Step 2: Deploy Qt frameworks and all standard plugins into the bundle.
    # This populates Contents/Frameworks/ (Qt frameworks) and Contents/PlugIns/
    # (platforms, imageformats, styles, tls, sqldrivers, …) for both Debug and
    # Release so the app is self-contained and runnable outside Qt Creator.
    add_custom_command(TARGET ProjectNotesRemoteHost POST_BUILD
        COMMAND "${_qt_bin}/macdeployqt"
            "$<TARGET_BUNDLE_DIR:ProjectNotesRemoteHost>"
            -always-overwrite
        COMMENT "Deploying Qt into app bundle (macdeployqt)"
        VERBATIM
    )

    # Step 3: Copy vcpkg dylibs (libpq, libssl, libcrypto, …) into Contents/Frameworks/,
    # fix install names and cross-references, and patch every libqsqlpsql.dylib in the
    # bundle (both MacOS/sqldrivers/ and PlugIns/sqldrivers/ placed by macdeployqt).
    add_custom_command(TARGET ProjectNotesRemoteHost POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:ProjectNotesRemoteHost>/../Frameworks"
        COMMAND ${CMAKE_COMMAND}
            "-DVCPKG_LIB=$<IF:$<CONFIG:Debug>,${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/debug/lib,${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/lib>"
            "-DVCPKG_GLOBAL_LIB=$<IF:$<CONFIG:Debug>,${Z_VCPKG_ROOT_DIR}/installed/${VCPKG_TARGET_TRIPLET}/debug/lib,${Z_VCPKG_ROOT_DIR}/installed/${VCPKG_TARGET_TRIPLET}/lib>"
            "-DFRAMEWORKS=$<TARGET_FILE_DIR:ProjectNotesRemoteHost>/../Frameworks"
            "-DBUNDLE=$<TARGET_BUNDLE_DIR:ProjectNotesRemoteHost>"
            "-DEXECUTABLE=$<TARGET_FILE:ProjectNotesRemoteHost>"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/BundleMacLibs.cmake"
        COMMENT "Bundling vcpkg dylibs into Contents/Frameworks and patching rpaths"
        VERBATIM
    )
endif()

target_link_libraries(ProjectNotesRemoteHost
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Sql
        Qt6::Network
)

# ── Copy vcpkg PostgreSQL runtime DLLs next to the executable ─────────────────
# Qt's QPSQL driver plugin (qsqlpsql.dll) needs libpq.dll at runtime.
# vcpkg puts release DLLs in x64-windows/bin and debug in x64-windows/debug/bin.
# Copying them here means the exe finds them in both Debug and Release without
# any PATH changes in Qt Creator run configurations.
if(WIN32)
    set(_VCPKG_BIN_DIR "${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows")
    set(_PG_DLLS libpq.dll libpgtypes.dll libecpg.dll libecpg_compat.dll)

    foreach(_dll ${_PG_DLLS})
        set(_release_dll "${_VCPKG_BIN_DIR}/bin/${_dll}")
        set(_debug_dll "${_VCPKG_BIN_DIR}/debug/bin/${_dll}")
        if(EXISTS "${_release_dll}" OR EXISTS "${_debug_dll}")
            add_custom_command(TARGET ProjectNotesRemoteHost POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "$<IF:$<CONFIG:Debug>,${_debug_dll},${_release_dll}>"
                    "$<TARGET_FILE_DIR:ProjectNotesRemoteHost>/${_dll}"
                COMMENT "Copying ${_dll} for $<CONFIG>"
            )
        endif()
    endforeach()
endif()

# ── Notes ─────────────────────────────────────────────────────────────────────
# The developer must have libpq (PostgreSQL client library) installed:
#   Linux:   sudo apt install libpq-dev   (or dnf install libpq-devel)
#   macOS:   brew install libpq
#   Windows: install PostgreSQL and add its lib/ folder to PATH
#
# macOS .icns generation (run once on a Mac):
#   mkdir sqlsyncadmin.iconset
#   for s in 16 32 64 128 256 512; do
#     cp icons/sqlsyncadmin_${s}.png sqlsyncadmin.iconset/icon_${s}x${s}.png
#     cp icons/sqlsyncadmin_$((s*2)).png sqlsyncadmin.iconset/icon_${s}x${s}@2x.png 2>/dev/null || true
#   done
#   iconutil -c icns sqlsyncadmin.iconset -o icons/sqlsyncadmin.icns
