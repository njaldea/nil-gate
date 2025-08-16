add_test_executable(
    traits_test
    traits/unbiased/node.cpp
    traits/unbiased/inputs.cpp
    traits/unbiased/req_outputs.cpp
    traits/unbiased/opt_outputs.cpp
)
target_link_libraries(traits_test PRIVATE gate)
target_link_libraries(traits_test PRIVATE GTest::gmock)
target_link_libraries(traits_test PRIVATE GTest::gtest)
target_link_libraries(traits_test PRIVATE GTest::gtest_main)

add_test_executable(
    traits_biased_test
    traits/biased/node.cpp
    traits/biased/inputs.cpp
    traits/biased/req_outputs.cpp
    traits/biased/opt_outputs.cpp
)
target_link_libraries(traits_biased_test PRIVATE gate)
target_link_libraries(traits_biased_test PRIVATE GTest::gmock)
target_link_libraries(traits_biased_test PRIVATE GTest::gtest)
target_link_libraries(traits_biased_test PRIVATE GTest::gtest_main)

add_test_executable(
    nodes_test
    nodes/Scoped.cpp
    nodes/Deferred.cpp
)
target_link_libraries(nodes_test PRIVATE gate)
target_link_libraries(nodes_test PRIVATE GTest::gmock)
target_link_libraries(nodes_test PRIVATE GTest::gtest)
target_link_libraries(nodes_test PRIVATE GTest::gtest_main)

add_test_executable(
    gate_test
    gate.cpp
)
target_link_libraries(gate_test PRIVATE gate)
target_link_libraries(gate_test PRIVATE GTest::gmock)
target_link_libraries(gate_test PRIVATE GTest::gtest)
target_link_libraries(gate_test PRIVATE GTest::gtest_main)