project(sandbox)

set(ENABLE_SANDBOX  OFF CACHE BOOL "[0 | OFF - 1 | ON]: build sandbox?")
if(NOT ENABLE_SANDBOX)
    return()
endif()

find_package(Boost REQUIRED)

add_executable(${PROJECT_NAME}_basic basic.cpp)
target_link_libraries(${PROJECT_NAME}_basic PRIVATE gate)

add_executable(${PROJECT_NAME}_opt opt.cpp)
target_link_libraries(${PROJECT_NAME}_opt PRIVATE gate)
target_link_libraries(${PROJECT_NAME}_opt PRIVATE Boost::boost)

add_executable(${PROJECT_NAME}_runner runner.cpp)
target_link_libraries(${PROJECT_NAME}_runner PRIVATE gate)
target_link_libraries(${PROJECT_NAME}_runner PRIVATE Boost::boost)

add_executable(${PROJECT_NAME}_uniform uniform.cpp)
target_link_libraries(${PROJECT_NAME}_uniform PRIVATE gate)
target_link_libraries(${PROJECT_NAME}_uniform PRIVATE Boost::boost)