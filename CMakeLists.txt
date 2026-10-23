cmake_minimum_required(VERSION 3.10)
project(GeneticAlgorithmToolbox CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4 /EHsc)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# The toolbox: every .cpp in src/ except the CVRP demo program.
file(GLOB TOOLBOX_SOURCES CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/src/*.cpp)
list(FILTER TOOLBOX_SOURCES EXCLUDE REGEX ".*/main\\.cpp$")

add_library(gatoolbox STATIC ${TOOLBOX_SOURCES})
target_include_directories(gatoolbox PUBLIC ${PROJECT_SOURCE_DIR}/include)

# The CVRP solver:  ga_cvrp [trials] [seed]
add_executable(ga_cvrp ${PROJECT_SOURCE_DIR}/src/main.cpp)
target_link_libraries(ga_cvrp PRIVATE gatoolbox)

# One test program per tests/test_*.cpp; run them all with `ctest`.
enable_testing()
file(GLOB TEST_SOURCES CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/tests/test_*.cpp)
foreach(test_source ${TEST_SOURCES})
    get_filename_component(test_name ${test_source} NAME_WE)
    add_executable(${test_name} ${test_source})
    target_include_directories(${test_name} PRIVATE ${PROJECT_SOURCE_DIR}/tests)
    target_link_libraries(${test_name} PRIVATE gatoolbox)
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()
