cmake_minimum_required(VERSION 3.27.1)

project(
    nil-gate
    VERSION 1.6.0
    LANGUAGES CXX
    DESCRIPTION "Tiny, header‑only, change‑driven DAG library (C++20) with C-API"
)

include(cmake/package.cmake)
include(cmake/toolchain.cmake)
include(cmake/quality.cmake)
include(cmake/test.cmake)
include(cmake/coverage.cmake)
include(cmake/cpack.cmake)

add_subdirectory(src)
add_subdirectory(sandbox)