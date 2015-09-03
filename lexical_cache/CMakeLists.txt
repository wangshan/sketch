cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
project(lexical_cache)

## Compiler flags
#if(CMAKE_COMPILER_IS_GNUCXX)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
#endif()

# you can use this if cmake > 3.1
#set_property(TARGET tgt PROPERTY CXX_STANDARD 11)

# this has to be in the project's root cmake file
enable_testing()

include_directories(${PROJECT_SOURCE_DIR}/include)

add_subdirectory(test)

### install headers
install(FILES 
    ${PROJECT_SOURCE_DIR}/include/lexical_cache/lexical_cache.h
    DESTINATION ${PROJECT_SOURCE_DIR}/dist/include)

