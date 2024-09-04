cmake_minimum_required(VERSION 3.20)
project(gbadisasm)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 17)

# Collect all sources in the src directory
file(GLOB_RECURSE SOURCES src/*.c)
set(CAPSTONE_SOURCE capstone-5.0.3)

# Configure capstone
set(CAPSTONE_ARCHITECTURE_DEFAULT OFF)
set(CAPSTONE_ARM_SUPPORT 1)
set(CAPSTONE_STATIC YES)
set(CAPSTONE_SHARED NO)
add_subdirectory(${CAPSTONE_SOURCE})

# Add your executable target
add_executable(gbadisasm ${SOURCES})
target_include_directories(gbadisasm PRIVATE ${CAPSTONE_SOURCE}/include)
target_link_libraries(gbadisasm PRIVATE capstone)
