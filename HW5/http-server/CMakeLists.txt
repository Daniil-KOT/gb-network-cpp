cmake_minimum_required(VERSION 3.14)

project(http-server C CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(${PROJECT_NAME}_SRC
    main.cpp
    http-server.h
    http-server.cpp
    socket-runnable.h
    socket-runnable.cpp
    server-control.h
    server-control.cpp
    response-header.h)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Core Network REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Network REQUIRED)

add_executable("${PROJECT_NAME}"
  "${${PROJECT_NAME}_SRC}"
)
target_link_libraries("${PROJECT_NAME}" Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Network)
