cmake_minimum_required(VERSION 3.10)

project(sniffer-pcap++ C CXX)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")

include(FindPkgConfig)

pkg_check_modules(PCAPPP REQUIRED PcapPlusPlus)

find_package(PCAP REQUIRED)

set(${PROJECT_NAME}_SRC sniffer-pcap++.cpp)

source_group(source FILES ${${PROJECT_NAME}_SRC})

add_executable("${PROJECT_NAME}" "${${PROJECT_NAME}_SRC}")

target_include_directories("${PROJECT_NAME}" SYSTEM PRIVATE "${PCAPPP_INCLUDE_DIRS}")

target_link_directories("${PROJECT_NAME}" PRIVATE "${PCAPPP_LIBRARY_DIRS}")

target_link_libraries("${PROJECT_NAME}" "${PCAPP_LIBRARIES}")

target_link_libraries("${PROJECT_NAME}" socket-wrapper pcap::pcap)
