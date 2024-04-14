cmake_minimum_required(VERSION 3.27.1)

project(nil-gate CXX)

include(cmake/package.cmake)
include(cmake/toolchain.cmake)
include(cmake/quality.cmake)
include(cmake/test.cmake)
include(cmake/coverage.cmake)

add_subdirectory(src)
add_test_subdirectory()
