cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
project(lexical_cache)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
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

