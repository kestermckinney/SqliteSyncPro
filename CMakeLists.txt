cmake_minimum_required(VERSION 3.16)

# Tell vcpkg to build dynamic libraries instead of static.
# Must come before project() so the vcpkg toolchain file sees it.
if(NOT VCPKG_TARGET_TRIPLET)
    if(APPLE)
        execute_process(COMMAND uname -m
            OUTPUT_VARIABLE _host_arch OUTPUT_STRIP_TRAILING_WHITESPACE)
        if(_host_arch MATCHES "arm64")
            set(VCPKG_TARGET_TRIPLET "arm64-osx-dynamic" CACHE STRING "vcpkg target triplet")
        else()
            set(VCPKG_TARGET_TRIPLET "x64-osx-dynamic" CACHE STRING "vcpkg target triplet")
        endif()
    endif()
endif()

project(SqliteSyncPro VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Network Sql Widgets Test)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Network Sql Widgets Test)

enable_testing()

add_subdirectory(src)

if(NOT IOS)
    add_subdirectory(tests)
    add_subdirectory(examples/invoice)
    add_subdirectory(admin)
endif()

