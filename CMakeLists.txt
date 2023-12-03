CMAKE_MINIMUM_REQUIRED(VERSION 3.20)
project(ExternalSort)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${ExternalSort_SOURCE_DIR}/bin)

add_executable(main main.cpp exsort.cpp exsort.h)