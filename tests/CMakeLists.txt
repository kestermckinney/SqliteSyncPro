include_directories(${CMAKE_SOURCE_DIR}/src)

function(add_qt_test name source)
    add_executable(${name} ${source})
    target_link_libraries(${name}
        PRIVATE
            SqliteSyncProLib
            Qt${QT_VERSION_MAJOR}::Test
    )
    add_test(NAME ${name} COMMAND ${name})
endfunction()

add_qt_test(tst_schemainspector tst_schemainspector.cpp)
add_qt_test(tst_authmanager     tst_authmanager.cpp)
add_qt_test(tst_syncengine      tst_syncengine.cpp)
add_qt_test(tst_threading       tst_threading.cpp)
