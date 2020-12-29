cmake_minimum_required(VERSION 3.16.3)
project(2lab)

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

set(CMAKE_CXX_STANDARD 14)

add_executable(cxxgrep main.cpp test.cpp)

llvm_map_components_to_libnames(llvm_libs support core irreader)

target_link_libraries(cxxgrep LLVM -lclang-10
        pthread
        )
