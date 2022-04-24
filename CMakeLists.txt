cmake_minimum_required(VERSION 3.13)
project(mtree VERSION 0.1.0.0
  DESCRIPTION "distanced-based metric-space data structure")

set(CMAKE_BUILD_TYPE Debug)


add_library(mtree INTERFACE)
target_include_directories(mtree INTERFACE include/)

add_executable(testmtree tests/test_mtree.cpp)
target_compile_options(testmtree PUBLIC -g -O0 -Wall -Wno-unused-variable)
target_link_libraries(testmtree mtree)


add_executable(runmtree tests/run_mtree.cpp)
target_compile_options(runmtree PUBLIC -g -Ofast -Wall -Wno-unused-variable)
target_link_libraries(runmtree mtree)

include(CTest)
add_test(NAME test1 COMMAND testmtree)

install(TARGETS mtree PUBLIC_HEADER DESTINATION include)

