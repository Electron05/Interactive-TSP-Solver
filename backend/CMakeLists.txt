cmake_minimum_required(VERSION 3.15)

project(AntTSPSolver VERSION 1.0 LANGUAGES CXX)

add_executable(solver
    src/main.cpp
    src/tsp_solver.cpp
)

target_compile_features(solver PRIVATE cxx_std_20)


find_package(Threads REQUIRED)
find_package(Boost REQUIRED COMPONENTS system)

target_include_directories(solver PRIVATE include)
target_link_libraries(solver PRIVATE
    Threads::Threads
    Boost::system)


if(MSVC)
    target_compile_options(solver PRIVATE /W4)
else()
    target_compile_options(solver PRIVATE -Wall -Wextra -Wpedantic)
endif()