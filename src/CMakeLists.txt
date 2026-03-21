find_package(OpenSSL REQUIRED)

add_library(SqliteSyncProLib STATIC
    syncconfig.h
    syncresult.h
    schemainspector.h
    schemainspector.cpp
    httpclient.h
    httpclient.cpp
    authmanager.h
    authmanager.cpp
    rowencryption.h
    rowencryption.cpp
    syncengine.h
    syncengine.cpp
    syncworker.h
    syncworker.cpp
    syncloopworker.h
    syncloopworker.cpp
    sqlitesyncpro.h
    sqlitesyncpro.cpp
    syncapisettingsdialog.h syncapisettingsdialog.cpp syncapisettingsdialog.ui
)

target_include_directories(SqliteSyncProLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(SqliteSyncProLib
    PUBLIC
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Network
        Qt${QT_VERSION_MAJOR}::Sql
        Qt${QT_VERSION_MAJOR}::Widgets
    PRIVATE
        OpenSSL::Crypto
)
