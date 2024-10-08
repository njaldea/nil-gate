add_test_executable(
    traits_test
    traits/unbiased/node.cpp
    traits/unbiased/inputs.cpp
    traits/unbiased/sync_outputs.cpp
    traits/unbiased/async_outputs.cpp
)
target_link_libraries(traits_test PRIVATE gate)
target_link_libraries(traits_test PRIVATE GTest::gmock)
target_link_libraries(traits_test PRIVATE GTest::gtest)
target_link_libraries(traits_test PRIVATE GTest::gtest_main)

add_test_executable(
    traits_biased_test
    traits/biased/node.cpp
    traits/biased/inputs.cpp
    traits/biased/sync_outputs.cpp
    traits/biased/async_outputs.cpp
)
target_link_libraries(traits_biased_test PRIVATE gate)
target_link_libraries(traits_biased_test PRIVATE GTest::gmock)
target_link_libraries(traits_biased_test PRIVATE GTest::gtest)
target_link_libraries(traits_biased_test PRIVATE GTest::gtest_main)
