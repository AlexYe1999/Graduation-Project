cmake_minimum_required(VERSION 3.10.0)
project(Graduation-Project)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_subdirectory(source)