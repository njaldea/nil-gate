include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}-config-version.cmake
    VERSION ${CMAKE_PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}-config-version.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${CMAKE_PROJECT_NAME}
)

install(FILES
    "${CMAKE_SOURCE_DIR}/LICENSE"
    DESTINATION ${CMAKE_INSTALL_DATADIR}/doc/${CMAKE_PROJECT_NAME}
)

function(install_config CONFIG)
    configure_package_config_file(
        ${CONFIG}
        ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}-config.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_DATADIR}/cmake/${CMAKE_PROJECT_NAME}
        NO_SET_AND_CHECK_MACRO
    )
endfunction()

function(nil_install_targets TARGET)
    install(TARGETS ${TARGET} EXPORT nil-${TARGET}-targets)
    install(
        EXPORT nil-${TARGET}-targets
        NAMESPACE nil::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${CMAKE_PROJECT_NAME}
    )
endfunction()

function(nil_install_headers TARGET TYPE)
    target_include_directories(
        ${TARGET}
        ${TYPE}
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/publish>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/${CMAKE_PROJECT_NAME}/${CMAKE_PROJECT_VERSION}>
    )
    install(DIRECTORY publish/ DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/${CMAKE_PROJECT_NAME}/${CMAKE_PROJECT_VERSION}")
endfunction()
