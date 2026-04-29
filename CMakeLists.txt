cmake_minimum_required(VERSION 3.27.1)

project(
    nil-gate
    VERSION 1.6.7
    LANGUAGES CXX
    DESCRIPTION "Tiny, header‑only, change‑driven DAG library (C++20) with C-API"
)

find_package(nil-xalt CONFIG REQUIRED)
set(CPACK_DEBIAN_PACKAGE_DEPENDS "nil-xalt-dev (>= ${nil-xalt_VERSION})")

find_package(nil-cmake CONFIG REQUIRED)
install_config("cmake/config.cmake.in")

add_subdirectory(src)
add_subdirectory(sandbox)