cmake_minimum_required(VERSION 3.16)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Sql Network)

qt_add_executable(invoice_demo
    main.cpp
    MainWindow.h
    MainWindow.cpp
    backend/InvoiceController.h
    backend/InvoiceController.cpp
)

target_link_libraries(invoice_demo
    PRIVATE
        SqliteSyncProLib
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Sql
        Qt6::Network
)

set_target_properties(invoice_demo PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE    TRUE
)
