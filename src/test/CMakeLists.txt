add_test_executable(traits_test traits/node.cpp)
target_link_libraries(traits_test PRIVATE gate)
target_link_libraries(traits_test PRIVATE GTest::gmock)
target_link_libraries(traits_test PRIVATE GTest::gtest)
target_link_libraries(traits_test PRIVATE GTest::gtest_main)

add_test_executable(traits_biased_test traits/biased_node.cpp)
target_link_libraries(traits_biased_test PRIVATE gate)
target_link_libraries(traits_biased_test PRIVATE GTest::gmock)
target_link_libraries(traits_biased_test PRIVATE GTest::gtest)
target_link_libraries(traits_biased_test PRIVATE GTest::gtest_main)
