### gmock+gtest
set(GMOCK_VERSION 1.7.0)
set(GMOCK_DIR gmock-${GMOCK_VERSION}
    CACHE PATH "The path to the GoogleMock framework.")

add_subdirectory(${GMOCK_DIR} ${CMAKE_BINARY_DIR}/gmock)

# all tests are build here
#include_directories(
#    ${PROJECT_SOURCE_DIR}/src/**/*)
include_directories(
    SYSTEM
    ${GMOCK_DIR}/include
    ${GMOCK_DIR}/gtest/include
    ${PROJECT_SOURCE_DIR}/test/unit
    )

add_executable(StringToFloatPointTest unit/StringToFloatPointTest.cpp)
target_link_libraries(StringToFloatPointTest gtest gtest_main gmock gmock_main)

add_executable(StringToFloatPointPerfTest perf/StringToFloatPointPerfTest.cpp)
target_link_libraries(StringToFloatPointPerfTest gtest gtest_main gmock gmock_main)

#set_tests_properties(test1 [test2...] PROPERTIES prop1 value1 prop2 value2)
# see the list of properties here:
# http://www.cmake.org/cmake/help/v3.0/manual/cmake-properties.7.html
add_custom_target(check
    COMMAND ${CMAKE_CTEST_COMMAND}
    DEPENDS StringToFloatPointTest StringToFloatPointPerfTest)

add_custom_target(unit
    COMMAND ${CMAKE_BINARY_DIR}/test/StringToFloatPointTest
    DEPENDS StringToFloatPointTest)

add_test(UnitTest StringToFloatPointTest)
add_test(PerfTest StringToFloatPointPerfTest)
