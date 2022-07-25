cmake_minimum_required(VERSION 3.10)

project(openssl-server C CXX)

find_package(OpenSSL REQUIRED)

add_executable("${PROJECT_NAME}" openssl-server.cpp)
target_link_libraries("${PROJECT_NAME}" pthread OpenSSL::SSL OpenSSL::Crypto socket-wrapper)

if(WIN32)
  target_link_libraries("${PROJECT_NAME}" wsock32 ws2_32)
endif()

