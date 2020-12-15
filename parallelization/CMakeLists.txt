cmake_minimum_required(VERSION 2.8.10)

project(examples CXX)

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)

#if(COMPILER_SUPPORTS_CXX11)
#    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O3")
#else()
#    message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
#endif()


find_package(MPI REQUIRED)
find_package(OpenMP REQUIRED)
include_directories(${RS_INCLUDE_DIRS})
include_directories(${MPI_INCLUDE_PATH})


add_executable(baseline baseline.cpp)
add_executable(baseline-8ecc baseline.cpp)
add_executable(baseline-no-simd baseline.cpp)
add_executable(baseline-no-corrupt no_corrupt.cpp)
add_executable(omp omp.cpp)
add_executable(omp-8ecc omp.cpp)
add_executable(mpi mpi.cpp)
target_link_libraries(omp PRIVATE OpenMP::OpenMP_CXX)
target_link_libraries(omp-8ecc PRIVATE OpenMP::OpenMP_CXX)
target_link_libraries(mpi PRIVATE ${MPI_LIBRARIES})

add_custom_command(
        TARGET omp POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_SOURCE_DIR}/parallelization/collect.py
                ${CMAKE_CURRENT_BINARY_DIR}/collect.py)

target_compile_options(baseline-no-simd PRIVATE -fno-tree-vectorize -mno-avx)
target_compile_options(baseline-8ecc PRIVATE -DECC_LENGTH=8)
target_compile_options(omp-8ecc PRIVATE -DECC_LENGTH=8)
