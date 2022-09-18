cmake_minimum_required(VERSION 3.18)

project(pp VERSION 0.1.0)

include(CTest)

set(CMAKE_C_STANDARD 11)

find_package(OpenMP REQUIRED)

add_subdirectory(lab1)
