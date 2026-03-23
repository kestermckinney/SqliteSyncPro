cmake_minimum_required(VERSION 3.16)

project(SQLSyncAdmin VERSION 0.1.0 LANGUAGES CXX)

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
    set(ICNS_FILE "${CMAKE_CURRENT_SOURCE_DIR}/icons/sqlsyncadmin.icns")
    if(EXISTS "${ICNS_FILE}")
        set_source_files_properties("${ICNS_FILE}" PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
        list(APPEND PLATFORM_SOURCES "${ICNS_FILE}")
    endif()
endif()

# ── Executable ────────────────────────────────────────────────────────────────

qt_add_executable(SQLSyncAdmin
    main.cpp
    backend/ServerMode.h
    backend/AdminController.h
    backend/AdminController.cpp
    backend/SetupWorker.h
    backend/SetupWorker.cpp
    backend/TeardownWorker.h
    backend/TeardownWorker.cpp
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

if(APPLE)
    set_target_properties(SQLSyncAdmin PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_ICON_FILE sqlsyncadmin
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.sqlitesyncpro.sqladmin"
        MACOSX_BUNDLE_BUNDLE_NAME "SQLSync Administrator"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    )
endif()

target_link_libraries(SQLSyncAdmin
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Sql
        Qt6::Network
)

if(WIN32)
    add_custom_command(TARGET SQLSyncAdmin POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different ${CMAKE_CURRENT_BINARY_DIR}/../vcpkg_installed/x64-windows/bin/ ${CMAKE_CURRENT_BINARY_DIR}/
        COMMAND ${CMAKE_COMMAND} -E echo "Copying vcpkg dlls"
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/../vcpkg_installed/x64-windows/bin
    )
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
