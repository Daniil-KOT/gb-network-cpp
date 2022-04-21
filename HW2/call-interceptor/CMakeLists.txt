cmake_minimum_required(VERSION 3.10)

project(call-interceptor C CXX)

#set(CMAKE_COMPILER_IS_GNUCC TRUE)

add_library("${PROJECT_NAME}" SHARED call_interceptor.cpp)
target_link_libraries("${PROJECT_NAME}" dl)
set_property(TARGET "${PROJECT_NAME}" PROPERTY POSITION_INDEPENDENT_CODE ON)

