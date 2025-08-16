project(gate)

find_package(nil-xalt CONFIG REQUIRED)

set(
    HEADERS
        publish/nil/gate.hpp
        publish/nil/gate/Batch.hpp
        publish/nil/gate/Core.hpp
        publish/nil/gate/Diffs.hpp
        publish/nil/gate/INode.hpp
        publish/nil/gate/IPort.hpp
        publish/nil/gate/ICallable.hpp
        publish/nil/gate/types.hpp
        publish/nil/gate/uniform_api.hpp
        publish/nil/gate/bias/compatibility.hpp
        publish/nil/gate/bias/portify.hpp
        publish/nil/gate/bias/is_port_type_valid.hpp
        publish/nil/gate/bias/nil.hpp
        publish/nil/gate/runners/Immediate.hpp
        publish/nil/gate/traits/compatibility.hpp
        publish/nil/gate/traits/portify.hpp
        publish/nil/gate/traits/is_port_type_valid.hpp
        publish/nil/gate/detail/Port.hpp
        publish/nil/gate/detail/Node.hpp
        publish/nil/gate/detail/traits/node.hpp
        publish/nil/gate/detail/validation.hpp
        publish/nil/gate/ports/Batch.hpp
        publish/nil/gate/ports/Mutable.hpp
        publish/nil/gate/ports/ReadOnly.hpp
        publish/nil/gate/nodes/Scoped.hpp
)

add_library(${PROJECT_NAME} INTERFACE ${HEADERS})

target_link_libraries(
    ${PROJECT_NAME} INTERFACE nil::xalt
)

target_include_directories(
    ${PROJECT_NAME}
    INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/publish>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

add_test_subdirectory()

nil_install_headers()
nil_install_targets(${PROJECT_NAME})
