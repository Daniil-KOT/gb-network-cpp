cmake_minimum_required(VERSION 3.10)

project(sniffer-pcap++ C CXX)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")

#find_package(pcapplusplus REQUIRED)
find_package(PCAP REQUIRED)

set(${PROJECT_NAME}_SRC sniffer-pcap++.cpp)

source_group(source FILES ${${PROJECT_NAME}_SRC})

add_executable("${PROJECT_NAME}" "${${PROJECT_NAME}_SRC}")

target_include_directories("${PROJECT_NAME}" SYSTEM PRIVATE "/usr/local/include/pcapplusplus")

target_link_libraries("${PROJECT_NAME}" socket-wrapper libCommon++.a libPacket++.a libPcap++.a pcap::pcap libpthread.a)
